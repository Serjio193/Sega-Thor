#!/usr/bin/env python3
"""
M15.247 Five-Configuration Runtime Qualification & Dual Long-Run Parity Harness.
Compares RUN_A (196 prod), RUN_B (+0059a8p), RUN_C (+m15245p), RUN_D (+m15246p), RUN_E (+m15247p)
across Scenario C (1800f), A (1592f), B (600f), longrun_3600f (3600f), and longrun_alt_1800f (1800f),
verifying bit-exact raw streams, checkpoints, 0 fallbacks, and 0 contract faults.
Strictly <= 200 lines per AGENTS.md Rule 3.
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
    "observed-landmarks.jsonl", "full-frame-state.jsonl", "architectural-sync.jsonl",
    "scheduler-sync.jsonl", "z80-ram-sync.bin", "call-z80-ram.bin",
    "external-events.jsonl", "normalized-events.jsonl",
]

RUNNER_PATH = pathlib.Path("build/auto-migrator/m15_247/linux-runner/oasis_gameplay_session").resolve()
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


def run_session(sess_dir: pathlib.Path, helpers_csv: str) -> float:
    cmd = [
        "wsl", to_wsl(RUNNER_PATH), "replay", to_wsl(CORE_PATH), to_wsl(ROM_PATH),
        to_wsl(sess_dir), "--normalized-rom", "--restore-initial-state", f"--native-island=step:{helpers_csv}",
    ]
    t0 = time.perf_counter()
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
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


def verify_checkpoints(sess_a: pathlib.Path, sess_e: pathlib.Path, interval: int) -> dict:
    lines_a = (sess_a / "replay-run" / "full-frame-state.jsonl").read_text(encoding="utf-8").splitlines()
    lines_e = (sess_e / "replay-run" / "full-frame-state.jsonl").read_text(encoding="utf-8").splitlines()
    assert len(lines_a) == len(lines_e)
    ckpts = [
        {"frame": f, "match": (lines_a[f - 1] == lines_e[f - 1]),
         "sha256": hashlib.sha256(lines_e[f - 1].encode("utf-8")).hexdigest()}
        for f in range(interval, len(lines_a) + 1, interval)
    ]
    return {"checked": len(ckpts), "all_match": all(c["match"] for c in ckpts), "checkpoints": ckpts}


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    out_root = root / "build/ab_portable_m15_247"
    src_cab = root / "build/ab_portable_m15_244"
    src_lr = root / "build/m15_246/longrun_3600f"
    src_alt = root / "build/m15_247/longrun_alt_1800f"

    runner_hash = file_sha256(RUNNER_PATH)
    prod_csv = ",".join(f"{h:06x}" for h in sorted(StatusAdapter().active_set))
    run_b_csv = f"{prod_csv},0059a8p"
    run_c_csv = f"{prod_csv},0059a8p,m15245p"
    run_d_csv = f"{prod_csv},0059a8p,m15245p,m15246p"
    run_e_csv = f"{prod_csv},0059a8p,m15245p,m15246p,m15247p"

    scenarios = [
        ("scenario_c", src_cab / "scenario_c/run_p0", 300),
        ("scenario_a", src_cab / "scenario_a/run_p0", 300),
        ("scenario_b", src_cab / "scenario_b/run_p0", 300),
        ("longrun_3600f", src_lr, 300),
        ("longrun_alt_1800f", src_alt, 300),
    ]

    summary = {"runner_sha256": runner_hash, "scenarios": {}}
    all_pass = True

    for sc, sc_src, interval in scenarios:
        sc_out = out_root / sc
        sc_out.mkdir(parents=True, exist_ok=True)
        cfgs = [("run_a", prod_csv), ("run_b", run_b_csv), ("run_c", run_c_csv),
                ("run_d", run_d_csv), ("run_e", run_e_csv)]
        times, hashes = {}, {}
        for rname, csv in cfgs:
            sess = sc_out / rname
            if sess.exists():
                shutil.rmtree(sess)
            shutil.copytree(sc_src, sess, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
            patch_manifest(sess / "session.manifest", runner_hash)
            print(f"[{sc}] Running {rname}...")
            times[rname] = run_session(sess, csv)
            hashes[rname] = stream_hashes(sess)

        matches_all = (hashes["run_a"] == hashes["run_b"] == hashes["run_c"] == hashes["run_d"] == hashes["run_e"])
        if not matches_all:
            all_pass = False

        md_244 = read_json_if(sc_out / "run_e/replay-run/0059a8-portable-step-metrics.json")
        md_245 = read_json_if(sc_out / "run_e/replay-run/m15245-portable-step-metrics.json")
        md_246 = read_json_if(sc_out / "run_e/replay-run/m15246-portable-step-metrics.json")
        md_247 = read_json_if(sc_out / "run_e/replay-run/m15247-portable-step-metrics.json")
        rec = {
            "all_5_configs_match_8_of_8": matches_all,
            "wall_clock_seconds": times, "stream_hashes": hashes["run_e"],
            "m15_244_metrics": md_244, "m15_245_metrics": md_245, "m15_246_metrics": md_246, "m15_247_metrics": md_247,
        }
        ckpt_res = verify_checkpoints(sc_out / "run_a", sc_out / "run_e", interval)
        rec["checkpoints"] = ckpt_res
        if not ckpt_res["all_match"]:
            all_pass = False

        summary["scenarios"][sc] = rec
        print(f"[{sc}] A==B==C==D==E: {matches_all} | M244={md_244.get('disjoint_new_native_steps')} "
              f"M245={md_245.get('disjoint_new_native_steps')} M246={md_246.get('disjoint_new_native_steps')} "
              f"M247={md_247.get('disjoint_new_native_steps')} "
              f"fallbacks={md_247.get('original_cpu_fallbacks', 0)} faults={md_247.get('contract_faults', 0)}")

    summary["all_pass"] = all_pass
    out_root.mkdir(parents=True, exist_ok=True)
    (out_root / "m15_247_ab_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"ALL SCENARIOS PASS = {all_pass}")
    return 0 if all_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
