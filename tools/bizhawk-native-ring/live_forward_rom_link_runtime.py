#!/usr/bin/env python3
"""Run a bounded or interactive Worker-1B session and retain validated FLOW_V1 evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

EVIDENCE_DIR = Path(__file__).parents[2] / "src" / "tools" / "thor_evidence"
sys.path.insert(0, str(EVIDENCE_DIR))
sys.path.insert(0, str(EVIDENCE_DIR.parent))

from live_forward_cartographer import LiveForwardCartographer
from live_forward_rom_link import LiveForwardRomLinker
from live_forward_rom_link_audit import audit as audit_rom_link
from live_forward_scaling_runtime import resolve_runtime_paths, run_one
from live_forward_segment_spool import LiveForwardSegmentSpool
from live_session_progress import LiveSessionProgressPublisher
from flow_handoff_runtime import FlowHandoffRuntime
from identity import ROM_SHA, ROM_SIZE
from live_forward_worker_control import (
    CORE_RESERVE_CAP, NATIVE_BUDGET_CAP, PROCESS_BUDGET_CAP, SYSTEM_RESERVE,
    LiveWorkerControlPublisher, system_memory,
)
from live_forward_worker_control_model import (
    INT32_MAX, allocation_preflight, calculate_resource_budget,
    load_next_run_config,
)


WORKER_COUNT = 128
CYCLES_PER_WORKER = 100
WORKER_DEPTH = 512
WORKER_MEMORY = 512 * 1024
POSTRUN_ELIGIBLE_OUTCOMES = frozenset({
    "STOPPED_AFTER_EMUHAWK_EXIT", "STOPPED_DISK_RESERVE", "STOPPED_END_GAME",
    "STOPPED_FRAME_LIMIT",
})


def should_launch_postrun_analysis(status: str) -> bool:
    """Only analyze a sealed interactive run with a terminal stop outcome."""
    return status in POSTRUN_ELIGIBLE_OUTCOMES


def _instrumentation_identity(install: Path, script: Path) -> str:
    inputs = {"profile": "FLOW_V1", "worker": "M12-AUTO67-LIVE-FORWARD-WORKER-1B",
        "wbx_sha256": hashlib.sha256((install / "dll" / "gpgx.wbx").read_bytes()).hexdigest(),
        "lua_sha256": hashlib.sha256(script.read_bytes()).hexdigest(),
        "trace_contract_sha256": hashlib.sha256(
            Path(__file__).with_name("live_forward_trace.h").read_bytes()).hexdigest()}
    encoded = json.dumps(inputs, sort_keys=True, separators=(",", ":")).encode("ascii")
    return hashlib.sha256(encoded).hexdigest()


def _run_postrun_in_process(receipt: Path, output_dir: Path, rom: Path,
                            decoder: Path, range_tool: Path, startup_state, master_root: Path,
                            flow_runtime: FlowHandoffRuntime | None = None):
    """Run the coordinator in this runtime process; files are diagnostics only."""
    from live_forward_postrun_coordinator import (PostRunContext, PostRunCoordinator,
                                                   ProgressEventSink)
    from master_v2_runtime_bridge import (build_contribution, promote)
    analysis_dir = output_dir / "post-run-analysis"
    analysis_dir.mkdir(parents=True, exist_ok=True)
    status = analysis_dir / "status.json"
    report = analysis_dir / "report.json"
    window = Path(__file__).with_name("live_forward_postrun_window.py")
    python_gui = Path(sys.executable).with_name("pythonw.exe")
    gui_exec = str(python_gui) if python_gui.is_file() else sys.executable
    flags = 0 if python_gui.is_file() else getattr(subprocess, "CREATE_NO_WINDOW", 0)
    subprocess.Popen([gui_exec, str(window), "--status", str(status),
                      "--report", str(report)], cwd=str(Path(__file__).parent),
                     creationflags=flags)
    sealed = json.loads(receipt.read_text(encoding="utf-8"))
    runtime = sealed["runtime"]
    source = sealed.get("flow_handoff") or sealed.get("raw_segment_spool", {})
    contribution = build_contribution(
        receipt, flow_runtime.stage5_memory if flow_runtime else None,
        flow_runtime.stats() if flow_runtime else None)
    context = PostRunContext(
        run_id=int(runtime["run_id"]), rom_path=rom, master_root=master_root,
        receipt_path=receipt, decoder=decoder, range_tool=range_tool, startup=startup_state,
        segment_total=int(source["segments"]), configuration=sealed.get("next_run_config", {}),
        flow_session=(flow_runtime.stage5_memory if flow_runtime else None),
        flow_stats=(flow_runtime.stats() if flow_runtime else None),
        flow_stage6=(flow_runtime.stage6_result if flow_runtime else None))
    sink = ProgressEventSink(context.run_id, status)
    coordinator = PostRunCoordinator()
    thread = coordinator.run_background(context, sink, report)
    thread.join()
    result = coordinator.result
    if result is not None and result.complete and startup_state is not None:
        stage5 = result.stages.get("REFRESHING MAP", {})
        canonical_generation = Path(str(stage5["generation_dir"])).resolve()
        canonical_root = canonical_generation.parents[1]
        promotion = promote(startup_state.master_path,
                            startup_state.pointer.path, master_root, canonical_root,
                            output_dir, receipt, rom, int(runtime["run_id"]), contribution)
        result.report["master_v2_promotion"] = promotion
        report.write_text(json.dumps(result.report, indent=2, sort_keys=True) + "\n",
                          encoding="utf-8")
        shutil.rmtree(master_root, ignore_errors=True)
    return result, status, report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    default_install = Path(os.getenv("BIZHAWK_INSTALL", r"C:\Dev\SegaThorTools\BizHawk-m12-w2-1-frame-coherent-20260920"))
    default_rom = Path(os.getenv("BEYOND_OASIS_ROM", str(Path(__file__).parents[2] / "local-roms" / "Beyond Oasis (USA).md")))
    default_script = Path(__file__).with_name("live_forward_scaling.lua")
    default_config = Path(__file__).parents[2] / "build" / "thor-evidence" / "live-worker-control" / "next-run.json"

    parser.add_argument("--install", type=Path, default=default_install,
                        help=f"BizHawk install directory (default: {default_install})")
    parser.add_argument("--rom", type=Path, default=default_rom,
                        help=f"Beyond Oasis canonical ROM (default: {default_rom})")
    parser.add_argument("--script", type=Path, default=default_script,
                        help=f"Lua harness script (default: {default_script})")
    parser.add_argument("--decoder", type=Path, default=None,
                        help="Legacy PC-list decoder (optional, only for legacy 2B campaign)")
    parser.add_argument("--range-tool", type=Path, default=None,
                        help="Legacy Stage 7 range decoder (optional)")
    parser.add_argument("--output-dir", type=Path, default=None,
                        help="Output directory (default: build/thor-evidence/live-worker-control/session-<timestamp>)")
    parser.add_argument("--native-budget-bytes", type=int, default=256 * 1024 * 1024)
    parser.add_argument("--process-budget-bytes", type=int, default=512 * 1024 * 1024)
    parser.add_argument("--core-reserve-bytes", type=int, default=128 * 1024 * 1024)
    parser.add_argument("--system-reserve-bytes", type=int, default=1 * 1024 * 1024 * 1024)
    parser.add_argument("--max-frames", type=int, default=1800)
    parser.add_argument("--max-total-frames", type=int, default=0,
                        help="Stop continuous FLOW capture after this total frame count (0 disables)")
    parser.add_argument("--timeout", type=int, default=1800)
    parser.add_argument("--cadence", type=int, default=300,
                        help="Inter-wave frame cadence (default: 300)")
    parser.add_argument("--natural-input", action=argparse.BooleanOptionalAction, default=False,
                        help="Enable automated natural controller input schedule (default: False)")
    parser.add_argument("--control-window", action=argparse.BooleanOptionalAction, default=True,
                        help="Open the separate 2H Worker Control desktop window (default: True)")
    parser.add_argument("--until-closed", action=argparse.BooleanOptionalAction, default=True,
                        help="Run validated Worker cycles until BizHawk closes or END GAME (default: True)")
    parser.add_argument("--master-startup", action="store_true",
                        help="Validate MASTER V2 startup authority before launching runtime (legacy)")
    parser.add_argument("--in-process-postrun", action=argparse.BooleanOptionalAction, default=True,
                        help="Run in-process Stage 1-9 post-run analysis and display monitor window (default: True)")
    parser.add_argument("--workers", type=int, default=None,
                        help="Worker count override (default: from next-run.json or 128)")
    parser.add_argument("--depth", type=int, default=None,
                        help="Chain depth override (default: from next-run.json or 512)")
    parser.add_argument("--memory-bytes", type=int, default=WORKER_MEMORY,
                        help=f"Worker memory bytes (default: {WORKER_MEMORY})")
    parser.add_argument("--next-run-config", type=Path, default=default_config)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.output_dir is None:
        timestamp = time.strftime("%Y%m%d-%H%M%S")
        args.output_dir = Path(__file__).parents[2] / "build" / "thor-evidence" / "live-worker-control" / f"session-{timestamp}"
    if not args.until_closed and args.decoder is None:
        parser.error("--decoder is required for bounded 2B campaign mode")
    default_range_tool = Path(__file__).parents[2] / "build" / "oasis_re_assemble_range.exe"
    if args.range_tool is None and default_range_tool.is_file():
        args.range_tool = default_range_tool
    args.memory_bytes = args.memory_bytes or WORKER_MEMORY
    args.rounds = 0 if args.until_closed else CYCLES_PER_WORKER
    resolve_runtime_paths(args)
    if args.decoder is not None:
        args.decoder = args.decoder.resolve()
    if args.range_tool is not None:
        args.range_tool = args.range_tool.resolve()
    args.next_run_config = args.next_run_config.resolve()
    try:
        next_config, config_origin = load_next_run_config(args.next_run_config)
    except ValueError as error:
        parser.error(str(error))
    worker_count = args.workers if args.workers is not None else next_config["worker_count"]
    worker_depth = args.depth if args.depth is not None else next_config["chain_depth"]
    if args.output_dir.exists():
        parser.error(f"campaign output already exists; preserve it and choose a new path: {args.output_dir}")
    if not (args.install / "EmuHawk.exe").is_file() or not args.rom.is_file() or \
            not args.script.is_file() or not (args.install / "dll" / "gpgx.wbx").is_file():
        parser.error(f"install ({args.install}), ROM ({args.rom}), Lua script ({args.script}), or GPGX WBX does not exist")
    if args.decoder is not None and not args.decoder.is_file():
        parser.error(f"PC-list decoder does not exist: {args.decoder}")
    if args.range_tool is not None and not args.range_tool.is_file():
        parser.error(f"range decoder does not exist: {args.range_tool}")
    rom_data = args.rom.read_bytes()
    rom_sha = hashlib.sha256(rom_data).hexdigest()
    if len(rom_data) != ROM_SIZE or rom_sha != ROM_SHA:
        parser.error("canonical USA ROM size or SHA-256 mismatch")
    os.environ["LF_NATURAL_INPUT"] = "1" if args.natural_input else "0"
    os.environ["LF_WAVE_CADENCE_FRAMES"] = str(args.cadence)
    os.environ["LF_CHUNK_PREFIX"] = "live-discovery-wave"
    os.environ["LF_MEMORY"] = str(args.memory_bytes)
    master_root = args.output_dir / "master-v2-work"
    startup_report = None
    startup_state = None
    if args.master_startup:
        try:
            startup_path = Path(__file__).with_name("master_startup_authority.py")
            sys.path.insert(0, str(startup_path.parent))
            from master_startup_authority import load_startup
            startup = load_startup(Path(__file__).parents[2], args.rom)
            startup_state = startup
            startup_report = {"authority": "MASTER_V2", "legacy_reads": 0,
                              "legacy_fallback": "DISABLED", "generation_id": startup.generation_id,
                              "master_sha256": startup.master_sha256,
                              "metrics": startup.metrics.__dict__}
        except ValueError as error:
            parser.error(str(error))
    args.output_dir.mkdir(parents=True)
    if args.in_process_postrun and startup_state is not None:
        from master_v2_runtime_bridge import materialize_rolling_base
        materialize_rolling_base(startup_state.master_path, master_root)
    instrumentation_identity = _instrumentation_identity(args.install, args.script)
    args.end_game_path = (args.output_dir / "end-game-request.txt").resolve()
    args.end_game_path.unlink(missing_ok=True)
    os.environ["LF_END_GAME_PATH"] = str(args.end_game_path)
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
    session = None if args.until_closed else LiveForwardCartographer(
        rom_sha, _instrumentation_identity(args.install, args.script))
    linker = None if args.until_closed else LiveForwardRomLinker(rom_sha)
    progress = None if session is None else LiveSessionProgressPublisher(
        args.output_dir / "live-session-progress.json", args.output_dir.name,
        rom_sha, worker_count * CYCLES_PER_WORKER)

    def publish_progress(state: str, error: str | None = None) -> None:
        if progress is None or session is None:
            return
        metrics = session.metrics()
        progress.publish(state, session.segments_admitted, session.segments_rejected,
                         int(metrics["nodes"]), int(metrics["edges"]),
                         int(metrics["source_owned_bytes"]), error)

    def on_segment(segment: dict[str, object], rows: list[tuple[int, ...]], data: bytes) -> None:
        assert session is not None and linker is not None
        session.admit(segment, rows, data)
        linker.stage(segment, rows, data)
        publish_progress("RUNNING")

    report_path = args.output_dir / "live-forward-rom-link-2b-receipt.json"
    interactive_report_path = args.output_dir / "live-worker-interactive-receipt.json"
    runtime = projection = saved = None
    session_path = args.output_dir / "session-rom-link.sqlite"
    session_saved = False
    control = None
    spool = None
    flow_runtime = None
    spool_receipt = None
    try:
        publish_progress("STARTING")
        spool = LiveForwardSegmentSpool(args.output_dir / "continuous-runtime-evidence")
        on_status = None
        if args.control_window:
            control = LiveWorkerControlPublisher(
                args.output_dir / "live-worker-control-status.json",
                args.next_run_config,
                args.output_dir / "live-worker-control-preview.txt",
                args.output_dir, args.end_game_path)
            control.start()
            on_status = control.update
        def on_segment_fn(segment: dict[str, object], rows: list[tuple[int, ...]], data: bytes) -> None:
            if spool:
                spool.admit(segment, rows, data)
            if session is not None:
                on_segment(segment, rows, data)
        if flow_runtime:
            on_segment_fn = flow_runtime.submit
        runtime = run_one(args, "interactive" if args.until_closed else "rom-link",
                          worker_count, worker_depth,
                          on_segment_fn,
                          on_status)
        if spool:
            spool_receipt = spool.close()
        if flow_runtime:
            flow_runtime.close()
            spool_receipt = flow_runtime.stats()
        if control:
            control.finish(None, int(runtime.get("audited_segments", 0)))
        preflight = allocation_preflight(next_config, runtime.get("plan"),
            int(runtime.get("allocation_budget_bytes", budget["native_budget_bytes"])),
            str(runtime.get("preflight_reason")) if runtime.get("preflight_reason") else None)
        preflight["system_total_ram_bytes"] = memory["system_total_ram_bytes"]
        preflight["system_available_ram_bytes"] = available
        preflight["config_origin"] = config_origin
        if args.until_closed:
            status = str(runtime.get("outcome", "UNKNOWN"))
            report = {"checkpoint": "M12-LIVE-WORKER-INTERACTIVE",
                "status": status, "run_mode": "UNTIL_EMUHAWK_CLOSE",
                "runtime": runtime, "flow_handoff": spool_receipt,
                "raw_segment_spool": spool_receipt,
                "instrumentation_identity": instrumentation_identity,
                "next_run_config": next_config, "config_origin": config_origin,
                "control_window_enabled": args.control_window,
                "cartographer_archivist_run": False,
                "bounded_campaign_pass_claimed": False,
                "source_owned_delta": 0, "production_architecture_changed": False,
                "startup_authority": startup_report,
                "postrun_mode": "IN_PROCESS" if args.in_process_postrun else ("NONE" if not args.in_process_postrun else "SUBPROCESS")}
            postrun_started = should_launch_postrun_analysis(status) if args.in_process_postrun else False
            report["postrun_analysis_started"] = postrun_started
            if not postrun_started:
                report["postrun_analysis_skip_reason"] = "POSTRUN_DISABLED" if not args.in_process_postrun else "RUN_NOT_SEALED"
            interactive_report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n",
                                               encoding="utf-8")
            postrun_result = None
            if postrun_started and args.in_process_postrun:
                postrun_result, _, _ = _run_postrun_in_process(
                    interactive_report_path, args.output_dir, args.rom, args.decoder,
                    args.range_tool,
                    startup_state, master_root, flow_runtime)
                report["postrun_result"] = {"overall_status": postrun_result.overall_status,
                    "events": len(postrun_result.events), "error": postrun_result.error}
                interactive_report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n",
                                                   encoding="utf-8")
            print(json.dumps({"status": status, "receipt": str(interactive_report_path.resolve()),
                              "flow_handoff": spool_receipt,
                              "postrun_analysis_started": postrun_started}, indent=2))
            return 0 if status in ("STOPPED_AFTER_EMUHAWK_EXIT", "STOPPED_DISK_RESERVE",
                                   "STOPPED_END_GAME", "STOPPED_FRAME_LIMIT") else 2
        if runtime.get("outcome") != "PASS" or runtime.get("audited_segments") != \
                worker_count * CYCLES_PER_WORKER:
            raise RuntimeError("STOP_ROM_LINK_RUNTIME_SEGMENT_AUDIT")
        publish_progress("FINALIZING")
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
        publish_progress("CLOSED")
        status = projection["status"]
        report = {"checkpoint": "M12-ROM-RANGE-LINKAGE-2B", "status": status,
            "runtime": runtime, "rom_projection": projection, "saved_session": saved,
            "independent_audit": audit_report, "preflight": preflight,
            "flow_handoff": spool_receipt, "raw_segment_spool": spool_receipt,
            "instrumentation_identity": instrumentation_identity,
            "next_run_config": next_config, "config_origin": config_origin,
            "control_window_enabled": args.control_window,
            "session_path": str(session_path.resolve()), "source_owned_delta": 0,
            "postrun_mode": "IN_PROCESS" if args.in_process_postrun else "NONE",
            "production_architecture_changed": False, "startup_authority": startup_report}
        report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        if args.in_process_postrun:
            postrun_result, _, _ = _run_postrun_in_process(
                report_path, args.output_dir, args.rom, args.decoder, args.range_tool,
                startup_state, master_root, flow_runtime)
            report["postrun_result"] = {"overall_status": postrun_result.overall_status,
                                        "events": len(postrun_result.events),
                                        "error": postrun_result.error}
            report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n",
                                   encoding="utf-8")
        print(json.dumps({"status": status, "receipt": str(report_path.resolve()),
                          "session": str(session_path.resolve())}, indent=2))
        return 0 if status == "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE" else 2
    except Exception as error:
        publish_progress("FAILED", str(error))
        if flow_runtime and flow_runtime.error is None:
            flow_runtime.fail(error)
        if spool:
            spool_receipt = spool.close()
        admitted = session.segments_admitted if session else int(
            (spool_receipt or {}).get("segments", 0))
        if control:
            control.finish(None, admitted, str(error))
        status = (projection or {}).get("status")
        if not status and runtime is None and admitted == 0:
            status = "STOP_ROM_LINK_EMUHAWK_STARTUP"
        status = status or str(error).split(":", 1)[0]
        if not status.startswith("STOP_ROM_LINK_"):
            status = "STOP_ROM_LINK_RUNTIME_OR_AUDIT_FAILURE"
        error_report_path = interactive_report_path if args.until_closed else report_path
        checkpoint = "M12-LIVE-WORKER-INTERACTIVE" if args.until_closed else "M12-ROM-RANGE-LINKAGE-2B"
        error_report_path.write_text(json.dumps({"checkpoint": checkpoint,
            "status": status, "error": str(error), "runtime": runtime,
            "rom_projection": projection, "saved_session": saved,
            "session_path": str(session_path.resolve()), "session_saved": session_saved,
            "admitted_segments": admitted,
            "flow_handoff": spool_receipt,
            "preflight": preflight, "next_run_config": next_config,
            "config_origin": config_origin, "control_window_enabled": args.control_window},
            indent=2, sort_keys=True) + "\n", encoding="utf-8")
        raise
    finally:
        if spool:
            spool.close()
        if session:
            session.close()


if __name__ == "__main__":
    raise SystemExit(main())
