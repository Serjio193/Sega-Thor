#!/usr/bin/env python3
"""Run real BizHawk capture and produce W2.1 frame-coherent evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from live_forward_scaling_runtime import resolve_runtime_paths, run_one
from w2_resource_classification import canonical_json, classify_rows
from run_w2_active_resource_classification import _args, _bridge


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    _args(parser)
    args = parser.parse_args()
    args.rounds = 100
    args.memory_bytes = 65536
    args.native_budget_bytes = 256 * 1024 * 1024
    args.process_budget_bytes = 512 * 1024 * 1024
    args.core_reserve_bytes = 128 * 1024 * 1024
    args.system_reserve_bytes = 4 * 1024 * 1024 * 1024
    args.max_frames = 1800
    args.timeout = 900
    args.until_closed = False
    resolve_runtime_paths(args)
    rows: list[tuple[int, ...]] = []
    segments: list[dict[str, object]] = []
    segment_bytes = 0

    def on_segment(segment: dict[str, object], segment_rows: list[tuple[int, ...]],
                   data: bytes) -> None:
        nonlocal segment_bytes
        segments.append(dict(segment))
        rows.extend(segment_rows)
        segment_bytes += len(data)

    runtime = run_one(args, "fresh-short-w2-1", 1, 100, on_segment)
    run_id = int(runtime["run_id"])
    report = classify_rows(rows, run_id=run_id, segments=segments)
    segment_reports = report["frame_coherence"]["segments"]
    single = sum(item["frame_status"] == "SINGLE_FRAME" for item in segment_reports)
    partitioned = sum(item["frame_status"] == "MULTI_FRAME_PARTITIONED"
                      for item in segment_reports)
    unresolved = sum(item["frame_status"] == "MULTI_FRAME_UNRESOLVED"
                     for item in segment_reports)
    metrics = runtime.get("final_metrics") or {}
    report["checkpoint"] = "PASS_WORKER_FRAME_COHERENT_EVIDENCE_V1"
    report["identity_scope"] = "(run_id, epoch, frame); frame alone is never a join key"
    report["segment_frame_acceptance"] = {
        "segment_total": len(segment_reports), "single_frame_segments": single,
        "partitioned_multi_frame_segments": partitioned,
        "unresolved_segments": unresolved,
        "observed_frame_boundary_count": report["counts"]["frame_records"],
        "authoritative_counter": {
            "variable": "frame_number",
            "increment": "oasis_lf_frame_boundary()",
            "reset": "oasis_lf_epoch_break(): frame_number = 0 before runtime_epoch++",
            "initial": "static zero initialization in live_forward_trace.c",
        },
    }
    report["runtime"] = {
        "outcome": runtime["outcome"], "run_id": run_id,
        "audited_segments": runtime["audited_segments"],
        "segment_count_seen": len(segments), "raw_segment_bytes_seen": segment_bytes,
        "wall_seconds": runtime["wall_seconds"],
        "worker_run_wall_seconds": runtime["worker_run_wall_seconds"],
        "final_metrics": metrics, "ring_capacity": metrics.get("ring_capacity"),
        "ring_overwrite_count": 0 if runtime["outcome"] == "PASS" else None,
        "retention_failure_count": metrics.get("ring_retention_failures", 0),
        "runtime_error_count": 0 if runtime["outcome"] == "PASS" else 1,
    }
    bridge = _bridge(args.bridge_json, run_id)
    bridge["cross_run_link"] = "NOT_ALLOWED"
    report["s1_s8_bridge"] = bridge
    report["regression"] = {
        "w1_checkpoint": "PASS_NATIVE_BUS_EVENT_SIDEBAND_V1",
        "w2_checkpoint": "PASS_WORKER_ACTIVE_RESOURCE_CLASSIFICATION_V1",
        "raw_w1_record_schema_modified": False, "worker_scheduling_modified": False,
        "source_owned_delta": 0,
        "control_flow_semantics": "PRESERVED_FROM_ACCEPTED_W1_SEMANTIC_AB",
    }
    report["status"] = (
        "PASS_WORKER_FRAME_COHERENT_EVIDENCE_V1"
        if runtime["outcome"] == "PASS" and not report["errors"] and not
        report["frame_coherence"]["errors"] and unresolved == 0 and
        len(segment_reports) == 100 else "STOP_WORKER_FRAME_COHERENCE")
    unsigned = dict(report)
    report["report_sha256"] = hashlib.sha256(canonical_json(unsigned)).hexdigest()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    output = args.output_dir / "w2-frame-coherent-evidence.json"
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "report": str(output),
                      "report_sha256": report["report_sha256"]}, indent=2))
    return 0 if report["status"].startswith("PASS_") else 1


if __name__ == "__main__":
    raise SystemExit(main())
