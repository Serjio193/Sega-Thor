#!/usr/bin/env python3
"""
Three-scenario A/B verification runner for 0x00AD76 Sprite Engine Cluster.
Verifies bit-for-bit stream equivalence across Scenarios C, A, and B.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter
from tools.thor_evidence.receipt_verifier import ReceiptVerifier

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

RUNNER_PATH = pathlib.Path("build/auto-migrator/00ad76/linux-runner/oasis_gameplay_session").resolve()
CORE_PATH = pathlib.Path("build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so").resolve()
ROM_PATH = pathlib.Path("local-roms/Beyond Oasis (USA).md").resolve()


def file_sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def to_wsl_path(path: pathlib.Path) -> str:
    resolved = path.resolve()
    drive = resolved.drive.replace(":", "").lower()
    posix = resolved.as_posix()
    if resolved.drive:
        posix = posix[len(resolved.drive):]
    return f"/mnt/{drive}{posix}"


def patch_manifest(manifest_path: pathlib.Path, runner_hash: str):
    text = manifest_path.read_text(encoding="utf-8").replace("\r\n", "\n")
    lines = text.splitlines()
    new_lines = []
    for line in lines:
        if line.startswith("runner_path="):
            new_lines.append(f"runner_path={to_wsl_path(RUNNER_PATH)}")
        elif line.startswith("runner_sha256="):
            new_lines.append(f"runner_sha256={runner_hash}")
        else:
            new_lines.append(line)
    content = "\n".join(new_lines) + "\n"
    manifest_path.write_bytes(content.encode("utf-8"))


def execute_wsl_run(session_dir: pathlib.Path, helpers_csv: str) -> tuple[bool, str]:
    cmd = [
        "wsl",
        to_wsl_path(RUNNER_PATH),
        "replay",
        to_wsl_path(CORE_PATH),
        to_wsl_path(ROM_PATH),
        to_wsl_path(session_dir),
        "--normalized-rom",
        "--restore-initial-state",
        f"--native-island=step:{helpers_csv}",
    ]
    print(f"Executing: {' '.join(cmd[:6])} ...")
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    out = proc.stdout + "\n" + proc.stderr
    (session_dir / "replay_log.txt").write_text(out, encoding="utf-8")
    replay_dir = session_dir / "replay-run"
    if not replay_dir.exists():
        return False, f"Replay dir missing: {out[-1000:]}"
    return True, out


def compare_streams(dir_a: pathlib.Path, dir_b: pathlib.Path) -> tuple[bool, dict]:
    results = {}
    all_match = True
    for s in STREAM_NAMES:
        pa = dir_a / "replay-run" / s
        pb = dir_b / "replay-run" / s
        if not pa.exists() or not pb.exists():
            results[s] = {"match": False, "error": "file_missing"}
            all_match = False
            continue
        sz_a = pa.stat().st_size
        sz_b = pb.stat().st_size
        ha = file_sha256(pa)
        hb = file_sha256(pb)
        m = (ha == hb and sz_a == sz_b)
        if not m:
            all_match = False
        results[s] = {
            "match": m,
            "size_a": sz_a,
            "size_b": sz_b,
            "sha256_a": ha,
            "sha256_b": hb,
        }
    z80_sync = results.get("z80-ram-sync.bin", {}).get("sha256_a")
    z80_call = results.get("call-z80-ram.bin", {}).get("sha256_a")
    if z80_sync and z80_call and z80_sync == z80_call:
        raise RuntimeError("Z80 stream collision: z80-ram-sync.bin == call-z80-ram.bin")
    return all_match, results


def run_single_scenario(name: str, src_dir: pathlib.Path, dest_root: pathlib.Path,
                         helpers_a_csv: str, helpers_b_csv: str, runner_hash: str) -> dict:
    scen_dir = dest_root / name
    dir_a = scen_dir / "run_a"
    dir_b = scen_dir / "run_b"
    print(f"\n========================================\nSetting up {name} in {scen_dir}\n========================================")
    for d in (dir_a, dir_b):
        d.mkdir(parents=True, exist_ok=True)
        if (d / "replay-run").exists():
            shutil.rmtree(d / "replay-run")
        for fname in ["initial.state", "input.polls", "landmarks.jsonl", "session.manifest"]:
            shutil.copy2(src_dir / fname, d / fname)
        patch_manifest(d / "session.manifest", runner_hash)

    print(f"--- Running {name} Run A (196 baseline) ---")
    ok_a, msg_a = execute_wsl_run(dir_a, helpers_a_csv)
    if not ok_a:
        print(f"Run A failed: {msg_a}")
        sys.exit(1)

    print(f"--- Running {name} Run B (196 + 0x00AD76 Cluster) ---")
    ok_b, msg_b = execute_wsl_run(dir_b, helpers_b_csv)
    if not ok_b:
        print(f"Run B failed: {msg_b}")
        sys.exit(1)

    all_match, stream_results = compare_streams(dir_a, dir_b)
    metrics_b_path = dir_b / "replay-run" / "00ad76-step-metrics.json"
    metrics_b = json.loads(metrics_b_path.read_text(encoding="utf-8")) if metrics_b_path.exists() else {}

    print(f"Results for {name}: all_match={all_match}")
    for s, info in stream_results.items():
        print(f"  {s}: match={info['match']} ({info['size_a']} B, sha={info['sha256_a'][:8]}..)")
    print("Run B 00ad76 metrics:", json.dumps(metrics_b, indent=2))

    return {
        "scenario": name,
        "all_match": all_match,
        "match_count": sum(1 for v in stream_results.values() if v.get("match")),
        "streams": stream_results,
        "metrics_b": metrics_b,
    }


def main():
    runner_hash = file_sha256(RUNNER_PATH)
    print(f"Runner: {RUNNER_PATH} (SHA256: {runner_hash})")

    adapter = StatusAdapter()
    helpers_a = sorted(adapter.active_set)
    helpers_b_csv = ",".join(f"{x:06x}" for x in helpers_a) + ",00ad76"
    helpers_a_csv = ",".join(f"{x:06x}" for x in helpers_a)

    print(f"Active helpers A count: {len(helpers_a)}")
    print(f"Active helpers B count: {len(helpers_a) + 1}")

    dest_root = pathlib.Path("build/ab_three_scenario_00ad76")
    dest_root.mkdir(parents=True, exist_ok=True)

    scenarios = [
        ("scenario_c", pathlib.Path("build/ab_three_scenario/scenario_c/run_a")),
        ("scenario_a", pathlib.Path("build/ab_three_scenario/scenario_a/run_a")),
        ("scenario_b", pathlib.Path("build/ab_three_scenario/scenario_b/run_a")),
    ]

    summary_file = dest_root / "three_scenario_results.json"
    all_results = {}
    overall_pass = True

    if "--verify-only" in sys.argv and summary_file.exists():
        print(f"Loading existing replay results from {summary_file} (--verify-only mode)...")
        all_results = json.loads(summary_file.read_text(encoding="utf-8"))
        for scen_name, scen_data in all_results.items():
            dir_a = dest_root / scen_name / "run_a"
            dir_b = dest_root / scen_name / "run_b"
            m, stream_res = compare_streams(dir_a, dir_b)
            if not m:
                overall_pass = False
    else:
        for name, src in scenarios:
            res = run_single_scenario(name, src, dest_root, helpers_a_csv, helpers_b_csv, runner_hash)
            all_results[name] = res
            if not res["all_match"]:
                print(f"Scenario {name} FAILED! Halting before next scenario.")
                overall_pass = False
                break
        summary_file.write_text(json.dumps(all_results, indent=2) + "\n", encoding="utf-8")

    print(f"\n========================================\nOVERALL THREE-SCENARIO PASS: {overall_pass}\n========================================")
    if not overall_pass:
        sys.exit(1)


if __name__ == "__main__":
    main()
