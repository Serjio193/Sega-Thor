#!/usr/bin/env python3
"""
Combined Native Cluster Qualification Runner (M15.240).
Runs P0 vs P_COMBINED on Scenarios C, A, B.
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

RUNNER_PATH = pathlib.Path("build/auto-migrator/m15_240/linux-runner/oasis_gameplay_session").resolve()
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


def execute_wsl_run(session_dir: pathlib.Path, helpers_csv: str, profile_dir: pathlib.Path = None, scen_name: str = "") -> tuple[bool, str]:
    cmd = [
        "wsl",
    ]
    if profile_dir is not None:
        cmd.extend([
            "env",
            f"OASIS_RESIDUAL_PROFILE_DIR={to_wsl_path(profile_dir)}",
            f"OASIS_SCENARIO_NAME={scen_name}",
        ])
    cmd.extend([
        to_wsl_path(RUNNER_PATH),
        "replay",
        to_wsl_path(CORE_PATH),
        to_wsl_path(ROM_PATH),
        to_wsl_path(session_dir),
        "--normalized-rom",
        "--restore-initial-state",
        f"--native-island=step:{helpers_csv}",
    ])
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
        match = (ha == hb) and (ha != "")
        if not match:
            all_match = False
        results[s] = {
            "match": match,
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

    # Slots 222..229 (Previous Experimental Clusters)
    prev_exp = "02575c,00ad76,00a9a0,008f22,002fda,002512,0023cc,00d168"
    # Slots 230..232 (M15.236 Scheduler-Safe Recompiled Clusters)
    m15_236_exp = "00d266,062732,062722"
    # Slots 233..236 (M15.237 Scheduler-Safe Recompiled Clusters)
    m15_237_exp = "005660,006e7a,062694,062a78"
    # Slots 237..238 (M15.238 Non-Leaf & Dynamic Jump Table Recompiled Clusters)
    m15_238_exp = "00d2b4,0059a8"
    # Slots 239..241 (M15.239 Residual-Driven Partitioned Clusters)
    m15_239_exp = "00e682,015ae2,00c9b2"
    # Slot 242 (M15.240 Real-ROM AI Patrol Timer & Selector Recompiled Cluster)
    # Note: Candidates 0x00E6BA and 0x00F0D2 are forensically documented & quarantined
    # due to cycle-sensitive Booth/divider timing drift, pending sub-cycle calibration.
    m15_240_exp = "016294"
    p_combined_csv = f"{p0_csv},{prev_exp},{m15_236_exp},{m15_237_exp},{m15_238_exp},{m15_239_exp},{m15_240_exp}"

    work_root = pathlib.Path("build/ab_combined_m15_240").resolve()
    work_root.mkdir(parents=True, exist_ok=True)

    scenarios = [
        ("scenario_c", pathlib.Path("build/ab_three_scenario/scenario_c/run_a")),
        ("scenario_a", pathlib.Path("build/ab_three_scenario/scenario_a/run_a")),
        ("scenario_b", pathlib.Path("build/ab_three_scenario/scenario_b/run_a")),
    ]

    final_results = {}

    for scen_name, scen_src in scenarios:
        print(f"\n=======================================================")
        print(f"RUNNING P0 vs P_COMBINED ON {scen_name.upper()}")
        print("P0: 196 Baseline Helpers")
        print("P_COMBINED: 196 Baseline + Slots 222..244")
        print("=======================================================\n")
        sdir = work_root / scen_name
        dir_p0 = sdir / "run_p0"
        dir_p_comb = sdir / "run_p_comb"

        for label, rdir, rcsv in [("P0", dir_p0, p0_csv), ("P_COMBINED", dir_p_comb, p_combined_csv)]:
            print(f"Executing {scen_name} {label}...")
            setup_run_dir(rdir, scen_src, runner_hash)
            prof_dir = (work_root / "profile" / f"{scen_name}_{label.lower()}") if (scen_name == "scenario_c" and label == "P_COMBINED") else None
            if prof_dir:
                prof_dir.mkdir(parents=True, exist_ok=True)
            ok, out = execute_wsl_run(rdir, rcsv, profile_dir=prof_dir, scen_name=scen_name)
            if not ok:
                print(f"Execution failed for {scen_name} {label}!")
                print(out)
                sys.exit(1)

        set_p0 = get_stream_hashes(dir_p0)
        set_p_comb = get_stream_hashes(dir_p_comb)
        all_match, comp = compare_two_stream_sets(set_p0, set_p_comb)
        print(f"{scen_name.upper()} Streams match: {all_match} (8/8)")
        for s in STREAM_NAMES:
            info = comp[s]
            status = "MATCH" if info["match"] else "MISMATCH"
            print(f"  [{status}] {s:28s} {info['sha256_a'][:16]}...")
        if not all_match:
            print(f"FATAL: Stream mismatch in {scen_name}!")
            sys.exit(1)

        final_results[scen_name] = {
            "all_match": all_match,
            "streams": comp,
            "p0_dir": str(dir_p0),
            "p_comb_dir": str(dir_p_comb),
        }

    out_json = work_root / "m15_240_ab_qualification_summary.json"
    out_json.write_text(json.dumps(final_results, indent=2), encoding="utf-8")
    print(f"\nSaved qualification summary to {out_json}")
    print("ALL 24/24 STREAMS BIT-EXACT ACROSS ALL THREE SCENARIOS!")


if __name__ == "__main__":
    main()
