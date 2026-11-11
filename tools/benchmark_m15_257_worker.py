#!/usr/bin/env python3
"""
WSL worker for M15.257 Phase 9 Benchmark.
Executes 10 interleaved paired trials of RUN_G, RUN_L, RUN_M for a single scenario
on a pinned CPU core inside native /tmp storage, measuring exact per-process
wall-clock time and host CPU time (utime + stime via os.wait4).
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import os
import pathlib
import shutil
import sys
import time


def file_sha256(p: pathlib.Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def patch_manifest(manifest_path: pathlib.Path, runner_wsl: str, runner_hash: str) -> None:
    lines = manifest_path.read_text(encoding="utf-8").replace("\r\n", "\n").splitlines()
    out = [
        f"runner_path={runner_wsl}" if l.startswith("runner_path=") else
        f"runner_sha256={runner_hash}" if l.startswith("runner_sha256=") else l
        for l in lines
    ]
    manifest_path.write_bytes(("\n".join(out) + "\n").encode("utf-8"))


def run_process_wait4(cmd: list[str]) -> tuple[float, float, float]:
    t0 = time.perf_counter()
    pid = os.fork()
    if pid == 0:
        devnull = os.open(os.devnull, os.O_RDWR)
        os.dup2(devnull, 1)
        os.dup2(devnull, 2)
        os.close(devnull)
        os.execv(cmd[0], cmd)
        os._exit(127)
    _, status, rusage = os.wait4(pid, 0)
    wall_sec = time.perf_counter() - t0
    if not os.WIFEXITED(status) or os.WEXITSTATUS(status) != 0:
        raise RuntimeError(f"Runner exited abnormally: status={status}")
    return wall_sec, float(rusage.ru_utime), float(rusage.ru_stime)


def main() -> int:
    if len(sys.argv) < 7:
        print("Usage: worker.py <scen> <src_wsl> <out_json> <runner_wsl> <core_wsl> <rom_wsl> <prod_csv> [trials=10]")
        return 1

    scen = sys.argv[1]
    src_wsl = pathlib.Path(sys.argv[2])
    out_json = pathlib.Path(sys.argv[3])
    runner_wsl = sys.argv[4]
    core_wsl = sys.argv[5]
    rom_wsl = sys.argv[6]
    prod_csv = sys.argv[7]
    trials = int(sys.argv[8]) if len(sys.argv) > 8 else 10

    runner_hash = file_sha256(pathlib.Path(runner_wsl))
    tmp_dir = pathlib.Path(f"/tmp/bench_m15_257/{scen}")
    if tmp_dir.exists():
        shutil.rmtree(tmp_dir)
    shutil.copytree(src_wsl, tmp_dir, ignore=shutil.ignore_patterns("replay-run", "replay_log.txt"))
    patch_manifest(tmp_dir / "session.manifest", runner_wsl, runner_hash)

    cfgs = [
        ("run_g", f"{prod_csv},0059a8p,m15245p,m15246p,m15247p,m15248p,m15249p"),
        ("run_l", f"{prod_csv},0059a8p,m15245p,m15246p,m15247p,m15248p,m15249p,m15250k,m15256p"),
        ("run_m", f"{prod_csv},0059a8p,m15245p,m15246p,m15247p,m15248p,m15249p,m15250k,m15256p,m15257m"),
    ]

    raw_trials = {k: [] for k, _ in cfgs}
    stream_hash = None

    for t in range(trials):
        # Rotate order to balance thermal/frequency bias
        order = cfgs[t % 3:] + cfgs[:t % 3]
        for cname, helpers_csv in order:
            rr = tmp_dir / "replay-run"
            if rr.exists():
                shutil.rmtree(rr)
            rl = tmp_dir / "replay_log.txt"
            if rl.exists():
                rl.unlink()

            cmd = [
                runner_wsl, "replay", core_wsl, rom_wsl,
                str(tmp_dir), "--normalized-rom", "--restore-initial-state",
                f"--native-island=step:{helpers_csv}"
            ]
            wall_s, u_s, s_s = run_process_wait4(cmd)
            cpu_s = u_s + s_s
            ffs_hash = file_sha256(rr / "full-frame-state.jsonl")
            if stream_hash is None:
                stream_hash = ffs_hash
            elif ffs_hash != stream_hash:
                raise RuntimeError(f"Parity divergence in {scen} {cname} trial {t}!")

            raw_trials[cname].append({
                "trial": t,
                "wall_sec": wall_s,
                "cpu_sec": cpu_s,
                "user_sec": u_s,
                "sys_sec": s_s,
                "full_frame_sha256": ffs_hash,
            })

    if tmp_dir.exists():
        shutil.rmtree(tmp_dir)

    out_json.parent.mkdir(parents=True, exist_ok=True)
    out_json.write_text(json.dumps({
        "scenario": scen,
        "runner_sha256": runner_hash,
        "expected_stream_sha256": stream_hash,
        "trials": trials,
        "raw_trials": raw_trials,
    }, indent=2), encoding="utf-8")
    print(f"[{scen}] Completed {trials}x3={trials*3} runs; stream_sha256={stream_hash[:12]}...", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
