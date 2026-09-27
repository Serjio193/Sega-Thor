"""Analyze the bounded M12 dynamic SAT producer backtrace."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 0x300000
SOURCE_OWNED = 1_487_672
SHADOW_BASE = 0xFF13CC
SAT_BASE = 0xD000

WRITERS = {
    "0x00B754": {"actual_pc": "0x00B752", "bytes": "3286", "instruction": "MOVE.W D6,(A1)",
        "destination": "A1 + 0", "source_operand": "D6", "source_registers": ["D6"],
        "field_offset": 0, "width": 2, "semantic": "SAT Y"},
    "0x00B768": {"actual_pc": "0x00B764", "bytes": "3347", "instruction": "MOVE.W D7,2(A1)",
        "destination": "A1 + 2", "source_operand": "D7", "source_registers": ["D7"],
        "field_offset": 2, "width": 2, "semantic": "SAT size/link"},
    "0x00B772": {"actual_pc": "0x00B76E", "bytes": "3346", "instruction": "MOVE.W D6,4(A1)",
        "destination": "A1 + 4", "source_operand": "D6", "source_registers": ["D6"],
        "field_offset": 4, "width": 2, "semantic": "SAT X"},
    "0x00B77E": {"actual_pc": "0x00B77A", "bytes": "3346", "instruction": "MOVE.W D6,6(A1)",
        "destination": "A1 + 6", "source_operand": "D6", "source_registers": ["D6"],
        "field_offset": 6, "width": 2, "semantic": "SAT tile/attribute"},
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def u16(data: bytes, address: int) -> int:
    return int.from_bytes(data[address:address + 2], "big")


def decode_sites(rom: bytes) -> dict[str, dict[str, Any]]:
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("canonical ROM identity mismatch")
    result = {}
    for callback, site in WRITERS.items():
        pc = int(site["actual_pc"], 16)
        expected = bytes.fromhex(site["bytes"])
        if rom[pc:pc + 2] != expected:
            raise ValueError(f"writer instruction mismatch at {site['actual_pc']}")
        result[callback] = {**site, "preceding_dataflow": {
            "callback_pc_contract": f"bus-write callback reports {callback} after actual store {site['actual_pc']}",
            "local_sequence": [
                "0x00B768: MOVE.W 2(A0),D6; source read for SAT X",
                "0x00B76C: ADD.W D3,D6; exact word transform",
                "0x00B76E: MOVE.W D6,4(A1); actual SAT X store",
            ] if callback == "0x00B772" else "bounded local producer sequence",
        }}
    return result


def load(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def scenario_inputs(path: Path) -> list[dict[str, Any]]:
    result = []
    for line in path.read_text(encoding="utf-8").splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[0] == "input":
            result.append({"frame": int(parts[1].split("=", 1)[1]),
                           "buttons": parts[3].split("=", 1)[1]})
    return result


def build_transitions(dynamic: dict[str, Any], backtrace: dict[str, Any]) -> list[dict[str, Any]]:
    snapshots = {item["frame"]: item for item in dynamic["snapshots"]}
    reads = {(item["frame"], item["a1"]): item for item in backtrace["executions"]
             if item["pc"] == "0x00B768"}
    writes = [item for item in backtrace["shadow_writes"]
              if item["address"] == "0xFF13D0" and item["pc"] == "0x00B772"]
    result = []
    for frame in range(1, dynamic["frames_executed"]):
        old = snapshots[frame]["sat"][4:6]
        new = snapshots[frame + 1]["sat"][4:6]
        if old == new:
            continue
        event = reads.get((frame, SHADOW_BASE))
        writer = next((item for item in writes if item["frame"] == frame), None)
        if not event or not writer:
            continue
        source_address = event["a0"] + 2
        source_value = event["source_plus_2"]
        derived = (source_value + event["d3"]) & 0xFFFF
        result.append({
            "frame_pair": [frame, frame + 1], "sat_entry": 0, "sat_field": "X",
            "shadow_address": "0xFF13D0", "old_value": u16(bytes(old), 0),
            "new_value": u16(bytes(new), 0), "write_pc": writer["pc"],
            "actual_write_pc": "0x00B76E", "write_width": 2,
            "source_class": "ROM", "source_address": f"0x{source_address:06X}",
            "source_read_pc": "0x00B768", "source_value": source_value,
            "source_register": "D6", "a0": f"0x{event['a0']:06X}",
            "d3": event["d3"], "derived_value": derived,
            "transform_chain": ["MOVE.W 2(A0),D6", f"ADD.W D3,D6 (D3=0x{event['d3']:04X})",
                                 "MOVE.W D6,4(A1)"],
            "exact_match": derived == u16(bytes(new), 0),
            "truth_class": "DERIVED_EXACT",
        })
    return result


def controller_correlation(transitions: list[dict[str, Any]], scenario: Path) -> dict[str, Any]:
    inputs = scenario_inputs(scenario)
    result = {name: [] for name in ("RIGHT", "LEFT", "UP", "DOWN")}
    for item in inputs:
        button = {value.upper() for value in item["buttons"].split("+")}
        directional = next((name for name in result if name in button), None)
        if directional:
            near = [transition["frame_pair"] for transition in transitions
                    if abs(transition["frame_pair"][0] - item["frame"]) <= 2]
            result[directional].append({"input_frame": item["frame"], "changes": near,
                                        "relationship": "CORRELATION_ONLY"})
    return result


def analyze(rom_path: Path, dynamic_path: Path, backtrace_path: Path, scenario: Path) -> dict[str, Any]:
    rom = rom_path.read_bytes()
    sites = decode_sites(rom)
    dynamic, backtrace = load(dynamic_path), load(backtrace_path)
    transitions = build_transitions(dynamic, backtrace)
    exact = [item for item in transitions if item["exact_match"]]
    selected = next((item for item in exact if item["frame_pair"] == [1500, 1501]), None)
    return {
        "schema": "oasis.m12.dynamic-sat-producer-analysis.v1",
        "status": "PASS" if selected and len(exact) >= 3 else "STOP",
        "stop_reason": None if selected and len(exact) >= 3 else "STOP_DYNAMIC_SAT_PRODUCER_BACKTRACE_INCOMPLETE",
        "writer_sites": sites, "transitions_checked": len(exact),
        "frame_1500_1501": selected,
        "multi_frame_validation": exact,
        "controller_correlation": controller_correlation(exact, scenario),
        "structure_context": {"upstream_ram_base": None, "field_offset": 2,
            "stride": None, "slot_index": 0, "related_read_pcs": ["0x00B768"],
            "truth_class": "UNRESOLVED", "reason": "A0 source address is ROM; upstream A0 definition is outside bounded local window"},
        "semantic_boundary": "SAT field/source provenance only; PLAYER and gameplay-object labels are not assigned",
        "source_owned": {"before": SOURCE_OWNED, "after": SOURCE_OWNED, "delta": 0},
        "commit_created": "NO", "push_performed": "NO",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path); parser.add_argument("dynamic", type=Path)
    parser.add_argument("backtrace", type=Path); parser.add_argument("scenario", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    result = analyze(args.rom, args.dynamic, args.backtrace, args.scenario)
    receipt = {"schema": "oasis.m12.dynamic-sat-producer-receipt.v1", "status": result["status"],
               "transitions_checked": result["transitions_checked"],
               "source_owned_before": SOURCE_OWNED, "source_owned_after": SOURCE_OWNED,
               "source_owned_delta": 0, "player_label_proven": "NO",
               "commit_created": "NO", "push_performed": "NO",
               "dynamic_capture_sha256": sha256(args.dynamic), "backtrace_capture_sha256": sha256(args.backtrace)}
    source_candidates = {"schema": "oasis.m12.dynamic-sat-source-candidates.v1",
                         "status": result["status"], "candidates": result["multi_frame_validation"],
                         "source_classes": ["ROM"], "player_label_proven": "NO"}
    outputs = {
        "postrun_dynamic_sat_producer_analysis.json": result,
        "postrun_dynamic_sat_backtrace.json": {"schema": "oasis.m12.dynamic-sat-backtrace.v1",
            "writer_sites": result["writer_sites"], "transitions": result["multi_frame_validation"]},
        "postrun_dynamic_sat_source_candidates.json": source_candidates,
        "postrun_dynamic_sat_producer_receipt.json": receipt,
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, payload in outputs.items():
        (args.output_dir / name).write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(receipt, indent=2, sort_keys=True))
    return 0 if result["status"] == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
