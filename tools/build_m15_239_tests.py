#!/usr/bin/env python3
"""
Build and execute M15.239 differential tests in WSL.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
import pathlib
import subprocess
import sys

def run_test(name, src_files, out_bin):
    root = pathlib.Path(".").resolve()
    wsl_root = f"/mnt/{root.drive.replace(':', '').lower()}{root.as_posix()[len(root.drive):]}"

    cmd = [
        "wsl", "g++", "-std=c++20", "-O3",
        "-I", f"{wsl_root}/src",
        "-I", f"{wsl_root}",
        "-o", f"{wsl_root}/{out_bin}",
    ] + [f"{wsl_root}/{f}" for f in src_files]

    print(f"Compiling {name}...")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("COMPILATION FAILED:")
        print(res.stderr)
        sys.exit(1)

    print(f"Running {name}...")
    run_cmd = ["wsl", f"{wsl_root}/{out_bin}"]
    run_res = subprocess.run(run_cmd, capture_output=True, text=True)
    print(run_res.stdout)
    if run_res.returncode != 0:
        print("TEST RUN FAILED:")
        print(run_res.stderr)
        sys.exit(1)

    print(f"{name} PASSED!\n")

def main():
    # 1. Mixed ABI and IRQ differential test
    run_test(
        "Mixed ABI and IRQ Differential Test",
        [
            "tests/m68k_mixed_abi_and_irq_test.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_0059a8.cpp",
        ],
        "build/test_mixed_abi_and_irq"
    )

    # 2. Candidate Differential Test
    run_test(
        "M15.239 Candidate Differential Test",
        [
            "tests/m68k_candidate_diff_m15_239_test.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_00e682.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_00e682_part0.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_00e682_part1.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_015ae2.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_015ae2_part0.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_015ae2_part1.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_00c9b2.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_00c9b2_part0.cpp",
            "src/tools/hybrid/generated/m68k_recompiled_00c9b2_part1.cpp",
        ],
        "build/test_candidate_diff_m15_239"
    )

    # 3. Thor IR Mode A vs Mode B Differential Test
    run_test(
        "Thor IR Mode A vs Mode B Differential Test",
        [
            "tests/thor_ir_mode_differential_test.cpp",
        ],
        "build/test_thor_ir_diff"
    )

if __name__ == "__main__":
    main()
