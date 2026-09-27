import json
from pathlib import Path

audits_path = Path("build/m12-w6-live-discovery/natural/count-128/segment-audits.jsonl")
waves = {}
with audits_path.open("r", encoding="utf-8") as f:
    for line in f:
        item = json.loads(line)
        gen = item["generation"]
        if gen not in waves:
            waves[gen] = {
                "min_entry_frame": item["entry_frame"],
                "max_exit_frame": item["exit_frame"],
                "min_stream": item["entry_stream_sequence"],
                "max_stream": item["exit_stream_sequence"],
                "total_records": 0,
                "worker_count": 0,
            }
        w = waves[gen]
        w["min_entry_frame"] = min(w["min_entry_frame"], item["entry_frame"])
        w["max_exit_frame"] = max(w["max_exit_frame"], item["exit_frame"])
        w["min_stream"] = min(w["min_stream"], item["entry_stream_sequence"])
        w["max_stream"] = max(w["max_stream"], item["exit_stream_sequence"])
        w["total_records"] += item["record_count"]
        w["worker_count"] += 1

print(f"Total waves in audits: {len(waves)}")
sample_keys = [1, 2, 3, 4, 5, 10, 20, 50, 80, 100, 105, 106]
for g in sorted(waves.keys()):
    if g in sample_keys or g == len(waves):
        w = waves[g]
        print(f"Wave {g:3d}: entry_frame={w['min_entry_frame']:5d}, exit_frame={w['max_exit_frame']:5d}, records={w['total_records']:7d}, workers={w['worker_count']}")
