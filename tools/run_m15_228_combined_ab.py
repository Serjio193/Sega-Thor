#!/usr/bin/env python3
"""
Combined Native Cluster Qualification Runner (M15.228).
Wired for Slot 222 (0x02575C Entity Engine) + Slot 223 (0x00AD76 Sprite Engine Mode 0) + Slot 224 (0x00A9A0 Sprite Engine Mode 1).
Tests P0 vs P3 across Scenario C, Scenario A, and Scenario B.
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

RUNNER_PATH = pathlib.Path("build/auto-migrator/m15_228/linux-runner/oasis_gameplay_session").resolve()
CORE_PATH = pathlib.Path("build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so").resolve()
ROM_PATH = pathlib.Path("local-roms/Beyond Oasis (USA).md").resolve()


def to_wsl_path(path: pathlib.Path) -> str:
    resolved = path.resolve()
    drive = resolved.drive.replace(":", "").lower()
    posix = resolved.as_posix()
    if resolved.drive:
        posix = posix[len(resolved.drive):]
    return f"/mnt/{drive}{posix}"


def file_sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


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
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    out = proc.stdout + "\n" + proc.stderr
    (session_dir / "replay_log.txt").write_text(out, encoding="utf-8")
    replay_dir = session_dir / "replay-run"
    if not replay_dir.exists():
        return False, f"Replay dir missing: {out[-1000:]}"
    return True, out


def get_stream_hashes(session_dir: pathlib.Path) -> dict[str, dict]:
    hashes = {}
    replay_dir = session_dir / "replay-run"
    for s in STREAM_NAMES:
        p = replay_dir / s
        if p.exists():
            hashes[s] = {
                "size": p.stat().st_size,
                "sha256": file_sha256(p)
            }
        else:
            hashes[s] = {"size": 0, "sha256": ""}
    return hashes


def compare_two_stream_sets(set_a: dict[str, dict], set_b: dict[str, dict]) -> tuple[bool, dict]:
    results = {}
    all_match = True
    for s in STREAM_NAMES:
        info_a = set_a.get(s, {})
        info_b = set_b.get(s, {})
        ha = info_a.get("sha256", "")
        hb = info_b.get("sha256", "")
        sa = info_a.get("size", 0)
        sb = info_b.get("size", 0)
        m = (ha == hb and sa == sb and ha != "")
        if not m:
            all_match = False
        results[s] = {
            "match": m,
            "sha256_a": ha,
            "sha256_b": hb,
            "size_a": sa,
            "size_b": sb,
        }
    z80_sync = set_a.get("z80-ram-sync.bin", {}).get("sha256")
    z80_call = set_a.get("call-z80-ram.bin", {}).get("sha256")
    if z80_sync and z80_call and z80_sync == z80_call:
        raise RuntimeError("Z80 stream collision: z80-ram-sync.bin == call-z80-ram.bin")
    return all_match, results


def setup_run_dir(dest_dir: pathlib.Path, src_dir: pathlib.Path, runner_hash: str):
    dest_dir.mkdir(parents=True, exist_ok=True)
    if (dest_dir / "replay-run").exists():
        shutil.rmtree(dest_dir / "replay-run")
    for fname in ["initial.state", "input.polls", "landmarks.jsonl", "session.manifest"]:
        shutil.copy2(src_dir / fname, dest_dir / fname)
    patch_manifest(dest_dir / "session.manifest", runner_hash)


def main():
    runner_hash = file_sha256(RUNNER_PATH)
    print(f"Runner: {RUNNER_PATH} (SHA256: {runner_hash})")

    adapter = StatusAdapter()
    helpers_base = sorted(adapter.active_set)
    p0_csv = ",".join(f"{x:06x}" for x in helpers_base)
    p3_csv = p0_csv + ",02575c,00ad76,00a9a0"

    work_root = pathlib.Path("build/ab_combined_m15_228").resolve()
    work_root.mkdir(parents=True, exist_ok=True)

    scenarios = [
        ("scenario_c", pathlib.Path("build/ab_three_scenario/scenario_c/run_a")),
        ("scenario_a", pathlib.Path("build/ab_three_scenario/scenario_a/run_a")),
        ("scenario_b", pathlib.Path("build/ab_three_scenario/scenario_b/run_a")),
    ]

    final_results = {}

    for scen_name, scen_src in scenarios:
        print(f"\n=======================================================")
        print(f"RUNNING P0 vs P3 ON {scen_name.upper()}")
        print("P0: 196 Baseline Helpers")
        print("P3: 196 Baseline + 02575c (Slot 222) + 00ad76 (Slot 223) + 00a9a0 (Slot 224)")
        print("=======================================================\n")
        sdir = work_root / scen_name
        dir_p0 = sdir / "run_p0"
        dir_p3 = sdir / "run_p3"

        for label, rdir, rcsv in [("P0", dir_p0, p0_csv), ("P3", dir_p3, p3_csv)]:
            if (rdir / "replay-run" / "full-frame-state.jsonl").exists():
                print(f"Skipping execution for {scen_name} {label} (already exists)")
            else:
                print(f"Executing {scen_name} {label}...")
                setup_run_dir(rdir, scen_src, runner_hash)
                ok, msg = execute_wsl_run(rdir, rcsv)
                if not ok:
                    print(f"FAILED {scen_name} {label}: {msg}")
                    sys.exit(1)

        set_p0 = get_stream_hashes(dir_p0)
        set_p3 = get_stream_hashes(dir_p3)
        match, diff = compare_two_stream_sets(set_p0, set_p3)
        print(f"{scen_name} P3 vs P0: match={match}")
        for s, info in diff.items():
            if not info["match"]:
                print(f"  MISMATCH {s}: P0={info['sha256_a'][:8]}.. vs P3={info['sha256_b'][:8]}..")
        if not match:
            print(f"FATAL: {scen_name} P3 does not match P0 bit-exact!")
            sys.exit(1)
        final_results[scen_name] = {"p0_vs_p3": diff}
        shutil.copytree(dir_p0, sdir / "run_a", dirs_exist_ok=True)
        shutil.copytree(dir_p3, sdir / "run_b", dirs_exist_ok=True)

    # Save summary results
    summary_path = work_root / "combined_qualification_results.json"
    summary_path.write_text(json.dumps(final_results, indent=2), encoding="utf-8")
    print(f"\nSUCCESS: All 24/24 streams match bit-exact across Scenarios C, A, and B!")
    print(f"Summary written to {summary_path}")


if __name__ == "__main__":
    main()
