#!/usr/bin/env python3
"""
Build and execute m68k_recompiler_differential_test in WSL.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
import pathlib
import subprocess
import sys

def main():
    root = pathlib.Path(".").resolve()
    wsl_root = f"/mnt/{root.drive.replace(':', '').lower()}{root.as_posix()[len(root.drive):]}"

    src_files = [
        "tests/m68k_recompiler_differential_test.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_002512.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_002512_p0.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_002512_p1.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_002512_p2.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_002512_p3.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_002512_p4.cpp",
        "src/tools/hybrid/m68k_002512_instruction_step.cpp",
        "src/tools/hybrid/m68k_002512_calc_step.cpp",
        "src/tools/hybrid/m68k_002512_calc_down_step.cpp",
        "src/tools/hybrid/m68k_002512_upload_step.cpp",
        "src/tools/hybrid/m68k_002512_step_ops.cpp",
        "src/tools/hybrid/m68k_002512_metrics.cpp",
    ]

    out_bin = "build/test_recompiler_diff"
    cmd = [
        "wsl", "g++", "-std=c++20", "-O3",
        "-I", f"{wsl_root}/src",
        "-I", f"{wsl_root}",
        "-o", f"{wsl_root}/{out_bin}",
    ] + [f"{wsl_root}/{f}" for f in src_files]

    print("Compiling recompiler differential test...")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("COMPILATION FAILED:")
        print(res.stderr)
        sys.exit(1)

    print("Running recompiler differential test...")
    run_cmd = ["wsl", f"{wsl_root}/{out_bin}"]
    run_res = subprocess.run(run_cmd, capture_output=True, text=True)
    print(run_res.stdout)
    if run_res.returncode != 0:
        print("TEST RUN FAILED:")
        print(run_res.stderr)
        sys.exit(1)

    print("Recompiler differential test PASSED!")

if __name__ == "__main__":
    main()
