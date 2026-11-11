#!/usr/bin/env python3
"""
M15.257 Phase 1 & Phase 3: Low-Overhead Host CPU Profiling & Heavy-Kernel Forensics.
Measures process CPU and wall time across 6 scenarios with invariant-TSC profiling:
  - Validates lower overhead against M15.256 baseline (27.33%).
  - Quantifies Musashi host CPU share.
  - Measures dynamic entries, steps, and host CPU time for 0x00C8AA and 0x00D406.
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


def main():
    root = pathlib.Path(__file__).resolve().parent.parent
    runner_hash = file_sha256(RUNNER_PATH)
    print(f"M15.257 Runner SHA256: {runner_hash}")

    adapter = StatusAdapter()
    prod_csv = ",".join(f"{h:06x}" for h in sorted(adapter.active_set))
    run_l_csv = f"{prod_csv},0059a8p,m15245p,m15246p,m15247p,m15248p,m15249p,m15250k,m15256p"

    work_dir = root / "build" / "host_cpu_profile_m15_257"
    if work_dir.exists():
        shutil.rmtree(work_dir)
    work_dir.mkdir(parents=True, exist_ok=True)
    prof_out_dir = work_dir / "profiles"
    prof_out_dir.mkdir(parents=True, exist_ok=True)

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

    results = {}
    tot_base_wall = 0.0
    tot_inst_wall = 0.0

    for name, src_dir, frames in scenarios:
        print(f"\n--- Profiling Scenario: {name} ({frames} frames) ---")
        base_dir = work_dir / name / "baseline"
        shutil.copytree(src_dir, base_dir, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
        patch_manifest(base_dir / "session.manifest", runner_hash)
        print("  Running uninstrumented baseline...")
        bw, _ = run_session(base_dir, run_l_csv, profile_dir=None)
        print(f"  Uninstrumented wall time: {bw:.4f} s")

        inst_dir = work_dir / name / "instrumented"
        shutil.copytree(src_dir, inst_dir, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
        patch_manifest(inst_dir / "session.manifest", runner_hash)
        print("  Running low-overhead instrumented profile...")
        iw, _ = run_session(inst_dir, run_l_csv, profile_dir=prof_out_dir, scen_name=name)
        print(f"  Instrumented wall time: {iw:.4f} s")

        tot_base_wall += bw
        tot_inst_wall += iw

        prof_file = prof_out_dir / f"host_cpu_profile_{name}.json"
        pdata = json.loads(prof_file.read_text(encoding="utf-8")) if prof_file.exists() else {}

        delta_pct = ((iw - bw) / bw) * 100.0 if bw > 0 else 0.0
        results[name] = {
            "frames": frames,
            "baseline_wall_sec": bw,
            "instrumented_wall_sec": iw,
            "overhead_percent": delta_pct,
            "profile_data": pdata,
        }

    agg_overhead = ((tot_inst_wall - tot_base_wall) / tot_base_wall) * 100.0

    tot_cpu_ns = sum(r["profile_data"].get("total_cpu_ns", 0) for r in results.values())
    tot_musashi_ns = sum(r["profile_data"].get("musashi_cpu_ns", 0) for r in results.values())
    tot_prod_ns = sum(r["profile_data"].get("production_native_cpu_ns", 0) for r in results.values())
    tot_port_ns = sum(r["profile_data"].get("portable_cpu_ns", 0) for r in results.values())
    tot_other_ns = sum(r["profile_data"].get("other_runner_cpu_ns", 0) for r in results.values())
    tot_audio_ns = sum(r["profile_data"].get("audio_z80_cpu_ns", 0) for r in results.values())

    tot_c8aa_entries = sum(r["profile_data"].get("heavy_kernels", {}).get("c8aa", {}).get("entries", 0) for r in results.values())
    tot_c8aa_steps = sum(r["profile_data"].get("heavy_kernels", {}).get("c8aa", {}).get("steps", 0) for r in results.values())
    tot_c8aa_cpu_ns = sum(r["profile_data"].get("heavy_kernels", {}).get("c8aa", {}).get("cpu_ns", 0) for r in results.values())

    tot_d406_entries = sum(r["profile_data"].get("heavy_kernels", {}).get("d406", {}).get("entries", 0) for r in results.values())
    tot_d406_steps = sum(r["profile_data"].get("heavy_kernels", {}).get("d406", {}).get("steps", 0) for r in results.values())
    tot_d406_cpu_ns = sum(r["profile_data"].get("heavy_kernels", {}).get("d406", {}).get("cpu_ns", 0) for r in results.values())

    divisor = float(tot_cpu_ns) if tot_cpu_ns > 0 else 1.0

    summary = {
        "runner_sha256": runner_hash,
        "aggregate_overhead_percent": agg_overhead,
        "tot_baseline_wall_sec": tot_base_wall,
        "tot_instrumented_wall_sec": tot_inst_wall,
        "aggregate_percentages": {
            "musashi": (tot_musashi_ns / divisor) * 100.0,
            "production_native": (tot_prod_ns / divisor) * 100.0,
            "portable": (tot_port_ns / divisor) * 100.0,
            "other_runner": (tot_other_ns / divisor) * 100.0,
            "audio_z80": (tot_audio_ns / divisor) * 100.0,
        },
        "heavy_kernels_aggregate": {
            "0x00C8AA": {
                "entries": tot_c8aa_entries,
                "exclusive_steps": tot_c8aa_steps,
                "host_cpu_ns": tot_c8aa_cpu_ns,
                "host_cpu_ms": tot_c8aa_cpu_ns / 1_000_000.0,
            },
            "0x00D406": {
                "entries": tot_d406_entries,
                "exclusive_steps": tot_d406_steps,
                "host_cpu_ns": tot_d406_cpu_ns,
                "host_cpu_ms": tot_d406_cpu_ns / 1_000_000.0,
            }
        },
        "scenarios": results,
    }

    out_file = root / "data" / "thor-evidence" / "m15_257_host_cpu_profile_summary.json"
    out_file.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"\nWrote profile summary to {out_file}")
    print(f"Aggregate Profiler Overhead: {agg_overhead:.2f}% (vs 27.33% before)")
    print(f"Musashi Host CPU Share: {(tot_musashi_ns / divisor) * 100.0:.2f}%")
    print(f"0x00C8AA: entries={tot_c8aa_entries}, steps={tot_c8aa_steps}, host_cpu={tot_c8aa_cpu_ns / 1e6:.2f} ms")
    print(f"0x00D406: entries={tot_d406_entries}, steps={tot_d406_steps}, host_cpu={tot_d406_cpu_ns / 1e6:.2f} ms")


if __name__ == "__main__":
    main()
