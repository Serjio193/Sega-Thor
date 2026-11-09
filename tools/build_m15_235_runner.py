#!/usr/bin/env python3
"""
Build script for isolated Linux runner with M15.235 Batch Recompiled Functions:
Slots 222..229 + Slots 230..238 (Recompiled Independent Leaf Functions).
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
    runner_dir = repo_root / "build" / "auto-migrator" / "m15_235" / "linux-runner"
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

    step_set_code = step_set_code.replace("std::array<bool, 222> g_enabled{};", "std::array<bool, 239> g_enabled{};", 1)

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
        "    CHK(step_m68k_062610, 230);\n"
        "    CHK(step_m68k_005660, 231);\n"
        "    CHK(step_m68k_062986, 232);\n"
        "    CHK(step_m68k_06293a, 233);\n"
        "    CHK(step_m68k_062900, 234);\n"
        "    CHK(step_m68k_0626a6, 235);\n"
        "    CHK(step_m68k_062a24, 236);\n"
        "    CHK(step_m68k_062694, 237);\n"
        "    CHK(step_m68k_0629ee, 238);"
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
        '    g_enabled[230] = step_helper_selected_exact(enabled_helpers, "062610");\n'
        '    g_enabled[231] = step_helper_selected_exact(enabled_helpers, "005660");\n'
        '    g_enabled[232] = step_helper_selected_exact(enabled_helpers, "062986");\n'
        '    g_enabled[233] = step_helper_selected_exact(enabled_helpers, "06293a");\n'
        '    g_enabled[234] = step_helper_selected_exact(enabled_helpers, "062900");\n'
        '    g_enabled[235] = step_helper_selected_exact(enabled_helpers, "0626a6");\n'
        '    g_enabled[236] = step_helper_selected_exact(enabled_helpers, "062a24");\n'
        '    g_enabled[237] = step_helper_selected_exact(enabled_helpers, "062694");\n'
        '    g_enabled[238] = step_helper_selected_exact(enabled_helpers, "0629ee");'
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
        '#include "tools/hybrid/m68k_002fda_metrics.hpp"\n'
        '#include "tools/hybrid/m68k_002512_metrics.hpp"\n'
        '#include "tools/hybrid/m68k_0023cc_metrics.hpp"\n'
        '#include "tools/hybrid/m68k_00d168_instruction_step.hpp"\n'
        '#include "tools/hybrid/m68k_batch_step.hpp"'
    )
    metrics_code = base_metrics.replace(inc_metrics_marker, new_metrics_inc, 1)

    write_marker = 'write_00dbca_step_metrics(parent / "00dbca-step-metrics.json");'
    new_write = (
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
    metrics_code = metrics_code.replace(write_marker, new_write, 1)

    metrics_cpp = runner_dir / "m68k_gameplay_step_metrics_overlay.cpp"
    metrics_cpp.write_text(metrics_code, encoding="utf-8")
    print("Generated m68k_gameplay_step_metrics_overlay.cpp")

    repo_src_wsl = to_wsl_path(repo_root / "src")
    repo_inc_wsl = to_wsl_path(repo_root / "include")
    libretro_inc_wsl = to_wsl_path(repo_root / "build" / "9c40-hook-source" / "libretro" / "libretro-common" / "include")

    sources = [
        # Batch Step implementation & Generated C++
        repo_root / "src" / "tools" / "hybrid" / "m68k_batch_step.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062610.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062610_p0.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062610_p1.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062610_p2.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_005660.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062986.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062986_p0.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062986_p1.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_06293a.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_06293a_p0.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_06293a_p1.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_06293a_p2.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062900.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_0626a6.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062a24.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062a24_p0.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062a24_p1.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062a24_p2.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_062694.cpp",
        repo_root / "src" / "tools" / "hybrid" / "generated" / "m68k_recompiled_0629ee.cpp",
        # Overlays
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

    # Base runner objects from previous verified runner (m15_234)
    prev_rsp = repo_root / "build" / "auto-migrator" / "m15_234" / "linux-runner" / "objects.rsp"
    prev_objs = [l.strip() for l in prev_rsp.read_text(encoding="utf-8").splitlines() if l.strip()]

    final_objs = [
        o for o in prev_objs
        if not o.endswith("m68k_gameplay_step_set_overlay.o")
        and not o.endswith("m68k_gameplay_step_metrics_overlay.o")
    ]

    for obj in compiled_objs:
        final_objs.append(to_wsl_path(obj))

    rsp_file = runner_dir / "objects.rsp"
    rsp_file.write_text("\n".join(final_objs) + "\n", encoding="utf-8")
    print(f"Written objects.rsp with {len(final_objs)} objects")

    print("Linking oasis_gameplay_session for M15.235...")
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
