#!/usr/bin/env python3
"""
Build script for isolated Linux runner with M15.239 profiling and recompiled functions.
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
    runner_dir = repo_root / "build" / "auto-migrator" / "m15_239" / "linux-runner"
    runner_dir.mkdir(parents=True, exist_ok=True)
    runner_exe = runner_dir / "oasis_gameplay_session"

    print(f"Target runner directory: {runner_dir}")

    # 1. Generate step_set overlay: slots 222..238
    base_step_set = (repo_root / "src" / "tools" / "hybrid" / "m68k_gameplay_step_set.cpp").read_text(encoding="utf-8")

    inc_marker = '#include "tools/hybrid/m68k_9d70_instruction_step.hpp"'
    new_inc = (
        '#include "tools/hybrid/m68k_02575c_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_00a9a0_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_008f22_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_002fda_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_002512_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_0023cc_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_00d168_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_batch_step.hpp"\n'
        + inc_marker
    )
    step_set_code = base_step_set.replace(inc_marker, new_inc, 1)
    step_set_code = step_set_code.replace("std::array<bool, 222> g_enabled{};", "std::array<bool, 242> g_enabled{};", 1)

    last_chk = "CHK(step_m68k_00dbca, 221);"
    new_chk = (
        last_chk + "\n"
        "    CHK(step_m68k_02575c, 222);\n"
        "    CHK(step_m68k_00ad76, 223);\n"
        "    CHK(step_m68k_00a9a0, 224);\n"
        "    CHK(step_m68k_008f22, 225);\n"
        "    CHK(step_m68k_002fda, 226);\n"
        "    CHK(step_m68k_002512, 227);\n"
        "    CHK(step_m68k_0023cc, 228);\n"
        "    CHK(step_m68k_00d168, 229);\n"
        "    CHK(step_m68k_batch_00d266, 230);\n"
        "    CHK(step_m68k_batch_062732, 231);\n"
        "    CHK(step_m68k_batch_062722, 232);\n"
        "    CHK(step_m68k_batch_005660, 233);\n"
        "    CHK(step_m68k_batch_006e7a, 234);\n"
        "    CHK(step_m68k_batch_062694, 235);\n"
        "    CHK(step_m68k_batch_062a78, 236);\n"
        "    CHK(step_m68k_batch_00d2b4, 237);\n"
        "    CHK(step_m68k_batch_0059a8, 238);\n"
        "    CHK(step_m68k_batch_00e682, 239);\n"
        "    CHK(step_m68k_batch_015ae2, 240);\n"
        "    CHK(step_m68k_batch_00c9b2, 241);"
    )
    step_set_code = step_set_code.replace(last_chk, new_chk, 1)

    tail_marker = "for (std::size_t i = 0; i < tail_vals.size(); ++i) g_enabled[101U + i] = tail_vals[i];"
    new_tail = (
        tail_marker + '\n'
        '    g_enabled[222] = step_helper_selected_exact(enabled_helpers, "02575c");\n'
        '    g_enabled[223] = step_helper_selected_exact(enabled_helpers, "00ad76");\n'
        '    g_enabled[224] = step_helper_selected_exact(enabled_helpers, "00a9a0");\n'
        '    g_enabled[225] = step_helper_selected_exact(enabled_helpers, "008f22");\n'
        '    g_enabled[226] = step_helper_selected_exact(enabled_helpers, "002fda");\n'
        '    g_enabled[227] = step_helper_selected_exact(enabled_helpers, "002512");\n'
        '    g_enabled[228] = step_helper_selected_exact(enabled_helpers, "0023cc");\n'
        '    g_enabled[229] = step_helper_selected_exact(enabled_helpers, "00d168");\n'
        '    g_enabled[230] = step_helper_selected_exact(enabled_helpers, "00d266");\n'
        '    g_enabled[231] = step_helper_selected_exact(enabled_helpers, "062732");\n'
        '    g_enabled[232] = step_helper_selected_exact(enabled_helpers, "062722");\n'
        '    g_enabled[233] = step_helper_selected_exact(enabled_helpers, "005660");\n'
        '    g_enabled[234] = step_helper_selected_exact(enabled_helpers, "006e7a");\n'
        '    g_enabled[235] = step_helper_selected_exact(enabled_helpers, "062694");\n'
        '    g_enabled[236] = step_helper_selected_exact(enabled_helpers, "062a78");\n'
        '    g_enabled[237] = step_helper_selected_exact(enabled_helpers, "00d2b4");\n'
        '    g_enabled[238] = step_helper_selected_exact(enabled_helpers, "0059a8");\n'
        '    g_enabled[239] = step_helper_selected_exact(enabled_helpers, "00e682");\n'
        '    g_enabled[240] = step_helper_selected_exact(enabled_helpers, "015ae2");\n'
        '    g_enabled[241] = step_helper_selected_exact(enabled_helpers, "00c9b2");'
    )
    step_set_code = step_set_code.replace(tail_marker, new_tail, 1)

    overlay_cpp = runner_dir / "m68k_gameplay_step_set_overlay.cpp"
    overlay_cpp.write_text(step_set_code, encoding="utf-8")
    print("Generated m68k_gameplay_step_set_overlay.cpp")

    # 2. Generate metrics overlay
    base_metrics = (repo_root / "src" / "tools" / "hybrid" / "m68k_gameplay_step_metrics.cpp").read_text(encoding="utf-8")

    inc_metrics_marker = '#include "tools/hybrid/m68k_00dbca_instruction_step.hpp"'
    new_metrics_inc = (
        inc_metrics_marker + '\n'
        '#include "tools/hybrid/m68k_02575c_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_00a9a0_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_008f22_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_002fda_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_002512_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_0023cc_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_00d168_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_batch_step.hpp"'
    )
    metrics_code = base_metrics.replace(inc_metrics_marker, new_metrics_inc, 1)

    write_marker = 'write_00dbca_step_metrics(parent / "00dbca-step-metrics.json");'
    new_writes = (
        write_marker + '\n'
        '        write_02575c_step_metrics(parent / "02575c-step-metrics.json");\n'
        '        write_00ad76_step_metrics(parent / "00ad76-step-metrics.json");\n'
        '        write_00a9a0_step_metrics(parent / "00a9a0-step-metrics.json");\n'
        '        write_008f22_step_metrics(parent / "008f22-step-metrics.json");\n'
        '        write_002fda_step_metrics(parent / "002fda-step-metrics.json");\n'
        '        write_002512_step_metrics(parent / "002512-step-metrics.json");\n'
        '        write_0023cc_step_metrics(parent / "0023cc-step-metrics.json");\n'
        '        write_00d168_step_metrics(parent / "00d168-step-metrics.json");\n'
        '        write_batch_step_metrics(parent);'
    )
    metrics_code = metrics_code.replace(write_marker, new_writes, 1)

    metrics_cpp = runner_dir / "m68k_gameplay_step_metrics_overlay.cpp"
    metrics_cpp.write_text(metrics_code, encoding="utf-8")
    print("Generated m68k_gameplay_step_metrics_overlay.cpp")

    # 3. Compile overlays, batch step, recompiled cpp, trace, and residual profiler
    repo_src_wsl = to_wsl_path(repo_root / "src")
    repo_inc_wsl = to_wsl_path(repo_root / "include")
    libretro_inc_wsl = to_wsl_path(repo_root / "build" / "9c40-hook-source" / "libretro" / "libretro-common" / "include")

    sources = [
        repo_root / "src" / "tools" / "hybrid" / "m68k_batch_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "m68k_residual_profiler.cpp",
        repo_root / "src" / "tools" / "hybrid" / "gameplay_session_step_trace.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_00d266.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062732.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062722.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_005660.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_006e7a.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062694.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062a78.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_00d2b4.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_0059a8.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_00e682.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_00e682_part0.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_00e682_part1.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_015ae2.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_015ae2_part0.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_015ae2_part1.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_00c9b2.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_00c9b2_part0.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_00c9b2_part1.cpp",
        overlay_cpp,
        metrics_cpp,
    ]

    compiled_objs = []
    for src in sources:
        obj = runner_dir / f"{src.stem}.o"
        cmd = [
            "wsl", "g++", "-O3", "-DNDEBUG", "-std=c++20",
            f"-I{repo_src_wsl}", f"-I{repo_inc_wsl}", f"-I{libretro_inc_wsl}",
            "-c", to_wsl_path(src),
            "-o", to_wsl_path(obj)
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"Error compiling {src.name}:\n{res.stderr}")
            sys.exit(1)
        compiled_objs.append(obj)

    prev_rsp = repo_root / "build" / "auto-migrator" / "m15_238" / "linux-runner" / "objects.rsp"
    prev_objs = [l.strip() for l in prev_rsp.read_text(encoding="utf-8").splitlines() if l.strip()]

    compiled_names = {src.stem + ".o" for src in sources}

    final_objs = [
        o for o in prev_objs
        if not o.endswith("m68k_gameplay_step_set_overlay.o")
        and not o.endswith("m68k_gameplay_step_metrics_overlay.o")
        and not o.endswith("gameplay_session_step_trace.o")
        and Path(o).name not in compiled_names
    ]

    for obj in compiled_objs:
        final_objs.append(to_wsl_path(obj))

    rsp_file = runner_dir / "objects.rsp"
    rsp_file.write_text("\n".join(final_objs) + "\n", encoding="utf-8")
    print(f"Written objects.rsp with {len(final_objs)} objects")

    print("Linking oasis_gameplay_session for M15.239...")
    core_lib_wsl = to_wsl_path(base_build_dir / "liboasis_core.a")
    re_tooling_lib = base_build_dir / "liboasis_re_tooling.a"
    re_tooling_lib_wsl = to_wsl_path(re_tooling_lib) if re_tooling_lib.exists() else ""
    link_cmd = [
        "wsl", "g++", "-O3", "-DNDEBUG", "-std=c++20",
        f"@{to_wsl_path(rsp_file)}",
        "-o", to_wsl_path(runner_exe),
        core_lib_wsl,
    ]
    if re_tooling_lib_wsl:
        link_cmd.append(re_tooling_lib_wsl)
    link_cmd.extend(["-ldl", "-lpthread"])
    res = subprocess.run(link_cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"FAILED to link oasis_gameplay_session:\n{res.stderr}")
        sys.exit(1)

    runner_hash = file_sha256(runner_exe)
    print(f"SUCCESS: Built {runner_exe}")
    print(f"RUNNER_SHA256 = {runner_hash}")


if __name__ == "__main__":
    main()
