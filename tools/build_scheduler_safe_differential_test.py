#!/usr/bin/env python3
"""
Build and execute m68k_scheduler_safe_differential_test in WSL.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
import pathlib
import subprocess
import sys

def main():
    root = pathlib.Path(".").resolve()
    wsl_root = f"/mnt/{root.drive.replace(':', '').lower()}{root.as_posix()[len(root.drive):]}"

    src_files = [
        "tests/m68k_scheduler_safe_differential_test.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00d266.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_062732.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_062722.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00c012.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_005660.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_006e7a.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_062694.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_062a78.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00d2b4.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_0059a8.cpp",
    ]

    out_bin = "build/test_scheduler_safe_diff"
    cmd = [
        "wsl", "g++", "-std=c++20", "-O3",
        "-I", f"{wsl_root}/src",
        "-I", f"{wsl_root}",
        "-o", f"{wsl_root}/{out_bin}",
    ] + [f"{wsl_root}/{f}" for f in src_files]

    print("Compiling Scheduler-Safe Recompiler differential test...")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("COMPILATION FAILED:")
        print(res.stderr)
        sys.exit(1)

    print("Running Scheduler-Safe Recompiler differential test...")
    run_cmd = ["wsl", f"{wsl_root}/{out_bin}"]
    run_res = subprocess.run(run_cmd, capture_output=True, text=True)
    print(run_res.stdout)
    if run_res.returncode != 0:
        print("TEST RUN FAILED:")
        print(run_res.stderr)
        sys.exit(1)

    print("Scheduler-Safe Recompiler differential test PASSED!")

if __name__ == "__main__":
    main()
