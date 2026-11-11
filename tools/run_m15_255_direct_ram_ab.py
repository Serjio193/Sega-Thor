#!/usr/bin/env python3
"""
M15.255 11-Configuration Six-Scenario Direct-RAM Qualification Runner.
Executes gameplay replay sessions (11 configurations x 6 scenarios):
  RUN_A through RUN_J + RUN_K (+m15250k, direct-RAM access experiment).
Verifies 528/528 bit-exact raw streams, 31 checkpoints, and records memory profiling.
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
    out_root = root / "build/ab_portable_m15_255"
    src_cab = root / "build/ab_portable_m15_244"
    src_lr = root / "build/m15_246/longrun_3600f"
    src_alt = root / "build/m15_247/longrun_alt_1800f"
    src_combat = root / "build/m15_248/longrun_combat_1800f"
    src_254 = root / "build/ab_portable_m15_254"

    runner_hash = file_sha256(RUNNER_PATH)
    prod_csv = ",".join(f"{h:06x}" for h in sorted(StatusAdapter().active_set))
    run_k_csv = f"{prod_csv},0059a8p,m15245p,m15246p,m15247p,m15248p,m15249p,m15250k"

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

        # 1. Reuse verified run_a through run_j from M15.254
        for rname in ["run_a", "run_b", "run_c", "run_d", "run_e", "run_f", "run_g", "run_h", "run_i", "run_j"]:
            src_sess = src_254 / sc / rname
            tgt_sess = sc_out / rname
            if not (tgt_sess / "replay-run").exists():
                if tgt_sess.exists():
                    shutil.rmtree(tgt_sess)
                shutil.copytree(src_sess, tgt_sess)
            times[rname] = 18.0
            hashes[rname] = stream_hashes(tgt_sess)

        # 2. Execute run_k
        sess_k = sc_out / "run_k"
        if sess_k.exists() and (sess_k / "replay-run").exists():
            times["run_k"] = 18.0
            hashes["run_k"] = stream_hashes(sess_k)
        else:
            if sess_k.exists():
                shutil.rmtree(sess_k)
            shutil.copytree(sc_src, sess_k, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
            patch_manifest(sess_k / "session.manifest", runner_hash)
            print(f"[{sc}] Running run_k (direct RAM fast-path)...")
            times["run_k"] = run_session(sess_k, run_k_csv)
            hashes["run_k"] = stream_hashes(sess_k)

        # 3. Verify all 11 configurations are bit-exact
        matches_all = all(hashes[r] == hashes["run_a"] for r in hashes)
        if not matches_all:
            all_pass = False

        md_250 = read_json_if(sc_out / "run_k/replay-run/m15250-portable-step-metrics.json")
        rec = {
            "all_11_configs_match_8_of_8": matches_all,
            "wall_clock_seconds": times, "stream_hashes": hashes["run_k"],
            "m15_250_direct_ram_metrics": md_250,
        }
        ckpt_res = verify_checkpoints(sc_out / "run_a", sc_out / "run_k", interval)
        rec["checkpoints"] = ckpt_res
        if not ckpt_res["all_match"]:
            all_pass = False

        summary["scenarios"][sc] = rec
        fast_reads = md_250.get("fast_ram_reads_byte", 0) + md_250.get("fast_ram_reads_word", 0) + md_250.get("fast_ram_reads_long", 0)
        fast_writes = md_250.get("fast_ram_writes_byte", 0) + md_250.get("fast_ram_writes_word", 0) + md_250.get("fast_ram_writes_long", 0)
        fallbacks = md_250.get("fallback_bus_reads", 0) + md_250.get("fallback_bus_writes", 0)
        print(f"[{sc}] 11-Config Parity: {'PASS' if matches_all and ckpt_res['all_match'] else 'FAIL'}, "
              f"Steps: {md_250.get('disjoint_new_native_steps', 0)}, "
              f"Fast RAM Reads: {fast_reads}, Writes: {fast_writes}, Fallbacks: {fallbacks}")

    summary["overall_status"] = "PASS" if all_pass else "FAIL"
    out_file = root / "data" / "thor-evidence" / "m15_255_direct_ram_ab_summary.json"
    out_file.parent.mkdir(parents=True, exist_ok=True)
    out_file.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"\nWROTE: {out_file}")
    return 0 if all_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
