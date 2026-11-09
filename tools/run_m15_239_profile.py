#!/usr/bin/env python3
"""
Run Scenario C with authoritative residual interpreter profiling.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.thor_auto_migrator.status_adapter import StatusAdapter


def to_wsl_path(path: Path) -> str:
    resolved = Path(path).resolve()
    drive = resolved.drive.replace(":", "").lower()
    posix = resolved.as_posix()
    if resolved.drive:
        posix = posix[len(resolved.drive):]
    return f"/mnt/{drive}{posix}"


def file_sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


def patch_manifest(manifest_path: Path, runner_path: Path, runner_hash: str):
    text = manifest_path.read_text(encoding="utf-8").replace("\r\n", "\n")
    lines = text.splitlines()
    new_lines = []
    for line in lines:
        if line.startswith("runner_path="):
            new_lines.append(f"runner_path={to_wsl_path(runner_path)}")
        elif line.startswith("runner_sha256="):
            new_lines.append(f"runner_sha256={runner_hash}")
        else:
            new_lines.append(line)
    content = "\n".join(new_lines) + "\n"
    manifest_path.write_bytes(content.encode("utf-8"))


def main():
    repo_root = Path(".").resolve()
    runner_path = repo_root / "build" / "auto-migrator" / "m15_239" / "linux-runner" / "oasis_gameplay_session"
    core_path = repo_root / "build" / "scheduler-fixture-serializerfix-20261013" / "genesis_plus_gx_libretro.so"
    rom_path = repo_root / "local-roms" / "Beyond Oasis (USA).md"
    src_dir = repo_root / "build" / "ab_three_scenario" / "scenario_c" / "run_a"
    session_dir = repo_root / "build" / "m15_239_profile_session"
    profile_dir = repo_root / "build" / "m15_239_profile"

    session_dir.mkdir(parents=True, exist_ok=True)
    profile_dir.mkdir(parents=True, exist_ok=True)

    if (session_dir / "replay-run").exists():
        shutil.rmtree(session_dir / "replay-run")

    for fname in ["initial.state", "input.polls", "landmarks.jsonl", "session.manifest"]:
        shutil.copy2(src_dir / fname, session_dir / fname)

    runner_hash = file_sha256(runner_path)
    patch_manifest(session_dir / "session.manifest", runner_path, runner_hash)

    adapter = StatusAdapter()
    helpers_base = sorted(adapter.active_set)
    p0_csv = ",".join(f"{x:06x}" for x in helpers_base)

    prev_exp = "02575c,00ad76,00a9a0,008f22,002fda,002512,0023cc,00d168"
    m15_236_exp = "00d266,062732,062722"
    m15_237_exp = "005660,006e7a,062694,062a78"
    m15_238_exp = "00d2b4,0059a8"
    p_combined_csv = f"{p0_csv},{prev_exp},{m15_236_exp},{m15_237_exp},{m15_238_exp}"

    cmd = [
        "wsl",
        "env",
        f"OASIS_RESIDUAL_PROFILE_DIR={to_wsl_path(profile_dir)}",
        "OASIS_SCENARIO_NAME=scenario_c",
        to_wsl_path(runner_path),
        "replay",
        to_wsl_path(core_path),
        to_wsl_path(rom_path),
        to_wsl_path(session_dir),
        "--normalized-rom",
        "--restore-initial-state",
        f"--native-island=step:{p_combined_csv}",
    ]

    print("Running authoritative residual profiler across Scenario C (1800 frames)...")
    proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    out = proc.stdout + "\n" + proc.stderr
    (session_dir / "replay_log.txt").write_text(out, encoding="utf-8")
    print(f"Replay exit code: {proc.returncode}")
    print(proc.stdout[-500:] if len(proc.stdout) > 500 else proc.stdout)

    summary_file = profile_dir / "scenario_c_disjoint_profile_summary.json"
    if summary_file.exists():
        print("\nDisjoint Summary from runner:")
        print(summary_file.read_text(encoding="utf-8"))
    else:
        print(f"ERROR: {summary_file} was not generated!")
        sys.exit(1)


if __name__ == "__main__":
    main()
