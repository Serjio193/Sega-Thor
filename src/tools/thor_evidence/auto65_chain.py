"""AUTO65 chain identity, novelty and shared-prefix primitives.

This module is deliberately evidence-shaped: hashes index records, while the
normalised nodes and edges decide structural equivalence.
"""

from __future__ import annotations

import hashlib
import json
from typing import Any, Iterable


NOVELTY_CLASSES = (
    "KNOWN", "KNOWN_NEW_INSTANCE", "NEW_CHAIN", "NEW_BRANCH", "NEW_EDGE",
    "NEW_ROOT", "NEW_CONSUMER", "NEW_WRITER", "NEW_ROM_ACTIVITY",
    "DOMAIN_EXPANSION", "NEW_STRUCTURE_CANDIDATE", "CONFLICT", "UNKNOWN",
)
TERMINAL_STATES = (
    "PROVEN", "STRUCTURE_ENUMERATED", "PROMOTED", "KNOWN_DUPLICATE",
    "CANCELLED_KNOWN", "BLOCKED", "EXHAUSTED", "BOUNDED_UNRESOLVED",
    "CONFLICT",
)


def canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"))


def digest(value: Any) -> str:
    return hashlib.sha256(canonical(value).encode()).hexdigest()


def _node(kind: str, identity: str, **extra: Any) -> dict[str, Any]:
    result = {"kind": kind, "id": identity}
    result.update(extra)
    return result


def _edge(source: str, target: str, kind: str, **extra: Any) -> dict[str, Any]:
    result = {"source": source, "target": target, "kind": kind}
    result.update(extra)
    return result


def _candidate(scenario: dict[str, Any], nodes: list[dict[str, Any]],
              edges: list[dict[str, Any]], source: dict[str, Any],
              obligation: str) -> dict[str, Any]:
    structure = {"nodes": nodes, "edges": edges}
    prefix = {"nodes": nodes[:-1], "edges": edges[:-1]}
    return {
        "id": "CHAIN-" + digest(structure)[:16],
        "scenario": scenario["id"],
        "epoch": 1,
        "nodes": nodes,
        "edges": edges,
        "prefix": prefix,
        "tail": edges[-1],
        "structure": structure,
        "fingerprint": digest({"kind": "auto65-capture", "scenario": scenario,
                                "obligation": obligation, "structure": structure}),
        "obligation": obligation,
        "source": source,
    }


def derive_candidates(report: dict[str, Any]) -> tuple[list[dict[str, Any]], int]:
    """Derive only chains represented by observed report rows.

    Repeated runtime instances are retained as raw observations but collapse to
    one structural candidate. This gives the scheduler a real, bounded fanout.
    """
    scenario = {"id": report["scenario_id"], "state": report["start_state"],
                "family": report.get("scenario_family", report["scenario_id"])}
    candidates: dict[str, dict[str, Any]] = {}
    raw_count = 0

    writes = report.get("writes", [])
    raw_count += len(writes)
    for row in writes:
        pc = row.get("pc")
        address = row.get("address")
        if not isinstance(pc, str) or not isinstance(address, str):
            continue
        nodes = [_node("ROOT", "RESET:" + report["start_state"]),
                 _node("WRITER", "PC:" + pc), _node("RAM", "RAM:" + address)]
        edges = [_edge(nodes[0]["id"], nodes[1]["id"], "RESET_REACH"),
                 _edge(nodes[1]["id"], nodes[2]["id"], "WRITE",
                       width=row.get("flags"), value=row.get("value"))]
        item = _candidate(scenario, nodes, edges, row, "writer coverage and reset closure")
        candidates.setdefault(item["id"], item)

    callers = report.get("caller_discrimination", {}).get("caller_hits", [])
    raw_count += len(callers)
    static = report.get("caller_discrimination", {}).get("static_callers", [])
    by_pc = {row.get("call_site"): row for row in static}
    for row in callers:
        pc = row.get("pc")
        relation = by_pc.get(pc)
        if not relation:
            continue
        nodes = [_node("ROOT", "RESET:" + report["start_state"]),
                 _node("CALLER", "PC:" + pc),
                 _node("CONSUMER", "PC:" + relation["target"])]
        edges = [_edge(nodes[0]["id"], nodes[1]["id"], "RESET_REACH"),
                 _edge(nodes[1]["id"], nodes[2]["id"], "CALL",
                       return_address=relation.get("expected_return_address"))]
        item = _candidate(scenario, nodes, edges, row,
                          "runtime caller to statically verified target")
        candidates.setdefault(item["id"], item)

    activities = [row for row in report.get("target_hits", [])
                  if isinstance(row.get("count"), int) and row["count"] > 0]
    raw_count += len(activities)
    for row in activities:
        address = row["address"]
        nodes = [_node("ROOT", "RESET:" + report["start_state"]),
                 _node("ROM_ACTIVITY", "ROM_PC:" + address)]
        edges = [_edge(nodes[0]["id"], nodes[1]["id"], "EXECUTION",
                       occurrences=row["count"])]
        item = _candidate(scenario, nodes, edges, row,
                          "runtime target activity requires provenance closure")
        candidates.setdefault(item["id"], item)
    return sorted(candidates.values(), key=lambda item: item["id"]), raw_count


def structural_equal(left: dict[str, Any], right: dict[str, Any]) -> bool:
    return left.get("structure") == right.get("structure")


def prefix_equal(left: dict[str, Any], right: dict[str, Any]) -> bool:
    return left.get("prefix") == right.get("prefix")


def _all_edges(knowledge: Iterable[dict[str, Any]]) -> set[str]:
    return {canonical(edge) for item in knowledge for edge in item.get("edges", [])}


def _nodes(knowledge: Iterable[dict[str, Any]]) -> set[str]:
    return {node["id"] for item in knowledge for node in item.get("nodes", [])}


def classify(candidate: dict[str, Any], knowledge: list[dict[str, Any]]) -> str:
    for item in knowledge:
        if structural_equal(candidate, item):
            return "KNOWN_NEW_INSTANCE"
    prefixes = [item for item in knowledge if prefix_equal(candidate, item)]
    if prefixes:
        return "NEW_BRANCH"
    candidate_nodes = _nodes([candidate])
    known_nodes = _nodes(knowledge)
    candidate_edges = {canonical(edge) for edge in candidate["edges"]}
    known_edges = _all_edges(knowledge)
    if candidate_edges & known_edges and candidate_edges - known_edges:
        return "NEW_EDGE"
    if any(node["kind"] == "WRITER" and node["id"] not in known_nodes
           for node in candidate["nodes"]):
        return "NEW_WRITER"
    if any(node["kind"] == "CONSUMER" and node["id"] not in known_nodes
           for node in candidate["nodes"]):
        return "NEW_CONSUMER"
    if any(node["kind"] == "ROM_ACTIVITY" and node["id"] not in known_nodes
           for node in candidate["nodes"]):
        return "NEW_ROM_ACTIVITY"
    if candidate_nodes - known_nodes:
        return "NEW_CHAIN"
    return "UNKNOWN"


def conflict(candidate: dict[str, Any], knowledge: list[dict[str, Any]]) -> bool:
    """Detect a same structural edge with contradictory evidence payload."""
    for item in knowledge:
        for left in candidate["edges"]:
            for right in item.get("edges", []):
                if (left["source"], left["target"], left["kind"]) != \
                        (right["source"], right["target"], right["kind"]):
                    continue
                if left.get("value") is not None and right.get("value") is not None \
                        and left["value"] != right["value"]:
                    return True
    return False


def cluster(candidates: list[dict[str, Any]]) -> dict[str, list[str]]:
    result: dict[str, list[str]] = {}
    for item in candidates:
        key = digest(item["prefix"])
        result.setdefault(key, []).append(item["id"])
    return {key: sorted(value) for key, value in sorted(result.items())}


def chain_terminal(candidate: dict[str, Any], static: dict[str, Any]) -> tuple[str, str]:
    if candidate["nodes"][-1]["kind"] == "ROM_ACTIVITY":
        return "BOUNDED_UNRESOLVED", "activity has no causal source in bounded static evidence"
    if static.get("status") == "PROVEN":
        return "PROVEN", static.get("reason", "bounded static/runtime chain closed")
    return "BOUNDED_UNRESOLVED", static.get("reason", "bounded evidence exhausted")
