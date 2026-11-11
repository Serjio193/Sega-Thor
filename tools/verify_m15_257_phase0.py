#!/usr/bin/env python3
"""Phase 0 verification for M15.257."""
import glob
import hashlib
import os
import pathlib
import subprocess
import sys

def sha256_file(filepath):
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def verify_phase0():
    sys.path.insert(0, ".")

    # 1. ROM
    rom_path = "local-roms/Beyond Oasis (USA).md"
    expected_rom = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
    actual_rom = sha256_file(rom_path)
    assert actual_rom == expected_rom, f"ROM hash mismatch: {actual_rom} != {expected_rom}"
    print(f"ROM_SHA256: {actual_rom} (OK)")

    # 2. Core
    core_path = "build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so"
    expected_core = "ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134"
    actual_core = sha256_file(core_path)
    assert actual_core == expected_core, f"Core hash mismatch: {actual_core} != {expected_core}"
    print(f"CORE_SHA256: {actual_core} (OK)")

    # 3. Production implementation source files across 196 active helpers
    from tools.thor_auto_migrator.status_adapter import StatusAdapter
    adapter = StatusAdapter()
    root = pathlib.Path(".")
    hashes = []
    for h in sorted(adapter.active_set):
        files = list(root.glob(f"src/tools/hybrid/m68k_{h:06x}*"))
        for f in sorted(files):
            hashes.append((str(f), hashlib.sha256(f.read_bytes()).hexdigest()))
    prod_src_hash = hashlib.sha256("".join(h[1] for h in hashes).encode()).hexdigest()
    expected_prod_src_hash = "28b6617d71065897eb5c2584e70f5b3c6e34ad92244e41f587cae4b2ec0c4ce7"
    assert prod_src_hash == expected_prod_src_hash, f"Prod source hash mismatch: {prod_src_hash} != {expected_prod_src_hash}"
    print(f"PROD_SRC_FILES: {len(hashes)} (OK)")
    print(f"PROD_SRC_HASH: {prod_src_hash} (OK)")

    # 4. Production active helper count from inventory
    res = subprocess.run([sys.executable, "tools/verify_production_helper_sum.py"], capture_output=True, text=True)
    assert "Total active helpers from inventory: 196" in res.stdout, f"Helper count error: {res.stdout}"
    print("PRODUCTION_ACTIVE_HELPERS: 196 (OK)")

    # 5. Standalone worktree status
    res = subprocess.run(["git", "-C", "C:/Github/Sega-Thor-Standalone", "status", "--short"], capture_output=True, text=True)
    assert res.stdout.strip() == "", f"Standalone repo dirty: {res.stdout}"
    print("STANDALONE_REPO_CLEAN: TRUE (OK)")

    # 6. Verify 146 reference fixtures and 12 mutation contracts under WSL
    res = subprocess.run(["wsl", "bash", "-c", "build/m15_256_diff_test"], capture_output=True, text=True)
    assert "146/146" in res.stdout, f"Fixtures error: {res.stdout}"
    assert "12/12" in res.stdout, f"Mutation error: {res.stdout}"
    print("DIFF_FIXTURES: 146/146 PASS (OK)")
    print("MUTATION_CONTRACTS: 12/12 PASS (OK)")

    print("\nPHASE 0 BASELINE AND GOVERNANCE: 100% REPRODUCED & VERIFIED")

if __name__ == "__main__":
    verify_phase0()
