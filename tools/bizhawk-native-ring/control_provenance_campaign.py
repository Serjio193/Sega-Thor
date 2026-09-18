#!/usr/bin/env python3
"""Run a bounded BizHawk FLOW_V1 capability or canonical witness campaign."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from auto67_control_provenance import ROM_SHA, analyze_flow_segment
from control_provenance_audit import audit_segment
from live_forward_scaling_runtime import run_one


def micro_rom() -> bytes:
    """Small 68000 loop that executes a ROM pointer call and offset jump."""
    rom = bytearray(0x1000)
    rom[0:4] = (0x00FF0000).to_bytes(4, "big")
    rom[4:8] = (0x00000200).to_bytes(4, "big")
    program = bytes.fromhex(
        "207900000500"      # MOVEA.L ($500).L,A0
        "4E90"              # JSR (A0)
        "207C00000300"      # MOVEA.L #$300,A0
        "303900000510"      # MOVE.W ($510).L,D0
        "48C0"              # EXT.L D0
        "D1C0"              # ADDA.L D0,A0
        "4ED0"              # JMP (A0)
    )
    rom[0x200:0x200 + len(program)] = program
    rom[0x300:0x302] = bytes.fromhex("4E75")
    rom[0x500:0x504] = (0x00000300).to_bytes(4, "big")
    rom[0x510:0x512] = bytes.fromhex("0280")
    rom[0x580:0x586] = bytes.fromhex("4EF900000200")
    return bytes(rom)


def _append(path: Path, value: dict[str, object]) -> None:
    with path.open("a", encoding="utf-8", newline="\n") as target:
        target.write(json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n")


def run(args: argparse.Namespace) -> dict[str, object]:
    args.install = args.install.resolve()
    args.script = args.script.resolve()
    args.output_dir = args.output_dir.resolve()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    if args.mode == "micro":
        args.rom = args.output_dir / "control-provenance-micro-rom.md"
        args.rom.write_bytes(micro_rom())
    args.rom = args.rom.resolve()
    if not args.install.joinpath("EmuHawk.exe").is_file():
        raise FileNotFoundError("isolated BizHawk EmuHawk.exe is missing")
    raw_rom = args.rom.read_bytes()
    rom_hash = hashlib.sha256(raw_rom).hexdigest()
    if args.mode == "canonical" and rom_hash != ROM_SHA:
        raise ValueError("STOP_CONTROL_PROVENANCE_ROM_IDENTITY_MISMATCH")
    event_path = args.output_dir / f"{args.mode}-control-provenance.jsonl"
    audit_path = args.output_dir / f"{args.mode}-control-provenance-audit.jsonl"
    event_path.unlink(missing_ok=True)
    audit_path.unlink(missing_ok=True)
    totals = {"runtime_indirect_consumers": 0, "audited_facts": 0,
              "pointer_relations": 0, "offset_relations": 0,
              "jump_table_entries": 0, "false_provenance_detections": 0,
              "flow_mismatches": 0, "identity_conflicts": 0}

    def on_segment(segment, rows, records_blob):
        analysis = analyze_flow_segment(segment, rows, raw_rom, rom_hash)
        if analysis["status"] != "PASS_CONTROL_PROVENANCE_V1":
            raise ValueError(analysis["status"])
        audited = audit_segment(raw_rom, segment, rows, analysis,
                                ROM_SHA if args.mode == "canonical" else rom_hash)
        for key in totals:
            totals[key] += int(audited[key])
        _append(event_path, {"segment_identity": analysis["segment_identity"],
                             "status": analysis["status"],
                             "consumers": analysis["consumers"]})
        _append(audit_path, {"segment_identity": analysis["segment_identity"], **audited})

    runtime_args = SimpleNamespace(
        install=args.install, rom=args.rom, script=args.script,
        output_dir=args.output_dir / args.mode, rounds=100,
        memory_bytes=65536, native_budget_bytes=256 * 1024 * 1024,
        process_budget_bytes=512 * 1024 * 1024,
        core_reserve_bytes=128 * 1024 * 1024,
        system_reserve_bytes=4 * 1024 * 1024 * 1024,
        max_frames=args.max_frames, timeout=args.timeout)
    receipt = run_one(runtime_args, "natural", 1, 20, on_segment)
    receipt["mode"] = args.mode
    receipt["rom_sha256"] = rom_hash
    receipt["control_provenance_totals"] = totals
    receipt["analysis_jsonl"] = str(event_path)
    receipt["independent_audit_jsonl"] = str(audit_path)
    receipt["status"] = "PASS_CONTROL_PROVENANCE_MICRO" if args.mode == "micro" else \
        "PASS_CONTROL_PROVENANCE_CANONICAL_WITNESS" if totals["audited_facts"] else \
        "PASS_CONTROL_PROVENANCE_CANONICAL_WITNESS_NONE"
    result_path = args.output_dir / f"{args.mode}-control-provenance-campaign.json"
    result_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("micro", "canonical"), required=True)
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--script", type=Path,
        default=ROOT / "tools/bizhawk-native-ring/live_forward_scaling.lua")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--max-frames", type=int, default=1800)
    parser.add_argument("--timeout", type=int, default=1800)
    args = parser.parse_args()
    if args.mode == "canonical" and args.rom is None:
        parser.error("--rom is required for canonical mode")
    result = run(args)
    print(json.dumps({"status": result["status"],
                      "audited_segments": result["audited_segments"],
                      "control_provenance_totals": result["control_provenance_totals"]}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
