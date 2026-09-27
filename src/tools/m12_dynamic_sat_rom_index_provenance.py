"""Emit fail-closed M12 A0/ROM-index provenance artifacts."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
SOURCE_OWNED = 1_487_672
BASE_POINTER_PC = "0x00B6AA"
BASE_POINTER_ADDRESS = 0x03F326
BASE_POINTER_VALUE = 0x154B98


def load(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def a0_provenance() -> dict[str, Any]:
    return {
        "a0_value_at_00B768": "runtime-observed-per-entry",
        "a0_definitions": [
            {
                "path": "B730-entry runtime path",
                "a0_def_pc": None,
                "a0_def_instruction": "entry A0 value is consumed; no local A0 definition in 0x00B730..0x00B78C",
                "a0_base": None,
                "a0_offset": None,
                "a0_index_register": None,
                "classification": "UNRESOLVED",
                "boundary": "caller reaching definition is outside the bounded producer slice",
            },
            {
                "path": "separate setup entry at 0x00B6AA",
                "a0_def_pc": BASE_POINTER_PC,
                "a0_def_instruction": "MOVEA.L ($0003F326).L,A0",
                "a0_base": "ROM/RAM absolute read at 0x0003F326; ROM image word is 0x00154B98",
                "a0_offset": "ADD.W D0,D0; ADDA.W D0,A0; ADDA.W D5,A0; later ADDQ.L #6,A0",
                "a0_index_register": ["D0", "D5"],
                "classification": "OBSERVED_STATIC_VERIFIED",
                "boundary": "not observed executing on the accepted B730-tail capture; not promoted as its reaching definition",
            },
        ],
        "single_static_setup_path": {
            "def_pc": BASE_POINTER_PC,
            "instruction": "MOVEA.L ($0003F326).L,A0",
            "read_address": "0x0003F326",
            "decoded_rom_value": "0x00154B98",
            "dataflow": [
                "0x00B6BC ADD.W D0,D0",
                "0x00B6BE ADDA.W D0,A0",
                "0x00B6C8 ADDA.W D5,A0",
                "0x00B6CA MOVE.W (A0)+,D5",
                "0x00B724 ADDQ.L #6,A0",
            ],
        },
    }


def transitions(backtrace: dict[str, Any], producer: dict[str, Any]) -> list[dict[str, Any]]:
    runtime = [item for item in backtrace["executions"] if item["pc"] == "0x00B768"]
    by_frame: dict[int, list[dict[str, Any]]] = {}
    for item in runtime:
        by_frame.setdefault(item["frame"], []).append(item)
    result = []
    for item in producer["multi_frame_validation"]:
        frame = item["frame_pair"][0]
        entry = next((value for value in by_frame.get(frame, []) if value["a1"] == 16716748), None)
        if not entry:
            continue
        result.append({
            "frame": frame,
            "a0": f"0x{entry['a0']:06X}",
            "rom_source_address": item["source_address"],
            "record_index": None,
            "index_source": "UNRESOLVED",
            "sat_x_output": f"0x{item['new_value']:04X}",
            "source_value": f"0x{item['source_value']:04X}",
            "writer_pc": item["write_pc"],
            "truth_class": "OBSERVED_WITH_UNRESOLVED_A0_SELECTOR",
        })
    return result


def analyze(rom_path: Path, capture: Path, producer_path: Path) -> dict[str, Any]:
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("canonical ROM identity mismatch")
    capture_data = load(capture)
    producer = load(producer_path)
    rows = transitions(capture_data, producer)
    return {
        "schema": "oasis.m12.dynamic-sat-rom-index-provenance.v1",
        "status": "STOP",
        "stop_reason": "STOP_A0_REACHING_DEFINITION_FOR_B730_ENTRY_UNRESOLVED",
        "accepted_baseline": "PASS_DYNAMIC_SAT_PRODUCER_BACKTRACE_V1",
        "a0_provenance": a0_provenance(),
        "rom_address_decomposition": {
            "proven_source_address": "0x0017435C",
            "source_read_pc": "0x00B768",
            "field_offset_from_observed_a0": 2,
            "rom_table_base": None,
            "record_stride": None,
            "record_index": None,
            "classification": "UNRESOLVED",
            "reason": "B730 entry A0 selector construction is not yet closed",
        },
        "index_provenance": {
            "index_register": None,
            "index_value": None,
            "index_def_pc": None,
            "index_source_class": "UNRESOLVED",
            "index_source_address": None,
        },
        "selection_semantics": {
            "unique_rom_record_indices": None,
            "index_changes_across_frames": "UNRESOLVED",
            "correlation": "not assigned; SAT X changes are observed but selector causality is not closed",
        },
        "static_table_structure": {
            "table_range": None,
            "record_count": None,
            "record_stride": None,
            "fields_per_record": None,
            "observed_usage": [
                {"field_offset": 2, "consumer_pc": "0x00B768", "sat_destination_field": "X", "truth_class": "OBSERVED"},
                {"field_offset": 4, "consumer_pc": "0x00B772", "sat_destination_field": "not assigned here", "truth_class": "OBSERVED"},
            ],
        },
        "caller_parameter_provenance": {
            "caller_pcs": ["0x000DEA", "0x000E3A", "0x000EFE", "0x03B448", "0x03CE86"],
            "call_instructions": "JMP/JSR absolute 0x00B730",
            "input_registers": ["A0", "A1", "D0", "D1", "D2", "D3", "D4"],
            "parameter_registers": ["A0", "A1", "D0", "D1", "D2", "D3", "D4"],
            "parameter_source_pcs": None,
            "ram_derived_selector_affecting_a0": "UNRESOLVED",
        },
        "multi_frame_validation": rows,
        "transitions_checked": len(rows),
        "player_label_proven": "NO",
        "source_owned": {"before": SOURCE_OWNED, "after": SOURCE_OWNED, "delta": 0},
        "commit_created": "NO",
        "push_performed": "NO",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("capture", type=Path)
    parser.add_argument("producer", type=Path)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    report = analyze(args.rom, args.capture, args.producer)
    report_dir = args.output_dir
    report_dir.mkdir(parents=True, exist_ok=True)
    receipt = {
        "schema": "oasis.m12.dynamic-sat-rom-index-provenance-receipt.v1",
        "status": report["status"],
        "stop_reason": report["stop_reason"],
        "transitions_checked": report["transitions_checked"],
        "source_owned_before": SOURCE_OWNED,
        "source_owned_after": SOURCE_OWNED,
        "source_owned_delta": 0,
        "player_label_proven": "NO",
        "capture_sha256": sha256(args.capture),
        "producer_analysis_sha256": sha256(args.producer),
        "commit_created": "NO",
        "push_performed": "NO",
    }
    outputs = {
        "postrun_dynamic_sat_rom_index_analysis.json": report,
        "postrun_dynamic_sat_a0_provenance.json": report["a0_provenance"],
        "postrun_dynamic_sat_source_candidates.json": {
            "schema": "oasis.m12.dynamic-sat-rom-index-source-candidates.v1",
            "status": "STOP",
            "candidates": report["multi_frame_validation"],
            "unresolved_boundary": report["stop_reason"],
        },
        "postrun_dynamic_sat_rom_index_receipt.json": receipt,
    }
    for name, value in outputs.items():
        (report_dir / name).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(receipt, indent=2, sort_keys=True))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
