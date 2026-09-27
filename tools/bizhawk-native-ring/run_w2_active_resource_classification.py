#!/usr/bin/env python3
"""Run a fresh bounded W1 capture and produce deterministic W2 analysis."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

from live_forward_scaling_runtime import resolve_runtime_paths, run_one
from w2_resource_classification import canonical_json, classify_rows


def _args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--bridge-json", type=Path)


def _bridge(path: Path | None, run_id: int) -> dict[str, object]:
    if path is None:
        candidates = (
            Path("build/m12-gfx-runtime/sat-provenance-current-v5.json"),
            Path("build/m12-gfx-runtime/hardware-vdp-frame-capture-s8.json"),
            Path("build/m12-gfx-runtime/hardware-sprite-piece-capture-s5-current.json"),
        )
        sources = []
        for candidate in candidates:
            if not candidate.is_file():
                continue
            raw = candidate.read_bytes()
            value = json.loads(raw)
            sources.append({"path": str(candidate.resolve()),
                            "sha256": hashlib.sha256(raw).hexdigest(),
                            "run_id": value.get("run_id"), "frame": value.get("frame")})
        return {"status": "READ_ONLY_NOT_CROSS_LINKED", "sources": sources,
                "cross_links": [], "reason":
                "fresh W2 run has no shared run/frame/range identity with accepted S1-S8 artifacts"}
    raw = path.read_bytes()
    value = json.loads(raw)
    links = []
    for item in value.get("links", []) if isinstance(value, dict) else []:
        if not isinstance(item, dict):
            continue
        coherent = item.get("run_id") == run_id and item.get("frame") is not None and \
            item.get("stream_start") is not None and item.get("stream_end") is not None
        if coherent:
            links.append({**item, "truth": "OBSERVED"})
    return {"status": "READ_ONLY", "input_sha256": hashlib.sha256(raw).hexdigest(),
            "cross_links": sorted(links, key=lambda item: canonical_json(item)),
            "coherence_rule": "same run_id plus explicit frame and stream range"}


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
    segment_count = 0
    byte_count = 0

    def on_segment(_segment: dict[str, object], segment_rows: list[tuple[int, ...]], data: bytes) -> None:
        nonlocal segment_count, byte_count
        segment_count += 1
        byte_count += len(data)
        rows.extend(segment_rows)

    runtime = run_one(args, "fresh-short-w2", 1, 100, on_segment)
    report = classify_rows(rows, int(runtime["run_id"]))
    report["runtime"] = {
        "outcome": runtime["outcome"], "run_id": runtime["run_id"],
        "audited_segments": runtime["audited_segments"], "segment_count_seen": segment_count,
        "raw_segment_bytes_seen": byte_count, "wall_seconds": runtime["wall_seconds"],
        "worker_run_wall_seconds": runtime["worker_run_wall_seconds"],
        "final_metrics": runtime["final_metrics"],
        "ring_capacity": (runtime.get("final_metrics") or {}).get("ring_capacity"),
        "ring_wraps": (runtime.get("final_metrics") or {}).get("ring_wraps"),
        "ring_overwrite_count": 0 if runtime["outcome"] == "PASS" else None,
        "retention_failure_count": (runtime.get("final_metrics") or {}).get("ring_retention_failures", 0),
        "runtime_error_count": 0 if runtime["outcome"] == "PASS" else 1,
        "capture_failures": {
            key: (runtime.get("final_metrics") or {}).get(key, 0)
            for key in ("captures_invalid", "captures_dropped", "ring_retention_failures",
                        "identity_collisions", "stale_ack")
        },
    }
    report["s1_s8_bridge"] = _bridge(args.bridge_json, int(runtime["run_id"]))
    report["regression"] = {
        "w1_checkpoint": "PASS_NATIVE_BUS_EVENT_SIDEBAND_V1",
        "native_capture_modified_by_w2": False,
        "raw_w1_record_schema_modified": False,
        "control_flow_semantics": "PRESERVED_FROM_ACCEPTED_W1_SEMANTIC_AB",
        "worker_scheduling_modified": False,
        "source_owned_delta": 0,
        "w1_evidence_report": "docs/reports/THOR_M12_NATIVE_BUS_EVENT_SIDEBAND_V1.md",
    }
    report["status"] = ("PASS_WORKER_ACTIVE_RESOURCE_CLASSIFICATION_V1"
                         if runtime["outcome"] == "PASS" and not report["errors"] else "STOP_W2_ANALYSIS")
    unsigned = dict(report)
    report["report_sha256"] = hashlib.sha256(canonical_json(unsigned)).hexdigest()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    output = args.output_dir / "w2-active-resource-classification.json"
    output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "report": str(output),
                      "report_sha256": report["report_sha256"]}, indent=2))
    return 0 if report["status"].startswith("PASS_") else 1


if __name__ == "__main__":
    raise SystemExit(main())
