#!/usr/bin/env python3
"""
Build script for isolated M15.250/M15.255 Linux gameplay runner with:
  - 196 active production helpers (unchanged)
  - slot 0059a8p (M15.244 portable baseline, 130 ops, default-disabled)
  - slot m15245p (M15.245 expanded entity handlers, 508 ops, default-disabled)
  - slot m15246p (M15.246 connected high-frequency callees, 375 ops, default-disabled)
  - slot m15247p (M15.247 unowned connected handlers/palette routines, 91 ops, default-disabled)
  - slot m15248p (M15.248 high-frequency unowned hotpath callees, 240 ops, default-disabled)
  - slot m15249p (M15.249 high-displacement connected hotpaths, 118 ops, default-disabled)
  - slot m15250p (M15.250 single-step hotpaths, default-disabled)
  - slot m15250f (M15.252 single-basic-block fusion, default-disabled)
  - slot m15250m (M15.254 bounded multi-block fusion, default-disabled)
  - slot m15250k (M15.255 direct Work RAM fast-path experiment, default-disabled)
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
    m249_dir = repo_root / "build" / "auto-migrator" / "m15_249" / "linux-runner"
    runner_dir = repo_root / "build" / "auto-migrator" / "m15_250" / "linux-runner"
    runner_dir.mkdir(parents=True, exist_ok=True)
    runner_exe = runner_dir / "oasis_gameplay_session"

    m249_step = (m249_dir / "m68k_gameplay_step_set_overlay.cpp").read_text(encoding="utf-8")
    m249_metrics = (m249_dir / "m68k_gameplay_step_metrics_overlay.cpp").read_text(encoding="utf-8")

    step_code = m249_step.replace(
        '#include "tools/thor_ir/exp249_dispatch_hybrid_adapter.hpp"',
        '#include "tools/thor_ir/exp249_dispatch_hybrid_adapter.hpp"\n#include "tools/thor_ir/exp250_dispatch_hybrid_adapter.hpp"\n#include "tools/thor_ir/exp250_fused_hybrid_adapter.hpp"\n#include "tools/thor_ir/exp250_direct_ram_adapter.hpp"',
        1,
    )
    m_arr = re.search(r"std::array<bool,\s*(\d+)>\s*g_enabled\{\};", step_code)
    old_sz = int(m_arr.group(1))
    new_sz = old_sz + 4
    step_code = step_code.replace(m_arr.group(0), f"std::array<bool, {new_sz}> g_enabled{{}};", 1)
    step_code = step_code.replace(
        "#undef CHK",
        f"    CHK(oasis::recompiler::ir::step_m68k_portable_m15250, {old_sz});\n"
        f"    CHK(oasis::recompiler::ir::step_m68k_portable_m15250_fused, {old_sz + 1});\n"
        f"    CHK(oasis::recompiler::ir::step_m68k_portable_m15250_multiblock, {old_sz + 2});\n"
        f"    CHK(oasis::recompiler::ir::step_m68k_portable_m15250_direct_ram, {old_sz + 3});\n#undef CHK",
        1,
    )
    init_hook = (
        f'oasis::recompiler::ir::reset_exp249_dispatch_hybrid_metrics();\n'
        f'    g_enabled[{old_sz}] = step_helper_selected_exact(enabled_helpers, "m15250p");\n'
        f'    g_enabled[{old_sz + 1}] = step_helper_selected_exact(enabled_helpers, "m15250f");\n'
        f'    g_enabled[{old_sz + 2}] = step_helper_selected_exact(enabled_helpers, "m15250m");\n'
        f'    g_enabled[{old_sz + 3}] = step_helper_selected_exact(enabled_helpers, "m15250k");\n'
        f'    using GetMemData = void* (*)(unsigned);\n'
        f'    using GetMemSize = std::size_t (*)(unsigned);\n'
        f'    auto get_mem = core.get<GetMemData>("retro_get_memory_data");\n'
        f'    auto get_sz = core.get<GetMemSize>("retro_get_memory_size");\n'
        f'    if (get_mem && get_sz) {{\n'
        f'        oasis::recompiler::ir::set_m15250_direct_ram(static_cast<std::uint8_t*>(get_mem(2)), get_sz(2));\n'
        f'    }}\n'
        f'    oasis::recompiler::ir::reset_exp250_memory_profile_metrics();\n'
        f'    oasis::recompiler::ir::reset_exp250_dispatch_hybrid_metrics();'
    )
    step_code = step_code.replace("oasis::recompiler::ir::reset_exp249_dispatch_hybrid_metrics();", init_hook, 1)
    overlay_cpp = runner_dir / "m68k_gameplay_step_set_overlay.cpp"
    overlay_cpp.write_text(step_code, encoding="utf-8")

    metrics_code = m249_metrics.replace(
        '#include "tools/thor_ir/exp249_dispatch_hybrid_adapter.hpp"',
        '#include "tools/thor_ir/exp249_dispatch_hybrid_adapter.hpp"\n#include "tools/thor_ir/exp250_dispatch_hybrid_adapter.hpp"\n#include "tools/thor_ir/exp250_direct_ram_adapter.hpp"',
        1,
    )
    dump_250 = """
        {
            const auto& m250 = oasis::recompiler::ir::get_exp250_dispatch_hybrid_metrics();
            const auto& mem = oasis::recompiler::ir::get_exp250_memory_profile_metrics();
            std::ofstream ofs250(parent / "m15250-portable-step-metrics.json");
            if (ofs250) {
                ofs250 << "{\\n"
                       << "  \\"m15250_entries\\": " << m250.m15250_entries << ",\\n"
                       << "  \\"m15250_guest_steps\\": " << m250.m15250_guest_steps << ",\\n"
                       << "  \\"disjoint_new_native_steps\\": " << m250.disjoint_new_native_steps << ",\\n"
                       << "  \\"original_cpu_fallbacks\\": " << m250.original_cpu_fallbacks << ",\\n"
                       << "  \\"valid_uncompiled_exits\\": " << m250.valid_uncompiled_exits << ",\\n"
                       << "  \\"cpu_exceptions\\": " << m250.cpu_exceptions << ",\\n"
                       << "  \\"contract_faults\\": " << m250.contract_faults << ",\\n"
                       << "  \\"completed_returns\\": " << m250.completed_returns << ",\\n"
                       << "  \\"fused_blocks\\": " << m250.fused_blocks << ",\\n"
                       << "  \\"multi_block_entries\\": " << m250.multi_block_entries << ",\\n"
                       << "  \\"multi_block_chained_blocks\\": " << m250.multi_block_chained_blocks << ",\\n"
                       << "  \\"internal_cfg_edges_traversed\\": " << m250.internal_cfg_edges_traversed << ",\\n"
                       << "  \\"reads_byte\\": " << mem.reads_byte << ",\\n"
                       << "  \\"reads_word\\": " << mem.reads_word << ",\\n"
                       << "  \\"reads_long\\": " << mem.reads_long << ",\\n"
                       << "  \\"writes_byte\\": " << mem.writes_byte << ",\\n"
                       << "  \\"writes_word\\": " << mem.writes_word << ",\\n"
                       << "  \\"writes_long\\": " << mem.writes_long << ",\\n"
                       << "  \\"ram_canonical_accesses\\": " << mem.ram_canonical_accesses << ",\\n"
                       << "  \\"ram_mirror_accesses\\": " << mem.ram_mirror_accesses << ",\\n"
                       << "  \\"rom_accesses\\": " << mem.rom_accesses << ",\\n"
                       << "  \\"vdp_accesses\\": " << mem.vdp_accesses << ",\\n"
                       << "  \\"mmio_accesses\\": " << mem.mmio_accesses << ",\\n"
                       << "  \\"other_accesses\\": " << mem.other_accesses << ",\\n"
                       << "  \\"fast_eligible_reads\\": " << mem.fast_eligible_reads << ",\\n"
                       << "  \\"fast_eligible_writes\\": " << mem.fast_eligible_writes << ",\\n"
                       << "  \\"unaligned_word_long\\": " << mem.unaligned_word_long << ",\\n"
                       << "  \\"boundary_crossing\\": " << mem.boundary_crossing << ",\\n"
                       << "  \\"fast_ram_reads_byte\\": " << mem.fast_ram_reads_byte << ",\\n"
                       << "  \\"fast_ram_reads_word\\": " << mem.fast_ram_reads_word << ",\\n"
                       << "  \\"fast_ram_reads_long\\": " << mem.fast_ram_reads_long << ",\\n"
                       << "  \\"fast_ram_writes_byte\\": " << mem.fast_ram_writes_byte << ",\\n"
                       << "  \\"fast_ram_writes_word\\": " << mem.fast_ram_writes_word << ",\\n"
                       << "  \\"fast_ram_writes_long\\": " << mem.fast_ram_writes_long << ",\\n"
                       << "  \\"fallback_bus_reads\\": " << mem.fallback_bus_reads << ",\\n"
                       << "  \\"fallback_bus_writes\\": " << mem.fallback_bus_writes << "\\n"
                       << "}\\n";
            }
        }
        write_batch_step_metrics(parent);"""
    metrics_code = metrics_code.replace("write_batch_step_metrics(parent);", dump_250, 1)
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

    prev_objs = [l.strip() for l in (m249_dir / "objects.rsp").read_text(encoding="utf-8").splitlines() if l.strip()]
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
