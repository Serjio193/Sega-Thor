#!/usr/bin/env python3
"""
M15.245 Auto-Splitting Portable C++20 Emitter.
Splits instruction steppers, routers, AND Mode B basic-block runners into <=320-line headers.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from pathlib import Path
from typing import Dict, List, Tuple

from tools.thor_recompiler.insn_lowering_m15_245 import emit_insn_body

EXP_COMMON_HPP = """#pragma once
#include "tools/thor_ir/generated/entity_dispatch_common.hpp"
"""


def _split_list(items: list, chunk_size: int) -> List[list]:
    return [items[i:i + chunk_size] for i in range(0, len(items), chunk_size)]


def emit_m15_245_cpp_files(ir_doc: dict, gen_dir: Path) -> Tuple[dict, List[str]]:
    gen_dir.mkdir(parents=True, exist_ok=True)
    common_path = gen_dir / "exp_dispatch_common.hpp"
    common_path.write_text(EXP_COMMON_HPP, encoding="utf-8")

    all_insns = []
    for b in ir_doc["blocks"]:
        all_insns.extend(b["instructions"])
    all_insns.sort(key=lambda x: int(x["pc"], 16))
    compiled_pcs = [int(x["pc"], 16) for x in all_insns]

    parts_written: List[str] = ["exp_dispatch_common.hpp"]
    source_map: Dict[str, dict] = {}
    max_lines_per_part = 260

    cur_lines = [
        "#pragma once",
        '#include "tools/thor_ir/generated_m15_245/exp_dispatch_common.hpp"',
        "namespace oasis::recompiler::ir {",
    ]
    part_idx = 0
    insn_parts: List[str] = []

    def flush_part():
        nonlocal cur_lines, part_idx
        if len(cur_lines) <= 3:
            return
        cur_lines.append("} // namespace oasis::recompiler::ir")
        fname = f"exp_dispatch_part{part_idx}.hpp"
        (gen_dir / fname).write_text("\n".join(cur_lines) + "\n", encoding="utf-8")
        parts_written.append(fname)
        insn_parts.append(fname)
        part_idx += 1
        cur_lines = [
            "#pragma once",
            '#include "tools/thor_ir/generated_m15_245/exp_dispatch_common.hpp"',
            "namespace oasis::recompiler::ir {",
        ]

    for insn in all_insns:
        body = emit_insn_body(insn)
        if len(cur_lines) + len(body) + 2 > max_lines_per_part:
            flush_part()
        start_ln = len(cur_lines) + 1
        cur_lines.extend(body)
        end_ln = len(cur_lines)
        fname = f"exp_dispatch_part{part_idx}.hpp"
        source_map[insn["pc"]] = {
            "ir_op_id": insn["ir_op_id"],
            "cpp_file": f"src/tools/thor_ir/generated_m15_245/{fname}",
            "cpp_lines": f"{start_ln}..{end_ln}",
        }
    flush_part()

    # Auto-split Mode A router across sub-headers (~180 PCs per sub-router => <=220 lines)
    router_chunks = _split_list(compiled_pcs, 180)
    router_parts: List[str] = []
    for ri, rpcs in enumerate(router_chunks):
        r_lines = ["#pragma once"]
        for pn in insn_parts:
            r_lines.append(f'#include "tools/thor_ir/generated_m15_245/{pn}"')
        r_lines.append("namespace oasis::recompiler::ir {")
        r_lines.append(f"template <typename Bus> inline bool step_exp_chunk_{ri}(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {{")
        r_lines.append("  switch (pc & kBusAddressMask) {")
        for p in rpcs:
            r_lines.append(f"  case 0x{p:06X}U: return step_ir_0x{p:06X}(bus, out_pc);")
        r_lines.append("  default: return false; }")
        r_lines.append("}")
        r_lines.append("} // namespace oasis::recompiler::ir")
        rname = f"exp_dispatch_router_p{ri}.hpp"
        (gen_dir / rname).write_text("\n".join(r_lines) + "\n", encoding="utf-8")
        parts_written.append(rname)
        router_parts.append(rname)

    top_r = ["#pragma once"]
    for rn in router_parts:
        top_r.append(f'#include "tools/thor_ir/generated_m15_245/{rn}"')
    top_r.append("namespace oasis::recompiler::ir {")
    top_r.append("inline bool is_compiled_exp_dispatch_pc(std::uint32_t pc) noexcept {")
    top_r.append("  switch (pc & kBusAddressMask) {")
    for i in range(0, len(compiled_pcs), 8):
        cases = " ".join(f"case 0x{p:06X}U:" for p in compiled_pcs[i:i + 8])
        top_r.append(f"  {cases} return true;")
    top_r.append("  default: return false; }")
    top_r.append("}")
    top_r.append("template <typename Bus> inline bool step_one_exp_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {")
    top_r.append("  const std::uint32_t m = pc & kBusAddressMask;")
    for ri, rpcs in enumerate(router_chunks):
        lo, hi = rpcs[0], rpcs[-1]
        top_r.append(f"  if (m >= 0x{lo:06X}U && m <= 0x{hi:06X}U) return step_exp_chunk_{ri}(bus, m, out_pc);")
    top_r.append("  return false;")
    top_r.append("}")
    top_r.append("} // namespace oasis::recompiler::ir")
    (gen_dir / "exp_dispatch_router.hpp").write_text("\n".join(top_r) + "\n", encoding="utf-8")
    parts_written.append("exp_dispatch_router.hpp")

    # Auto-split Mode B basic-block dispatcher across sub-headers <=260 lines each
    mb_parts: List[str] = []
    cur_mb: List[str] = [
        "#pragma once",
        '#include "tools/thor_ir/generated_m15_245/exp_dispatch_router.hpp"',
        "namespace oasis::recompiler::ir {",
    ]
    mb_idx = 0
    mb_ranges: List[Tuple[int, int, int]] = []  # (idx, lo_pc, hi_pc)
    cur_block_starts: List[int] = []

    def flush_mb():
        nonlocal cur_mb, mb_idx, cur_block_starts
        if not cur_block_starts:
            return
        cur_mb.append("  default: return false; }")
        cur_mb.append("}")
        cur_mb.append("} // namespace oasis::recompiler::ir")
        mname = f"exp_dispatch_mode_b_p{mb_idx}.hpp"
        (gen_dir / mname).write_text("\n".join(cur_mb) + "\n", encoding="utf-8")
        parts_written.append(mname)
        mb_parts.append(mname)
        mb_ranges.append((mb_idx, cur_block_starts[0], cur_block_starts[-1]))
        mb_idx += 1
        cur_block_starts = []
        cur_mb = [
            "#pragma once",
            '#include "tools/thor_ir/generated_m15_245/exp_dispatch_router.hpp"',
            "namespace oasis::recompiler::ir {",
        ]

    def open_mb_fn():
        cur_mb.append(f"inline bool run_exp_mb_chunk_{mb_idx}(PortableIrBus& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {{")
        cur_mb.append("  switch (pc) {")

    open_mb_fn()
    for b in ir_doc["blocks"]:
        spc = int(b["start_pc"], 16)
        b_lines = [f"  case 0x{spc:06X}U: {{"]
        for insn in b["instructions"]:
            ipc = int(insn["pc"], 16)
            b_lines.append(f"    if (!step_ir_0x{ipc:06X}(bus, next_pc)) return true; if (next_pc == stop_pc) return true;")
        b_lines.append("    return true; }")
        if cur_block_starts and len(cur_mb) + len(b_lines) + 4 > 250:
            flush_mb()
            open_mb_fn()
        cur_mb.extend(b_lines)
        cur_block_starts.append(spc)
    flush_mb()

    top_mb = ["#pragma once"]
    for mn in mb_parts:
        top_mb.append(f'#include "tools/thor_ir/generated_m15_245/{mn}"')
    top_mb.extend([
        "namespace oasis::recompiler::ir {",
        "inline PortableExecutionResult run_exp_dispatch_mode_b_ir(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  PortableIrBus bus{env}; env.pc = entry_pc & kBusAddressMask; PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;",
        "  std::uint32_t next_pc = env.pc; int guard = 0;",
        "  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {",
        "    if (!is_compiled_exp_dispatch_pc(env.pc)) { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }",
        "    bool handled = false;",
    ])
    for idx_m, lo, hi in mb_ranges:
        top_mb.append(f"    if (env.pc >= 0x{lo:06X}U && env.pc <= 0x{hi:06X}U) handled = run_exp_mb_chunk_{idx_m}(bus, env.pc, stop_pc, next_pc);")
    top_mb.extend([
        "    if (!handled) step_one_exp_dispatch_ir(bus, env.pc, next_pc);",
        "  }",
        "  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }",
        "  else if (env.yielded || (env.pc != stop_pc && !is_compiled_exp_dispatch_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;",
        "  for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }",
        "  res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;",
        "  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;",
        "}",
        "} // namespace oasis::recompiler::ir",
    ])
    (gen_dir / "exp_dispatch_mode_b.hpp").write_text("\n".join(top_mb) + "\n", encoding="utf-8")
    parts_written.append("exp_dispatch_mode_b.hpp")

    umb = [
        "#pragma once",
        '#include "tools/thor_ir/generated_m15_245/exp_dispatch_mode_b.hpp"',
        "namespace oasis::recompiler::ir {",
        "inline PortableExecutionResult run_exp_dispatch_mode_a(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  PortableIrBus bus{env}; env.pc = entry_pc & kBusAddressMask; PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;",
        "  std::uint32_t next_pc = env.pc; int guard = 0;",
        "  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {",
        "    if (!is_compiled_exp_dispatch_pc(env.pc)) { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }",
        "    if (!step_one_exp_dispatch_ir(bus, env.pc, next_pc)) break;",
        "  }",
        "  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }",
        "  else if (env.yielded || (env.pc != stop_pc && !is_compiled_exp_dispatch_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;",
        "  for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }",
        "  res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;",
        "  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;",
        "}",
        "inline PortableExecutionResult run_exp_dispatch_mode_b(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  return run_exp_dispatch_mode_b_ir(env, entry_pc, stop_pc);",
        "}",
        "} // namespace oasis::recompiler::ir",
    ]
    (gen_dir / "exp_dispatch_portable_routines.hpp").write_text("\n".join(umb) + "\n", encoding="utf-8")
    parts_written.append("exp_dispatch_portable_routines.hpp")

    return source_map, parts_written
