#!/usr/bin/env python3
"""
M15.247 Auto-Splitting Portable C++20 Emitter.
Emits standalone M15.247 math/shift runtime helpers (exp247_dispatch_common.hpp)
and partitions instruction steppers, routers, and Mode B basic-block runners
into <= 320-line headers under src/tools/thor_ir/generated_m15_247/.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

from pathlib import Path
from typing import Dict, List, Tuple

from tools.thor_recompiler.insn_lowering_m15_247 import emit_m15_247_insn_body

EXP247_COMMON_HPP = """#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"

namespace oasis::recompiler::ir {

struct PortableIrBus247 : PortableIrBus246 {
  explicit PortableIrBus247(PortableGuestEnvironment& e) noexcept : PortableIrBus246{e} {}
};

inline int m15247_calc_mulu_cycles(std::uint16_t src) noexcept {
  int mc = 38 * 7;
  while (src != 0U) { if ((src & 1U) != 0U) mc += 14; src >>= 1U; }
  return mc;
}

} // namespace oasis::recompiler::ir
"""


def _split_list(items: list, chunk_size: int) -> List[list]:
    return [items[i:i + chunk_size] for i in range(0, len(items), chunk_size)]


def emit_m15_247_cpp_files(ir_doc: dict, gen_dir: Path) -> Tuple[dict, List[str]]:
    gen_dir.mkdir(parents=True, exist_ok=True)
    common_path = gen_dir / "exp247_dispatch_common.hpp"
    common_path.write_text(EXP247_COMMON_HPP, encoding="utf-8")

    all_insns = [ins for b in ir_doc["blocks"] for ins in b["instructions"]]
    all_insns.sort(key=lambda x: int(x["pc"], 16))
    compiled_pcs = [int(x["pc"], 16) for x in all_insns]

    parts_written: List[str] = ["exp247_dispatch_common.hpp"]
    source_map: Dict[str, dict] = {}
    max_lines_per_part = 260
    hdr = '#include "tools/thor_ir/generated_m15_247/exp247_dispatch_common.hpp"'

    cur_lines = ["#pragma once", hdr, "namespace oasis::recompiler::ir {"]
    part_idx = 0
    insn_parts: List[str] = []

    def flush_part():
        nonlocal cur_lines, part_idx
        if len(cur_lines) <= 3:
            return
        cur_lines.append("} // namespace oasis::recompiler::ir")
        fname = f"exp247_dispatch_part{part_idx}.hpp"
        (gen_dir / fname).write_text("\n".join(cur_lines) + "\n", encoding="utf-8")
        parts_written.append(fname)
        insn_parts.append(fname)
        part_idx += 1
        cur_lines = ["#pragma once", hdr, "namespace oasis::recompiler::ir {"]

    for insn in all_insns:
        body = emit_m15_247_insn_body(insn)
        if len(cur_lines) + len(body) + 2 > max_lines_per_part:
            flush_part()
        start_ln = len(cur_lines) + 1
        cur_lines.extend(body)
        end_ln = len(cur_lines)
        fname = f"exp247_dispatch_part{part_idx}.hpp"
        source_map[insn["pc"]] = {
            "ir_op_id": insn["ir_op_id"],
            "cpp_file": f"src/tools/thor_ir/generated_m15_247/{fname}",
            "cpp_lines": f"{start_ln}..{end_ln}",
        }
    flush_part()

    router_chunks = _split_list(compiled_pcs, 180)
    router_parts: List[str] = []
    for ri, rpcs in enumerate(router_chunks):
        r_lines = ["#pragma once"] + [f'#include "tools/thor_ir/generated_m15_247/{pn}"' for pn in insn_parts]
        r_lines.append("namespace oasis::recompiler::ir {")
        r_lines.append(f"template <typename Bus> inline bool step_exp247_router_p{ri}(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {{")
        r_lines.append("  switch (pc) {")
        for p in rpcs:
            r_lines.append(f"    case 0x{p:06X}U: return step_ir_0x{p:06X}(bus, out_pc);")
        r_lines.append("    default: return false;")
        r_lines.append("  }")
        r_lines.append("}")
        r_lines.append(f"inline constexpr bool is_compiled_exp247_p{ri}(std::uint32_t pc) noexcept {{")
        r_lines.append("  switch (pc) {")
        for p in rpcs:
            r_lines.append(f"    case 0x{p:06X}U: return true;")
        r_lines.append("    default: return false;")
        r_lines.append("  }")
        r_lines.append("}")
        r_lines.append("} // namespace oasis::recompiler::ir")
        rfname = f"exp247_dispatch_router_p{ri}.hpp"
        (gen_dir / rfname).write_text("\n".join(r_lines) + "\n", encoding="utf-8")
        parts_written.append(rfname)
        router_parts.append(rfname)

    top_r = ["#pragma once"] + [f'#include "tools/thor_ir/generated_m15_247/{rp}"' for rp in router_parts]
    top_r.extend([
        "namespace oasis::recompiler::ir {",
        "inline constexpr bool is_compiled_exp247_dispatch_pc(std::uint32_t pc) noexcept {",
        "  return " + " || ".join([f"is_compiled_exp247_p{i}(pc)" for i in range(len(router_parts))]) + ";",
        "}",
        "template <typename Bus>",
        "inline bool step_one_exp247_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {",
    ])
    for ri in range(len(router_parts)):
        top_r.append(f"  if (is_compiled_exp247_p{ri}(pc)) return step_exp247_router_p{ri}(bus, pc, out_pc);")
    top_r.extend([
        "  return false;",
        "}",
        "} // namespace oasis::recompiler::ir",
    ])
    (gen_dir / "exp247_dispatch_router.hpp").write_text("\n".join(top_r) + "\n", encoding="utf-8")
    parts_written.append("exp247_dispatch_router.hpp")

    # Mode B basic-block chunks
    rhdr = '#include "tools/thor_ir/generated_m15_247/exp247_dispatch_router.hpp"'
    mb_parts: List[str] = []
    cur_mb = ["#pragma once", rhdr, "namespace oasis::recompiler::ir {"]
    mb_idx = 0
    mb_ranges: List[Tuple[int, int, int]] = []
    cur_block_starts: List[int] = []

    def open_mb():
        cur_mb.append(f"inline bool run_exp247_mb_chunk_{mb_idx}(PortableIrBus247& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {{")
        cur_mb.append("  switch (pc) {")

    def flush_mb():
        nonlocal cur_mb, mb_idx, cur_block_starts
        if not cur_block_starts:
            return
        cur_mb.extend(["  default: return false; }", "}", "} // namespace oasis::recompiler::ir"])
        mname = f"exp247_dispatch_mode_b_p{mb_idx}.hpp"
        (gen_dir / mname).write_text("\n".join(cur_mb) + "\n", encoding="utf-8")
        parts_written.append(mname)
        mb_parts.append(mname)
        mb_ranges.append((mb_idx, cur_block_starts[0], cur_block_starts[-1]))
        mb_idx += 1
        cur_block_starts = []
        cur_mb = ["#pragma once", rhdr, "namespace oasis::recompiler::ir {"]

    open_mb()
    for b in ir_doc["blocks"]:
        spc = int(b["start_pc"], 16)
        b_lines = [f"  case 0x{spc:06X}U: {{"]
        for insn in b["instructions"]:
            ipc = int(insn["pc"], 16)
            b_lines.append(f"    if (!step_ir_0x{ipc:06X}(bus, next_pc)) return true; if (next_pc == stop_pc) return true;")
        b_lines.append("    return true; }")
        if cur_block_starts and len(cur_mb) + len(b_lines) + 4 > 250:
            flush_mb()
            open_mb()
        cur_mb.extend(b_lines)
        cur_block_starts.append(spc)
    flush_mb()

    top_mb = ["#pragma once"] + [f'#include "tools/thor_ir/generated_m15_247/{mn}"' for mn in mb_parts] + [
        "namespace oasis::recompiler::ir {",
        "inline bool step_exp247_mb_block(PortableIrBus247& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {",
        "  bool handled = false;",
    ]
    for idx_m, lo, hi in mb_ranges:
        top_mb.append(f"  if (!handled && pc >= 0x{lo:06X}U && pc <= 0x{hi:06X}U) handled = run_exp247_mb_chunk_{idx_m}(bus, pc, stop_pc, next_pc);")
    top_mb.extend([
        "  return handled || step_one_exp247_dispatch_ir(bus, pc, next_pc);",
        "}",
        "inline PortableExecutionResult finalize_exp247_res(PortableIrBus247& bus, std::uint32_t stop_pc) {",
        "  PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;",
        "  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = bus.div_zero ? 5U : (bus.env.address_error ? 3U : 2U); }",
        "  else if (bus.env.yielded || (bus.env.pc != stop_pc && !is_compiled_exp247_dispatch_pc(bus.env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;",
        "  for (int i = 0; i < 8; ++i) { res.d[i] = bus.env.d_regs[i]; res.a[i] = bus.env.a_regs[i]; }",
        "  res.pc = bus.env.pc; res.ccr = bus.env.read_ccr(); res.cycles = bus.env.total_cycles; res.writes = bus.writes;",
        "  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;",
        "}",
        "inline PortableExecutionResult run_exp247_dispatch_mode_b(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  PortableIrBus247 bus{env}; env.pc = entry_pc & kBusAddressMask; std::uint32_t next_pc = env.pc; int guard = 0;",
        "  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {",
        "    if (!is_compiled_exp247_dispatch_pc(env.pc)) break;",
        "    if (!step_exp247_mb_block(bus, env.pc, stop_pc, next_pc)) break;",
        "  }",
        "  return finalize_exp247_res(bus, stop_pc);",
        "}",
        "} // namespace oasis::recompiler::ir",
    ])
    (gen_dir / "exp247_dispatch_mode_b.hpp").write_text("\n".join(top_mb) + "\n", encoding="utf-8")
    parts_written.append("exp247_dispatch_mode_b.hpp")

    umb = [
        "#pragma once",
        '#include "tools/thor_ir/generated_m15_247/exp247_dispatch_mode_b.hpp"',
        "namespace oasis::recompiler::ir {",
        "inline PortableExecutionResult run_exp247_dispatch_mode_a(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  PortableIrBus247 bus{env}; env.pc = entry_pc & kBusAddressMask; std::uint32_t next_pc = env.pc; int guard = 0;",
        "  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {",
        "    if (!is_compiled_exp247_dispatch_pc(env.pc)) break;",
        "    if (!step_one_exp247_dispatch_ir(bus, env.pc, next_pc)) break;",
        "  }",
        "  return finalize_exp247_res(bus, stop_pc);",
        "}",
        "} // namespace oasis::recompiler::ir",
    ]
    (gen_dir / "exp247_dispatch_portable_routines.hpp").write_text("\n".join(umb) + "\n", encoding="utf-8")
    parts_written.append("exp247_dispatch_portable_routines.hpp")
    return source_map, parts_written
