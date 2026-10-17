import os
import sys
import shutil
import hashlib
import json
import subprocess
from pathlib import Path

BASE_SESSION = Path("/mnt/c/Github/Sega-Thor/build/session-008bac-run-B")
BUILD_DIR = Path("/mnt/c/Github/Sega-Thor/build")
CORE = Path("/mnt/c/Github/Sega-Thor/build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so")
ROM = Path("/mnt/c/Github/Sega-Thor/build/fixed_count_records/canonical.rom")
RUNNER = Path("/mnt/c/Github/Sega-Thor/build/helper10-9d70-linux-release-20261013/src/tools/hybrid/oasis_gameplay_session")

SESSION_A = BUILD_DIR / "session-00deec-run-A"
SESSION_B = BUILD_DIR / "session-00deec-run-B"

ALL_PREV_HELPERS = "613b2,61e48,61934,6147e,623ac,6273a,62788,60000,60286,62ae0,626bc,0027fc,002992,6138e,00e682,0613f8,016ebc,009ad6,009bf2,0027a6,00b15e,00a8da,017d12,6281a,6282e,01ac90,01b102,00350e,008e22,00a7e6,008e90,00f788,00a196,00a342,00b34c,00c980,00c998,008dd4,00557a,005670,008b8e,008bac"
RUN_B_HELPERS = ALL_PREV_HELPERS + ",00deec"

def get_sha256(p: Path) -> str:
    h = hashlib.sha256()
    with open(p, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def prepare_session(sess_dir: Path):
    sess_dir.mkdir(parents=True, exist_ok=True)
    replay_run = sess_dir / "replay-run"
    if replay_run.exists():
        shutil.rmtree(replay_run)

    # Copy essential inputs if not present
    for fname in ["initial.state", "input.polls", "landmarks.jsonl"]:
        dst = sess_dir / fname
        src = BASE_SESSION / fname
        if not dst.exists() or dst.stat().st_size != src.stat().st_size:
            shutil.copy2(src, dst)

    # Read manifest from base and update runner sha256
    manifest_lines = (BASE_SESSION / "session.manifest").read_text().splitlines()
    runner_sha = get_sha256(RUNNER)
    new_lines = []
    for line in manifest_lines:
        if line.startswith("runner_path="):
            new_lines.append(f"runner_path={RUNNER}")
        elif line.startswith("runner_sha256="):
            new_lines.append(f"runner_sha256={runner_sha}")
        else:
            new_lines.append(line)
    (sess_dir / "session.manifest").write_text("\n".join(new_lines) + "\n")
    print(f"Prepared {sess_dir} with runner_sha256={runner_sha}")

def get_census_target_count(census_path: Path, target_hex: str) -> int:
    if not census_path.exists():
        return 0
    try:
        with open(census_path) as f:
            data = json.load(f)
        targets = data.get("targets", [])
        if isinstance(targets, list):
            for info in targets:
                t = info.get("ENTRY_PC", "")
                if target_hex.lower() in t.lower():
                    return info.get("CALL_COUNT", 0)
    except Exception as e:
        print(f"Error reading census: {e}")
    return 0

def main():
    print("=== Starting 00DEEC Replay Preparation ===")
    prepare_session(SESSION_A)
    prepare_session(SESSION_B)

    cmd_a = [
        str(RUNNER),
        "replay",
        str(CORE),
        str(ROM),
        str(SESSION_A),
        "--normalized-rom",
        "--restore-initial-state",
        f"--native-island=step:{ALL_PREV_HELPERS}"
    ]

    print("\n=== Running Run A (0x00DEEC OFF) ===")
    print("Command:", " ".join(cmd_a))
    env_a = os.environ.copy()
    env_a["RUN_NAME"] = "A"
    res_a = subprocess.run(cmd_a, capture_output=True, text=True, env=env_a)
    print("Run A stdout:\n", res_a.stdout)
    if res_a.returncode != 0:
        print("Run A stderr:\n", res_a.stderr)
        sys.exit(res_a.returncode)

    cmd_b = [
        str(RUNNER),
        "replay",
        str(CORE),
        str(ROM),
        str(SESSION_B),
        "--normalized-rom",
        "--restore-initial-state",
        f"--native-island=step:{RUN_B_HELPERS}"
    ]

    print("\n=== Running Run B (0x00DEEC ON) ===")
    print("Command:", " ".join(cmd_b))
    env_b = os.environ.copy()
    env_b["RUN_NAME"] = "B"
    res_b = subprocess.run(cmd_b, capture_output=True, text=True, env=env_b)
    print("Run B stdout:\n", res_b.stdout)
    if res_b.returncode != 0:
        print("Run B stderr:\n", res_b.stderr)
        sys.exit(res_b.returncode)

    files_to_check = [
        "observed-landmarks.jsonl",
        "full-frame-state.jsonl",
        "architectural-sync.jsonl",
        "scheduler-sync.jsonl",
        "z80-ram-sync.bin",
        "call-z80-ram.bin",
        "external-events.jsonl",
        "normalized-events.jsonl"
    ]
    all_match = True
    print("\n=== Stream Hash Equivalence Check ===")
    for fn in files_to_check:
        fa = SESSION_A / "replay-run" / fn
        fb = SESSION_B / "replay-run" / fn
        if not fa.exists() or not fb.exists():
            print(f"ERROR: {fn} missing in one of runs!")
            all_match = False
            continue
        sha_a = get_sha256(fa)
        sha_b = get_sha256(fb)
        match = (sha_a == sha_b)
        status = "MATCH" if match else "MISMATCH"
        print(f"  {fn:28s} {status} (SHA256={sha_a})")
        if not match:
            print(f"    Run A: {sha_a}")
            print(f"    Run B: {sha_b}")
            all_match = False

    print(f"\nAll streams match: {all_match}")

    # Check landmark-diagnostic
    diag_b = SESSION_B / "replay-run" / "landmark-diagnostic.json"
    if diag_b.exists():
        print(f"\nLandmark diagnostic Run B:\n{diag_b.read_text()}")

    # Read metrics
    m00deec_path = SESSION_B / "replay-run" / "00deec-step-metrics.json"
    if m00deec_path.exists():
        print(f"\n00deec metrics in Run B:\n{m00deec_path.read_text()}")
    else:
        print("\nERROR: 00deec-step-metrics.json not found in Run B!")

    # Check Census for 0x00DEEC
    census_a = SESSION_A / "replay-run" / "call-census.json"
    census_b = SESSION_B / "replay-run" / "call-census.json"
    count_a = get_census_target_count(census_a, "00deec")
    count_b = get_census_target_count(census_b, "00deec")
    print(f"\nCensus calls to 0x00DEEC: Run A = {count_a}, Run B = {count_b}")

    if not all_match:
        sys.exit(1)

if __name__ == "__main__":
    main()
