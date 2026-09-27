#!/usr/bin/env python3
"""Progressively test natural and forced Worker pool scaling."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from live_forward_scaling_runtime import resolve_runtime_paths, run_one


def run_phase(args: argparse.Namespace, phase: str) -> dict[str, object]:
    results: list[dict[str, object]] = []
    start_count = getattr(args, "min_count", None) or (1 if phase == "natural" else 2)
    no_growth, count, stop_reason = 0, start_count, "MAX_COUNT_REACHED"
    while count <= args.max_count:
        depth = getattr(args, "depth", None) or (20 if phase == "natural" else max(20, count))
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
    parser.add_argument("--depth", type=int, default=None)
    parser.add_argument("--min-count", type=int, default=None)
    args = parser.parse_args()
    resolve_runtime_paths(args)
    if args.depth and args.memory_bytes == 65536:
        if args.depth >= 1000:
            args.memory_bytes = 1048576
            args.native_budget_bytes = max(args.native_budget_bytes, 512 * 1024 * 1024)
        elif args.depth >= 512:
            args.memory_bytes = 524288
            args.native_budget_bytes = max(args.native_budget_bytes, 384 * 1024 * 1024)
        elif args.depth >= 100:
            args.memory_bytes = 262144
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


import sys
if __name__ == "__main__":
    sys.exit(main())
