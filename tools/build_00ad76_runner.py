#!/usr/bin/env python3
"""
Build script for isolated Linux runner with 0x00AD76 Sprite Engine Cluster runtime integration.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
from pathlib import Path
import subprocess
import sys

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

def main():
    repo_root = Path(".").resolve()
    base_build_dir = repo_root / "build" / "helper10-9d70-linux-release-20261013"
    runner_dir = repo_root / "build" / "auto-migrator" / "00ad76" / "linux-runner"
    runner_dir.mkdir(parents=True, exist_ok=True)
    runner_exe = runner_dir / "oasis_gameplay_session"

    print(f"Target runner directory: {runner_dir}")

    # Generate step_set overlay: slot 222 (02575c) and slot 223 (00ad76)
    base_step_set = (repo_root / "src" / "tools" / "hybrid" / "m68k_gameplay_step_set.cpp").read_text(encoding="utf-8")

    # 1. Add header includes
    inc_marker = '#include "tools/hybrid/m68k_9d70_instruction_step.hpp"'
    new_inc = (
        '#include "tools/hybrid/m68k_02575c_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"\n'
        + inc_marker
    )
    step_set_code = base_step_set.replace(inc_marker, new_inc, 1)

    # 2. Update g_enabled size from 222 to 224
    step_set_code = step_set_code.replace("std::array<bool, 222> g_enabled{};", "std::array<bool, 224> g_enabled{};", 1)

    # 3. Add CHK after slot 221
    last_chk = "CHK(step_m68k_00dbca, 221);"
    new_chk = (
        last_chk + "\n"
        "    CHK(step_m68k_02575c, 222);\n"
        "    CHK(step_m68k_00ad76, 223);"
    )
    step_set_code = step_set_code.replace(last_chk, new_chk, 1)

    # 4. Add tail check for 02575c and 00ad76
    tail_marker = "for (std::size_t i = 0; i < tail_vals.size(); ++i) g_enabled[101U + i] = tail_vals[i];"
    new_tail = (
        tail_marker + '\n'
        '    g_enabled[222] = step_helper_selected_exact(enabled_helpers, "02575c");\n'
        '    g_enabled[223] = step_helper_selected_exact(enabled_helpers, "00ad76");'
    )
    step_set_code = step_set_code.replace(tail_marker, new_tail, 1)

    overlay_cpp = runner_dir / "m68k_gameplay_step_set_overlay.cpp"
    overlay_cpp.write_text(step_set_code, encoding="utf-8")
    print("Generated m68k_gameplay_step_set_overlay.cpp")

    # Generate metrics overlay
    base_metrics = (repo_root / "src" / "tools" / "hybrid" / "m68k_gameplay_step_metrics.cpp").read_text(encoding="utf-8")
    inc_metrics_marker = '#include "tools/hybrid/m68k_00dbca_instruction_step.hpp"'
    new_metrics_inc = (
        inc_metrics_marker + '\n'
        '#include "tools/hybrid/m68k_02575c_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"'
    )
    metrics_code = base_metrics.replace(inc_metrics_marker, new_metrics_inc, 1)

    write_marker = 'write_00dbca_step_metrics(parent / "00dbca-step-metrics.json");'
    new_write = (
        write_marker + '\n'
        '        write_02575c_step_metrics(parent / "02575c-step-metrics.json");\n'
        '        write_00ad76_step_metrics(parent / "00ad76-step-metrics.json");'
    )
    metrics_code = metrics_code.replace(write_marker, new_write, 1)

    metrics_cpp = runner_dir / "m68k_gameplay_step_metrics_overlay.cpp"
    metrics_cpp.write_text(metrics_code, encoding="utf-8")
    print("Generated m68k_gameplay_step_metrics_overlay.cpp")

    # Paths in WSL format
    repo_src_wsl = to_wsl_path(repo_root / "src")
    repo_inc_wsl = to_wsl_path(repo_root / "include")
    libretro_inc_wsl = to_wsl_path(repo_root / "build" / "9c40-hook-source" / "libretro" / "libretro-common" / "include")

    sources = [
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_step_ops.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_instruction_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_ingress_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_attrib_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_camera_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_piece_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_render_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_shadow_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_commit_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_00ad76_metrics.cpp",
        repo_root / "src" / "tools" / "hybrid" / "gameplay_session_step_trace.cpp",
        overlay_cpp,
        metrics_cpp,
    ]

    compiled_objs = []
    for src in sources:
        obj = runner_dir / f"{src.stem}.o"
        cmd = [
            "wsl", "g++", "-O3", "-DNDEBUG", "-std=c++20",
            f"-I{repo_src_wsl}", f"-I{repo_inc_wsl}", f"-I{libretro_inc_wsl}",
            "-c", to_wsl_path(src), "-o", to_wsl_path(obj)
        ]
        print(f"Compiling {src.name}...")
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"FAILED to compile {src.name}:\n{res.stderr}")
            sys.exit(1)
        compiled_objs.append(obj)

    # Copy objects from 02575c objects.rsp
    prev_rsp = repo_root / "build" / "auto-migrator" / "02575c" / "linux-runner" / "objects.rsp"
    prev_objs = [l.strip() for l in prev_rsp.read_text(encoding="utf-8").splitlines() if l.strip()]

    final_objs = [
        o for o in prev_objs
        if not o.endswith("m68k_gameplay_step_set.o")
        and not o.endswith("m68k_gameplay_step_metrics.o")
        and not o.endswith("m68k_gameplay_step_set_overlay.o")
        and not o.endswith("m68k_gameplay_step_metrics_overlay.o")
        and not "gameplay_session_step_trace" in o
    ]
    for obj in compiled_objs:
        final_objs.append(to_wsl_path(obj))

    rsp_file = runner_dir / "objects.rsp"
    rsp_file.write_text("\n".join(final_objs) + "\n", encoding="utf-8")

    # Link runner
    core_lib_wsl = to_wsl_path(base_build_dir / "liboasis_core.a")
    re_tooling_lib = base_build_dir / "liboasis_re_tooling.a"
    re_tooling_lib_wsl = to_wsl_path(re_tooling_lib) if re_tooling_lib.exists() else ""
    link_cmd = [
        "wsl", "g++", "-O3", "-DNDEBUG",
        f"@{to_wsl_path(rsp_file)}",
        "-o", to_wsl_path(runner_exe),
        core_lib_wsl,
    ]
    if re_tooling_lib_wsl:
        link_cmd.append(re_tooling_lib_wsl)
    link_cmd.append("-ldl")
    print("Linking oasis_gameplay_session...")
    res_link = subprocess.run(link_cmd, capture_output=True, text=True)
    if res_link.returncode != 0 or not runner_exe.exists():
        print(f"LINK FAILED:\n{res_link.stderr}")
        sys.exit(1)

    sha = file_sha256(runner_exe)
    print(f"SUCCESS: Runner built at {runner_exe}")
    print(f"RUNNER_SHA256 = {sha}")

if __name__ == "__main__":
    main()
