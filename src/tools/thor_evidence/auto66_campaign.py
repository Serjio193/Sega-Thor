"""AUTO66 scheduler-driven campaign over the bounded AUTO65 engine."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path
from typing import Any

try:
    from auto65_campaign import load_scenario, static_probe
    from auto65_chain import (NOVELTY_CLASSES, chain_terminal, classify, cluster,
                              conflict, derive_candidates, digest)
except ModuleNotFoundError:
    import importlib.util

    def load_sibling(name: str, filename: str) -> Any:
        spec = importlib.util.spec_from_file_location(
            name, Path(__file__).with_name(filename))
        module = importlib.util.module_from_spec(spec)
        assert spec.loader
        spec.loader.exec_module(module)
        return module

    campaign = load_sibling("auto65_campaign", "auto65_campaign.py")
    chain = load_sibling("auto65_chain", "auto65_chain.py")
    load_scenario, static_probe = campaign.load_scenario, campaign.static_probe
    NOVELTY_CLASSES = chain.NOVELTY_CLASSES
    chain_terminal, classify = chain.chain_terminal, chain.classify
    cluster, conflict = chain.cluster, chain.conflict
    derive_candidates, digest = chain.derive_candidates, chain.digest


ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
BASELINE = "1942dfdbd1f0cdbcf5226a464c3e41abd16ebb1a"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def scenario_metadata(path: Path) -> dict[str, Any]:
    item = load_scenario(path)
    targets: list[str] = []
    ram_watch: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line.startswith("target_address="):
            targets.append(line.split("=", 1)[1])
        elif line.startswith("ram_watch_address="):
            ram_watch.append(line.split("=", 1)[1])
    item["targets"] = targets
    item["ram_watch"] = ram_watch
    item["planner_fingerprint"] = digest({
        "file_sha256": item["file_sha256"], "start_state": item["start_state"],
        "inputs": item["inputs"], "targets": targets, "ram_watch": ram_watch,
    })
    return item


def parse_max_frames(stop_condition: str) -> int:
    try:
        return int(stop_condition.split("max_frames:", 1)[1])
    except (IndexError, ValueError) as exc:
        raise ValueError("scenario must declare max_frames stop_condition") from exc


def seed_metrics(seed: dict[str, Any]) -> dict[str, int]:
    old = seed["metrics"]
    return {
        "runtime_events": old["runtime_events"],
        "candidate_chains": old["candidate_chains"],
        "known_rejected": old["known_complete_rejected"],
        "new_chains": old["new_chains"], "new_branches": old["new_branches"],
        "new_edges": old["new_edges"], "investigations_created": old["investigations_created"],
        "peak_active": old["investigations_active_peak"],
        "investigations_completed": old["investigations_completed"],
        "blocked": old["investigations_blocked"], "exhausted": old["investigations_exhausted"],
        "shared_dependency_reuse": old["shared_dependency_merges"],
        "shared_capture_reuse": old["shared_capture_reuse_count"],
        "focused_runtime_runs": old["focused_runtime_runs"],
        "static_requests": old["static_requests"],
        "chains_closed": old["investigations_completed"],
        "structures_enumerated": old["structures_enumerated"],
        "promotion_candidates": old["promotion_candidates"],
        "bytes_promoted": old["bytes_promoted"],
    }


def seed_scenario(seed: dict[str, Any], report: dict[str, Any], path: Path) -> dict[str, Any]:
    candidates, raw = derive_candidates(report)
    groups = cluster(candidates)
    first_pass = seed.get("history", [{}])[0].get("classifications", [])
    novelty = {key: 0 for key in NOVELTY_CLASSES}
    for item in first_pass:
        novelty[item["novelty"]] += 1
    return {
        "scenario_id": report["scenario_id"], "source": "AUTO65_ACCEPTED_CAPTURE",
        "path": str(path.resolve()), "status": "FIXED_POINT",
        "raw_observations": raw, "candidate_chains": len(candidates), "known": 0,
        "novelty": {key: novelty.get(key, 0) for key in NOVELTY_CLASSES},
        "clustered_investigations": sorted(len(value) for value in groups.values()),
        "investigations_created": seed["metrics"]["investigations_created"],
        "chains_closed": seed["metrics"]["investigations_completed"],
        "structures_enumerated": seed["metrics"]["structures_enumerated"],
        "bytes_promoted": seed["metrics"]["bytes_promoted"],
        "emulator_launches": 1, "capture_reused": True,
    }


def planner_score(item: dict[str, Any], frontier_count: int) -> tuple[int, int, int, str]:
    return (frontier_count + len(item["targets"]), len(item["inputs"]),
            parse_max_frames(item["stop_condition"]), item["id"])


def plan_next_scenario(pool: list[dict[str, Any]], prior: set[str],
                       frontier_count: int) -> tuple[dict[str, Any] | None, list[dict[str, Any]]]:
    attempted: list[dict[str, Any]] = []
    eligible: list[dict[str, Any]] = []
    for item in pool:
        if item["planner_fingerprint"] in prior:
            attempted.append({"scenario_id": item["id"], "decision": "REJECTED_EQUIVALENT",
                              "fingerprint": item["planner_fingerprint"]})
        else:
            eligible.append(item)
    selected = max(eligible, key=lambda item: planner_score(item, frontier_count), default=None)
    if selected:
        attempted.append({"scenario_id": selected["id"], "decision": "SELECTED",
                          "score": planner_score(selected, frontier_count),
                          "fingerprint": selected["planner_fingerprint"]})
    return selected, attempted


def run_discovery(scenario: dict[str, Any], emulator: Path, lua: Path, rom: Path,
                  output_dir: Path) -> tuple[dict[str, Any], dict[str, Any]]:
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    stem = scenario["id"]
    report_path, trace_path = output_dir / f"{stem}.report.json", output_dir / f"{stem}.trace.txt"
    environment = os.environ.copy()
    environment.update({
        "OASIS_SCENARIO_FILE": str(Path(scenario["path"])),
        "OASIS_NATURAL_TRACE_OUTPUT": str(trace_path),
        "OASIS_NATURAL_REPORT_OUTPUT": str(report_path),
        "OASIS_SCENARIO_FAMILY": f"auto66_scheduler_selected_{stem}",
        "OASIS_VARIANT_ID": f"auto66_{stem}",
        "OASIS_MAX_TARGET_SNAPSHOTS": "16",
    })
    command = [str(emulator.resolve()), "--chromeless", f"--lua={lua.resolve()}",
               str(rom.resolve())]
    log_path = output_dir / f"{stem}.launcher.log"
    with log_path.open("w", encoding="utf-8") as log:
        result = subprocess.run(command, cwd=emulator.parent, env=environment,
                                stdout=log, stderr=subprocess.STDOUT,
                                text=True, check=False, timeout=180)
    if result.returncode != 0 or not report_path.exists():
        detail = log_path.read_text(encoding="utf-8", errors="replace")[-1000:]
        raise RuntimeError(f"bounded discovery failed for {stem}: {detail}")
    return json.loads(report_path.read_text(encoding="utf-8")), {
        "command": command, "returncode": result.returncode, "report": str(report_path),
        "trace": str(trace_path), "launcher_log": str(log_path),
    }


def new_state(seed: dict[str, Any], seed_report: dict[str, Any], seed_path: Path,
              coverage: dict[str, Any], pool: list[dict[str, Any]]) -> dict[str, Any]:
    first = scenario_metadata(seed_path)
    metrics = seed_metrics(seed)
    return {
        "schema": "oasis.m12.auto66.multi-scenario-campaign.v1", "baseline": BASELINE,
        "rom_sha256": ROM_SHA, "coverage_schema": coverage.get("schema"),
        "frontier_count": len(coverage.get("derived_frontiers", [])),
        "scenario_pool": [{"id": item["id"], "fingerprint": item["planner_fingerprint"]}
                           for item in pool],
        "scenario_fingerprints": [first["planner_fingerprint"]],
        "scenarios": [seed_scenario(seed, seed_report, seed_path)],
        "planner": [], "transitions": [{"from": seed_report["scenario_id"],
          "event": "FIXED_POINT", "next": "NEED_NEW_SCENARIO"}],
        "captures": [{"scenario_id": seed_report["scenario_id"], "source": "AUTO65",
                       "fingerprint": first["planner_fingerprint"]}],
        "knowledge": list(seed.get("knowledge", [])),
        "investigations": dict(seed.get("investigations", {})),
        "novelty": dict(seed.get("novelty", {})), "metrics": metrics,
        "fixed_point": False,
    }


def process_scenario(state: dict[str, Any], scenario: dict[str, Any], report: dict[str, Any],
                     capture: dict[str, Any], static_tool: Path | None, rom: Path,
                     output_dir: Path) -> None:
    candidates, raw = derive_candidates(report)
    known = 0
    classifications: list[dict[str, Any]] = []
    capture_id = "CAPTURE-AUTO66-" + digest(report["scenario_id"])[:16]
    state["captures"].append({"id": capture_id, "scenario_id": report["scenario_id"],
                               "fingerprint": scenario["planner_fingerprint"], **capture})
    state["scenario_fingerprints"].append(scenario["planner_fingerprint"])
    state["metrics"]["runtime_events"] += raw
    state["metrics"]["candidate_chains"] += len(candidates)
    state["metrics"]["shared_capture_reuse"] += max(0, len(candidates) - 1)
    for candidate in candidates:
        novelty = classify(candidate, state["knowledge"])
        if conflict(candidate, state["knowledge"]):
            novelty = "CONFLICT"
        state["novelty"][novelty] = state["novelty"].get(novelty, 0) + 1
        if novelty in {"KNOWN", "KNOWN_NEW_INSTANCE"}:
            known += 1
            state["metrics"]["known_rejected"] += 1
            classifications.append({"candidate": candidate["id"], "novelty": novelty,
                                    "investigation": "CANCELLED_KNOWN"})
            continue
        inv_id = "INV-AUTO66-" + candidate["id"]
        investigation = {"id": inv_id, "scenario_id": scenario["id"],
                         "known_prefix": candidate["prefix"], "novelty_start": candidate["tail"],
                         "evidence_lineage": [capture_id], "static_requests": [],
                         "runtime_requests": [], "status": "ACTIVE", "novelty": novelty}
        state["investigations"][inv_id] = investigation
        state["metrics"]["investigations_created"] += 1
        if novelty == "NEW_CHAIN": state["metrics"]["new_chains"] += 1
        if novelty == "NEW_BRANCH": state["metrics"]["new_branches"] += 1
        if novelty in {"NEW_EDGE", "NEW_BRANCH", "NEW_WRITER", "NEW_CONSUMER",
                       "NEW_ROM_ACTIVITY", "NEW_ROOT"}:
            state["metrics"]["new_edges"] += 1
        static = {"status": "BOUNDED_UNRESOLVED", "reason": "no bounded static source"}
        if candidate["nodes"][1]["kind"] == "WRITER":
            writer_pc = candidate["nodes"][1]["id"].split(":", 1)[1]
            static = static_probe(static_tool, rom, writer_pc,
                                  output_dir / f"auto66-{scenario['id']}-writer-static")
        state["metrics"]["static_requests"] += 1
        investigation["static_requests"].append(static)
        investigation["status"], investigation["resolution"] = chain_terminal(candidate, static)
        if investigation["status"] in {"PROVEN", "STRUCTURE_ENUMERATED"}:
            state["metrics"]["investigations_completed"] += 1
            state["metrics"]["chains_closed"] += 1
        elif investigation["status"] == "BOUNDED_UNRESOLVED":
            state["metrics"]["blocked"] += 1
        state["knowledge"].append(candidate)
        classifications.append({"candidate": candidate["id"], "novelty": novelty,
                                "investigation": inv_id, "status": investigation["status"]})
    groups = cluster(candidates)
    state["metrics"]["shared_dependency_reuse"] += sum(max(0, len(value) - 1)
                                                         for value in groups.values())
    state["metrics"]["peak_active"] = max(state["metrics"]["peak_active"],
                                            len(state["investigations"]))
    novelty = {key: 0 for key in NOVELTY_CLASSES}
    for item in classifications: novelty[item["novelty"]] += 1
    state["scenarios"].append({"scenario_id": scenario["id"], "source": "AUTO66",
        "status": "EXECUTED", "raw_observations": raw, "candidate_chains": len(candidates),
        "known": known, "novelty": novelty,
        "clustered_investigations": sorted(len(value) for value in groups.values()),
        "investigations_created": len([x for x in classifications if x["investigation"] != "CANCELLED_KNOWN"]),
        "chains_closed": sum(x.get("status") in {"PROVEN", "STRUCTURE_ENUMERATED"}
                              for x in classifications), "structures_enumerated": 0,
        "bytes_promoted": 0, "emulator_launches": 1, "capture_reused": len(candidates) > 1,
        "capture": capture})


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--coverage", type=Path, required=True)
    parser.add_argument("--graph", type=Path, required=True)
    parser.add_argument("--seed-state", type=Path, required=True)
    parser.add_argument("--seed-report", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--scenario", type=Path, action="append", required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--lua", type=Path, required=True)
    parser.add_argument("--static-tool", type=Path)
    parser.add_argument("--discovery-report", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    coverage = json.loads(args.coverage.read_text(encoding="utf-8"))
    graph = json.loads(args.graph.read_text(encoding="utf-8"))
    seed = json.loads(args.seed_state.read_text(encoding="utf-8"))
    seed_report = json.loads(args.seed_report.read_text(encoding="utf-8"))
    if sha256(args.rom) != ROM_SHA or seed.get("rom_sha256") != ROM_SHA:
        raise SystemExit("canonical ROM identity mismatch")
    pool = [scenario_metadata(path) for path in args.scenario]
    seed_path = next((path for path in args.scenario
                      if scenario_metadata(path)["id"] == seed_report["scenario_id"]), None)
    if seed_path is None:
        raise SystemExit("seed report scenario is not in the supplied scenario pool")
    state = new_state(seed, seed_report, seed_path, coverage, pool)
    selected, decisions = plan_next_scenario(pool, set(state["scenario_fingerprints"]),
                                              state["frontier_count"])
    state["planner"].append({"event": "NEED_NEW_SCENARIO", "decisions": decisions})
    if selected is None:
        state["fixed_point"] = True
        state["stop_reason"] = "useful scenario pool exhausted"
    else:
        state["transitions"].append({"from": seed_report["scenario_id"],
            "event": "NEED_NEW_SCENARIO", "to": selected["id"], "autonomous": True})
        if args.discovery_report:
            report = json.loads(args.discovery_report.read_text(encoding="utf-8"))
            capture = {"source": "AUTO66_PRECAPTURED_REPORT", "returncode": 0,
                       "report": str(args.discovery_report.resolve())}
        else:
            report, capture = run_discovery(selected, args.emulator, args.lua, args.rom,
                                            args.output.parent / "captures")
        process_scenario(state, selected, report, capture, args.static_tool, args.rom,
                         args.output.parent)
        state["fixed_point"] = True
        state["stop_reason"] = "useful scenario pool exhausted after selected scenario"
    state["output"] = str(args.output)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"schema": state["schema"], "selected": selected["id"] if selected else None,
                      "metrics": state["metrics"], "transitions": state["transitions"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
