#!/usr/bin/env python3
"""
Build and execute m68k_target_c_differential_test in WSL.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
import pathlib
import subprocess
import sys

def main():
    root = pathlib.Path(".").resolve()
    wsl_root = f"/mnt/{root.drive.replace(':', '').lower()}{root.as_posix()[len(root.drive):]}"

    src_files = [
        "tests/m68k_target_c_differential_test.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00d168.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00d168_p0.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00d168_p1.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00d168_p2.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00d168_p3.cpp",
        "src/tools/hybrid/generated/m68k_recompiled_00d168_p4.cpp",
        "src/tools/hybrid/m68k_00d168_instruction_step.cpp",
        "src/tools/hybrid/m68k_camera_scroll_step_ops.cpp",
        "src/tools/hybrid/m68k_camera_scroll_step_ops_alu.cpp",
    ]

    out_bin = "build/test_target_c_diff"
    cmd = [
        "wsl", "g++", "-std=c++20", "-O3",
        "-I", f"{wsl_root}/src",
        "-I", f"{wsl_root}",
        "-o", f"{wsl_root}/{out_bin}",
    ] + [f"{wsl_root}/{f}" for f in src_files]

    print("Compiling Target C differential test...")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("COMPILATION FAILED:")
        print(res.stderr)
        sys.exit(1)

    print("Running Target C differential test...")
    run_cmd = ["wsl", f"{wsl_root}/{out_bin}"]
    run_res = subprocess.run(run_cmd, capture_output=True, text=True)
    print(run_res.stdout)
    if run_res.returncode != 0:
        print("TEST RUN FAILED:")
        print(run_res.stderr)
        sys.exit(1)

    print("Target C differential test PASSED!")

if __name__ == "__main__":
    main()
