#!/usr/bin/env python3
"""
Build script for isolated M15.248 Linux gameplay runner with:
  - 196 active production helpers (unchanged)
  - slot 0059a8p (M15.244 portable baseline, 130 ops, default-disabled)
  - slot m15245p (M15.245 expanded entity handlers, 508 ops, default-disabled)
  - slot m15246p (M15.246 connected high-frequency callees, 375 ops, default-disabled)
  - slot m15247p (M15.247 unowned connected handlers/palette routines, 91 ops, default-disabled)
  - slot m15248p (M15.248 high-frequency unowned hotpath callees, 240 ops, default-disabled)
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import hashlib
from pathlib import Path
import re
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


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent
    base_build_dir = repo_root / "build" / "helper10-9d70-linux-release-20261013"
    m247_dir = repo_root / "build" / "auto-migrator" / "m15_247" / "linux-runner"
    runner_dir = repo_root / "build" / "auto-migrator" / "m15_248" / "linux-runner"
    runner_dir.mkdir(parents=True, exist_ok=True)
    runner_exe = runner_dir / "oasis_gameplay_session"

    m247_step = (m247_dir / "m68k_gameplay_step_set_overlay.cpp").read_text(encoding="utf-8")
    m247_metrics = (m247_dir / "m68k_gameplay_step_metrics_overlay.cpp").read_text(encoding="utf-8")

    step_code = m247_step.replace(
        '#include "tools/thor_ir/exp247_dispatch_hybrid_adapter.hpp"',
        '#include "tools/thor_ir/exp247_dispatch_hybrid_adapter.hpp"\n#include "tools/thor_ir/exp248_dispatch_hybrid_adapter.hpp"',
        1,
    )
    m_arr = re.search(r"std::array<bool,\s*(\d+)>\s*g_enabled\{\};", step_code)
    old_sz = int(m_arr.group(1))
    new_sz = old_sz + 1
    step_code = step_code.replace(m_arr.group(0), f"std::array<bool, {new_sz}> g_enabled{{}};", 1)
    step_code = step_code.replace(
        "#undef CHK",
        f"    CHK(oasis::recompiler::ir::step_m68k_portable_m15248, {old_sz});\n#undef CHK",
        1,
    )
    step_code = step_code.replace(
        "oasis::recompiler::ir::reset_exp247_dispatch_hybrid_metrics();",
        f'oasis::recompiler::ir::reset_exp247_dispatch_hybrid_metrics();\n'
        f'    g_enabled[{old_sz}] = step_helper_selected_exact(enabled_helpers, "m15248p");\n'
        f"    oasis::recompiler::ir::reset_exp248_dispatch_hybrid_metrics();",
        1,
    )
    overlay_cpp = runner_dir / "m68k_gameplay_step_set_overlay.cpp"
    overlay_cpp.write_text(step_code, encoding="utf-8")

    metrics_code = m247_metrics.replace(
        '#include "tools/thor_ir/exp247_dispatch_hybrid_adapter.hpp"',
        '#include "tools/thor_ir/exp247_dispatch_hybrid_adapter.hpp"\n#include "tools/thor_ir/exp248_dispatch_hybrid_adapter.hpp"',
        1,
    )
    dump_248 = """
        {
            const auto& m248 = oasis::recompiler::ir::get_exp248_dispatch_hybrid_metrics();
            std::ofstream ofs248(parent / "m15248-portable-step-metrics.json");
            if (ofs248) {
                ofs248 << "{\\n"
                       << "  \\"m15248_entries\\": " << m248.m15248_entries << ",\\n"
                       << "  \\"m15248_guest_steps\\": " << m248.m15248_guest_steps << ",\\n"
                       << "  \\"disjoint_new_native_steps\\": " << m248.disjoint_new_native_steps << ",\\n"
                       << "  \\"original_cpu_fallbacks\\": " << m248.original_cpu_fallbacks << ",\\n"
                       << "  \\"valid_uncompiled_exits\\": " << m248.valid_uncompiled_exits << ",\\n"
                       << "  \\"cpu_exceptions\\": " << m248.cpu_exceptions << ",\\n"
                       << "  \\"contract_faults\\": " << m248.contract_faults << ",\\n"
                       << "  \\"completed_returns\\": " << m248.completed_returns << ",\\n"
                       << "  \\"root_hits\\": [";
                for (int i = 0; i < 15; ++i) ofs248 << (i ? "," : "") << m248.root_hits[i];
                ofs248 << "]\\n}\\n";
            }
        }
        write_batch_step_metrics(parent);"""
    metrics_code = metrics_code.replace("write_batch_step_metrics(parent);", dump_248, 1)
    metrics_cpp = runner_dir / "m68k_gameplay_step_metrics_overlay.cpp"
    metrics_cpp.write_text(metrics_code, encoding="utf-8")

    repo_src_wsl = to_wsl_path(repo_root / "src")
    repo_inc_wsl = to_wsl_path(repo_root / "include")
    libretro_inc_wsl = to_wsl_path(repo_root / "build" / "9c40-hook-source" / "libretro" / "libretro-common" / "include")

    compiled = {}
    for src in (overlay_cpp, metrics_cpp):
        obj = runner_dir / f"{src.stem}.o"
        cmd = [
            "wsl", "g++", "-O3", "-DNDEBUG", "-std=c++20",
            f"-I{repo_src_wsl}", f"-I{repo_inc_wsl}", f"-I{libretro_inc_wsl}",
            "-c", to_wsl_path(src), "-o", to_wsl_path(obj),
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print(f"Compile error in {src.name}:\n{res.stderr}", file=sys.stderr)
            return 1
        compiled[obj.name] = to_wsl_path(obj)

    prev_objs = [l.strip() for l in (m247_dir / "objects.rsp").read_text(encoding="utf-8").splitlines() if l.strip()]
    final_objs = [compiled.get(Path(o).name, o) for o in prev_objs]
    rsp_file = runner_dir / "objects.rsp"
    rsp_file.write_text("\n".join(final_objs) + "\n", encoding="utf-8")

    core_lib_wsl = to_wsl_path(base_build_dir / "liboasis_core.a")
    re_tooling_lib = base_build_dir / "liboasis_re_tooling.a"
    link_cmd = ["wsl", "g++", "-O3", "-DNDEBUG", "-std=c++20", f"@{to_wsl_path(rsp_file)}", "-o", to_wsl_path(runner_exe), core_lib_wsl]
    if re_tooling_lib.exists():
        link_cmd.append(to_wsl_path(re_tooling_lib))
    link_cmd.extend(["-ldl", "-lpthread"])
    res = subprocess.run(link_cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Link error:\n{res.stderr}", file=sys.stderr)
        return 1

    print(f"SUCCESS: Built {runner_exe}")
    print(f"RUNNER_SHA256 = {file_sha256(runner_exe)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
