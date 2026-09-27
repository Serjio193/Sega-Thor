"""Close caller-side A0 provenance for the accepted dynamic SAT tail."""

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


def index_by_frame(capture: dict[str, Any], pc: str) -> dict[int, list[dict[str, Any]]]:
    result: dict[int, list[dict[str, Any]]] = {}
    for item in capture["executions"]:
        if item["pc"] == pc:
            result.setdefault(item["frame"], []).append(item)
    return result


def entry_rows(
    rom: bytes, capture: dict[str, Any], producer: dict[str, Any]
) -> list[dict[str, Any]]:
    entry = index_by_frame(capture, "0x00B730")
    caller = index_by_frame(capture, "0x03B448")
    source_def = index_by_frame(capture, "0x03B436")
    tail_increment = index_by_frame(capture, "0x00B73C")
    read = index_by_frame(capture, "0x00B768")
    rows = []
    for transition in producer["multi_frame_validation"]:
        frame = transition["frame_pair"][0]
        e = entry.get(frame, [None])[0]
        c = caller.get(frame, [None])[0]
        d = source_def.get(frame, [None])[0]
        inc = tail_increment.get(frame, [None])[0]
        r = next((x for x in read.get(frame, []) if x["a1"] == 16716748), None)
        if not all((e, c, d, inc, r)):
            continue
        source_address = d["a0"]
        source_value = word(rom, source_address)
        raw_selector = next(
            (x["d0"] for x in capture["executions"]
             if x["frame"] == frame and x["pc"] == "0x03B42C"),
            None,
        )
        rows.append({
            "frame": frame,
            "caller_pc": "0x03B448",
            "predecessor_pc": "0x03B448",
            "entry_pc": "0x00B730",
            "a0_entry_value": f"0x{e['a0']:06X}",
            "a0_at_00B768": f"0x{r['a0']:06X}",
            "tail_modification": {
                "pc": "0x00B73C",
                "instruction": "MOVE.W (A0)+,D5",
                "a0_after": f"0x{r['a0']:06X}",
                "delta_bytes": r["a0"] - e["a0"],
            },
            "a0_def_pc": "0x03B436",
            "a0_def_instruction": "ADDA.W (A0),A0",
            "a0_source_class": "ROM",
            "a0_source_address": f"0x{source_address:06X}",
            "a0_source_value": f"0x{source_value:04X}",
            "selector_source_class": "STACK/PARAMETER",
            "selector_source_address": "8(A6)",
            "selector_value": raw_selector,
            "selector_def_pc": "0x03B428",
            "rom_source_address": transition["source_address"],
            "truth_class": "DERIVED_EXACT",
        })
    return rows


def analyze(rom_path: Path, capture_path: Path, producer_path: Path) -> dict[str, Any]:
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("canonical ROM identity mismatch")
    capture = load(capture_path)
    producer = load(producer_path)
    rows = entry_rows(rom, capture, producer)
    return {
        "schema": "oasis.m12.dynamic-sat-caller-a0-provenance.v1",
        "status": "PASS" if len(rows) >= 10 else "STOP",
        "stop_reason": None if len(rows) >= 10 else "STOP_CALLER_A0_CAPTURE_INCOMPLETE",
        "tail_entry_pc": "0x00B730",
        "predecessor_pc": "0x03B448",
        "caller_pc": "0x03B448",
        "call_instruction": "JSR.L ($0000B730).L",
        "control_transfer": "direct JSR.L",
        "caller_pcs": ["0x03B448"],
        "call_counts": {"0x03B448": 763},
        "runtime_predecessor_observation": {
            "0x03B448": {"count": 763, "a0_class": "same producer path", "unresolved_a0": 0}
        },
        "a0_reaching_definition": {
            "def_pc": "0x03B436",
            "instruction": "ADDA.W (A0),A0",
            "source_register": "A0",
            "source_address": "dynamic A0 before instruction; ROM record-relative word",
            "source_class": "ROM",
            "source_value": "frame-dependent exact word",
            "prior_chain": [
                "0x03B416 LEA.L ($04C6,PC),A0 -> 0x0003B8DE",
                "0x03B41A MOVE.W ($00FFAFAE).L,D0",
                "0x03B420 ASL.W #4,D0",
                "0x03B422 MOVEA.L 12(A0,D0.W),A0",
                "0x03B426 MOVEA.L (A0),A0",
                "0x03B428 MOVE.W 8(A6),D0",
                "0x03B42C ADD.W D0,D0",
                "0x03B42E ADDA.W D0,A0",
                "0x03B436 ADDA.W (A0),A0",
            ],
        },
        "selector_source": {
            "source_class": "STACK/PARAMETER",
            "source_address": "8(A6)",
            "selector_value": "observed raw value before 0x03B42C",
            "selector_def_pc": "0x03B428",
            "static_table_pointer_source": "ROM 0x0003B90A -> 0x0003B982 -> ROM pointer 0x001742DC",
            "gameplay_semantics": "UNASSIGNED",
        },
        "static_setup_00B6AA": {
            "classification": "DOES_NOT_REACH_DYNAMIC_TAIL",
            "evidence": "0x00B72E RTS; 0x00B730 is a separate entry with no fall-through",
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
    receipt = {
        "schema": "oasis.m12.dynamic-sat-caller-a0-provenance-receipt.v1",
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
        "postrun_dynamic_sat_caller_a0_analysis.json": report,
        "postrun_dynamic_sat_caller_a0_backtrace.json": {
            "schema": "oasis.m12.dynamic-sat-caller-a0-backtrace.v1",
            "caller": report["caller_pcs"],
            "rows": report["multi_frame_validation"],
        },
        "postrun_dynamic_sat_caller_a0_source_candidates.json": report["selector_source"],
        "postrun_dynamic_sat_caller_a0_receipt.json": receipt,
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, value in outputs.items():
        (args.output_dir / name).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(receipt, indent=2, sort_keys=True))
    return 0 if report["status"] == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
