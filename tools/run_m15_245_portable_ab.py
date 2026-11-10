#!/usr/bin/env python3
"""
M15.245 Three-Configuration Experimental Runtime & A/B/C Stream Qualification Harness.
Measures RUN_A (196 prod only), RUN_B (196 + M15.244 0059a8p), RUN_C (196 + 0059a8p + M15.245 m15245p)
across Scenario C (1,800f), Scenario A (1,592f), and Scenario B (600f), verifying all 24 raw streams
and recording runtime execution metrics + integrated wall-clock timings.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter

STREAM_NAMES = [
    "observed-landmarks.jsonl",
    "full-frame-state.jsonl",
    "architectural-sync.jsonl",
    "scheduler-sync.jsonl",
    "z80-ram-sync.bin",
    "call-z80-ram.bin",
    "external-events.jsonl",
    "normalized-events.jsonl",
]

RUNNER_PATH = pathlib.Path("build/auto-migrator/m15_245/linux-runner/oasis_gameplay_session").resolve()
CORE_PATH = pathlib.Path("build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so").resolve()
ROM_PATH = pathlib.Path("local-roms/Beyond Oasis (USA).md").resolve()


def to_wsl(p: pathlib.Path) -> str:
    r = p.resolve()
    drive = r.drive.replace(":", "").lower()
    posix = r.as_posix()[len(r.drive):] if r.drive else r.as_posix()
    return f"/mnt/{drive}{posix}"


def file_sha256(p: pathlib.Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def patch_manifest(manifest_path: pathlib.Path, runner_hash: str):
    lines = manifest_path.read_text(encoding="utf-8").replace("\r\n", "\n").splitlines()
    out = []
    for l in lines:
        if l.startswith("runner_path="):
            out.append(f"runner_path={to_wsl(RUNNER_PATH)}")
        elif l.startswith("runner_sha256="):
            out.append(f"runner_sha256={runner_hash}")
        else:
            out.append(l)
    manifest_path.write_bytes(("\n".join(out) + "\n").encode("utf-8"))


def run_session(sess_dir: pathlib.Path, helpers_csv: str) -> float:
    cmd = [
        "wsl", to_wsl(RUNNER_PATH), "replay", to_wsl(CORE_PATH), to_wsl(ROM_PATH),
        to_wsl(sess_dir), "--normalized-rom", "--restore-initial-state", f"--native-island=step:{helpers_csv}",
    ]
    t0 = time.perf_counter()
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    dt = time.perf_counter() - t0
    (sess_dir / "replay_log.txt").write_text(proc.stdout + "\n" + proc.stderr, encoding="utf-8")
    if proc.returncode != 0 or not (sess_dir / "replay-run").exists():
        raise RuntimeError(f"Replay failed in {sess_dir}: rc={proc.returncode} {proc.stderr[-500:]}")
    return dt


def stream_hashes(sess_dir: pathlib.Path) -> dict[str, str]:
    r = sess_dir / "replay-run"
    return {s: file_sha256(r / s) if (r / s).exists() else "" for s in STREAM_NAMES}


def read_json_if(p: pathlib.Path) -> dict:
    return json.loads(p.read_text(encoding="utf-8")) if p.exists() else {}


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    out_root = root / "build" / "ab_portable_m15_245"
    src_root = root / "build" / "ab_portable_m15_244"

    runner_hash = file_sha256(RUNNER_PATH)
    prod_csv = ",".join(f"{h:06x}" for h in sorted(StatusAdapter().active_set))
    run_b_csv = f"{prod_csv},0059a8p"
    run_c_csv = f"{prod_csv},0059a8p,m15245p"

    summary = {"runner_sha256": runner_hash, "scenarios": {}}
    all_pass = True

    for sc in ("scenario_c", "scenario_a", "scenario_b"):
        sc_src = src_root / sc / "run_p0"
        sc_out = out_root / sc
        sc_out.mkdir(parents=True, exist_ok=True)
        cfgs = [("run_a", prod_csv), ("run_b", run_b_csv), ("run_c", run_c_csv)]

        times = {}
        hashes = {}
        for rname, csv in cfgs:
            sess = sc_out / rname
            if sess.exists():
                shutil.rmtree(sess)
            shutil.copytree(sc_src, sess, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
            patch_manifest(sess / "session.manifest", runner_hash)
            print(f"[{sc}] Running {rname}...")
            times[rname] = run_session(sess, csv)
            hashes[rname] = stream_hashes(sess)

        a_eq_b = (hashes["run_a"] == hashes["run_b"])
        b_eq_c = (hashes["run_b"] == hashes["run_c"])
        a_eq_c = (hashes["run_a"] == hashes["run_c"])
        if not (a_eq_b and b_eq_c and a_eq_c):
            all_pass = False

        mb_244 = read_json_if(sc_out / "run_b" / "replay-run" / "0059a8-portable-step-metrics.json")
        mc_244 = read_json_if(sc_out / "run_c" / "replay-run" / "0059a8-portable-step-metrics.json")
        mc_245 = read_json_if(sc_out / "run_c" / "replay-run" / "m15245-portable-step-metrics.json")

        summary["scenarios"][sc] = {
            "a_vs_b_8_of_8": a_eq_b,
            "b_vs_c_8_of_8": b_eq_c,
            "a_vs_c_8_of_8": a_eq_c,
            "wall_clock_seconds": times,
            "stream_hashes": hashes["run_a"],
            "run_b_m15_244_metrics": mb_244,
            "run_c_m15_244_metrics": mc_244,
            "run_c_m15_245_metrics": mc_245,
        }
        print(f"[{sc}] A==B:{a_eq_b} B==C:{b_eq_c} A==C:{a_eq_c} | M15.244={mc_244.get('disjoint_new_native_steps')} | M15.245={mc_245}")

    summary["all_scenarios_pass_24_of_24"] = all_pass
    out_root.mkdir(parents=True, exist_ok=True)
    (out_root / "m15_245_ab_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"ALL SCENARIOS PASS = {all_pass}")
    return 0 if all_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
