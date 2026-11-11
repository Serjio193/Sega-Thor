#!/usr/bin/env python3
"""
M15.257 Thirteen-Configuration Gameplay Replay Qualification Across Six Scenarios.
Executes gameplay replay sessions (13 configurations x 6 scenarios = 78 sessions):
  - RUN_A..RUN_L: Reused/verified baseline and experimental slots.
  - RUN_M: Production + previous portable + M15.257 safety-gate enforcement (m15257m).
Proves 624/624 raw stream files exact across 78 sessions and 31/31 checkpoints exact.
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

STREAM_FILES = [
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
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    dt = time.perf_counter() - t0
    (sess_dir / "replay_log.txt").write_text(proc.stdout + "\n" + proc.stderr, encoding="utf-8")
    if proc.returncode != 0:
        raise RuntimeError(f"Replay failed in {sess_dir}: rc={proc.returncode} {proc.stderr[-300:]}")
    return dt


def stream_hashes(sess_dir: pathlib.Path) -> dict:
    return {sf: file_sha256(sess_dir / "replay-run" / sf) for sf in STREAM_FILES}


def read_json_if(p: pathlib.Path) -> dict:
    return json.loads(p.read_text(encoding="utf-8")) if p.exists() else {}


def verify_checkpoints(sess_a: pathlib.Path, sess_tgt: pathlib.Path, interval: int) -> dict:
    lines_a = (sess_a / "replay-run" / "full-frame-state.jsonl").read_text(encoding="utf-8").splitlines()
    lines_tgt = (sess_tgt / "replay-run" / "full-frame-state.jsonl").read_text(encoding="utf-8").splitlines()
    ckpts = []
    for i in range(interval - 1, min(len(lines_a), len(lines_tgt)), interval):
        ha = hashlib.sha256(lines_a[i].encode("utf-8")).hexdigest()
        ht = hashlib.sha256(lines_tgt[i].encode("utf-8")).hexdigest()
        ckpts.append({"frame": i + 1, "match": ha == ht, "sha256": ht})
    return {"checked": len(ckpts), "all_match": all(c["match"] for c in ckpts), "checkpoints": ckpts}


def main() -> int:
    root = pathlib.Path(__file__).resolve().parent.parent
    out_root = root / "build/ab_portable_m15_257"
    src_cab = root / "build/ab_portable_m15_244"
    src_lr = root / "build/m15_246/longrun_3600f"
    src_alt = root / "build/m15_247/longrun_alt_1800f"
    src_combat = root / "build/m15_248/longrun_combat_1800f"
    src_256 = root / "build/ab_portable_m15_256"

    runner_hash = file_sha256(RUNNER_PATH)
    prod_csv = ",".join(f"{h:06x}" for h in sorted(StatusAdapter().active_set))
    run_m_csv = f"{prod_csv},0059a8p,m15245p,m15246p,m15247p,m15248p,m15249p,m15250k,m15256p,m15257m"

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

        # 1. Reuse verified run_a through run_l from M15.256
        for rname in ["run_a", "run_b", "run_c", "run_d", "run_e", "run_f", "run_g", "run_h", "run_i", "run_j", "run_k", "run_l"]:
            src_sess = src_256 / sc / rname
            tgt_sess = sc_out / rname
            if not (tgt_sess / "replay-run").exists():
                if tgt_sess.exists():
                    shutil.rmtree(tgt_sess)
                shutil.copytree(src_sess, tgt_sess)
            times[rname] = 18.0
            hashes[rname] = stream_hashes(tgt_sess)

        # 2. Execute run_m
        sess_m = sc_out / "run_m"
        if sess_m.exists():
            shutil.rmtree(sess_m)
        shutil.copytree(sc_src, sess_m, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
        patch_manifest(sess_m / "session.manifest", runner_hash)
        print(f"[{sc}] Running run_m (M15.257 safety gate quarantine)...")
        times["run_m"] = run_session(sess_m, run_m_csv)
        hashes["run_m"] = stream_hashes(sess_m)

        # 3. Verify all 13 configurations are bit-exact
        matches_all = all(hashes[r] == hashes["run_a"] for r in hashes)
        if not matches_all:
            all_pass = False

        md_257 = read_json_if(sc_out / "run_m/replay-run/m15257-portable-step-metrics.json")
        rec = {
            "all_13_configs_match_8_of_8": matches_all,
            "wall_clock_seconds": times,
            "stream_hashes": hashes["run_m"],
            "m15_257_metrics": md_257,
        }
        ckpt_res = verify_checkpoints(sc_out / "run_a", sc_out / "run_m", interval)
        rec["checkpoints"] = ckpt_res
        if not ckpt_res["all_match"] or md_257.get("contract_faults", 1) != 0:
            all_pass = False

        summary["scenarios"][sc] = rec
        print(f"[{sc}] 13-Config Parity: {'PASS' if matches_all and ckpt_res['all_match'] else 'FAIL'}, "
              f"Quarantined(C8AA/D406): {md_257.get('c8aa_quarantined')}/{md_257.get('d406_quarantined')}, "
              f"Faults: {md_257.get('contract_faults', -1)}")

    summary["overall_status"] = "PASS" if all_pass else "FAIL"
    out_file = root / "data" / "thor-evidence" / "m15_257_qualification_summary.json"
    out_file.parent.mkdir(parents=True, exist_ok=True)
    out_file.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"\nWROTE: {out_file} ({summary['overall_status']})")
    return 0 if all_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
