"""Run one exact 128-Worker W1 acceptance campaign with record counting."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from live_forward_scaling_runtime import resolve_runtime_paths, run_one


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()
    args.rounds = 100
    args.memory_bytes = 65536
    args.native_budget_bytes = 256 * 1024 * 1024
    args.process_budget_bytes = 512 * 1024 * 1024
    args.core_reserve_bytes = 128 * 1024 * 1024
    args.system_reserve_bytes = 4 * 1024 * 1024 * 1024
    args.max_frames = 1800
    args.timeout = 1800
    args.until_closed = False
    resolve_runtime_paths(args)
    counts = {"instruction_records": 0, "bus_read_records": 0,
              "bus_write_records": 0, "frame_records": 0,
              "exception_records": 0, "cpu_stop_records": 0,
              "other_records": 0, "records": 0, "bytes": 0,
              "widths": {}, "domains": {}, "fetch_flag_events": 0}
    event_flag, subtype_mask, subtype_shift = 0x8000, 0x3800, 11

    def on_segment(_segment: dict[str, object], rows: list[tuple[int, ...]], data: bytes) -> None:
        counts["bytes"] += len(data)
        for row in rows:
            flags = row[5]
            counts["records"] += 1
            if flags & 1:
                counts["instruction_records"] += 1
            elif flags & 256:
                counts["exception_records"] += 1
            elif flags & 1024:
                counts["cpu_stop_records"] += 1
            elif flags & event_flag:
                subtype = (flags & subtype_mask) >> subtype_shift
                key = {1: "bus_read_records", 2: "bus_write_records",
                       3: "frame_records"}.get(subtype)
                if key:
                    counts[key] += 1
                    if subtype in (1, 2):
                        width = {1: 8, 2: 16, 3: 32}.get((row[6] >> 16) & 3, 0)
                        domain = (row[6] >> 18) & 7
                        counts["widths"][str(width)] = counts["widths"].get(str(width), 0) + 1
                        counts["domains"][str(domain)] = counts["domains"].get(str(domain), 0) + 1
                        counts["fetch_flag_events"] += bool(flags & 512)
                else:
                    counts["other_records"] += 1
            else:
                counts["other_records"] += 1

    result = run_one(args, "final-128-depth100-counted", 128, 100, on_segment)
    receipt = {"status": result["outcome"], "native_artifact_sha256": result["native_artifact_sha256"],
               "counts": counts, "runtime": result}
    args.output_dir.mkdir(parents=True, exist_ok=True)
    path = args.output_dir / "final-counted-receipt.json"
    path.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": receipt["status"], "receipt": str(path)}, indent=2))
    return 0 if receipt["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
