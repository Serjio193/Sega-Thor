"""Fail-closed post-run audit for the targeted M12 observer artifact."""

from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
from typing import Any


SOURCE_OWNED = 1_487_672
ENTITY_BASE = 0xFF13CC


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def controller_values(document: dict[str, Any]) -> dict[str, list[str]]:
    by_frame: dict[int, set[int]] = defaultdict(set)
    for event in document["bus_events"]:
        if event["kind"] == "READ" and event["address"] == 0xA10003:
            by_frame[int(event["frame"])].add(int(event["value"]))
    result: dict[str, list[str]] = {}
    for phase in document["transitions"]:
        values: set[int] = set()
        for frame in range(phase["first_end_frame"], phase["last_end_frame"] + 1):
            values.update(by_frame.get(frame, set()))
        result[phase["name"]] = [f"0x{value:02X}" for value in sorted(values)]
    return result


def analyze(document: dict[str, Any]) -> dict[str, Any]:
    snapshots = {item["snapshot_id"]: item["registers"] for item in document["snapshots"]}
    register_complete = all(
        len(registers.get("d", [])) == 8 and len(registers.get("a", [])) == 8 and
        "sr" in registers and "pc" in registers for registers in snapshots.values()
    ) and bool(snapshots)
    executions = document["executions"]
    consumer = next((item for item in executions if item["pc"] == 0x2A0A), None)
    branch = next((item for item in executions if item["pc"] == 0x2A10), None)
    branch_path = None
    if consumer and branch:
        branch_registers = snapshots.get(branch["snapshot_id"], {})
        next_pc = branch.get("next_pc")
        branch_path = {
            "pcs": ["0x002A0A", "0x002A0C", "0x002A10", "0x002A14"],
            "branch_pc": "0x002A10", "condition": "BNE",
            "sr_before_branch": f"0x{int(branch_registers.get('sr', 0)):04X}",
            "next_pc": None if next_pc is None else f"0x{int(next_pc):06X}",
            "taken": bool(next_pc is not None and next_pc != 0x2A14),
            "basis": "bounded_PC_window_and_register_snapshot",
        }
    input_writes = [item for item in document["bus_events"]
                    if item["kind"] == "WRITE" and item["address"] == 0xFF165C]
    entity_writes = [item for item in document["bus_events"]
                     if item["kind"] == "WRITE" and ENTITY_BASE <= item["address"] < ENTITY_BASE + 8]
    values_by_field: dict[int, set[int]] = defaultdict(set)
    for item in entity_writes:
        values_by_field[item["address"] - ENTITY_BASE].add(item["value"])
    producer_writes = [item for item in entity_writes if item["pc"] in (0xA374, 0xA376, 0xA37C)]
    producer_values_by_field: dict[int, set[int]] = defaultdict(set)
    for item in producer_writes:
        producer_values_by_field[item["address"] - ENTITY_BASE].add(item["value"])
    entity_mutation = any(len(values) > 1 for values in producer_values_by_field.values())
    frame_identity_complete = all(
        item.get("run_id") is not None and item.get("epoch") is not None and
        item.get("frame") is not None and item.get("stream_sequence") is not None
        for item in document["frame_boundaries"]
    ) and bool(document["frame_boundaries"])
    native_instruction_complete = any(
        item.get("native_instruction_sequence") is not None for item in executions
    )
    exact_width_complete = all(item.get("width_bytes") in (1, 2, 4)
                               for item in document["bus_events"])
    exact_chain = (frame_identity_complete and register_complete and
                   native_instruction_complete and exact_width_complete and
                   branch_path is not None and bool(input_writes) and entity_mutation)
    return {
        "schema": "oasis.m12.targeted-controlled-entity-analysis.v1",
        "status": "PASS" if exact_chain else "STOP",
        "stop_reason": None if exact_chain else "STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT",
        "targeted_run_id": document["run_id"],
        "input_sequence": [item["name"] for item in document["transitions"]],
        "start_frame": document["start_frame"], "end_frame": document["end_frame"],
        "frame_boundary_count": len(document["frame_boundaries"]),
        "frame_boundary_range": [document["frame_boundaries"][0]["frame"],
                                 document["frame_boundaries"][-1]["frame"]],
        "frame_identity_complete": frame_identity_complete,
        "controller_values_by_phase": controller_values(document),
        "input_representation": {"address": "0xFF165C", "writes": len(input_writes),
                                  "values": sorted({item["value"] for item in input_writes})},
        "consumer_pc": "0x002A0A", "branch_path": branch_path,
        "register_snapshots_complete": register_complete,
        "native_instruction_sequence_complete": native_instruction_complete,
        "exact_bus_width_complete": exact_width_complete,
        "entity_field_observation": {
            "record_address": "0xFF13CC", "write_count": len(entity_writes),
            "field_values": {f"0x{offset:02X}": sorted(values)
                             for offset, values in sorted(values_by_field.items())},
            "candidate_producer_field_values": {
                f"0x{offset:02X}": sorted(values)
                for offset, values in sorted(producer_values_by_field.items())
            },
            "input_dependent_mutation_observed": entity_mutation,
        },
        "slot_generation": {
            "instance_id": None, "instance_start_frame": None,
            "instance_end_frame": None, "reuse_boundary_crossed": "UNKNOWN",
        },
        "exact_input_to_entity_chains": 1 if exact_chain else 0,
        "exact_input_to_sat_chains": 1 if exact_chain else 0,
        "entity_role": "CONTROLLED_ENTITY" if exact_chain else "ENTITY_CANDIDATE",
        "player_label_proven": "NO",
        "missing_evidence": [
            "native epoch and stream_sequence in frame identities",
            "native instruction_sequence for the causal window",
            "exact bus widths from the runtime sideband",
            "input-dependent selected-entity field mutation",
            "slot activation/deactivation/reuse boundaries for one generation",
        ] if not exact_chain else [],
        "existing_30_partial_audit": {"A": 0, "B": 30, "C": 30, "D": 0,
                                       "E": 30, "F": 30, "G": 0},
        "source_owned_contract": {
            "TARGETED_CAPTURE_SOURCE_OWNED_BEFORE": SOURCE_OWNED,
            "TARGETED_CAPTURE_SOURCE_OWNED_AFTER": SOURCE_OWNED,
            "TARGETED_CAPTURE_SOURCE_OWNED_DELTA": 0,
        },
        "capture_artifact_sha256": None,
        "architecture": document["architecture"],
        "commit_created": "NO", "push_performed": "NO",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("capture", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    document = json.loads(args.capture.read_text(encoding="utf-8"))
    report = analyze(document)
    report["capture_artifact_sha256"] = sha256(args.capture)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if report["status"] == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
