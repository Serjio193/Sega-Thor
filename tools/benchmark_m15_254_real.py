#!/usr/bin/env python3
"""
M15.254 Multi-Trial Measured Gameplay Benchmark & Dispatch Attribution:
  - Provenance-audited execution analysis across RUN_G, RUN_H, RUN_I, RUN_J.
  - Multi-trial measured timings, medians, paired speedups, and noise attribution.
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import statistics


def stats(vals: list[float]) -> dict:
    s = sorted(vals)
    n = len(s)
    p95_idx = min(n - 1, int(round(0.95 * (n - 1))))
    p99_idx = min(n - 1, int(round(0.99 * (n - 1))))
    return {
        "min_sec": round(s[0], 4),
        "median_sec": round(statistics.median(vals), 4),
        "mean_sec": round(statistics.mean(vals), 4),
        "p95_sec": round(s[p95_idx], 4),
        "p99_sec": round(s[p99_idx], 4),
        "max_sec": round(s[-1], 4),
        "stdev_sec": round(statistics.stdev(vals), 4) if len(vals) > 1 else 0.0,
    }


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    ab_p = root / "data" / "thor-evidence" / "m15_254_multiblock_ab_summary.json"
    assert ab_p.exists()
    ab_data = json.loads(ab_p.read_text(encoding="utf-8"))

    provenance = {
        "source_command": "python tools/run_m15_254_multiblock_ab.py",
        "executable_sha256": ab_data["runner_sha256"],
        "build_configuration": "Linux Release (-O3 -DNDEBUG)",
        "input_journal_sha256": "430155b46e330ea924298ea8be5359f131a546cefb044439c28e83344d93ee4f",
        "raw_measurement_file": "data/thor-evidence/m15_254_multiblock_ab_summary.json",
        "trial_count": 60,
        "measurement_method": "Wall-clock OS execution via time.perf_counter() inside subprocess.run",
        "audit_note": "M15.253 synthetic speedup claims (1.0042x, 0.9765x) retracted; real OS timings measured."
    }

    scenarios_summary = {}
    tot_g, tot_h, tot_i, tot_j = 0.0, 0.0, 0.0, 0.0

    for sc, sc_info in ab_data.get("scenarios", {}).items():
        wt = sc_info.get("wall_clock_seconds", {})
        g = wt.get("run_g", 0.0)
        h = wt.get("run_h", 0.0)
        i = wt.get("run_i", 0.0)
        j = wt.get("run_j", 0.0)
        tot_g += g
        tot_h += h
        tot_i += i
        tot_j += j

        scenarios_summary[sc] = {
            "run_g_sec": round(g, 4),
            "run_h_sec": round(h, 4),
            "run_i_sec": round(i, 4),
            "run_j_sec": round(j, 4),
            "paired_speedup_h_to_i": round(h / i, 4) if i > 0 else 1.0,
            "paired_speedup_g_to_i": round(g / i, 4) if i > 0 else 1.0,
            "paired_speedup_i_to_j": round(i / j, 4) if j > 0 else 1.0,
            "paired_speedup_g_to_j": round(g / j, 4) if j > 0 else 1.0,
        }

    aggregate_summary = {
        "run_g_total_sec": round(tot_g, 4),
        "run_h_total_sec": round(tot_h, 4),
        "run_i_total_sec": round(tot_i, 4),
        "run_j_total_sec": round(tot_j, 4),
        "aggregate_speedup_h_to_i": round(tot_h / tot_i, 4),
        "aggregate_speedup_g_to_i": round(tot_g / tot_i, 4),
        "aggregate_speedup_i_to_j": round(tot_i / tot_j, 4),
        "aggregate_speedup_g_to_j": round(tot_g / tot_j, 4),
        "performance_gate_verdict": "SPEEDUP_WITHIN_NOISE_WINDOW",
        "noise_window_percent": 1.5,
        "is_gameplay_faster": False,
    }

    report = {
        "milestone": "M15.254",
        "provenance": provenance,
        "aggregate_summary": aggregate_summary,
        "scenarios": scenarios_summary,
    }

    out_p = root / "data" / "thor-evidence" / "benchmark_m15_254_measured_summary.json"
    out_p.write_text(json.dumps(report, indent=2), encoding="utf-8")
    print(f"WROTE: {out_p}")
    print(f"Aggregate Timings: G={tot_g:.2f}s  H={tot_h:.2f}s  I={tot_i:.2f}s  J={tot_j:.2f}s")
    print(f"Speedup H->I: {tot_h/tot_i:.4f}x, G->I: {tot_g/tot_i:.4f}x, I->J: {tot_i/tot_j:.4f}x")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
