"""Lossy host-side status publisher for the separate 2H operator window."""

from __future__ import annotations

import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys
import threading
import time
from collections import deque
from typing import Any

from live_forward_worker_control_model import (
    EVIDENCE_DISK_STOP_RESERVE_BYTES, EvidenceGrowthMeter, STATUS_SCHEMA, allocation_preflight,
    calculate_resource_budget, format_bytes, load_next_run_config, parse_live_control,
    parse_native_plan, process_memory_total, system_memory_values,
)


PUBLISH_INTERVAL_SECONDS = 0.25
EVIDENCE_SCAN_INTERVAL_SECONDS = 2.0
NATIVE_BUDGET_CAP = 256 * 1024 * 1024
PROCESS_BUDGET_CAP = 512 * 1024 * 1024
CORE_RESERVE_CAP = 128 * 1024 * 1024
# Keep one GiB for the host while allowing a 32/64 GiB desktop to start a
# small native plan when Windows reports a few GiB currently available.
SYSTEM_RESERVE = 1 * 1024 * 1024 * 1024
WORKER_MEMORY_BYTES = 512 * 1024
_EXCLUDED_EVIDENCE = {"live-worker-control-status.json", "live-worker-control-status.json.tmp"}


class _MemoryStatus(ctypes.Structure):
    _fields_ = [("length", ctypes.c_ulong), ("memory_load", ctypes.c_ulong),
                ("total_phys", ctypes.c_ulonglong), ("avail_phys", ctypes.c_ulonglong),
                ("total_page", ctypes.c_ulonglong), ("avail_page", ctypes.c_ulonglong),
                ("total_virtual", ctypes.c_ulonglong), ("avail_virtual", ctypes.c_ulonglong),
                ("avail_extended", ctypes.c_ulonglong)]


class _ProcessMemoryCounters(ctypes.Structure):
    _fields_ = [("cb", ctypes.c_ulong), ("faults", ctypes.c_ulong),
                ("peak_working_set", ctypes.c_size_t), ("working_set", ctypes.c_size_t),
                ("peak_paged_pool", ctypes.c_size_t), ("paged_pool", ctypes.c_size_t),
                ("peak_nonpaged_pool", ctypes.c_size_t), ("nonpaged_pool", ctypes.c_size_t),
                ("pagefile", ctypes.c_size_t), ("peak_pagefile", ctypes.c_size_t)]


def system_memory() -> dict[str, int | None]:
    if os.name != "nt":
        return {"system_total_ram_bytes": None, "system_available_ram_bytes": None}
    status = _MemoryStatus()
    status.length = ctypes.sizeof(status)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
        return {"system_total_ram_bytes": None, "system_available_ram_bytes": None}
    return {"system_total_ram_bytes": int(status.total_phys),
            "system_available_ram_bytes": int(status.avail_phys)}


def process_working_set(process_id: int | None) -> int | None:
    if os.name != "nt" or not process_id:
        return None
    handle = ctypes.windll.kernel32.OpenProcess(0x1000, False, process_id)
    if not handle:
        return None
    try:
        counters = _ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        ok = ctypes.windll.psapi.GetProcessMemoryInfo(
            handle, ctypes.byref(counters), counters.cb)
        return int(counters.working_set) if ok else None
    finally:
        ctypes.windll.kernel32.CloseHandle(handle)


def evidence_tree_bytes(root: Path) -> int:
    total = 0
    if not root.exists():
        return total
    for current, directories, files in os.walk(root, followlinks=False):
        directories[:] = [name for name in directories
                          if not (Path(current) / name).is_symlink()]
        for name in files:
            if name in _EXCLUDED_EVIDENCE:
                continue
            path = Path(current) / name
            try:
                if not path.is_symlink():
                    total += path.stat().st_size
            except OSError:
                continue
    return total


def evidence_growth_display(growth_text: str) -> str:
    reserve = format_bytes(EVIDENCE_DISK_STOP_RESERVE_BYTES)
    return f"Evidence disk growth: {growth_text}; auto-stop at {reserve} free"


def visible_worker_activity(values: dict[str, str],
                            activity: list[dict[str, object]] | None) -> list[dict[str, int | None]]:
    """Copy only host-audited summaries for Workers in the current status page."""
    if not activity or not values.get("LIVE_CONTROL"):
        return []
    try:
        visible = parse_live_control(values["LIVE_CONTROL"])["workers"]
    except (ValueError, KeyError):
        return []
    fields = ("audited_captures", "last_flow_records", "total_flow_records")
    result: list[dict[str, int | None]] = []
    for row in visible:
        worker_id = int(row["worker_id"])
        if worker_id >= len(activity):
            continue
        source = activity[worker_id]
        if int(source.get("worker_id", -1)) != worker_id:
            continue
        item: dict[str, int | None] = {"worker_id": worker_id}
        for field in fields:
            value = source.get(field)
            item[field] = None if value is None else int(value)
        result.append(item)
    return result


class LiveWorkerControlPublisher:
    """A lossy 4 Hz host thread; the emulator never waits for UI or disk I/O."""

    def __init__(self, snapshot_path: Path, config_path: Path, preview_path: Path,
                 evidence_root: Path, end_game_path: Path | None = None) -> None:
        self.snapshot_path = snapshot_path
        self.config_path = config_path
        self.preview_path = preview_path
        self.evidence_root = evidence_root
        self.end_game_path = end_game_path or evidence_root / "end-game-request.txt"
        self.window: subprocess.Popen[bytes] | None = None
        self._lock = threading.Lock()
        self._latest: dict[str, Any] = {}
        self._stopped = threading.Event()
        self._thread = threading.Thread(target=self._publish_loop,
                                        name="thor-worker-control-snapshot", daemon=True)
        self._evidence_meter = EvidenceGrowthMeter()
        self._last_evidence_scan = 0.0
        self._evidence_bytes = 0
        self._published: deque[float] = deque(maxlen=20)

    def start(self) -> None:
        self.snapshot_path.parent.mkdir(parents=True, exist_ok=True)
        self.snapshot_path.unlink(missing_ok=True)
        args = [sys.executable, str(Path(__file__).with_name(
            "live_forward_worker_control_window.py")),
            "--snapshot", str(self.snapshot_path), "--config", str(self.config_path),
            "--preview", str(self.preview_path), "--end-game", str(self.end_game_path)]
        flags = getattr(subprocess, "CREATE_NO_WINDOW", 0)
        self.window = subprocess.Popen(args, cwd=str(Path(__file__).parent),
            stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, creationflags=flags)
        self._thread.start()

    def update(self, values: dict[str, str], process: subprocess.Popen[bytes],
               started_at: float, audited_segments: int,
               worker_activity: list[dict[str, object]] | None = None) -> None:
        activity = visible_worker_activity(values, worker_activity)
        with self._lock:
            self._latest = {"values": dict(values), "emuhawk_pid": process.pid,
                            "emuhawk_running": process.poll() is None,
                            "started_at": started_at,
                            "audited_segments": audited_segments,
                            "worker_activity": activity}

    def finish(self, values: dict[str, str] | None, audited_segments: int,
               error: str | None = None) -> None:
        with self._lock:
            latest = dict(self._latest)
            if values is not None:
                latest["values"] = dict(values)
            latest["emuhawk_running"] = False
            latest["audited_segments"] = audited_segments
            latest["runtime_error"] = error
            self._latest = latest
        self._publish_once()
        self._stopped.set()
        self._thread.join(timeout=1.0)
        # The window owns its lifetime and can still save the next-run config.

    def _publish_loop(self) -> None:
        while not self._stopped.is_set():
            self._publish_once()
            self._stopped.wait(PUBLISH_INTERVAL_SECONDS)

    def _publish_once(self) -> None:
        now = time.monotonic()
        with self._lock:
            latest = dict(self._latest)
        values = latest.get("values", {})
        parsed: dict[str, Any] | None = None
        parse_error = None
        try:
            if values.get("LIVE_CONTROL"):
                parsed = parse_live_control(values["LIVE_CONTROL"])
        except (ValueError, KeyError) as error:
            parse_error = str(error)
        try:
            current_plan = parse_native_plan(values.get("PLAN", ""))
        except ValueError as error:
            current_plan, parse_error = None, str(error)

        memory = system_memory()
        python_ws = process_working_set(os.getpid())
        emuhawk_ws = process_working_set(latest.get("emuhawk_pid"))
        ui_ws = process_working_set(self.window.pid if self.window else None)
        current_native = current_plan.get("total_native_bytes") if current_plan else None
        ui_pid = self.window.pid if self.window else None
        thor_memory = process_memory_total([(os.getpid(), python_ws),
            (latest.get("emuhawk_pid"), emuhawk_ws), (ui_pid, ui_ws)])

        if now - self._last_evidence_scan >= EVIDENCE_SCAN_INTERVAL_SECONDS:
            try:
                self._evidence_bytes = evidence_tree_bytes(self.evidence_root)
            except OSError:
                pass
            self._last_evidence_scan = now
        growth = self._evidence_meter.sample(self._evidence_bytes, now)
        total_ram = memory["system_total_ram_bytes"]
        available_ram = memory["system_available_ram_bytes"]
        released = sum(int(value) for value in (python_ws, emuhawk_ws) if value is not None)
        estimated_available = min(total_ram, available_ram + released) \
            if total_ram is not None and available_ram is not None else available_ram

        native_budget = calculate_resource_budget(estimated_available,
            native_budget_cap=NATIVE_BUDGET_CAP, process_budget_cap=PROCESS_BUDGET_CAP,
            core_reserve_cap=CORE_RESERVE_CAP, system_reserve=SYSTEM_RESERVE,
            memory_bytes_each=WORKER_MEMORY_BYTES)
        preview = parsed.get("next_plan") if parsed else None
        next_plan_status = parsed.get("next_plan_status", "WAITING_FOR_NATIVE_PLANNER") \
            if parsed else "WAITING_FOR_NATIVE_PLANNER"
        projected_native = projected_total = projected_free = shortfall = None
        preflight = None
        if parsed and preview:
            projected_native = preview["total_native_bytes"]
            emu_base = max(0, int(emuhawk_ws or 0) -
                           min(int(current_native or 0), int(emuhawk_ws or 0)))
            process_base = sum(int(value or 0) for value in (python_ws, ui_ws)) + emu_base
            projected_total = process_base + projected_native
            projected_free = (estimated_available - projected_total
                              if estimated_available is not None else None)
            shortfall = max(0, projected_native - native_budget["native_budget_bytes"])
            preflight = allocation_preflight(parsed["next_run"], preview,
                native_budget["native_budget_bytes"])
            next_plan_status = "PREFLIGHT_REJECTED" if shortfall else "READY"
        elif parsed and parsed.get("next_plan_status") == "NATIVE_PLANNER_REJECTED":
            next_plan_status = "NATIVE_PLANNER_REJECTED"

        metrics = parsed["metrics"] if parsed else {}
        updated = time.time()
        self._published.append(now)
        hz = ((len(self._published) - 1) /
              (self._published[-1] - self._published[0])) if len(self._published) > 1 \
            and self._published[-1] > self._published[0] else 0.0
        rate = growth["rate_bytes_per_second"]
        growth_text = "CALCULATING…" if rate is None else (
            f"{rate * 3600 / (1024 ** 3):.1f} GiB/hour" if rate >= 1024 * 1024 / 60 else
            f"{rate * 60 / (1024 ** 2):.1f} MiB/min")
        runtime_state = "RUNNING" if latest.get("emuhawk_running") else \
            (values.get("RESULT", "WAITING") or "WAITING")
        activity = {item["worker_id"]: item for item in latest.get("worker_activity", [])}
        workers = []
        if parsed:
            for row in parsed["workers"]:
                workers.append({**row, **activity.get(row["worker_id"], {})})
        error_count = sum(int(metrics.get(key, 0)) for key in (
            "captures_invalid", "captures_dropped", "stale_ack",
            "identity_collisions", "ring_retention_failures", "unsupported_endings"))
        try:
            saved_config, saved_state = load_next_run_config(self.config_path)
        except ValueError:
            saved_config, saved_state = {"worker_count": None, "chain_depth": None}, "MALFORMED"
        memory_totals = system_memory_values(total_ram, available_ram)
        current_workers = parsed["worker_count"] if parsed else (
            current_plan["worker_count"] if current_plan else None)
        current_depth = parsed["depth"] if parsed else (
            current_plan["depth"] if current_plan else None)
        snapshot: dict[str, Any] = {
            "schema": STATUS_SCHEMA, "updated_at": updated,
            "runtime_state": runtime_state, "current_worker_count": current_workers,
            "current_depth": current_depth,
            "saved_next_worker_count": saved_config["worker_count"],
            "saved_next_depth": saved_config["chain_depth"],
            "config_state": saved_state,
            "current_native_worker_bytes": current_native,
            **memory, "system_used_ram_bytes": memory_totals["used_bytes"],
            "thor_process_memory_bytes": thor_memory,
            "emuhawk_process_memory_bytes": emuhawk_ws,
            "host_process_memory_bytes": python_ws,
            "ui_process_memory_bytes": ui_ws,
            "projected_next_run_native_worker_bytes": projected_native,
            "projected_next_run_allocation_bytes": projected_native,
            "projected_next_run_total_thor_bytes": projected_total,
            "projected_next_run_free_ram_bytes": projected_free,
            "projected_next_run_preflight": preflight,
            "next_native_plan": preview, "next_plan_status": next_plan_status,
            "evidence_bytes": self._evidence_bytes,
            "evidence_elapsed_seconds": growth["elapsed_seconds"],
            "evidence_rate_bytes_per_second": rate,
            "evidence_projected_gib_per_hour": growth["projected_gib_per_hour"],
            "evidence_growth_state": growth["state"],
            "evidence_growth_display": evidence_growth_display(growth_text),
            "ui_update_frequency_hz": hz, "segments": latest.get("audited_segments", 0),
            "worker_captures_started": metrics.get("captures_started", 0),
            "worker_captures_completed": metrics.get("captures_completed", 0),
            "retention_failures": metrics.get("ring_retention_failures", 0),
            "runtime_errors": error_count, "runtime_error": latest.get("runtime_error")
                or values.get("ERROR"),
            "worker_offset": parsed["worker_offset"] if parsed else 0,
            "workers": workers,
            "worker_rows_total": current_workers or 0,
            "worker_status_parse_error": parse_error,
            "frame_timing": {name: values.get(name) for name in
                ("PERF_BASELINE", "PERF_RECORDER", "PERF_WORKER")},
        }
        self._replace_snapshot(snapshot)

    def _replace_snapshot(self, snapshot: dict[str, Any]) -> None:
        temporary = self.snapshot_path.with_suffix(self.snapshot_path.suffix + ".tmp")
        try:
            temporary.write_text(json.dumps(snapshot, separators=(",", ":"),
                                            sort_keys=True) + "\n", encoding="utf-8")
            os.replace(temporary, self.snapshot_path)
        except OSError:
            temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    from live_forward_rom_link_runtime import main
    raise SystemExit(main())
