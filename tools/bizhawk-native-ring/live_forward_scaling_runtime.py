#!/usr/bin/env python3
"""Measure natural and forced native Worker scaling with 100 live cycles per slot."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
from typing import Callable

from live_forward_scaling_audit import (
    RECORD, integer_list, overlap_peak, read_record_slice, validate_segment,
)
from live_forward_worker_control_model import calculate_resource_budget


METRICS_NAMES = (
    "configured_workers", "workers_ever_used", "pending", "capturing",
    "complete", "occupied", "peak_capturing", "peak_complete",
    "peak_occupied", "captures_started", "captures_completed",
    "captures_validated", "captures_invalid", "captures_dropped",
    "stale_ack", "identity_collisions", "ring_retention_failures",
    "memory_limit_endings", "depth_limit_endings", "unsupported_endings",
    "records_produced", "instructions", "control_flow", "unique_entries",
    "duplicate_entries", "total_segment_bytes", "ring_wraps",
    "ring_capacity", "descriptor_bytes_each", "memory_bytes_each",
)
PLAN_NAMES = (
    "worker_count", "depth", "memory_bytes", "record_capacity",
    "identity_capacity", "descriptor_bytes_each", "descriptor_bytes_total",
    "result_bytes_each", "result_buffers_bytes_total", "pending_queue_bytes",
    "active_queue_bytes", "identity_table_bytes", "shared_ring_bytes",
    "instruction_stack_bytes", "dynamic_bytes", "total_native_bytes",
)
SW_SHOWNORMAL = 1
PROCESS_EXIT_DRAIN_SECONDS = 5.0
LAST_RUN_ID = 0


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


class TailLines:
    def __init__(self, path: Path):
        self.path, self.offset, self.pending = path, 0, b""
        self.values: dict[str, str] = {}

    def poll(self, final: bool = False) -> dict[str, str]:
        if not self.path.exists():
            return {}
        with self.path.open("rb") as source:
            source.seek(self.offset)
            data = source.read()
            self.offset = source.tell()
        lines = (self.pending + data).split(b"\n")
        self.pending = lines.pop()
        if final and self.pending:
            lines.append(self.pending)
            self.pending = b""
        changed: dict[str, str] = {}
        for line in lines:
            key, separator, value = line.decode("ascii", errors="replace").partition("=")
            if separator:
                value = value.removesuffix("\r")
                changed[key] = value
                if not key.startswith(("SEG_", "LIFECYCLE_")):
                    self.values[key] = value
        return changed


def available_memory() -> dict[str, int | None]:
    if os.name != "nt":
        return {"total_physical_bytes": None, "available_physical_bytes": None}
    status = _MemoryStatus()
    status.length = ctypes.sizeof(status)
    if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(status)):
        return {"total_physical_bytes": None, "available_physical_bytes": None}
    return {"total_physical_bytes": int(status.total_phys),
            "available_physical_bytes": int(status.avail_phys)}


def resolve_runtime_paths(args: argparse.Namespace) -> None:
    for name in ("install", "rom", "script", "output_dir"):
        setattr(args, name, getattr(args, name).resolve())


def working_set(process: subprocess.Popen[bytes]) -> int | None:
    if os.name != "nt":
        return None
    handle = ctypes.windll.kernel32.OpenProcess(0x1000, False, process.pid)
    if not handle:
        return None
    try:
        counters = _ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        ok = ctypes.windll.psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb)
        return int(counters.working_set) if ok else None
    finally:
        ctypes.windll.kernel32.CloseHandle(handle)


def configure_install(install: Path, output_dir: Path) -> Path:
    template = install / "config.ini"
    if not template.is_file():
        raise FileNotFoundError(f"isolated BizHawk config missing: {template}")
    config = json.loads(template.read_text(encoding="utf-8-sig"))
    config.update({"SingleInstanceMode": False, "RunInBackground": True,
                   "AcceptBackgroundInput": False, "StartPaused": False,
                   "AutoLoadLastSaveSlot": False, "AutoSaveLastSaveSlot": False,
                   "AutosaveSaveRAM": False})
    config.setdefault("Rewind", {})["Enabled"] = False
    for name in ("RecentRoms", "RecentMovies", "RecentLua", "RecentLuaSession"):
        config.setdefault(name, {})["AutoLoad"] = False
    for tool in config.get("CommonToolSettings", {}).values():
        if isinstance(tool, dict):
            tool["AutoLoad"] = False
    for item in config.get("PathEntries", {}).get("Paths", []):
        if item.get("Type") in ("Save RAM", "Savestates", "State"):
            item["Path"] = str(output_dir)
    path = output_dir / "isolated-bizhawk-config.ini"
    path.write_text(json.dumps(config, indent=2) + "\n", encoding="utf-8")
    return path


def run_one(args: argparse.Namespace, phase: str, count: int, depth: int,
    on_segment: Callable[[dict[str, object], list[tuple[int, ...]], bytes], None] | None = None,
    on_status: Callable[[dict[str, str], subprocess.Popen[bytes], float, int], None] | None = None
            ) -> dict[str, object]:
    output_dir = args.output_dir / phase / f"count-{count}"
    output_dir.mkdir(parents=True, exist_ok=True)
    raw, ack = output_dir / "live-forward-runtime.txt", output_dir / "live-forward-ack.txt"
    records1 = output_dir / "live-forward-wave-records-pass1.bin"
    records2 = output_dir / "live-forward-wave-records-pass2.bin"
    lua_log, console_log = output_dir / "bizhawk-lua.log", output_dir / "emuhawk-console.log"
    receipt_path = output_dir / "receipt.json"
    audit_path = output_dir / "segment-audits.jsonl"
    for path in (raw, ack, records1, records2, lua_log, receipt_path, audit_path):
        path.unlink(missing_ok=True)
    config = configure_install(args.install, output_dir)
    rom_sha = hashlib.sha256(args.rom.read_bytes()).hexdigest()
    artifact = args.install / "dll" / "gpgx.wbx"
    artifact_sha = hashlib.sha256(artifact.read_bytes()).hexdigest()
    memory = available_memory()
    available = memory["available_physical_bytes"]
    budget = calculate_resource_budget(available,
        native_budget_cap=args.native_budget_bytes,
        process_budget_cap=args.process_budget_bytes,
        core_reserve_cap=args.core_reserve_bytes,
        system_reserve=args.system_reserve_bytes,
        memory_bytes_each=args.memory_bytes)
    process_budget = budget["process_budget_bytes"]
    transport_budget = budget["host_transport_budget_bytes"]
    disk_transport_budget = (2 * count * args.memory_bytes +
        count * args.rounds * 1536 + count * 256 + 4 * 1024 * 1024)
    free_disk = shutil.disk_usage(output_dir).free
    core_reserve = budget["core_reserve_bytes"]
    native_budget = budget["native_budget_bytes"]
    global LAST_RUN_ID
    run_id = max(int(time.time()) + os.getpid() % 100000, LAST_RUN_ID + 1)
    LAST_RUN_ID = run_id
    env = os.environ.copy()
    env.update({"LF_OUTPUT": str(raw), "LF_ACK": str(ack), "LF_RECORD_DIR": str(output_dir),
                "LF_RUN_ID": str(run_id), "LF_COUNT": str(count), "LF_DEPTH": str(depth),
                "LF_MEMORY": str(args.memory_bytes), "LF_BUDGET_BYTES": str(native_budget),
                "LF_FREE_DISK_BYTES": str(free_disk),
                "LF_ROUNDS": str(args.rounds), "LF_MAX_FRAMES": str(args.max_frames),
                "BH_TEST_LOG": str(lua_log)})
    command = [str(args.install / "EmuHawk.exe"), f"--config={config}",
               f"--lua={args.script}", str(args.rom)]
    startup = subprocess.STARTUPINFO() if os.name == "nt" else None
    if startup:
        startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = SW_SHOWNORMAL
    if on_status:
        env.update({"LF_CONTROL_ENABLE": "1",
                    "LF_CONTROL_PREVIEW": str(output_dir / "live-worker-control-preview.txt")})
        env["LF_NEXT_WORKERS"] = str(count)
        env["LF_NEXT_DEPTH"] = str(depth)
    console_stream = console_log.open("wb")
    process = subprocess.Popen(command, cwd=args.install, env=env, startupinfo=startup,
                               stdout=console_stream, stderr=subprocess.STDOUT)
    tail = TailLines(raw)
    previous_exit = cycle_counts = lifecycle_counts = workers_summary = None
    global_entry, round_workers, overlap_by_round = [0], {}, {}
    audit_stream_deltas: dict[int, int] = {}
    audit_hash, audit_count, peak_ws = hashlib.sha256(), 0, 0
    started, failure = time.monotonic(), None
    process_exit_at: float | None = None
    pending_changes: dict[str, str] = {}
    last_status_publish = 0.0
    try:
        while time.monotonic() - started < args.timeout:
            changed = pending_changes or tail.poll()
            pending_changes = {}
            ws = working_set(process)
            if ws is not None:
                peak_ws = max(peak_ws, ws)
            now = time.monotonic()
            if on_status and now - last_status_publish >= 0.25:
                on_status(dict(tail.values), process, started, audit_count)
                last_status_publish = now
            for key, value in changed.items():
                if not key.startswith("SEG_"):
                    continue
                piece = key.split("_")
                if len(piece) != 3:
                    raise ValueError(f"malformed segment key: {key}")
                cycle, worker = int(piece[1]), int(piece[2])
                if cycle_counts is None:
                    previous_exit, cycle_counts, lifecycle_counts = [0] * count, [0] * count, [0] * count
                    workers_summary = [{"worker_id": index, "capture_count": 0,
                        "first_capture_id": None, "last_capture_id": None,
                        "first_entry_stream": None, "last_exit_stream": None,
                        "segment_chain_sha256": hashlib.sha256(b"").hexdigest()}
                        for index in range(count)]
                identity = (cycle, worker)
                if worker not in range(count) or cycle_counts[worker] + 1 != cycle:
                    raise ValueError(f"duplicate, skipped, or out-of-range segment {identity}")
                plan = integer_list(tail.values["PLAN"], len(PLAN_NAMES), "native plan")
                header_bytes = plan[7] - plan[3] * RECORD.size
                segment = validate_segment(key, value, records1, count, depth,
                    args.memory_bytes, header_bytes, run_id, previous_exit,
                    global_entry, cycle_counts, records2)
                if on_segment is not None:
                    offset = int(value.split("|")[4])
                    data = read_record_slice(records1, offset, int(segment["record_count"]), worker)
                    ready_segment = {**segment, "ready_for_cartographer": True}
                    on_segment(ready_segment, list(RECORD.iter_unpack(data)), data)
                current = round_workers.setdefault(cycle, [])
                if current and int(segment["entry_stream_sequence"]) <= int(
                        current[-1]["entry_stream_sequence"]):
                    raise ValueError(f"round {cycle}: Worker ENTRY sequence is not increasing")
                current.append(segment)
                worker_row = workers_summary[worker]
                worker_row["capture_count"] = cycle_counts[worker]
                worker_row["first_capture_id"] = worker_row["first_capture_id"] or segment["capture_id"]
                worker_row["last_capture_id"] = segment["capture_id"]
                worker_row["first_entry_stream"] = worker_row["first_entry_stream"] or segment[
                    "entry_stream_sequence"]
                worker_row["last_exit_stream"] = segment["exit_stream_sequence"]
                worker_row["segment_chain_sha256"] = hashlib.sha256(
                    (str(worker_row["segment_chain_sha256"]) + str(segment["segment_sha256"])).encode()
                ).hexdigest()
                encoded = (json.dumps(segment, sort_keys=True, separators=(",", ":")) + "\n").encode()
                with audit_path.open("ab") as target:
                    target.write(encoded)
                audit_hash.update(encoded)
                audit_count += 1
                with ack.open("a", encoding="ascii", newline="") as target:
                    target.write("|".join(str(item) for item in (cycle, worker,
                        segment["capture_id"], segment["generation"], run_id,
                        segment["epoch"], 1)) + "\n")
            for key, value in changed.items():
                if not key.startswith("LIFECYCLE_"):
                    continue
                piece = key.split("_")
                cycle, worker = int(piece[1]), int(piece[2])
                identity = (cycle, worker)
                if len(piece) != 3 or cycle_counts[worker] != cycle or \
                        lifecycle_counts[worker] + 1 != cycle:
                    raise ValueError(f"duplicate or unpaired per-Worker lifecycle receipt: {key}")
                counts = integer_list(value, 4, key)
                if cycle_counts[worker] != cycle or counts != [cycle] * 4:
                    raise ValueError(f"{key}: native FREE/CAPTURING/COMPLETE/ANALYZING cycle count mismatch")
                lifecycle_counts[worker] = cycle
                workers_summary[worker]["lifecycle_transition_counts"] = counts
            for key in changed:
                if key.startswith("ROUND_") and key.endswith("_ACKED"):
                    cycle = int(key[6:12])
                    if cycle not in round_workers or len(round_workers[cycle]) != count:
                        raise ValueError(f"round {cycle}: native FREE return count is incomplete")
                    if sum(value == cycle for value in lifecycle_counts) != count:
                        raise ValueError(f"round {cycle}: per-Worker lifecycle proof is incomplete")
                    delta_key = f"ROUND_{cycle:06d}_AUDIT_STREAM_DELTA"
                    stream_delta = int(tail.values.get(delta_key, "0"))
                    if stream_delta <= 0:
                        raise ValueError(f"round {cycle}: CPU did not advance during host audit")
                    audit_stream_deltas[cycle] = stream_delta
                    overlap_by_round[cycle] = overlap_peak(round_workers.pop(cycle))
            if tail.values.get("RESULT") == "FAIL":
                raise RuntimeError(tail.values.get("ERROR", "Lua lifecycle run failed"))
            if tail.values.get("RESULT") in ("RESOURCE_PREFLIGHT_REJECTED",
                "NATIVE_ALLOCATION_REJECTED", "HOST_TRANSPORT_PREFLIGHT_REJECTED"):
                break
            if tail.values.get("RESULT") == "PASS":
                break
            if process.poll() is not None:
                if process_exit_at is None:
                    process_exit_at = time.monotonic()
                pending_changes = tail.poll(final=True)
                if pending_changes:
                    continue
                if tail.values.get("RESULT") in ("PASS", "RESOURCE_PREFLIGHT_REJECTED",
                    "NATIVE_ALLOCATION_REJECTED", "HOST_TRANSPORT_PREFLIGHT_REJECTED"):
                    break
                if time.monotonic() - process_exit_at < PROCESS_EXIT_DRAIN_SECONDS:
                    time.sleep(0.01)
                    continue
                raise RuntimeError(f"EmuHawk exited early with code {process.returncode}")
            time.sleep(0.01)
        else:
            raise TimeoutError(f"100-cycle Worker scaling run exceeded {args.timeout}s")
        return_code = process.wait(timeout=15)
        console_stream.close()
        if return_code:
            raise RuntimeError(f"EmuHawk exited with code {return_code}")
        values = tail.values
        if on_status:
            on_status(dict(values), process, started, audit_count)
        plan = integer_list(values["PLAN"], len(PLAN_NAMES), "native allocation plan") \
            if values.get("PLAN") else []
        outcome = values.get("RESULT", "UNKNOWN")
        if outcome == "PASS":
            if audit_count != count * args.rounds:
                raise ValueError(f"audited {audit_count} segments, expected {count * args.rounds}")
            if sum(lifecycle_counts) != count * args.rounds:
                raise ValueError("per-Worker lifecycle receipts do not prove N*100 transitions")
            if any(cycles != args.rounds for cycles in cycle_counts):
                raise ValueError("at least one Worker completed fewer than 100 captures")
            if len(overlap_by_round) != args.rounds:
                raise ValueError("some native lifecycle rounds lack an exact FREE-return receipt")
        metrics = integer_list(values["FINAL_METRICS"], len(METRICS_NAMES), "final metrics") \
            if outcome == "PASS" else []
        recorder = integer_list(values["RECORDER_METRICS"], len(METRICS_NAMES), "recorder metrics") \
            if values.get("RECORDER_METRICS") else []
        captured = integer_list(values["CAPTURE_METRICS"], len(METRICS_NAMES), "capture metrics") \
            if values.get("CAPTURE_METRICS") else []
        summary: dict[str, object] = {
            "phase": phase, "configured_count": count, "depth": depth,
            "memory_bytes_each": args.memory_bytes, "required_cycles_per_worker": args.rounds,
            "required_completed_segments": count * args.rounds, "outcome": outcome,
            "plan": dict(zip(PLAN_NAMES, plan)) if plan else None,
            "preflight_reason": values.get("PREFLIGHT_REASON"),
            "allocation_budget_bytes": native_budget,
            "process_budget_bytes": process_budget, "host_transport_budget_bytes": transport_budget,
            "host_disk_transport_worst_case_bytes": disk_transport_budget,
            "available_host_disk_bytes": free_disk,
            "core_reserve_bytes": core_reserve, **memory,
            "peak_working_set_bytes": peak_ws or None, "rom_sha256": rom_sha,
            "native_artifact_sha256": artifact_sha, "run_id": run_id,
            "audited_segments": audit_count, "segment_audit_jsonl": str(audit_path.resolve()),
            "segment_audit_sha256": audit_hash.hexdigest(), "workers": workers_summary or [],
            "execution_window_overlap_by_round": overlap_by_round,
            "max_execution_window_overlap": max(overlap_by_round.values(), default=0),
            "host_audit_cpu_progress": {"rounds": len(audit_stream_deltas),
                "minimum_stream_records": min(audit_stream_deltas.values(), default=0),
                "total_stream_records": sum(audit_stream_deltas.values())},
            "recorder_metrics": dict(zip(METRICS_NAMES, recorder)) if recorder else None,
            "first_round_metrics": dict(zip(METRICS_NAMES, captured)) if captured else None,
            "final_metrics": dict(zip(METRICS_NAMES, metrics)) if metrics else None,
            "frame_timing": {key: values.get(key) for key in
                ("PERF_BASELINE", "PERF_RECORDER", "PERF_WORKER")},
            "capture_completion_wait_frames": int(values.get("CAPTURE_COMPLETION_WAIT_FRAMES", "0")),
            "ack_wait_frames": int(values.get("ACK_WAIT_FRAMES", "0")),
            "total_frames": int(values.get("TOTAL_FRAMES", "0")),
            "worker_run_wall_seconds": float(values.get("WORKER_RUN_WALL_SECONDS", "0")),
            "wall_seconds": time.monotonic() - started,
            "raw_log": str(raw.resolve()), "lua_log": str(lua_log.resolve()),
        }
        if outcome == "PASS":
            if metrics[9] != count * args.rounds or metrics[10] != count * args.rounds or \
                    metrics[11] != count * args.rounds or metrics[5] != 0 or metrics[1] != count:
                raise ValueError("native lifecycle counters do not reconcile with audited cycles")
            if metrics[12] or metrics[13] or metrics[14] or metrics[15] or metrics[16] or \
                    metrics[24] or metrics[23] != count * args.rounds:
                raise ValueError("native identity, audit, retention, or drop metrics are nonzero")
            summary["active_limit_pass"] = phase != "forced" or metrics[6] == count
        receipt_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({"phase": phase, "count": count, "outcome": outcome,
                          "audited_segments": audit_count, "receipt": str(receipt_path)}, indent=2))
        return summary
    except Exception as exc:
        failure = str(exc)
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
        console_stream.close()
        receipt = {"phase": phase, "configured_count": count, "depth": depth,
                   "required_cycles_per_worker": args.rounds,
                   "outcome": "STOP_MULTI_WORKER_LIFECYCLE_INVALID", "error": failure,
                   "audited_segments": audit_count,
                   "raw_log": str(raw.resolve()), "lua_log": str(lua_log.resolve()),
                   "console_log": str(console_log.resolve()),
                   "peak_working_set_bytes": peak_ws or None}
        receipt_path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        raise


def run_phase(args: argparse.Namespace, phase: str) -> dict[str, object]:
    results: list[dict[str, object]] = []
    no_growth, count, stop_reason = 0, (1 if phase == "natural" else 2), "MAX_COUNT_REACHED"
    while count <= args.max_count:
        depth = 20 if phase == "natural" else max(20, count)
        result = run_one(args, phase, count, depth)
        results.append(result)
        if result["outcome"] != "PASS":
            stop_reason = str(result["outcome"])
            break
        metrics = result.get("first_round_metrics") or {}
        previous = results[-2].get("first_round_metrics") or {} if len(results) > 1 else {}
        if phase == "natural" and previous:
            throughput = metrics["instructions"] - result["recorder_metrics"]["instructions"]
            previous_throughput = previous["instructions"] - results[-2]["recorder_metrics"]["instructions"]
            no_growth = no_growth + 1 if metrics["peak_capturing"] <= previous[
                "peak_capturing"] and throughput <= previous_throughput else 0
            if no_growth >= 2:
                stop_reason = "NATURAL_ACTIVE_SATURATION"
                break
        if phase == "forced" and not result.get("active_limit_pass"):
            stop_reason = "FORCED_CONCURRENCY_SATURATION"
            break
        count = 100000 if count == 65536 and args.max_count >= 100000 else count * 2
    verified = [item for item in results if item["outcome"] == "PASS"]
    first_failed = next((item["configured_count"] for item in results
                         if item["outcome"] != "PASS"), None)
    max_active = max((int(item["final_metrics"]["peak_capturing"])
                      for item in verified if item.get("final_metrics")), default=0)
    max_occupied = max((int(item["final_metrics"]["peak_occupied"])
                        for item in verified if item.get("final_metrics")), default=0)
    return {"phase": phase, "counts_tested": [item["configured_count"] for item in results],
            "max_configured_verified": max((item["configured_count"] for item in verified), default=0),
            "max_concurrent_capturing_verified": max_active,
            "max_occupied_verified": max_occupied,
            "natural_saturation_count": results[-1]["configured_count"]
                if stop_reason == "NATURAL_ACTIVE_SATURATION" else None,
            "first_failed_count": first_failed, "first_limit_type": stop_reason,
            "first_limit_detail": results[-1].get("outcome") if first_failed else None,
            "results": results}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--phase", choices=("natural", "forced", "both"), default="both")
    parser.add_argument("--memory-bytes", type=int, default=65536)
    parser.add_argument("--native-budget-bytes", type=int, default=256 * 1024 * 1024)
    parser.add_argument("--process-budget-bytes", type=int, default=512 * 1024 * 1024)
    parser.add_argument("--core-reserve-bytes", type=int, default=128 * 1024 * 1024)
    parser.add_argument("--system-reserve-bytes", type=int, default=4 * 1024 * 1024 * 1024)
    parser.add_argument("--max-frames", type=int, default=1800)
    parser.add_argument("--timeout", type=int, default=1800)
    parser.add_argument("--max-count", type=int, default=100000)
    parser.add_argument("--rounds", type=int, default=100)
    args = parser.parse_args()
    resolve_runtime_paths(args)
    if args.rounds != 100:
        parser.error("the acceptance gate requires exactly 100 completed cycles per Worker")
    if not (args.install / "EmuHawk.exe").is_file() or not args.rom.is_file() or not args.script.is_file():
        parser.error("install, ROM, or Lua script does not exist")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    phases = ("natural", "forced") if args.phase == "both" else (args.phase,)
    report = {"checkpoint": "M12-AUTO67-LIVE-FORWARD-WORKER-1B",
              "status": "IN_PROGRESS",
              "baseline_commit": "17780138a6e3d9c67c2bc83c76350c482c6a8140",
              "cycles_per_worker": 100, "memory_bytes_each": args.memory_bytes, "phases": []}
    for phase in phases:
        report["phases"].append(run_phase(args, phase))
    has_verified = any(phase["max_configured_verified"] > 0 for phase in report["phases"])
    report["status"] = "PASS_LIVE_FORWARD_MULTI_WORKER_SCALING" if has_verified else \
        "STOP_MULTI_WORKER_ALLOCATION_LIMIT"
    receipt = args.output_dir / "live-forward-scaling-receipt.json"
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "receipt": str(receipt)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
