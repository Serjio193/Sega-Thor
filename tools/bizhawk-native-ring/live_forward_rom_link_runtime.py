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
from live_forward_worker_control import (
    CORE_RESERVE_CAP, NATIVE_BUDGET_CAP, PROCESS_BUDGET_CAP, SYSTEM_RESERVE,
    LiveWorkerControlPublisher, system_memory,
)
from live_forward_worker_control_model import (
    INT32_MAX, allocation_preflight, calculate_resource_budget,
    load_next_run_config,
)


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
    parser.add_argument("--control-window", action="store_true",
                        help="open the separate 2H Worker Control window")
    parser.add_argument("--next-run-config", type=Path,
        default=Path(__file__).parents[2] / "build" / "thor-evidence" /
                "live-worker-control" / "next-run.json")
    args = parser.parse_args()
    args.memory_bytes, args.rounds = WORKER_MEMORY, CYCLES_PER_WORKER
    resolve_runtime_paths(args)
    args.decoder = args.decoder.resolve()
    args.next_run_config = args.next_run_config.resolve()
    try:
        next_config, config_origin = load_next_run_config(args.next_run_config)
    except ValueError as error:
        parser.error(str(error))
    worker_count = next_config["worker_count"]
    worker_depth = next_config["chain_depth"]
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
    memory = system_memory()
    available = memory["system_available_ram_bytes"]
    budget = calculate_resource_budget(available,
        native_budget_cap=args.native_budget_bytes or NATIVE_BUDGET_CAP,
        process_budget_cap=args.process_budget_bytes or PROCESS_BUDGET_CAP,
        core_reserve_cap=args.core_reserve_bytes or CORE_RESERVE_CAP,
        system_reserve=args.system_reserve_bytes or SYSTEM_RESERVE,
        memory_bytes_each=WORKER_MEMORY)
    preflight = allocation_preflight(next_config, None,
        budget["native_budget_bytes"],
        "MANAGED_NATIVE_ARGUMENT_RANGE" if worker_count > INT32_MAX or
        worker_depth > INT32_MAX else "PENDING_NATIVE_PLANNER")
    if worker_count > INT32_MAX or worker_depth > INT32_MAX:
        preflight["rejection_reason"] = "MANAGED_NATIVE_ARGUMENT_RANGE"
        preflight["system_total_ram_bytes"] = memory["system_total_ram_bytes"]
        preflight["system_available_ram_bytes"] = available
        report_path = args.output_dir / "live-forward-rom-link-2b-receipt.json"
        report_path.write_text(json.dumps({"checkpoint": "M12-ROM-RANGE-LINKAGE-2B",
            "status": "STOP_WORKER_CONTROL_PREFLIGHT_REJECTED",
            "next_run_config": next_config, "config_origin": config_origin,
            "preflight": preflight, "runtime_started": False,
            "source_owned_delta": 0}, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps({"status": "STOP_WORKER_CONTROL_PREFLIGHT_REJECTED",
            "receipt": str(report_path.resolve())}, indent=2))
        return 2
    session = LiveForwardCartographer(rom_sha, _instrumentation_identity(args.install, args.script))
    linker = LiveForwardRomLinker(rom_sha)

    def on_segment(segment: dict[str, object], rows: list[tuple[int, ...]], data: bytes) -> None:
        session.admit(segment, rows, data)
        linker.stage(segment, rows, data)

    report_path = args.output_dir / "live-forward-rom-link-2b-receipt.json"
    runtime = projection = saved = None
    session_path = args.output_dir / "session-rom-link.sqlite"
    session_saved = False
    control = None
    try:
        on_status = None
        if args.control_window:
            control = LiveWorkerControlPublisher(
                args.output_dir / "live-worker-control-status.json",
                args.next_run_config,
                args.output_dir / "live-worker-control-preview.txt",
                args.output_dir)
            control.start()
            on_status = control.update
        runtime = run_one(args, "rom-link", worker_count, worker_depth,
                          on_segment, on_status)
        if control:
            control.finish(None, int(runtime.get("audited_segments", 0)))
        preflight = allocation_preflight(next_config, runtime.get("plan"),
            int(runtime.get("allocation_budget_bytes", budget["native_budget_bytes"])),
            str(runtime.get("preflight_reason")) if runtime.get("preflight_reason") else None)
        preflight["system_total_ram_bytes"] = memory["system_total_ram_bytes"]
        preflight["system_available_ram_bytes"] = available
        preflight["config_origin"] = config_origin
        if runtime.get("outcome") != "PASS" or runtime.get("audited_segments") != \
                worker_count * CYCLES_PER_WORKER:
            raise RuntimeError("STOP_ROM_LINK_RUNTIME_SEGMENT_AUDIT")
        if session.segments_admitted != worker_count * CYCLES_PER_WORKER or \
                session.segments_rejected or session.metrics()["source_owned_bytes"] != 0:
            raise RuntimeError("STOP_ROM_LINK_CARTOGRAPHER_SEGMENT_RECONCILIATION")
        projection = linker.project(session.graph, args.rom, args.decoder,
                                    args.output_dir / "rom-link-evidence")
        saved = session.save_closed(session_path, worker_count * CYCLES_PER_WORKER)
        session_saved = True
        audit_report = audit_rom_link(session_path, args.rom,
            Path(projection["flow_records_path"]), Path(projection["flow_segments_path"]),
            Path(projection["ranges_path"]), args.decoder,
            args.output_dir / "independent-rom-link-audit.json")
        status = projection["status"]
        report = {"checkpoint": "M12-ROM-RANGE-LINKAGE-2B", "status": status,
            "runtime": runtime, "rom_projection": projection, "saved_session": saved,
            "independent_audit": audit_report, "preflight": preflight,
            "next_run_config": next_config, "config_origin": config_origin,
            "control_window_enabled": args.control_window,
            "session_path": str(session_path.resolve()), "source_owned_delta": 0,
            "production_architecture_changed": False}
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(json.dumps({"status": status, "receipt": str(report_path.resolve()),
                          "session": str(session_path.resolve())}, indent=2))
        return 0 if status == "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE" else 2
    except Exception as error:
        if control:
            control.finish(None, session.segments_admitted, str(error))
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
            "admitted_segments": session.segments_admitted,
            "preflight": preflight, "next_run_config": next_config,
            "config_origin": config_origin, "control_window_enabled": args.control_window},
            indent=2, sort_keys=True) + "\n", encoding="utf-8")
        raise
    finally:
        session.close()


if __name__ == "__main__":
    raise SystemExit(main())
