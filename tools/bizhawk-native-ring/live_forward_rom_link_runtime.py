#!/usr/bin/env python3
"""Run one bounded Worker-1B session and link audited FLOW_V1 records to ROM."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

EVIDENCE_DIR = Path(__file__).parents[2] / "src" / "tools" / "thor_evidence"
sys.path.insert(0, str(EVIDENCE_DIR))
sys.path.insert(0, str(EVIDENCE_DIR.parent))

from live_forward_cartographer import LiveForwardCartographer
from live_forward_rom_link import LiveForwardRomLinker
from live_forward_rom_link_audit import audit as audit_rom_link
from live_forward_scaling_runtime import resolve_runtime_paths, run_one
from identity import ROM_SHA, ROM_SIZE


WORKER_COUNT = 16
CYCLES_PER_WORKER = 100
WORKER_DEPTH = 20
WORKER_MEMORY = 64 * 1024


def _instrumentation_identity(install: Path, script: Path) -> str:
    inputs = {"profile": "FLOW_V1", "worker": "M12-AUTO67-LIVE-FORWARD-WORKER-1B",
        "wbx_sha256": hashlib.sha256((install / "dll" / "gpgx.wbx").read_bytes()).hexdigest(),
        "lua_sha256": hashlib.sha256(script.read_bytes()).hexdigest(),
        "trace_contract_sha256": hashlib.sha256(
            Path(__file__).with_name("live_forward_trace.h").read_bytes()).hexdigest()}
    encoded = json.dumps(inputs, sort_keys=True, separators=(",", ":")).encode("ascii")
    return hashlib.sha256(encoded).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--decoder", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--native-budget-bytes", type=int, default=256 * 1024 * 1024)
    parser.add_argument("--process-budget-bytes", type=int, default=512 * 1024 * 1024)
    parser.add_argument("--core-reserve-bytes", type=int, default=128 * 1024 * 1024)
    parser.add_argument("--system-reserve-bytes", type=int, default=4 * 1024 * 1024 * 1024)
    parser.add_argument("--max-frames", type=int, default=1800)
    parser.add_argument("--timeout", type=int, default=1800)
    args = parser.parse_args()
    args.memory_bytes, args.rounds = WORKER_MEMORY, CYCLES_PER_WORKER
    resolve_runtime_paths(args)
    args.decoder = args.decoder.resolve()
    if args.output_dir.exists():
        parser.error(f"campaign output already exists; preserve it and choose a new path: {args.output_dir}")
    if not (args.install / "EmuHawk.exe").is_file() or not args.rom.is_file() or \
            not args.script.is_file() or not (args.install / "dll" / "gpgx.wbx").is_file() or \
            not args.decoder.is_file():
        parser.error("install, ROM, Lua script, GPGX WBX or native decoder does not exist")
    rom_sha = hashlib.sha256(args.rom.read_bytes()).hexdigest()
    if args.rom.stat().st_size != ROM_SIZE or rom_sha != ROM_SHA:
        parser.error("canonical USA ROM size or SHA-256 mismatch")
    args.output_dir.mkdir(parents=True)
    session = LiveForwardCartographer(rom_sha, _instrumentation_identity(args.install, args.script))
    linker = LiveForwardRomLinker(rom_sha)

    def on_segment(segment: dict[str, object], rows: list[tuple[int, ...]], data: bytes) -> None:
        session.admit(segment, rows, data)
        linker.stage(segment, rows, data)

    report_path = args.output_dir / "live-forward-rom-link-2b-receipt.json"
    runtime = projection = saved = None
    session_path = args.output_dir / "session-rom-link.sqlite"
    session_saved = False
    try:
        runtime = run_one(args, "rom-link", WORKER_COUNT, WORKER_DEPTH, on_segment)
        if runtime.get("outcome") != "PASS" or runtime.get("audited_segments") != \
                WORKER_COUNT * CYCLES_PER_WORKER:
            raise RuntimeError("STOP_ROM_LINK_RUNTIME_SEGMENT_AUDIT")
        if session.segments_admitted != WORKER_COUNT * CYCLES_PER_WORKER or \
                session.segments_rejected or session.metrics()["source_owned_bytes"] != 0:
            raise RuntimeError("STOP_ROM_LINK_CARTOGRAPHER_SEGMENT_RECONCILIATION")
        projection = linker.project(session.graph, args.rom, args.decoder,
                                    args.output_dir / "rom-link-evidence")
        saved = session.save_closed(session_path, WORKER_COUNT * CYCLES_PER_WORKER)
        session_saved = True
        audit_report = audit_rom_link(session_path, args.rom,
            Path(projection["flow_records_path"]), Path(projection["flow_segments_path"]),
            Path(projection["ranges_path"]), args.decoder,
            args.output_dir / "independent-rom-link-audit.json")
        status = projection["status"]
        report = {"checkpoint": "M12-ROM-RANGE-LINKAGE-2B", "status": status,
            "runtime": runtime, "rom_projection": projection, "saved_session": saved,
            "independent_audit": audit_report,
            "session_path": str(session_path.resolve()), "source_owned_delta": 0,
            "production_architecture_changed": False}
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps({"status": status, "receipt": str(report_path.resolve()),
                          "session": str(session_path.resolve())}, indent=2))
        return 0 if status == "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE" else 2
    except Exception as error:
        status = (projection or {}).get("status")
        if not status and runtime is None and session.segments_admitted == 0:
            status = "STOP_ROM_LINK_EMUHAWK_STARTUP"
        status = status or str(error).split(":", 1)[0]
        if not status.startswith("STOP_ROM_LINK_"):
            status = "STOP_ROM_LINK_RUNTIME_OR_AUDIT_FAILURE"
        report_path.write_text(json.dumps({"checkpoint": "M12-ROM-RANGE-LINKAGE-2B",
            "status": status, "error": str(error), "runtime": runtime,
            "rom_projection": projection, "saved_session": saved,
            "session_path": str(session_path.resolve()), "session_saved": session_saved,
            "admitted_segments": session.segments_admitted},
            indent=2, sort_keys=True) + "\n", encoding="utf-8")
        raise
    finally:
        session.close()


if __name__ == "__main__":
    raise SystemExit(main())
