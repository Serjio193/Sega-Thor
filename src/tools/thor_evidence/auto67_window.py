"""Bounded native operator window for AUTO67 live snapshots."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


def _value(value: Any, default: str = "-") -> str:
    return default if value is None or value == "" else str(value)


def _metric_text(payload: dict[str, Any]) -> str:
    dispatcher = payload.get("dispatcher", {})
    metrics = dispatcher.get("metrics", {})
    lua = payload.get("lua", {})
    rolling = dispatcher.get("rolling_window", {})
    profile = dispatcher.get("dispatch_profile", {})
    chain_store = dispatcher.get("chain_store", {})
    monitor = chain_store.get("monitor", {})
    total_dispatch = profile.get("stages_us", {}).get(
        "T0_T8_dispatch_to_worker_start_us", {})
    frame_timing = lua.get("frame_timing", {})
    hooks = lua.get("hook_metrics", {})
    hook_frames = hooks.get("frames", {})
    capsules = dispatcher.get("capsules", {}) or {}
    lines = [
        f"FRAME {payload.get('frame', 0)}  EPOCH {lua.get('epoch', 0)}  "
        f"EVENT/s {metrics.get('event_rate', 0):.1f}",
        f"WORKERS {metrics.get('workers_busy', 0)}/{metrics.get('workers_configured', 0)} busy  "
        f"LEASES {metrics.get('worker_leases', 0)}  "
        f"RETURNS {metrics.get('worker_returns', 0)}  "
        f"ACTIVE COLLISIONS {metrics.get('active_collisions', 0)}  "
        f"DUP ACTIVE {metrics.get('duplicate_active_claims', 0)}",
        f"WINDOW {rolling.get('utilization', 0)}/{rolling.get('capacity', 0)}  "
        f"OVERWRITES {rolling.get('overwrites', 0)}  "
        f"SEED AGE avg/max {metrics.get('average_seed_age', 0):.3f}/"
        f"{metrics.get('max_seed_age', 0):.3f}s  "
        f"RAW BACKLOG {payload.get('raw_event_backlog_structure', 'UNKNOWN')}",
        f"DISPATCH T0-T8 us p50/p95/max {total_dispatch.get('p50', 0)}/"
        f"{total_dispatch.get('p95', 0)}/{total_dispatch.get('max', 0)}  "
        f"CLAIM LOCK us p95/max {profile.get('claim_lock_us', {}).get('p95', 0)}/"
        f"{profile.get('claim_lock_us', {}).get('max', 0)}",
        f"FRAME SPIKES >16/>33/>50ms "
        f"{frame_timing.get('over_16ms', 0)}/{frame_timing.get('over_33ms', 0)}/"
        f"{frame_timing.get('over_50ms', 0)}  "
        f"largest {frame_timing.get('largest', {}).get('duration_ms', 0):.3f}ms",
        f"HUNT attempts/success {metrics.get('hunt_attempts', 0)}/"
        f"{metrics.get('hunt_successes', 0)}  FRESH {metrics.get('fresh_candidates_available', 0)}  "
        f"FOCUSED {capsules.get('active_live', 0)}/{capsules.get('max_simultaneous_live', 0)}",
        f"CHAIN DB unique {monitor.get('total_unique_chains', 0)}  "
        f"SESSION NEW {monitor.get('new_unique_chains_this_session', 0)}  "
        f"EXACT DUP {monitor.get('exact_duplicates_rejected', 0)}  "
        f"WRITES {chain_store.get('persisted', 0)}  ERRORS {chain_store.get('write_errors', 0)}",
        f"UNRESOLVED {monitor.get('unresolved_chains', 0)}  "
        f"ROOTED {monitor.get('rooted_chains', 0)}  "
        f"CHAINS/1000 LEASES {monitor.get('chains_per_1000_leases', 0):.1f}  "
        f"DB {monitor.get('db_size_bytes', 0)} bytes",
        f"HOOKS {hooks.get('active_hooks', 0)}  CALLBACKS/frame p95/max "
        f"{hook_frames.get('callbacks_per_frame', {}).get('p95', 0)}/"
        f"{hook_frames.get('callbacks_per_frame', {}).get('max', 0)}  "
        f"CALLBACK us/frame p95/max "
        f"{hook_frames.get('callback_self_time_us_per_frame', {}).get('p95', 0):.0f}/"
        f"{hook_frames.get('callback_self_time_us_per_frame', {}).get('max', 0):.0f}",
        "",
        "WORKERS",
    ]
    for worker in dispatcher.get("workers", []):
        lines.append(
            f"W{int(worker.get('worker_id', 0)):02d} "
            f"{_value(worker.get('state'), 'STARTING'):<9} "
            f"stage={_value(worker.get('stage')):<22} "
            f"result={_value(worker.get('last_result')):<20} "
            f"chain={_value(worker.get('chain_fingerprint'))[:12]} "
            f"age={worker.get('seed_age', 0):.3f}s"
        )
    transitions = dispatcher.get("transition_history", [])
    if not transitions:
        transitions = [dict(item, worker_id=worker.get("worker_id", 0))
                       for worker in dispatcher.get("workers", [])
                       for item in worker.get("transitions", [])]
    lines.extend(["", "RECENT TRANSITIONS (bounded)"])
    for item in sorted(transitions, key=lambda entry: (
            entry.get("at", 0), entry.get("worker_id", 0)), reverse=True)[:80]:
        worker_id = item.get("worker_id", 0)
        chain = (item.get("chain") or "-")[:12]
        lines.append(
            f"W{int(worker_id):02d} {item.get('state', '-'):<9} "
            f"{_value(item.get('stage')):<20} {chain}"
        )
    return "\n".join(lines)


class LiveStatusWindow:
    """A separate native process rendering the replaceable snapshot file."""

    def __init__(self, snapshot_path: Path, title: str = "AUTO67 Live Operator") -> None:
        self.snapshot_path = snapshot_path
        self.title = title
        self.process: subprocess.Popen[bytes] | None = None

    def start(self) -> None:
        creationflags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        self.process = subprocess.Popen(
            [sys.executable, str(Path(__file__).resolve()),
             "--snapshot", str(self.snapshot_path), "--title", self.title],
            cwd=str(Path(__file__).resolve().parent),
            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, creationflags=creationflags)

    def stop(self) -> None:
        if self.process is None or self.process.poll() is not None:
            return
        self.process.terminate()
        try:
            self.process.wait(timeout=1)
        except subprocess.TimeoutExpired:
            self.process.kill()


def run_window(snapshot_path: Path, title: str) -> None:
    import tkinter as tk

    try:
        root = tk.Tk()
    except tk.TclError:
        return
    root.title(title)
    root.geometry("1180x820")
    root.minsize(760, 520)
    text = tk.Text(root, background="#101418", foreground="#e8eef2",
                   insertbackground="#e8eef2", font=("Consolas", 10),
                   padx=12, pady=10, wrap="none")
    text.pack(fill="both", expand=True)

    def close() -> None:
        root.destroy()

    def refresh() -> None:
        try:
            latest = json.loads(snapshot_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            latest = {}
        body = _metric_text(latest) if latest else "WAITING FOR LIVE SNAPSHOT"
        text.configure(state="normal")
        text.delete("1.0", "end")
        text.insert("1.0", body)
        text.configure(state="disabled")
        root.after(250, refresh)

    root.protocol("WM_DELETE_WINDOW", close)
    refresh()
    root.mainloop()


def main() -> int:
    parser = argparse.ArgumentParser(description="AUTO67 native operator window")
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--title", default="AUTO67 Live Operator")
    args = parser.parse_args()
    run_window(args.snapshot, args.title)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
