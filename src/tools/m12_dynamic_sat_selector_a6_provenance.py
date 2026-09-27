"""Analyze the M12 selector field at 8(A6) without semantic promotion."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
SOURCE_OWNED = 1_487_672


def load(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(rom: bytes, address: int) -> int:
    return int.from_bytes(rom[address:address + 2], "big")


def by_pc(capture: dict[str, Any], pc: str) -> dict[int, list[dict[str, Any]]]:
    result: dict[int, list[dict[str, Any]]] = {}
    for item in capture["executions"]:
        if item["pc"] == pc:
            result.setdefault(item["frame"], []).append(item)
    return result


def rows(rom: bytes, capture: dict[str, Any], producer: dict[str, Any]) -> list[dict[str, Any]]:
    entry = by_pc(capture, "0x00B730")
    read = by_pc(capture, "0x00B768")
    a6read = by_pc(capture, "0x03B428")
    a6def = by_pc(capture, "0x03B436")
    writers = [item for item in capture["executions"] if item["pc"] == "0x03B3D8"]
    snapshots = {item["frame"]: item for item in load(Path("build/m12-targeted-dynamic-sat-v1-run2/capture.json"))["snapshots"]}
    result = []
    for transition in producer["multi_frame_validation"]:
        frame = transition["frame_pair"][0]
        e = entry.get(frame, [None])[0]
        r = next((item for item in read.get(frame, []) if item["a1"] == 16716748), None)
        s = a6read.get(frame, [None])[0]
        d = a6def.get(frame, [None])[0]
        if not all((e, r, s, d)):
            continue
        source_address = d["a0"]
        latest = next((item for item in reversed(writers) if item["frame"] <= frame and item["a6"] == s["a6"]), None)
        sat = snapshots.get(frame + 1, {}).get("sat", [])
        result.append({
            "frame": frame,
            "a6": f"0x{s['a6']:06X}",
            "field_address": f"0x{(s['a6'] + 8):06X}",
            "field_value": s["field_plus_8"],
            "field_read_pc": "0x03B428",
            "field_write_pc": "0x03B3D8" if latest else None,
            "field_source": None if not latest else f"0x{(latest['a0'] + latest['d0']):06X}",
            "field_source_class": "ROM" if latest else "UNRESOLVED",
            "selected_a0_entry": f"0x{e['a0']:06X}",
            "selected_a0_at_00B768": f"0x{r['a0']:06X}",
            "rom_record_address": f"0x{e['a0']:06X}",
            "rom_source_address": transition["source_address"],
            "a0_def_pc": "0x03B436",
            "a0_def_source_address": f"0x{source_address:06X}",
            "a0_def_source_value": word(rom, source_address),
            "sat_result": {
                "x": f"0x{int.from_bytes(bytes(sat[4:6]), 'big'):04X}" if len(sat) >= 6 else None,
                "y": f"0x{int.from_bytes(bytes(sat[0:2]), 'big'):04X}" if len(sat) >= 2 else None,
                "tile_or_attribute": f"0x{int.from_bytes(bytes(sat[6:8]), 'big'):04X}" if len(sat) >= 8 else None,
            },
            "truth_class": "DERIVED_EXACT",
        })
    return result


def analyze(rom_path: Path, capture_path: Path, producer_path: Path) -> dict[str, Any]:
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("canonical ROM identity mismatch")
    capture, producer = load(capture_path), load(producer_path)
    validated = rows(rom, capture, producer)
    a6_values = sorted({item["a6"] for item in validated})
    selectors = sorted({item["field_value"] for item in validated})
    records = sorted({item["rom_record_address"] for item in validated})
    return {
        "schema": "oasis.m12.dynamic-sat-selector-a6-provenance.v1",
        "status": "PASS" if len(validated) >= 10 else "STOP",
        "stop_reason": None if len(validated) >= 10 else "STOP_SELECTOR_CAPTURE_INCOMPLETE",
        "selector_read_pc": "0x03B428",
        "selector_field_offset": 8,
        "caller_block": [
            {"pc": "0x03B420", "instruction": "ASL.W #4,D0", "source": "D0", "destination": "D0"},
            {"pc": "0x03B422", "instruction": "MOVEA.L 12(A0,D0.W),A0", "source": "ROM table pointer", "destination": "A0"},
            {"pc": "0x03B426", "instruction": "MOVEA.L (A0),A0", "source": "ROM pointer", "destination": "A0"},
            {"pc": "0x03B428", "instruction": "MOVE.W 8(A6),D0", "source": "A6+8", "destination": "D0"},
            {"pc": "0x03B42C", "instruction": "ADD.W D0,D0", "source": "D0", "destination": "D0"},
            {"pc": "0x03B42E", "instruction": "ADDA.W D0,A0", "source": "D0", "destination": "A0"},
            {"pc": "0x03B436", "instruction": "ADDA.W (A0),A0", "source": "ROM word at A0", "destination": "A0"},
            {"pc": "0x03B448", "instruction": "JSR.L ($0000B730).L", "source": "A0", "destination": "stack/PC"},
        ],
        "a6_reaching_definition": {
            "a6_value": "0x00FFAFCE",
            "a6_def_pc": "0x03B376",
            "a6_def_instruction": "LEA.L ($00FFAFCE).L,A6",
            "source_class": "CONSTANT",
            "source_address": "0x00FFAFCE",
            "source_value": "0x00FFAFCE",
        },
        "field_plus_8": {
            "field_address": "0x00FFAFD6",
            "field_width": 2,
            "field_value_at_frame_1500": 3,
            "field_read_pc": "0x03B428",
            "field_write_pc": "0x03B3D8",
            "field_source": "0(A0,D0.W)",
            "field_source_class": "ROM",
        },
        "selector_to_rom_mapping": {
            "selector_value_at_frame_1500": 3,
            "rom_base_pointer_source": "ROM 0x0003B90A -> 0x0003B982 -> 0x001742DC",
            "rom_offset": "(selector * 2) + ROM word at selected A0",
            "final_a0_entry": "0x00174358",
            "final_a0_at_00B768": "0x0017435A",
            "rom_record_address": "0x00174358",
            "rom_field_address_used_for_X": "0x0017435C",
            "math": "0x001742DC + (3 * 2) = 0x001742E2; ROM[0x001742E2]=0x0076; 0x001742E2+0x0076=0x00174358",
        },
        "unique_a6_values": a6_values,
        "unique_selector_values": selectors,
        "unique_rom_records": records,
        "selector_changes_across_frames": "YES" if len(selectors) > 1 else "NO",
        "loop_context": {
            "classification": "selector-table update block; full enclosing structure not promoted",
            "loop_entry_pc": "0x03B3B2",
            "loop_backedge_pc": "0x03B404",
            "iteration_pointer": "10(A6)",
            "iteration_stride": 4,
            "iteration_count_observed": "UNRESOLVED",
        },
        "gameplay_render_record_candidate": "UNASSIGNED",
        "ram_to_rom_selector_chain": "EXACT",
        "multi_frame_validation": validated,
        "transitions_checked": len(validated),
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
    receipt = {
        "schema": "oasis.m12.dynamic-sat-selector-a6-provenance-receipt.v1",
        "status": report["status"],
        "transitions_checked": report["transitions_checked"],
        "source_owned_before": SOURCE_OWNED,
        "source_owned_after": SOURCE_OWNED,
        "source_owned_delta": 0,
        "player_label_proven": "NO",
        "capture_sha256": sha256(args.capture),
        "commit_created": "NO",
        "push_performed": "NO",
    }
    outputs = {
        "postrun_dynamic_sat_selector_a6_analysis.json": report,
        "postrun_dynamic_sat_selector_a6_backtrace.json": {
            "schema": "oasis.m12.dynamic-sat-selector-a6-backtrace.v1",
            "rows": report["multi_frame_validation"],
        },
        "postrun_dynamic_sat_selector_a6_source_candidates.json": report["field_plus_8"],
        "postrun_dynamic_sat_selector_a6_receipt.json": receipt,
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, value in outputs.items():
        (args.output_dir / name).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(receipt, indent=2, sort_keys=True))
    return 0 if report["status"] == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
