#!/usr/bin/env python3
"""
M15.256 Phase 1: Host CPU Profiling & Subsystem Attribution Across Six Scenarios.
Measures process CPU time (CLOCK_PROCESS_CPUTIME_ID) vs wall time across:
  1. Musashi CPU execution
  2. Production native helpers
  3. Experimental portable execution
  4. Memory bus callbacks
  5. Scheduler and interrupt handling
  6. VDP rendering and DMA
  7. Audio / Z80 processing
  8. Input, replay and evidence recording
  9. Other runner overhead
Calculates profiler overhead and perturbation against uninstrumented baseline.
Strictly <= 250 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys
import time

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


def file_sha256(p: pathlib.Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def patch_manifest(manifest_path: pathlib.Path, runner_hash: str) -> None:
    lines = manifest_path.read_text(encoding="utf-8").replace("\r\n", "\n").splitlines()
    out = [
        f"runner_path={to_wsl(RUNNER_PATH)}" if l.startswith("runner_path=") else
        f"runner_sha256={runner_hash}" if l.startswith("runner_sha256=") else l
        for l in lines
    ]
    manifest_path.write_bytes(("\n".join(out) + "\n").encode("utf-8"))


def run_session(sess_dir: pathlib.Path, helpers_csv: str, profile_dir: pathlib.Path = None, scen_name: str = "") -> tuple[float, float]:
    env = os.environ.copy()
    if profile_dir is not None:
        env["OASIS_HOST_CPU_PROFILE_DIR"] = to_wsl(profile_dir)
        env["OASIS_SCENARIO_NAME"] = scen_name
    else:
        env.pop("OASIS_HOST_CPU_PROFILE_DIR", None)
        env.pop("OASIS_SCENARIO_NAME", None)

    cmd = [
        "wsl", "env",
        f"OASIS_HOST_CPU_PROFILE_DIR={env.get('OASIS_HOST_CPU_PROFILE_DIR', '')}",
        f"OASIS_SCENARIO_NAME={env.get('OASIS_SCENARIO_NAME', '')}",
        to_wsl(RUNNER_PATH), "replay", to_wsl(CORE_PATH), to_wsl(ROM_PATH),
        to_wsl(sess_dir), "--normalized-rom", "--restore-initial-state", f"--native-island=step:{helpers_csv}",
    ]
    t0_wall = time.perf_counter()
    t0_cpu = time.process_time()
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
    dt_wall = time.perf_counter() - t0_wall
    dt_cpu = time.process_time() - t0_cpu
    (sess_dir / "replay_log.txt").write_text(proc.stdout + "\n" + proc.stderr, encoding="utf-8")
    if proc.returncode != 0:
        raise RuntimeError(f"Replay failed in {sess_dir}: rc={proc.returncode} {proc.stderr[-300:]}")
    return dt_wall, dt_cpu


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    work_dir = root / "build/host_cpu_profile_m15_256"
    work_dir.mkdir(parents=True, exist_ok=True)
    prof_out = work_dir / "profiles"
    prof_out.mkdir(parents=True, exist_ok=True)

    src_cab = root / "build/ab_portable_m15_244"
    src_lr = root / "build/m15_246/longrun_3600f"
    src_alt = root / "build/m15_247/longrun_alt_1800f"
    src_combat = root / "build/m15_248/longrun_combat_1800f"

    scenarios = [
        ("scenario_a", src_cab / "scenario_a/run_p0", 1592),
        ("scenario_b", src_cab / "scenario_b/run_p0", 600),
        ("scenario_c", src_cab / "scenario_c/run_p0", 1800),
        ("longrun_3600f", src_lr, 3600),
        ("longrun_alt_1800f", src_alt, 1800),
        ("longrun_combat_1800f", src_combat, 1800),
    ]

    runner_hash = file_sha256(RUNNER_PATH)
    prod_csv = ",".join(f"{h:06x}" for h in sorted(StatusAdapter().active_set))
    run_k_csv = f"{prod_csv},0059a8p,m15245p,m15246p,m15247p,m15248p,m15249p,m15250k"

    print("=== M15.256 Phase 1: Host CPU Profiling & Attribution ===")
    results = {}
    tot_uninst_wall = 0.0
    tot_inst_wall = 0.0

    aggregated_totals = {
        "musashi_cpu_ns": 0, "production_native_cpu_ns": 0, "portable_cpu_ns": 0,
        "bus_cpu_ns": 0, "scheduler_cpu_ns": 0, "vdp_cpu_ns": 0,
        "audio_z80_cpu_ns": 0, "input_replay_evidence_cpu_ns": 0, "other_runner_cpu_ns": 0,
        "total_cpu_ns": 0, "total_wall_ns": 0, "idle_cpu_ns": 0, "total_frames": 0,
    }

    for sc_name, sc_src, frames in scenarios:
        print(f"\n--- Profiling Scenario: {sc_name} ({frames} frames) ---")
        sc_dir = work_dir / sc_name
        sc_dir.mkdir(parents=True, exist_ok=True)

        # 1. Uninstrumented baseline run
        sess_base = sc_dir / "uninstrumented"
        if sess_base.exists():
            shutil.rmtree(sess_base)
        shutil.copytree(sc_src, sess_base, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
        patch_manifest(sess_base / "session.manifest", runner_hash)
        base_wall, base_cpu = run_session(sess_base, run_k_csv)
        print(f"  Uninstrumented baseline: wall={base_wall:.3f}s")

        # 2. Instrumented profiling run
        sess_inst = sc_dir / "instrumented"
        if sess_inst.exists():
            shutil.rmtree(sess_inst)
        shutil.copytree(sc_src, sess_inst, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
        patch_manifest(sess_inst / "session.manifest", runner_hash)
        inst_wall, inst_cpu = run_session(sess_inst, run_k_csv, prof_out, sc_name)
        print(f"  Instrumented profile:    wall={inst_wall:.3f}s")

        pert = ((inst_wall - base_wall) / base_wall) * 100.0 if base_wall > 0 else 0.0
        tot_uninst_wall += base_wall
        tot_inst_wall += inst_wall

        prof_file = prof_out / f"host_cpu_profile_{sc_name}.json"
        pdata = json.loads(prof_file.read_text(encoding="utf-8")) if prof_file.exists() else {}

        results[sc_name] = {
            "frames": frames,
            "baseline_wall_sec": round(base_wall, 4),
            "instrumented_wall_sec": round(inst_wall, 4),
            "perturbation_percent": round(pert, 2),
            "profile_data": pdata,
        }

        for k in aggregated_totals:
            if k in pdata:
                aggregated_totals[k] += pdata[k]

    tot_pert = ((tot_inst_wall - tot_uninst_wall) / tot_uninst_wall) * 100.0 if tot_uninst_wall > 0 else 0.0
    agg_total_cpu = aggregated_totals["total_cpu_ns"] if aggregated_totals["total_cpu_ns"] > 0 else 1

    percentages = {
        "musashi": round(aggregated_totals["musashi_cpu_ns"] * 100.0 / agg_total_cpu, 2),
        "production_native": round(aggregated_totals["production_native_cpu_ns"] * 100.0 / agg_total_cpu, 2),
        "portable": round(aggregated_totals["portable_cpu_ns"] * 100.0 / agg_total_cpu, 2),
        "bus": round(aggregated_totals["bus_cpu_ns"] * 100.0 / agg_total_cpu, 2),
        "scheduler": round(aggregated_totals["scheduler_cpu_ns"] * 100.0 / agg_total_cpu, 2),
        "vdp": round(aggregated_totals["vdp_cpu_ns"] * 100.0 / agg_total_cpu, 2),
        "audio_z80": round(aggregated_totals["audio_z80_cpu_ns"] * 100.0 / agg_total_cpu, 2),
        "input_replay_evidence": round(aggregated_totals["input_replay_evidence_cpu_ns"] * 100.0 / agg_total_cpu, 2),
        "other": round(aggregated_totals["other_runner_cpu_ns"] * 100.0 / agg_total_cpu, 2),
    }

    summary = {
        "runner_sha256": runner_hash,
        "scenarios": results,
        "aggregated_totals": aggregated_totals,
        "aggregated_percentages": percentages,
        "overall_uninstrumented_wall_sec": round(tot_uninst_wall, 4),
        "overall_instrumented_wall_sec": round(tot_inst_wall, 4),
        "overall_profiler_overhead_percent": round(tot_pert, 2),
    }

    out_file = root / "data/thor-evidence/m15_256_host_cpu_profile_summary.json"
    out_file.parent.mkdir(parents=True, exist_ok=True)
    out_file.write_text(json.dumps(summary, indent=2), encoding="utf-8")

    print("\n===========================================================")
    print("M15.256 PHASE 1 HOST CPU ATTRIBUTION SUMMARY")
    print("===========================================================")
    print(f"Overall Uninstrumented Wall: {tot_uninst_wall:.3f} s")
    print(f"Overall Instrumented Wall:   {tot_inst_wall:.3f} s")
    print(f"Profiler Overhead:           {tot_pert:.2f} %")
    print("-----------------------------------------------------------")
    print(f"1. Musashi CPU:              {percentages['musashi']:6.2f} %")
    print(f"2. Production Native:        {percentages['production_native']:6.2f} %")
    print(f"3. Experimental Portable:    {percentages['portable']:6.2f} %")
    print(f"4. Memory Bus Callbacks:     {percentages['bus']:6.2f} %")
    print(f"5. Scheduler / Interrupts:   {percentages['scheduler']:6.2f} %")
    print(f"6. VDP Rendering & DMA:      {percentages['vdp']:6.2f} %")
    print(f"7. Audio / Z80 Processing:   {percentages['audio_z80']:6.2f} %")
    print(f"8. Input / Evidence Record:  {percentages['input_replay_evidence']:6.2f} %")
    print(f"9. Other Runner Overhead:    {percentages['other']:6.2f} %")
    print("===========================================================")
    print(f"Summary written to {out_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
