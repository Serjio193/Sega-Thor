#!/usr/bin/env python3
"""
M15.258 Phase 8 Independent Real OS Process Benchmark Orchestrator.
Runs 10 interleaved paired trials per config (RUN_G, RUN_L, RUN_M) across all 6 scenarios
(180 total OS process runs) pinned to isolated physical cores (0, 2, 4, 6, 8, 10) via taskset.
Measures OS wall-clock time and OS child process host CPU time (utime + stime), computes medians,
paired differences, standard deviation, and 95% confidence intervals.
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import math
import pathlib
import statistics
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter

RUNNER_PATH = pathlib.Path("build/auto-migrator/m15_250/linux-runner/oasis_gameplay_session").resolve()
CORE_PATH = pathlib.Path("build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so").resolve()
ROM_PATH = pathlib.Path("local-roms/Beyond Oasis (USA).md").resolve()


def to_wsl(p: pathlib.Path) -> str:
    r = p.resolve()
    drive = r.drive.replace(":", "").lower()
    posix = r.as_posix()[len(r.drive):] if r.drive else r.as_posix()
    return f"/mnt/{drive}{posix}"


def summarize(vals: list[float]) -> dict:
    s = sorted(vals)
    n = len(s)
    mean = statistics.mean(vals)
    stdev = statistics.stdev(vals) if n > 1 else 0.0
    t_95 = 2.262 if n == 10 else 1.96
    ci95 = t_95 * (stdev / math.sqrt(n)) if n > 1 else 0.0
    return {
        "min": round(s[0], 4),
        "median": round(statistics.median(vals), 4),
        "mean": round(mean, 4),
        "max": round(s[-1], 4),
        "stdev": round(stdev, 4),
        "ci95_halfwidth": round(ci95, 4),
        "ci95_pct": round((ci95 / mean) * 100.0, 2) if mean > 0 else 0.0,
    }


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    out_dir = root / "build" / "benchmark_m15_258"
    out_dir.mkdir(parents=True, exist_ok=True)

    src_cab = root / "build/ab_portable_m15_244"
    src_lr = root / "build/m15_246/longrun_3600f"
    src_alt = root / "build/m15_247/longrun_alt_1800f"
    src_cbt = root / "build/m15_248/longrun_combat_1800f"

    scenarios = [
        ("scenario_a", src_cab / "scenario_a/run_p0", 0),
        ("scenario_b", src_cab / "scenario_b/run_p0", 2),
        ("scenario_c", src_cab / "scenario_c/run_p0", 4),
        ("longrun_3600f", src_lr, 6),
        ("longrun_alt_1800f", src_alt, 8),
        ("longrun_combat_1800f", src_cbt, 10),
    ]

    adapter = StatusAdapter()
    prod_csv = ",".join(f"{h:06x}" for h in sorted(adapter.active_set))

    runner_wsl = to_wsl(RUNNER_PATH)
    core_wsl = to_wsl(CORE_PATH)
    rom_wsl = to_wsl(ROM_PATH)
    worker_wsl = to_wsl(root / "tools/benchmark_m15_258_worker.py")

    trials = 10
    procs = []
    print(f"Launching 6 core-pinned workers ({trials} paired trials x 3 configs = {trials*3*6} OS processes)...")
    for scen, src_dir, cpu_id in scenarios:
        out_json = out_dir / f"{scen}_raw.json"
        cmd = [
            "wsl", "taskset", "-c", str(cpu_id), "python3", worker_wsl,
            scen, to_wsl(src_dir), to_wsl(out_json),
            runner_wsl, core_wsl, rom_wsl, prod_csv, str(trials)
        ]
        procs.append((scen, out_json, subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)))

    for scen, _, p in procs:
        out, err = p.communicate()
        if out:
            print(out.strip())
        if p.returncode != 0:
            print(f"ERROR in {scen}: {err}")
            return 1

    per_scenario = {}
    comb_wall = {"run_g": [0.0]*trials, "run_l": [0.0]*trials, "run_m": [0.0]*trials}
    comb_cpu = {"run_g": [0.0]*trials, "run_l": [0.0]*trials, "run_m": [0.0]*trials}

    for scen, out_json, _ in procs:
        data = json.loads(out_json.read_text(encoding="utf-8"))
        rt = data["raw_trials"]
        sc_stat = {"expected_stream_sha256": data["expected_stream_sha256"], "wall": {}, "cpu": {}, "paired_g_to_m": {}}
        for c in ["run_g", "run_l", "run_m"]:
            walls = [x["wall_sec"] for x in rt[c]]
            cpus = [x["cpu_sec"] for x in rt[c]]
            sc_stat["wall"][c] = summarize(walls)
            sc_stat["cpu"][c] = summarize(cpus)
            for i in range(trials):
                comb_wall[c][i] += walls[i]
                comb_cpu[c][i] += cpus[i]

        diffs_wall = [rt["run_g"][i]["wall_sec"] - rt["run_m"][i]["wall_sec"] for i in range(trials)]
        diffs_cpu = [rt["run_g"][i]["cpu_sec"] - rt["run_m"][i]["cpu_sec"] for i in range(trials)]
        g_med = sc_stat["wall"]["run_g"]["median"]
        m_med = sc_stat["wall"]["run_m"]["median"]
        sc_stat["paired_g_to_m"] = {
            "wall_saved_sec": summarize(diffs_wall),
            "cpu_saved_sec": summarize(diffs_cpu),
            "speedup_ratio": round(g_med / m_med, 4) if m_med > 0 else 1.0,
        }
        sc_stat["raw_trials"] = rt
        per_scenario[scen] = sc_stat

    comb_wall_stat = {c: summarize(comb_wall[c]) for c in ["run_g", "run_l", "run_m"]}
    comb_cpu_stat = {c: summarize(comb_cpu[c]) for c in ["run_g", "run_l", "run_m"]}
    comb_diffs_cpu = [comb_cpu["run_g"][i] - comb_cpu["run_m"][i] for i in range(trials)]
    comb_diffs_wall = [comb_wall["run_g"][i] - comb_wall["run_m"][i] for i in range(trials)]

    g_comb_med = comb_wall_stat["run_g"]["median"]
    m_comb_med = comb_wall_stat["run_m"]["median"]
    speedup = round(g_comb_med / m_comb_med, 4) if m_comb_med > 0 else 1.0

    summary = {
        "schema": "oasis.m15_258.benchmark.v1",
        "source_command": "python tools/benchmark_m15_258_real.py",
        "runner_sha256": hashlib.sha256(RUNNER_PATH.read_bytes()).hexdigest(),
        "trials_per_config_per_scenario": trials,
        "total_os_processes": trials * 3 * len(scenarios),
        "combined_workload": {
            "wall_sec": comb_wall_stat,
            "cpu_sec": comb_cpu_stat,
            "paired_wall_saved_sec": summarize(comb_diffs_wall),
            "paired_cpu_saved_sec": summarize(comb_diffs_cpu),
            "speedup_run_g_to_run_m": speedup,
        },
        "scenarios": per_scenario,
    }

    out_file = root / "data/thor-evidence/benchmark_m15_258_measured_summary.json"
    out_file.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"\nWROTE: {out_file}")
    print(f"Combined Median Wall: RUN_G={comb_wall_stat['run_g']['median']}s, RUN_L={comb_wall_stat['run_l']['median']}s, RUN_M={comb_wall_stat['run_m']['median']}s")
    print(f"Combined Median CPU:  RUN_G={comb_cpu_stat['run_g']['median']}s, RUN_L={comb_cpu_stat['run_l']['median']}s, RUN_M={comb_cpu_stat['run_m']['median']}s")
    print(f"Combined Speedup RUN_G -> RUN_M: {speedup:.4f}x (CI95: +-{comb_wall_stat['run_m']['ci95_pct']}%)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
