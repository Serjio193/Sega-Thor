"""AUTO67 BizHawk launcher and bounded snapshot lifecycle."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path
from typing import Any

from auto67_capsule import CapsulePool
from auto67_dashboard import DASHBOARD_HTML
from auto67_live import BASELINE, ROM_SHA, Dispatcher
from auto67_persistence import LivePersistenceSink
from auto67_status import StatusPublisher


def run_live(args: argparse.Namespace) -> dict[str, Any]:
    rom = Path(args.rom).resolve()
    emulator = Path(args.emulator).resolve()
    lua = Path(args.lua).resolve()
    output = Path(args.output).resolve()
    if hashlib.sha256(rom.read_bytes()).hexdigest() != ROM_SHA:
        raise SystemExit("canonical ROM identity mismatch")
    output.parent.mkdir(parents=True, exist_ok=True)
    status_path = output.with_suffix(".status.json")
    final_path = output.with_suffix(".lua.json")
    stop_path = output.with_suffix(".stop")
    command_path = output.with_suffix(".capsule.commands")
    for path in (status_path, final_path, stop_path, command_path):
        if path.exists():
            path.unlink()
    capsule_pool = None
    capsule_dir = output.parent / (output.stem + ".capsules")
    if args.capsule_mode:
        capsule_dir.mkdir(parents=True, exist_ok=True)
        capsule_pool = CapsulePool(
            count=args.capsule_count, max_live=args.max_live_captures,
            command_path=command_path)
    chain_db = getattr(args, "chain_db", None) or getattr(args, "knowledge_db", None)
    chain_sink = LivePersistenceSink(chain_db) if chain_db else None
    if chain_sink is not None:
        chain_sink.start()
    dispatcher = Dispatcher(args.workers, args.window, args.worker_delay, capsule_pool,
                            chain_sink)
    dispatcher.start()
    environment = os.environ.copy()
    environment.update({
        "OASIS_LIVE_STATUS": str(status_path),
        "OASIS_LIVE_FINAL": str(final_path),
        "OASIS_LIVE_STOP": str(stop_path),
        "OASIS_LIVE_MAX_FRAMES": str(args.max_frames),
        "OASIS_LIVE_DEMO_INPUTS": args.demo_inputs,
        "OASIS_LIVE_STATE": str(args.state) if getattr(args, "state", None) else "",
        "OASIS_LIVE_WINDOW": str(args.window),
        "OASIS_LIVE_CAPTURE_DISABLED": "1" if args.capture_disabled else "0",
        "OASIS_LIVE_CAPTURE_MODE": args.capture_mode,
        "OASIS_CAPSULE_COMMANDS": str(command_path),
        "OASIS_CAPSULE_DIR": str(capsule_dir),
        "OASIS_AUTO67_HOOK_METRICS": str(lua.parent / "auto67_hook_metrics.lua"),
    })
    command = [str(emulator), f"--lua={lua}", str(rom)]
    started = time.monotonic()
    seen_sequence = -1
    launcher_log = output.with_suffix(".launcher.log")
    view_path = output.with_suffix(".view.json")
    view_mode = "none" if args.no_view else args.view_mode
    publisher = StatusPublisher(
        dispatcher, view_path, DASHBOARD_HTML,
        args.view_port if view_mode == "browser" else None,
        window_enabled=view_mode == "window")
    view_url = publisher.url

    with launcher_log.open("w", encoding="utf-8") as log:
        process = subprocess.Popen(command, cwd=emulator.parent, env=environment,
                                    stdout=log, stderr=subprocess.STDOUT, text=True)
        while process.poll() is None:
            lua_status = None
            if status_path.exists():
                try:
                    lua_status = json.loads(status_path.read_text(encoding="utf-8"))
                except (json.JSONDecodeError, OSError):
                    lua_status = None
            if lua_status:
                dispatcher.metrics["capture_poll_count"] += 1
                if dispatcher.capsule_pool is not None:
                    dispatcher.capsule_pool.sync(lua_status.get("capsules", []))
                for event in lua_status.get("discovery", lua_status.get("events", [])):
                    if int(event.get("seq", -1)) > seen_sequence:
                        seen_sequence = int(event["seq"])
                        dispatcher.ingest(event)
            if lua_status:
                publisher.publish(lua_status)
            time.sleep(args.poll_interval)
        process.wait(timeout=10)
    if final_path.exists():
        lua_final = json.loads(final_path.read_text(encoding="utf-8"))
    else:
        lua_final = dict(publisher.latest, capture_complete=False)
    if capsule_pool is not None:
        capsule_pool.sync(lua_final.get("capsules", []))
    publisher.publish(lua_final)
    dispatcher.stop()
    if chain_sink is not None:
        chain_sink.stop()
    if capsule_pool is not None:
        capsule_pool.stop()
    publisher.stop()
    elapsed = time.monotonic() - started
    result = {"schema": "oasis.m12.auto67.live-session.v1", "baseline": BASELINE,
              "rom_sha256": ROM_SHA, "command": command, "returncode": process.returncode,
              "session_seconds": elapsed, "workers_configured": args.workers,
              "dispatcher": dispatcher.snapshot(), "lua": lua_final,
              "raw_event_backlog": 0, "raw_event_backlog_structure": "NONEXISTENT",
              "launcher_log": str(launcher_log), "status_path": str(status_path),
              "capture_path": str(final_path), "capture_disabled": args.capture_disabled,
              "view_url": view_url, "view_snapshot": str(view_path),
              "view_mode": view_mode,
              "chain_store": chain_sink.snapshot() if chain_sink else {
                  "available": False},
              "capsule_mode": args.capsule_mode,
              "capsule_config": {"count": args.capsule_count,
                                  "capacity": 128 * 1024,
                                  "max_live": args.max_live_captures},
              "visualization_proof": {"two_workers_working":
                                       dispatcher.metrics["peak_workers_working"] >= 2,
                                       "worker_returned_idle": dispatcher.metrics["worker_returns"] > 0,
                                       "known_or_merge_visible":
                                       dispatcher.metrics["known_rejected_before_dispatch"] > 0 or
                                       dispatcher.metrics["investigation_merges"] > 0,
                                       "rolling_window_active":
                                       dispatcher.metrics["events_observed"] > 0}}
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="AUTO67 live opportunistic RE launcher")
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--lua", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--workers", type=int, choices=(1, 2, 4, 8, 16, 32, 64), default=16)
    parser.add_argument("--window", type=int, default=256)
    parser.add_argument("--worker-delay", type=float, default=0.003)
    parser.add_argument("--poll-interval", type=float, default=0.05)
    parser.add_argument("--max-frames", type=int, default=0,
                        help="bounded validation limit; zero means play until emulator closes")
    parser.add_argument("--demo-inputs", default="",
                        help="optional validation-only frame:buttons list, not a scenario")
    parser.add_argument("--state", type=Path, default=None,
                        help="optional exact developer savestate for deterministic replay")
    parser.add_argument("--capture-disabled", action="store_true")
    parser.add_argument("--capture-mode", choices=("continuous", "burst"), default="continuous",
                        help="burst is lossy discovery only; neither mode proves complete chains")
    parser.add_argument("--capsule-mode", action="store_true",
                        help="AUTO67.1 fixed 16-capsule path; no raw event FIFO")
    parser.add_argument("--capsule-count", type=int, default=16)
    parser.add_argument("--max-live-captures", type=int, choices=range(0, 17), default=1)
    parser.add_argument("--view-port", type=int, default=0)
    parser.add_argument("--view-mode", choices=("window", "browser", "none"), default="window",
                        help="native operator window by default; browser is explicit fallback")
    parser.add_argument("--no-view", action="store_true")
    parser.add_argument("--chain-db", "--knowledge-db", dest="chain_db", type=Path,
                        default=None, help="existing SQLite evidence sidecar for worker chains")
    args = parser.parse_args()
    result = run_live(args)
    print(json.dumps({"returncode": result["returncode"],
                      "metrics": result["dispatcher"]["metrics"],
                      "lua": result["lua"]}, sort_keys=True))
    return int(result["returncode"] or 0)


if __name__ == "__main__":
    raise SystemExit(main())
