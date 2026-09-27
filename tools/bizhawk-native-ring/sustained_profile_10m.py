#!/usr/bin/env python3
"""Run a sustained 10-minute Beyond Oasis profiling session with 128 Workers at depth 100."""

from __future__ import annotations

import argparse
import ctypes
import gc
import json
import os
from pathlib import Path
import time

from live_forward_scaling_audit import integer_list
from live_forward_scaling_runtime import (
    METRICS_NAMES, PLAN_NAMES, _ProcessMemoryCounters,
    resolve_runtime_paths, run_one,
)


def get_process_memory(pid: int) -> tuple[int, int, int, int]:
    """Return (working_set, private_bytes, peak_working_set, peak_private_bytes)."""
    handle = ctypes.windll.kernel32.OpenProcess(0x1000, False, pid)
    if not handle:
        return 0, 0, 0, 0
    try:
        counters = _ProcessMemoryCounters()
        counters.cb = ctypes.sizeof(counters)
        ok = ctypes.windll.psapi.GetProcessMemoryInfo(handle, ctypes.byref(counters), counters.cb)
        if ok:
            return (int(counters.working_set), int(counters.pagefile),
                    int(counters.peak_working_set), int(counters.peak_pagefile))
        return 0, 0, 0, 0
    finally:
        ctypes.windll.kernel32.CloseHandle(handle)


def compute_slope(samples: list[dict[str, object]], key: str) -> float:
    """Compute linear regression slope in MB per minute for samples between 60s and 600s."""
    points = [(s["elapsed_sec"] / 60.0, s[key] / (1024 * 1024))
              for s in samples if 60.0 <= s["elapsed_sec"] <= 600.0]
    if len(points) < 2:
        return 0.0
    n = len(points)
    mean_x = sum(p[0] for p in points) / n
    mean_y = sum(p[1] for p in points) / n
    denom = sum((p[0] - mean_x) ** 2 for p in points)
    if denom == 0:
        return 0.0
    numer = sum((p[0] - mean_x) * (p[1] - mean_y) for p in points)
    return numer / denom


def find_nearest_sample(samples: list[dict[str, object]], target_sec: float) -> dict[str, object]:
    """Find the sample closest to the target elapsed time."""
    if not samples:
        return {}
    return min(samples, key=lambda s: abs(s["elapsed_sec"] - target_sec))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", type=Path,
                        default=Path(r"C:\Dev\SegaThorTools\BizHawk-m12-w2-1-frame-coherent-20260920"))
    parser.add_argument("--rom", type=Path, default=Path(r"local-roms\Beyond Oasis (USA).md"))
    parser.add_argument("--script", type=Path,
                        default=Path(r"tools\bizhawk-native-ring\live_forward_scaling.lua"))
    parser.add_argument("--output-dir", type=Path,
                        default=Path(r"scaling-output-sustained-10m"))
    parser.add_argument("--duration", type=float, default=610.0,
                        help="Duration in seconds (default: 610s = >10 min)")
    parser.add_argument("--sample-interval", type=float, default=5.0,
                        help="Sampling interval in seconds (default: 5.0s)")
    parser.add_argument("--count", type=int, default=128)
    parser.add_argument("--depth", type=int, default=100)
    parser.add_argument("--memory-bytes", type=int, default=262144)
    args = parser.parse_args()

    args.rounds = 0
    args.until_closed = True
    args.max_frames = 1800
    args.timeout = int(args.duration + 240)
    args.native_budget_bytes = 256 * 1024 * 1024
    args.process_budget_bytes = 512 * 1024 * 1024
    args.core_reserve_bytes = 128 * 1024 * 1024
    args.system_reserve_bytes = 4 * 1024 * 1024 * 1024

    resolve_runtime_paths(args)
    count_dir = args.output_dir / "natural" / f"count-{args.count}"
    count_dir.mkdir(parents=True, exist_ok=True)
    end_game_file = count_dir / "end-game-request.txt"
    end_game_file.unlink(missing_ok=True)
    args.end_game_path = end_game_file

    samples: list[dict[str, object]] = []
    final_analysis_samples: list[dict[str, object]] = []
    state = {
        "last_sample": 0.0,
        "last_log": 0.0,
        "last_records": 0,
        "last_records_time": 0.0,
        "end_game_triggered": False,
        "end_game_time": 0.0,
        "plan": {},
        "peak_analysis_ws": 0,
        "peak_analysis_pagefile": 0,
    }

    startup_py_ws, startup_py_priv, _, _ = get_process_memory(os.getpid())

    def on_status(values: dict[str, str], process, started: float,
                  audit_count: int, workers_summary: list[dict[str, object]] | None) -> None:
        now = time.monotonic()
        elapsed = now - started

        if not state["plan"] and values.get("PLAN"):
            plan_vals = integer_list(values["PLAN"], len(PLAN_NAMES), "native plan")
            state["plan"] = dict(zip(PLAN_NAMES, plan_vals))

        # Check if 10-minute duration elapsed to trigger END GAME
        if elapsed >= args.duration and not state["end_game_triggered"]:
            state["end_game_triggered"] = True
            state["end_game_time"] = elapsed
            end_game_file.write_text("END_GAME\n", encoding="ascii")
            print(f"\n[SUSTAINED] Reached {elapsed:.1f}s >= {args.duration:.1f}s. Triggered END GAME at {end_game_file}")

        # If END GAME triggered, record high-frequency final analysis samples
        if state["end_game_triggered"] and process.poll() is None:
            ws, priv, peak_ws, peak_priv = get_process_memory(process.pid)
            state["peak_analysis_ws"] = max(state["peak_analysis_ws"], ws)
            state["peak_analysis_pagefile"] = max(state["peak_analysis_pagefile"], priv)
            final_analysis_samples.append({
                "elapsed_sec": round(elapsed, 3),
                "working_set": ws,
                "private_bytes": priv,
            })

        # Regular 5-second sampling
        if now - state["last_sample"] >= args.sample_interval or not samples:
            state["last_sample"] = now
            if process.poll() is None:
                ws, priv, peak_ws, peak_priv = get_process_memory(process.pid)
            else:
                ws, priv, peak_ws, peak_priv = 0, 0, 0, 0

            # Temporary host buffers size
            temp_bytes = sum(f.stat().st_size for f in count_dir.iterdir() if f.is_file())

            # Parse round metrics
            metrics_raw = values.get("ROUND_METRICS") or values.get("FINAL_METRICS") or ""
            metrics = integer_list(metrics_raw, len(METRICS_NAMES), "metrics") if metrics_raw else []

            active_workers = metrics[3] if len(metrics) > 3 else args.count
            records = metrics[20] if len(metrics) > 20 else 0
            retained_segment_bytes = metrics[25] if len(metrics) > 25 else 0
            ring_wraps = metrics[26] if len(metrics) > 26 else 0
            retention_failures = metrics[16] if len(metrics) > 16 else 0

            # Compute throughput
            interval_dt = now - state["last_records_time"] if state["last_records_time"] > 0 else elapsed
            interval_drec = records - state["last_records"] if state["last_records_time"] > 0 else records
            interval_tput = (interval_drec / interval_dt) if interval_dt > 0 else 0.0
            overall_tput = (records / elapsed) if elapsed > 0 else 0.0

            state["last_records"] = records
            state["last_records_time"] = now

            sample = {
                "elapsed_sec": round(elapsed, 2),
                "process_working_set_bytes": ws,
                "process_private_bytes": priv,
                "peak_process_working_set_bytes": peak_ws,
                "peak_process_private_bytes": peak_priv,
                "waterbox_native_heap_bytes": state["plan"].get("total_native_bytes", 0),
                "ring_allocation_bytes": state["plan"].get("shared_ring_bytes", 0),
                "active_worker_count": active_workers,
                "retained_segment_bytes": retained_segment_bytes,
                "temporary_host_buffers_bytes": temp_bytes,
                "records_produced": records,
                "interval_record_throughput": round(interval_tput, 1),
                "overall_record_throughput": round(overall_tput, 1),
                "ring_wraps": ring_wraps,
                "ring_retention_failures": retention_failures,
                "audited_segments": audit_count,
            }
            samples.append(sample)

            if now - state["last_log"] >= 30.0 or len(samples) == 1:
                state["last_log"] = now
                print(f"[{elapsed:6.1f}s] WS: {ws / (1024*1024):6.1f} MB | "
                      f"Priv: {priv / (1024*1024):6.1f} MB | "
                      f"Ring: {state['plan'].get('shared_ring_bytes', 0)/(1024*1024):.0f} MB | "
                      f"Wraps: {ring_wraps:3d} | "
                      f"Throughput: {interval_tput:8.0f} rec/s | "
                      f"Failures: {retention_failures}")

    print(f"Starting sustained 10-minute run (128 workers, depth {args.depth})...")
    print(f"Sampling every {args.sample_interval}s, total duration >= {args.duration}s\n")

    summary = run_one(args, "natural", args.count, args.depth, on_status=on_status)

    # Cleanup phase measurement
    gc.collect()
    time.sleep(0.5)
    post_cleanup_py_ws, post_cleanup_py_priv, _, _ = get_process_memory(os.getpid())

    # Compile final telemetry
    startup_sample = find_nearest_sample(samples, 0.0)
    init_steady_sample = samples[1] if len(samples) > 1 else startup_sample
    m1_sample = find_nearest_sample(samples, 60.0)
    m5_sample = find_nearest_sample(samples, 300.0)
    m10_sample = find_nearest_sample(samples, 600.0)

    all_ws = [s["process_working_set_bytes"] for s in samples if s["process_working_set_bytes"] > 0]
    all_priv = [s["process_private_bytes"] for s in samples if s["process_private_bytes"] > 0]

    peak_ws = max(all_ws, default=0)
    peak_priv = max(all_priv, default=0)

    ws_slope = compute_slope(samples, "process_working_set_bytes")
    priv_slope = compute_slope(samples, "process_private_bytes")

    # Classification logic
    # If slope is minimal (< 1.5 MB/min) and bounded, classify as STABLE or BOUNDED_OSCILLATING
    if abs(priv_slope) < 0.5 and (peak_priv - min(all_priv, default=0)) < 16 * 1024 * 1024:
        classification = "STABLE"
    elif abs(priv_slope) < 2.0:
        classification = "BOUNDED_OSCILLATING"
    else:
        classification = "CONTINUOUSLY_GROWING"

    final_metrics = summary.get("final_metrics") or {}
    total_retention_failures = final_metrics.get("ring_retention_failures", 0)
    total_records = final_metrics.get("records_produced", 0)
    total_wraps = final_metrics.get("ring_wraps", 0)

    report = {
        "checkpoint": "M12-W3-SUSTAINED-10MIN-PROFILE",
        "duration_target_seconds": args.duration,
        "actual_wall_seconds": summary.get("wall_seconds", 0.0),
        "outcome": summary.get("outcome"),
        "classification": classification,
        "ring_capacity_slots": 2097152,
        "ring_allocation_bytes": state["plan"].get("shared_ring_bytes", 0),
        "waterbox_native_heap_bytes": state["plan"].get("total_native_bytes", 0),
        "configured_workers": args.count,
        "depth": args.depth,
        "audited_segments": summary.get("audited_segments", 0),
        "records_produced": total_records,
        "ring_wraps": total_wraps,
        "ring_retention_failures": total_retention_failures,
        "memory_report": {
            "startup_launch": {
                "elapsed_sec": startup_sample.get("elapsed_sec"),
                "working_set_mb": round(startup_sample.get("process_working_set_bytes", 0) / (1024 * 1024), 2),
                "private_bytes_mb": round(startup_sample.get("process_private_bytes", 0) / (1024 * 1024), 2),
            },
            "startup_steady_state": {
                "elapsed_sec": init_steady_sample.get("elapsed_sec"),
                "working_set_mb": round(init_steady_sample.get("process_working_set_bytes", 0) / (1024 * 1024), 2),
                "private_bytes_mb": round(init_steady_sample.get("process_private_bytes", 0) / (1024 * 1024), 2),
            },
            "minute_1": {
                "elapsed_sec": m1_sample.get("elapsed_sec"),
                "working_set_mb": round(m1_sample.get("process_working_set_bytes", 0) / (1024 * 1024), 2),
                "private_bytes_mb": round(m1_sample.get("process_private_bytes", 0) / (1024 * 1024), 2),
            },
            "minute_5": {
                "elapsed_sec": m5_sample.get("elapsed_sec"),
                "working_set_mb": round(m5_sample.get("process_working_set_bytes", 0) / (1024 * 1024), 2),
                "private_bytes_mb": round(m5_sample.get("process_private_bytes", 0) / (1024 * 1024), 2),
            },
            "minute_10": {
                "elapsed_sec": m10_sample.get("elapsed_sec"),
                "working_set_mb": round(m10_sample.get("process_working_set_bytes", 0) / (1024 * 1024), 2),
                "private_bytes_mb": round(m10_sample.get("process_private_bytes", 0) / (1024 * 1024), 2),
            },
            "peak": {
                "working_set_mb": round(peak_ws / (1024 * 1024), 2),
                "private_bytes_mb": round(peak_priv / (1024 * 1024), 2),
            },
            "slopes": {
                "working_set_mb_per_min": round(ws_slope, 4),
                "private_bytes_mb_per_min": round(priv_slope, 4),
            },
            "end_game_analysis": {
                "peak_working_set_mb": round(state["peak_analysis_ws"] / (1024 * 1024), 2),
                "peak_private_bytes_mb": round(state["peak_analysis_pagefile"] / (1024 * 1024), 2),
            },
            "post_cleanup": {
                "python_working_set_mb": round(post_cleanup_py_ws / (1024 * 1024), 2),
                "python_private_bytes_mb": round(post_cleanup_py_priv / (1024 * 1024), 2),
                "python_ws_delta_from_start_mb": round((post_cleanup_py_ws - startup_py_ws) / (1024 * 1024), 2),
            },
        },
        "samples_count": len(samples),
        "samples": samples,
    }

    report_path = args.output_dir / "sustained-profile-report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    print("\n" + "=" * 60)
    print("SUSTAINED 10-MINUTE PROFILE SUMMARY")
    print("=" * 60)
    print(f"Outcome:                   {report['outcome']}")
    print(f"Memory Classification:     {report['classification']}")
    print(f"Wall Seconds:              {report['actual_wall_seconds']:.1f} s")
    print(f"Audited Segments:          {report['audited_segments']}")
    print(f"Records Produced:          {report['records_produced']:,}")
    print(f"Ring Wraps:                {report['ring_wraps']}")
    print(f"Ring Retention Failures:   {report['ring_retention_failures']}")
    print(f"Ring Allocation:           {report['ring_allocation_bytes'] / (1024 * 1024):.1f} MiB")
    print(f"Waterbox Native Heap:      {report['waterbox_native_heap_bytes'] / (1024 * 1024):.1f} MiB")
    print("-" * 60)
    start_launch = report['memory_report']['startup_launch']
    start_steady = report['memory_report']['startup_steady_state']
    print(f"Startup (Launch):          WS: {start_launch['working_set_mb']:.1f} MB | Priv: {start_launch['private_bytes_mb']:.1f} MB")
    print(f"Startup (Steady Core):     WS: {start_steady['working_set_mb']:.1f} MB | Priv: {start_steady['private_bytes_mb']:.1f} MB")
    print(f"1-Minute Memory:           WS: {report['memory_report']['minute_1']['working_set_mb']:.1f} MB | Priv: {report['memory_report']['minute_1']['private_bytes_mb']:.1f} MB")
    print(f"5-Minute Memory:           WS: {report['memory_report']['minute_5']['working_set_mb']:.1f} MB | Priv: {report['memory_report']['minute_5']['private_bytes_mb']:.1f} MB")
    print(f"10-Minute Memory:          WS: {report['memory_report']['minute_10']['working_set_mb']:.1f} MB | Priv: {report['memory_report']['minute_10']['private_bytes_mb']:.1f} MB")
    print(f"Peak Memory:               WS: {report['memory_report']['peak']['working_set_mb']:.1f} MB | Priv: {report['memory_report']['peak']['private_bytes_mb']:.1f} MB")
    print(f"Memory Slope (Priv):       {report['memory_report']['slopes']['private_bytes_mb_per_min']:+.4f} MB/min")
    print(f"Peak During Analysis:      WS: {report['memory_report']['end_game_analysis']['peak_working_set_mb']:.1f} MB | Priv: {report['memory_report']['end_game_analysis']['peak_private_bytes_mb']:.1f} MB")
    print(f"Python Post-Cleanup Delta: {report['memory_report']['post_cleanup']['python_ws_delta_from_start_mb']:+.2f} MB")
    print("=" * 60)
    print(f"Report written to: {report_path.resolve()}")

    return 0 if report["outcome"] in ("PASS", "STOPPED_END_GAME") and total_retention_failures == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
