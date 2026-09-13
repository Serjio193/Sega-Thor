"""AUTO65 bounded multi-chain campaign runner over a sealed broad capture."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from typing import Any

try:
    from auto65_chain import (NOVELTY_CLASSES, TERMINAL_STATES, chain_terminal,
                              classify, cluster, conflict, derive_candidates, digest)
except ModuleNotFoundError:
    import importlib.util
    _spec = importlib.util.spec_from_file_location(
        "auto65_chain", Path(__file__).with_name("auto65_chain.py"))
    _module = importlib.util.module_from_spec(_spec)
    assert _spec.loader
    _spec.loader.exec_module(_module)
    NOVELTY_CLASSES, TERMINAL_STATES = _module.NOVELTY_CLASSES, _module.TERMINAL_STATES
    chain_terminal, classify, cluster = _module.chain_terminal, _module.classify, _module.cluster
    conflict, derive_candidates, digest = _module.conflict, _module.derive_candidates, _module.digest


ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
BASELINE = "49875f51c2f3e818e7fdeba56fb430d07f3efaf2"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_scenario(path: Path) -> dict[str, Any]:
    fields: dict[str, Any] = {"path": str(path.resolve()), "inputs": [],
                              "id": path.stem, "start_state": "unknown",
                              "stop_condition": "unknown"}
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line.startswith("scenario_id="):
            fields["id"] = line.split("=", 1)[1]
        elif line.startswith("start_state="):
            fields["start_state"] = line.split("=", 1)[1]
        elif line.startswith("stop_condition="):
            fields["stop_condition"] = line.split("=", 1)[1]
        elif line.startswith("input "):
            fields["inputs"].append(line)
    fields["file_sha256"] = sha256(path)
    fields["identity"] = digest({key: fields[key] for key in
                                  ("id", "start_state", "stop_condition", "inputs")})
    return fields


def select_scenario(paths: list[Path], graph: dict[str, Any]) -> dict[str, Any]:
    open_states = {"OPEN", "ACTIVE", "WAITING_EVIDENCE", "IN_PROGRESS"}
    if any(node.get("status") in open_states for node in graph.get("nodes", {}).values()):
        raise ValueError("AUTO64 queue is not at a fixed point")
    choices = [load_scenario(path) for path in paths]
    choices = [item for item in choices if "quicksave1" not in item["id"].lower()]
    if not choices:
        raise ValueError("no materially different scenario available")
    return sorted(choices, key=lambda item: (-len(item["inputs"]), item["id"]))[0]


def capture_fingerprint(scenario: dict[str, Any], candidates: list[dict[str, Any]],
                       lineage: list[str]) -> str:
    obligations = sorted(item["obligation"] for item in candidates)
    watched = sorted({node["id"] for item in candidates for node in item["nodes"]})
    return digest({"scenario": scenario["identity"], "start_state": scenario["start_state"],
                   "inputs": scenario["inputs"], "watch_set": watched,
                   "obligations": obligations, "lineage": sorted(lineage)})


def static_probe(tool: Path | None, rom: Path, writer_pc: str, out: Path) -> dict[str, Any]:
    if not tool:
        return {"status": "UNAVAILABLE", "reason": "static tool not supplied"}
    pc = int(writer_pc, 16)
    start, end = max(0, pc - 2), pc + 4
    asm, report = out.with_suffix(".asm"), out.with_suffix(".json")
    command = [str(tool), str(rom), hex(start), hex(end), str(asm), str(report)]
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        return {"status": "BOUNDED_UNRESOLVED", "reason": result.stderr.strip() or "static tool rejected range",
                "command": command}
    facts = json.loads(report.read_text(encoding="utf-8"))
    return {"status": "PROVEN", "reason": "bounded decoder accepted the observed reset writer",
            "command": command, "instruction_count": len(facts.get("instructions", [])),
            "range": [start, end], "report": str(report)}


def initial_state(coverage: dict[str, Any], graph: dict[str, Any], scenario: dict[str, Any],
                  report: dict[str, Any]) -> dict[str, Any]:
    candidates, raw_count = derive_candidates(report)
    return {"schema": "oasis.m12.auto65.multi-chain-campaign.v1", "baseline": BASELINE,
            "rom_sha256": report.get("rom_sha256"), "scenario": scenario,
            "coverage_schema": coverage.get("schema"), "source_graph_schema": graph.get("schema"),
            "discovery": {"raw_observations": raw_count, "candidate_chains": len(candidates),
                           "candidate_ids": [item["id"] for item in candidates]},
            "knowledge": [], "investigations": {}, "captures": [], "history": [],
            "metrics": {key: 0 for key in (
                "discovery_runs", "focused_runtime_runs", "static_requests", "runtime_events",
                "candidate_chains", "candidate_branches", "known_complete_rejected", "known_instances",
                "new_chains", "new_branches", "new_edges", "new_writers", "new_consumers",
                "new_roots", "domain_expansions", "conflicts", "investigations_created",
                "investigations_cancelled_as_known", "investigations_completed", "investigations_blocked",
                "investigations_exhausted", "investigations_active_peak", "shared_dependency_merges",
                "shared_capture_reuse_count", "causal_edges_added", "structures_enumerated",
                "promotion_candidates", "bytes_promoted")},
            "novelty": {key: 0 for key in NOVELTY_CLASSES},
            "terminal_states": list(TERMINAL_STATES), "fixed_point": True}


def process(state: dict[str, Any], report: dict[str, Any], static_tool: Path | None,
            rom: Path, output: Path, replay: bool = False) -> dict[str, Any]:
    candidates, raw_count = derive_candidates(report)
    scenario = state["scenario"]
    if replay:
        candidates = candidates
    state["metrics"]["discovery_runs"] += 0 if replay else 1
    state["metrics"]["runtime_events"] += 0 if replay else raw_count
    state["metrics"]["candidate_chains"] = max(state["metrics"]["candidate_chains"], len(candidates))
    lineage = [item["id"] for item in state["knowledge"]]
    capture_id = "CAPTURE-" + digest({"report": report.get("scenario_id"),
                                       "sha256": state["scenario"]["file_sha256"]})[:16]
    if not replay:
        fp = capture_fingerprint(scenario, candidates, lineage)
        state["captures"].append({"id": capture_id, "fingerprint": fp, "kind": "BROAD_DISCOVERY",
                                   "investigations_consumed": [item["id"] for item in candidates]})
        state["metrics"]["shared_capture_reuse_count"] = max(0, len(candidates) - 1)
    known_before = list(state["knowledge"])
    classifications: list[dict[str, Any]] = []
    for candidate in candidates:
        novelty = classify(candidate, state["knowledge"])
        if conflict(candidate, state["knowledge"]):
            novelty = "CONFLICT"
        state["novelty"][novelty] += 1
        entry = {"candidate": candidate, "novelty": novelty, "investigation": None}
        if novelty in {"KNOWN_NEW_INSTANCE", "KNOWN"}:
            state["metrics"]["known_instances"] += novelty == "KNOWN_NEW_INSTANCE"
            state["metrics"]["known_complete_rejected"] += 1
            state["metrics"]["investigations_cancelled_as_known"] += 1
            entry["investigation"] = "CANCELLED_KNOWN"
        else:
            if novelty == "CONFLICT":
                state["metrics"]["conflicts"] += 1
            investigation = {"id": "INV-AUTO65-" + candidate["id"], "parent": None,
                             "children": [], "shared_dependency": digest(candidate["prefix"]),
                             "known_prefix": candidate["prefix"], "novelty_start": candidate["tail"],
                             "evidence_lineage": [capture_id], "static_requests": [],
                             "runtime_requests": [], "status": "ACTIVE", "novelty": novelty}
            if novelty == "NEW_CHAIN":
                state["metrics"]["new_chains"] += 1
            if novelty == "NEW_BRANCH":
                state["metrics"]["new_branches"] += 1
            if novelty == "NEW_WRITER":
                state["metrics"]["new_writers"] += 1
            if novelty == "NEW_CONSUMER":
                state["metrics"]["new_consumers"] += 1
            if novelty == "NEW_ROOT":
                state["metrics"]["new_roots"] += 1
            if novelty == "DOMAIN_EXPANSION":
                state["metrics"]["domain_expansions"] += 1
            if novelty in {"NEW_EDGE", "NEW_BRANCH", "NEW_WRITER", "NEW_CONSUMER",
                           "NEW_ROM_ACTIVITY", "NEW_ROOT"}:
                state["metrics"]["new_edges"] += 1
            state["metrics"]["investigations_created"] += 1
            state["investigations"][investigation["id"]] = investigation
            entry["investigation"] = investigation["id"]
            if novelty != "CONFLICT":
                state["knowledge"].append(candidate)
        classifications.append(entry)

    new_branch_count = sum(item["novelty"] == "NEW_BRANCH" for item in classifications)
    state["metrics"]["candidate_branches"] = max(state["metrics"]["candidate_branches"],
                                                   new_branch_count)
    if not replay:
        groups = cluster(candidates)
        state["metrics"]["shared_dependency_merges"] += sum(
            max(0, len(ids) - 1) for ids in groups.values())
    writer_groups = [item for item in candidates if item["nodes"][1]["kind"] == "WRITER"]
    static_facts: dict[str, dict[str, Any]] = {}
    if writer_groups and not replay:
        writer_pc = writer_groups[0]["nodes"][1]["id"].split(":", 1)[1]
        static_facts[writer_pc] = static_probe(tool=static_tool, rom=rom, writer_pc=writer_pc,
                                               out=output.with_name("auto65-writer-static"))
        state["metrics"]["static_requests"] += 1
        state["metrics"]["structures_enumerated"] += int(static_facts[writer_pc].get("status") == "PROVEN")
    for entry in classifications:
        inv_id = entry["investigation"]
        if not isinstance(inv_id, str) or inv_id == "CANCELLED_KNOWN":
            continue
        inv = state["investigations"][inv_id]
        candidate = entry["candidate"]
        if candidate["nodes"][1]["kind"] == "WRITER":
            writer_pc = candidate["nodes"][1]["id"].split(":", 1)[1]
            static = static_facts.get(writer_pc, {"status": "BOUNDED_UNRESOLVED"})
        elif candidate["nodes"][1]["kind"] == "CALLER":
            static = {"status": "PROVEN", "reason": "broad report carries static call bytes and observed callsite"}
            state["metrics"]["static_requests"] += 1 if not replay else 0
        else:
            static = {"status": "BOUNDED_UNRESOLVED", "reason": "static graph has no causal source for target activity"}
            state["metrics"]["static_requests"] += 1 if not replay else 0
        inv["static_requests"].append(static)
        inv["status"], inv["resolution"] = chain_terminal(candidate, static)
        if inv["status"] in {"PROVEN", "STRUCTURE_ENUMERATED"}:
            state["metrics"]["investigations_completed"] += 1
            state["metrics"]["causal_edges_added"] += len(candidate["edges"])
        elif inv["status"] == "BOUNDED_UNRESOLVED":
            state["metrics"]["investigations_blocked"] += 1
        if inv["status"] == "PROVEN" and candidate["nodes"][1]["kind"] == "WRITER":
            state["metrics"]["structures_enumerated"] += 1
    state["metrics"]["investigations_active_peak"] = max(state["metrics"]["investigations_active_peak"],
                                                          len(state["investigations"]))
    state["history"].append({"pass": "REPLAY" if replay else "FIRST", "known_before": len(known_before),
                             "candidates": len(candidates), "classifications": classifications})
    output.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return state


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--coverage", type=Path, required=True)
    parser.add_argument("--graph", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--scenario", type=Path, action="append", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--static-tool", type=Path)
    parser.add_argument("--replay", action="store_true")
    args = parser.parse_args()
    report = json.loads(args.report.read_text(encoding="utf-8"))
    if report.get("rom_sha256") != ROM_SHA or sha256(args.rom) != ROM_SHA:
        raise SystemExit("canonical ROM identity mismatch")
    coverage = json.loads(args.coverage.read_text(encoding="utf-8"))
    graph = json.loads(args.graph.read_text(encoding="utf-8"))
    if args.replay:
        state = json.loads(args.output.read_text(encoding="utf-8"))
    else:
        scenario = select_scenario(args.scenario, graph)
        state = initial_state(coverage, graph, scenario, report)
    state["replay"] = args.replay
    process(state, report, args.static_tool, args.rom, args.output, args.replay)
    print(json.dumps({"schema": state["schema"], "replay": args.replay,
                      "metrics": state["metrics"], "novelty": state["novelty"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
