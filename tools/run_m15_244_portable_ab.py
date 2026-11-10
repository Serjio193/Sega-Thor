#!/usr/bin/env python3
"""
M15.244 First Real Portable Gameplay Integration & Three-Scenario A/B Qualification Runner.
Runs Single-Variable A/B comparisons across Scenarios C (1,800f), A (1,592f), B (600f):
  Run A (run_p0): 196 active production helpers only.
  Run B (run_portable): 196 active production helpers + portable generated C++20 slot 0059a8p.
  Run C (run_p_comb_portable): 196 + M15.240 experimental slots + 0059a8p (Slot 238 replaced cleanly).
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

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

RUNNER_PATH = pathlib.Path("build/auto-migrator/m15_244/linux-runner/oasis_gameplay_session").resolve()
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


def run_session(sess_dir: pathlib.Path, helpers_csv: str):
    cmd = [
        "wsl", to_wsl(RUNNER_PATH), "replay", to_wsl(CORE_PATH), to_wsl(ROM_PATH),
        to_wsl(sess_dir), "--normalized-rom", "--restore-initial-state", f"--native-island=step:{helpers_csv}",
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    (sess_dir / "replay_log.txt").write_text(proc.stdout + "\n" + proc.stderr, encoding="utf-8")
    if proc.returncode != 0 or not (sess_dir / "replay-run").exists():
        raise RuntimeError(f"Replay failed in {sess_dir}: rc={proc.returncode} {proc.stderr[-500:]}")


def stream_hashes(sess_dir: pathlib.Path) -> dict[str, str]:
    r = sess_dir / "replay-run"
    return {s: file_sha256(r / s) if (r / s).exists() else "" for s in STREAM_NAMES}


def main():
    root = pathlib.Path(__file__).resolve().parent.parent
    out_root = root / "build" / "ab_portable_m15_244"
    src_root = root / "build" / "ab_combined_m15_240"

    runner_hash = file_sha256(RUNNER_PATH)
    adapter = StatusAdapter()
    prod_csv = ",".join(f"{h:06x}" for h in sorted(adapter.active_set))
    portable_csv = f"{prod_csv},0059a8p"
    comb_portable_csv = (
        f"{prod_csv},02575c,00ad76,00a9a0,008f22,002fda,002512,0023cc,"
        "00d168,00d266,062732,062722,005660,006e7a,062694,062a78,"
        "00d2b4,00e682,015ae2,00c9b2,016294,0059a8p"
    )

    summary = {"runner_sha256": runner_hash, "scenarios": {}}
    all_pass = True

    for sc in ("scenario_c", "scenario_a", "scenario_b"):
        sc_dir = out_root / sc
        if sc_dir.exists():
            shutil.rmtree(sc_dir)
        sc_dir.mkdir(parents=True)

        run_a = sc_dir / "run_p0"
        run_b = sc_dir / "run_portable"
        run_c = sc_dir / "run_p_comb_portable"
        for target in (run_a, run_b, run_c):
            target.mkdir(parents=True)
            for fn in ("initial.state", "input.polls", "landmarks.jsonl", "session.manifest"):
                shutil.copy2(src_root / sc / "run_p0" / fn, target / fn)
            patch_manifest(target / "session.manifest", runner_hash)

        print(f"[{sc}] Running Run A (196 prod)...")
        run_session(run_a, prod_csv)
        print(f"[{sc}] Running Run B (196 prod + 0059a8p portable)...")
        run_session(run_b, portable_csv)
        print(f"[{sc}] Running Run C (196 + comb + 0059a8p portable)...")
        run_session(run_c, comb_portable_csv)

        ha = stream_hashes(run_a)
        hb = stream_hashes(run_b)
        hc = stream_hashes(run_c)
        match_ab = all(ha[s] == hb[s] and ha[s] != "" for s in STREAM_NAMES)
        match_ac = all(ha[s] == hc[s] and ha[s] != "" for s in STREAM_NAMES)
        if not (match_ab and match_ac):
            all_pass = False

        pm_b = json.loads((run_b / "replay-run" / "0059a8-portable-step-metrics.json").read_text(encoding="utf-8"))
        pm_c = json.loads((run_c / "replay-run" / "0059a8-portable-step-metrics.json").read_text(encoding="utf-8"))
        print(f"[{sc}] A==B: {match_ab}, A==C: {match_ac} | B portable metrics: {pm_b} | C metrics: {pm_c}")
        summary["scenarios"][sc] = {
            "a_eq_b_8_of_8": match_ab,
            "a_eq_c_8_of_8": match_ac,
            "stream_hashes": ha,
            "run_b_portable_metrics": pm_b,
            "run_c_portable_metrics": pm_c,
        }

    (out_root / "m15_244_ab_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"ALL SCENARIOS PASS = {all_pass}")
    if not all_pass:
        sys.exit(1)


if __name__ == "__main__":
    main()
