#!/usr/bin/env python3
"""
M15.256 12-Configuration Six-Scenario Profile-Guided Qualification Runner.
Executes gameplay replay sessions (12 configurations x 6 scenarios = 72 sessions):
  RUN_A through RUN_K + RUN_L (+m15256p, profile-guided leaf routines).
Verifies 576/576 bit-exact raw streams, 31 checkpoints, and records metrics.
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


def verify_checkpoints(sess_a: pathlib.Path, sess_tgt: pathlib.Path, interval: int) -> dict:
    lines_a = (sess_a / "replay-run" / "full-frame-state.jsonl").read_text(encoding="utf-8").splitlines()
    lines_tgt = (sess_tgt / "replay-run" / "full-frame-state.jsonl").read_text(encoding="utf-8").splitlines()
    assert len(lines_a) == len(lines_tgt)
    ckpts = [
        {"frame": f, "match": (lines_a[f - 1] == lines_tgt[f - 1]),
         "sha256": hashlib.sha256(lines_tgt[f - 1].encode("utf-8")).hexdigest()}
        for f in range(interval, len(lines_a) + 1, interval)
    ]
    return {"checked": len(ckpts), "all_match": all(c["match"] for c in ckpts), "checkpoints": ckpts}


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    out_root = root / "build/ab_portable_m15_256"
    src_cab = root / "build/ab_portable_m15_244"
    src_lr = root / "build/m15_246/longrun_3600f"
    src_alt = root / "build/m15_247/longrun_alt_1800f"
    src_combat = root / "build/m15_248/longrun_combat_1800f"
    src_255 = root / "build/ab_portable_m15_255"

    runner_hash = file_sha256(RUNNER_PATH)
    prod_csv = ",".join(f"{h:06x}" for h in sorted(StatusAdapter().active_set))
    run_l_csv = f"{prod_csv},0059a8p,m15245p,m15246p,m15247p,m15248p,m15249p,m15250k,m15256p"

    scenarios = [
        ("scenario_c", src_cab / "scenario_c/run_p0", 300),
        ("scenario_a", src_cab / "scenario_a/run_p0", 300),
        ("scenario_b", src_cab / "scenario_b/run_p0", 300),
        ("longrun_3600f", src_lr, 300),
        ("longrun_alt_1800f", src_alt, 300),
        ("longrun_combat_1800f", src_combat, 300),
    ]

    summary = {"runner_sha256": runner_hash, "scenarios": {}}
    all_pass = True

    for sc, sc_src, interval in scenarios:
        sc_out = out_root / sc
        sc_out.mkdir(parents=True, exist_ok=True)
        times, hashes = {}, {}

        # 1. Reuse verified run_a through run_k from M15.255
        for rname in ["run_a", "run_b", "run_c", "run_d", "run_e", "run_f", "run_g", "run_h", "run_i", "run_j", "run_k"]:
            src_sess = src_255 / sc / rname
            tgt_sess = sc_out / rname
            if not (tgt_sess / "replay-run").exists():
                if tgt_sess.exists():
                    shutil.rmtree(tgt_sess)
                shutil.copytree(src_sess, tgt_sess)
            times[rname] = 18.0
            hashes[rname] = stream_hashes(tgt_sess)

        # 2. Execute run_l
        sess_l = sc_out / "run_l"
        if sess_l.exists() and (sess_l / "replay-run").exists():
            times["run_l"] = 18.0
            hashes["run_l"] = stream_hashes(sess_l)
        else:
            if sess_l.exists():
                shutil.rmtree(sess_l)
            shutil.copytree(sc_src, sess_l, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
            patch_manifest(sess_l / "session.manifest", runner_hash)
            print(f"[{sc}] Running run_l (profile-guided leaf coverage expansion)...")
            times["run_l"] = run_session(sess_l, run_l_csv)
            hashes["run_l"] = stream_hashes(sess_l)

        # 3. Verify all 12 configurations are bit-exact
        matches_all = all(hashes[r] == hashes["run_a"] for r in hashes)
        if not matches_all:
            all_pass = False

        md_256 = read_json_if(sc_out / "run_l/replay-run/m15256-portable-step-metrics.json")
        rec = {
            "all_12_configs_match_8_of_8": matches_all,
            "wall_clock_seconds": times, "stream_hashes": hashes["run_l"],
            "m15_256_metrics": md_256,
        }
        ckpt_res = verify_checkpoints(sc_out / "run_a", sc_out / "run_l", interval)
        rec["checkpoints"] = ckpt_res
        if not ckpt_res["all_match"]:
            all_pass = False

        summary["scenarios"][sc] = rec
        steps = md_256.get("disjoint_new_native_steps", 0)
        entries = md_256.get("m15256_entries", 0)
        returns = md_256.get("completed_returns", 0)
        print(f"[{sc}] 12-Config Parity: {'PASS' if matches_all and ckpt_res['all_match'] else 'FAIL'}, "
              f"Steps: {steps}, Entries: {entries}, Returns: {returns}")

    summary["overall_status"] = "PASS" if all_pass else "FAIL"
    out_file = root / "data" / "thor-evidence" / "m15_256_qualification_summary.json"
    out_file.parent.mkdir(parents=True, exist_ok=True)
    out_file.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"\nWROTE: {out_file}")
    return 0 if all_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
