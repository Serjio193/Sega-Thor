"""Analyze the bounded AUTO64 capture and advance the persisted queue."""

from __future__ import annotations

import argparse
import hashlib
import json
import importlib.util
from pathlib import Path
from typing import Any

try:
    from auto64_provenance import next_open
except ModuleNotFoundError:
    _spec = importlib.util.spec_from_file_location(
        "auto64_provenance", Path(__file__).with_name("auto64_provenance.py"))
    _module = importlib.util.module_from_spec(_spec)
    assert _spec.loader
    _spec.loader.exec_module(_module)
    next_open = _module.next_open


def read_raw(path: Path) -> list[dict[str, Any]]:
    events = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.strip():
            events.append(json.loads(line))
    return events


def target_events(events: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [item for item in events if item.get("kind") == "TARGET_ENTRY_CONTEXT"]


def prove_dynamic_caller(events: list[dict[str, Any]]) -> dict[str, Any]:
    targets = target_events(events)
    proven = []
    for event in targets:
        data = event.get("data", {})
        previous_pc = data.get("previous_pc")
        return_pc = data.get("stack_return_long")
        if data.get("instruction_pc") != 0xAF00:
            continue
        if previous_pc is None or return_pc is None or previous_pc >= 0xAF00:
            continue
        ring = data.get("previous_pc_ring", [])
        if isinstance(ring, dict):
            ring = list(ring.values())
        call_like = any(item.get("pc") == previous_pc for item in ring)
        if call_like and 0 < return_pc < 0x400000:
            proven.append({"caller_pc": previous_pc, "target_pc": data.get("instruction_pc"),
                           "return_pc": return_pc, "epoch": event.get("epoch"),
                           "frame": event.get("frame"), "seq": event.get("seq")})
    return {"status": "PROVEN" if proven else "NOT_PROVEN", "proven": proven,
            "target_contexts": len(targets)}


def observed_af20_addresses(events: list[dict[str, Any]]) -> list[int]:
    values = []
    for event in target_events(events):
        data = event.get("data", {})
        if data.get("instruction_pc") == 0xAF20 and data.get("registers", {}).get("A0") is not None:
            values.append(data["registers"]["A0"])
    return values


def observed_a6_candidates(events: list[dict[str, Any]]) -> list[dict[str, Any]]:
    candidates = []
    for event in target_events(events):
        candidate = event.get("data", {}).get("a6_definition_candidate")
        if candidate and candidate not in candidates:
            candidates.append(candidate)
    return candidates


def advance(graph: dict[str, Any], events: list[dict[str, Any]]) -> dict[str, Any]:
    caller_id = "INV-AUTO64-A6-CALLER"
    caller = graph["nodes"].get(caller_id)
    proof = prove_dynamic_caller(events)
    a6_candidates = observed_a6_candidates(events)
    if caller and caller["status"] == "OPEN":
        caller["raw_witnesses"].append({"class": "RUNTIME", "proof": proof})
        caller["known"].append(f"focused capture target contexts={proof['target_contexts']}")
        if proof["status"] == "PROVEN":
            caller["status"] = "RESOLVED"
            caller["resolution"] = "dynamic caller edge established in one epoch with stack return compatibility"
            for edge in proof["proven"][:1]:
                child_id = f"INV-AUTO64-A6-ORIGIN-{edge['caller_pc']:06X}"
                if child_id not in graph["nodes"]:
                    child = {
                        "id": child_id, "parent": caller_id,
                        "obligation": f"A6 origin before caller {edge['caller_pc']:#06x}",
                        "question": "Which instruction or inherited frame established A6 before this caller?",
                        "known": ["caller edge proven", "A6 origin still not observed"],
                        "missing": ["definition-before-use A6 edge"],
                        "evidence_classes": ["STATIC", "RUNTIME"], "status": "OPEN",
                        "children": [], "resolution": None, "raw_witnesses": [],
                        "derived_relations": [edge], "priority": 110,
                    }
                    graph["nodes"][child_id] = child
                    caller["children"].append(child_id)
        else:
            caller["status"] = "BLOCKED"
            caller["resolution"] = "static direct-call scan and focused runtime caller context did not prove a causal edge"
            caller["missing"].append("return-compatible dynamic call or predecessor definition")
    root = graph["nodes"].get("INV-AUTO64-A6")
    if root and a6_candidates:
        candidate = a6_candidates[0]
        candidate_pc = int(candidate["instruction_pc"])
        child_id = f"INV-AUTO64-A6-DEFINITION-{candidate_pc:06X}"
        if child_id not in graph["nodes"]:
            child = {
                "id": child_id, "parent": root["id"],
                "obligation": f"verify A6 definition candidate at {candidate_pc:#06x}",
                "question": "Does this instruction establish the A6 value used by AF02, and from what source?",
                "known": ["A6 changed before target within the same frame and bounded instruction window"],
                "missing": ["instruction semantics", "definition-before-use source class"],
                "evidence_classes": ["STATIC", "RUNTIME"], "status": "OPEN",
                "children": [], "resolution": None,
                "raw_witnesses": [{"class": "RUNTIME", "candidate": candidate}],
                "derived_relations": [{"type": "A6_VERSION_BEFORE_USE", "target": "0xAF02"}],
                "priority": 110,
                "fingerprint": f"{child_id}|A6 definition candidate|same-frame bounded version|STATIC,RUNTIME",
            }
            graph["nodes"][child_id] = child
            root["children"].append(child_id)
        root["status"] = "IN_PROGRESS"
        root["resolution"] = "caller edge unresolved; runtime produced a bounded A6-version child"
    if root and not a6_candidates and caller and caller["status"] in ("BLOCKED", "BOUNDED_UNRESOLVED"):
        root["status"] = "BLOCKED"
        root["resolution"] = "A6 remains inherited; no terminal source root claimed"
    graph["queue_history"].append({"frontier": caller_id, "action": "RUNTIME_FOCUSED",
                                    "capture": "AUTO64_FOCUSED_CALLER",
                                    "caller_proof": proof, "a6_candidates": a6_candidates,
                                    "next": next_open(graph)["id"] if next_open(graph) else None})
    return graph


def update_coverage(coverage: dict[str, Any], raw_path: Path,
                    analysis: dict[str, Any]) -> dict[str, Any]:
    coverage.setdefault("source_artifacts", []).append({
        "kind": "AUTO64_RUNTIME_RAW", "path": str(raw_path),
        "sha256": hashlib.sha256(raw_path.read_bytes()).hexdigest(),
        "capture_id": analysis.get("capture_id", "auto64-caller-b"),
    })
    entity = coverage.get("entities", {}).get("PC:0000AF00-0000AF22")
    if entity:
        entity["dimensions"]["EXECUTION"] = "OBSERVED"
        entity["dimensions"]["CALLER"] = "BLOCKED"
        entity["dimensions"]["POINTER_SOURCE"] = "PARTIAL"
        entity["provenance"].append({"artifact": "AUTO64 focused caller raw",
                                     "path": str(raw_path), "proof_state": "PARTIAL",
                                     "new_information": True,
                                     "question_answered": analysis["caller_proof"]["status"] == "PROVEN"})
    coverage["runtime_updates"] = [{"classification": "NEW_EDGE_CANDIDATE",
                                     "question_answered": analysis["caller_proof"]["status"] == "PROVEN",
                                     "actually_new_information": True,
                                     "a6_candidates": analysis.get("a6_candidates", [])}]
    return coverage


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", type=Path, required=True)
    parser.add_argument("--graph", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--coverage", type=Path)
    parser.add_argument("--candidate-static", type=Path)
    args = parser.parse_args()
    graph = json.loads(args.graph.read_text(encoding="utf-8"))
    events = read_raw(args.raw)
    graph = advance(graph, events)
    analysis = {"schema": "oasis.m12.auto64.provenance-analysis.v1",
                "capture_complete": any(e.get("kind") == "RAW_END" for e in events),
                "caller_proof": prove_dynamic_caller(events),
                "a6_candidates": observed_a6_candidates(events),
                "observed_af20_a0": observed_af20_addresses(events)}
    if args.candidate_static and args.candidate_static.exists():
        static = json.loads(args.candidate_static.read_text(encoding="utf-8"))
        child = next((item for item in graph["nodes"].values()
                      if item["id"].startswith("INV-AUTO64-A6-DEFINITION-") and item["status"] == "OPEN"), None)
        has_a6_write = any("A6" in item.get("mnemonic", "").upper() and
                           item.get("mnemonic", "").lower() in ("movea", "lea")
                           for item in static.get("instructions", []))
        if child and not has_a6_write:
            child["status"] = "EXHAUSTED"
            child["resolution"] = "bounded static slice at the runtime observation site has no A6 destination"
            child["block_reason"] = "candidate is an observation site, not a proven A6 definition"
            child["raw_witnesses"].append({"class": "STATIC", "path": str(args.candidate_static),
                                           "instruction_count": len(static.get("instructions", [])),
                                           "a6_write": False})
        root = graph["nodes"].get("INV-AUTO64-A6")
        if root and not next_open(graph):
            root["status"] = "BLOCKED"
            root["resolution"] = "all materially different current candidates reached justified fixed points"
            root["block_reason"] = "A6 source remains unresolved; no terminal root claimed"
    if args.coverage and args.coverage.exists():
        update_coverage(coverage := json.loads(args.coverage.read_text(encoding="utf-8")), args.raw, analysis)
        args.coverage.write_text(json.dumps(coverage, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    args.graph.write_text(json.dumps(graph, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    result = {**analysis,
              "next_frontier": next_open(graph)["id"] if next_open(graph) else None,
              "graph": str(args.graph)}
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
