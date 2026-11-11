#!/usr/bin/env python3
"""
M15.256 Auto-Splitting Portable C++20 Emitter.
Emits standalone M15.256 runtime helpers (exp256_dispatch_common.hpp)
and partitions instruction steppers, routers, and Mode B basic-block runners
into <= 260-line headers under src/tools/thor_ir/generated_m15_256/.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

from pathlib import Path
from typing import Dict, List, Tuple

from tools.thor_recompiler.insn_lowering_m15_256 import emit_m15_256_insn_body

EXP256_COMMON_HPP = """#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"

namespace oasis::recompiler::ir {

struct PortableIrBus256 : PortableIrBus250 {
  explicit PortableIrBus256(PortableGuestEnvironment& e) noexcept : PortableIrBus250{e} {}
};

} // namespace oasis::recompiler::ir
"""


def _split_list(items: list, chunk_size: int) -> List[list]:
    return [items[i:i + chunk_size] for i in range(0, len(items), chunk_size)]


def emit_m15_256_cpp_files(ir_doc: dict, gen_dir: Path) -> Tuple[dict, List[str]]:
    gen_dir.mkdir(parents=True, exist_ok=True)
    common_path = gen_dir / "exp256_dispatch_common.hpp"
    common_path.write_text(EXP256_COMMON_HPP, encoding="utf-8")

    all_insns = [ins for b in ir_doc["blocks"] for ins in b["instructions"]]
    all_insns.sort(key=lambda x: int(x["pc"], 16))
    compiled_pcs = [int(x["pc"], 16) for x in all_insns]

    source_map: Dict[str, dict] = {}
    parts_written: List[str] = ["exp256_dispatch_common.hpp"]

    max_lines_per_part = 260
    hdr = '#include "tools/thor_ir/generated_m15_256/exp256_dispatch_common.hpp"'

    cur_lines = ["#pragma once", hdr, "namespace oasis::recompiler::ir {"]
    part_idx = 0
    insn_parts: List[str] = []

    def flush_part():
        nonlocal cur_lines, part_idx
        if len(cur_lines) <= 3:
            return
        cur_lines.append("} // namespace oasis::recompiler::ir")
        fname = f"exp256_dispatch_part{part_idx}.hpp"
        (gen_dir / fname).write_text("\n".join(cur_lines) + "\n", encoding="utf-8")
        parts_written.append(fname)
        insn_parts.append(fname)
        part_idx += 1
        cur_lines = ["#pragma once", hdr, "namespace oasis::recompiler::ir {"]

    for ins in all_insns:
        body = emit_m15_256_insn_body(ins)
        if len(cur_lines) + len(body) + 1 > max_lines_per_part:
            flush_part()
        cur_lines.extend(body)
        cur_part = f"exp256_dispatch_part{part_idx}.hpp"
        source_map[ins["pc"]] = {
            "mnemonic": ins["mnemonic"],
            "size": ins["size"],
            "part": cur_part,
            "fn": f"step_ir_{ins['pc'].lower()}",
            "raw_words": ins["raw_words"],
        }
    flush_part()

    router_parts: List[str] = []
    chunk_size = 80
    pc_chunks = _split_list(compiled_pcs, chunk_size)

    for r_idx, chunk in enumerate(pc_chunks):
        rfname = f"exp256_dispatch_router_p{r_idx}.hpp"
        rlines = [
            "#pragma once",
            hdr,
        ]
        for ip in insn_parts:
            rlines.append(f'#include "tools/thor_ir/generated_m15_256/{ip}"')
        rlines.append("namespace oasis::recompiler::ir {")
        rlines.append(f"template <typename Bus> inline bool step_one_exp256_dispatch_ir_p{r_idx}(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {{")
        rlines.append("  switch (pc) {")
        for pc in chunk:
            rlines.append(f"    case 0x{pc:06X}U: return step_ir_0x{pc:06X}(bus, out_pc);")
        rlines.append("    default: return false;")
        rlines.append("  }")
        rlines.append("}")
        rlines.append(f"inline bool is_compiled_exp256_dispatch_pc_p{r_idx}(std::uint32_t pc) noexcept {{")
        rlines.append("  switch (pc) {")
        for pc in chunk:
            rlines.append(f"    case 0x{pc:06X}U:")
        rlines.append("      return true;")
        rlines.append("    default: return false;")
        rlines.append("  }")
        rlines.append("}")
        rlines.append("} // namespace oasis::recompiler::ir")
        (gen_dir / rfname).write_text("\n".join(rlines) + "\n", encoding="utf-8")
        parts_written.append(rfname)
        router_parts.append(rfname)

    m_lines = ["#pragma once"]
    for rp in router_parts:
        m_lines.append(f'#include "tools/thor_ir/generated_m15_256/{rp}"')
    m_lines.append("namespace oasis::recompiler::ir {")
    m_lines.append("template <typename Bus> inline bool step_one_exp256_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {")
    for r_idx in range(len(router_parts)):
        m_lines.append(f"  if (step_one_exp256_dispatch_ir_p{r_idx}(bus, pc, out_pc)) return true;")
    m_lines.append("  return false;")
    m_lines.append("}")
    m_lines.append("inline bool is_compiled_exp256_dispatch_pc(std::uint32_t pc) noexcept {")
    for r_idx in range(len(router_parts)):
        m_lines.append(f"  if (is_compiled_exp256_dispatch_pc_p{r_idx}(pc)) return true;")
    m_lines.append("  return false;")
    m_lines.append("}")
    m_lines.append("} // namespace oasis::recompiler::ir")
    (gen_dir / "exp256_dispatch_router.hpp").write_text("\n".join(m_lines) + "\n", encoding="utf-8")
    parts_written.append("exp256_dispatch_router.hpp")

    blocks = ir_doc["blocks"]
    b_chunks = _split_list(blocks, 15)
    mb_parts: List[str] = []

    for mb_idx, b_chunk in enumerate(b_chunks):
        mbfname = f"exp256_dispatch_mode_b_p{mb_idx}.hpp"
        mblines = [
            "#pragma once",
            hdr,
            '#include "tools/thor_ir/generated_m15_256/exp256_dispatch_router.hpp"',
            "namespace oasis::recompiler::ir {",
        ]
        for blk in b_chunk:
            b_id = blk["block_id"]
            mblines.append(f"template <typename Bus> inline bool run_exp256_block_{b_id}(Bus& bus, std::uint32_t& out_pc) noexcept {{")
            mblines.append("  std::uint32_t next_pc = 0;")
            for ins in blk["instructions"]:
                pc = int(ins["pc"], 16)
                mblines.append(f"  if (!step_ir_0x{pc:06X}(bus, next_pc)) return false;")
            mblines.append("  out_pc = next_pc; return true;")
            mblines.append("}")

        mblines.append(f"template <typename Bus> inline bool run_exp256_mode_b_chunk_{mb_idx}(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {{")
        mblines.append("  switch (pc) {")
        for blk in b_chunk:
            start_pc = blk["start_pc"]
            b_id = blk["block_id"]
            mblines.append(f"    case 0x{start_pc:06X}U: return run_exp256_block_{b_id}(bus, out_pc);")
        mblines.append("    default: return false;")
        mblines.append("  }")
        mblines.append("}")
        mblines.append("} // namespace oasis::recompiler::ir")
        (gen_dir / mbfname).write_text("\n".join(mblines) + "\n", encoding="utf-8")
        parts_written.append(mbfname)
        mb_parts.append(mbfname)

    mb_lines = ["#pragma once"]
    for mbp in mb_parts:
        mb_lines.append(f'#include "tools/thor_ir/generated_m15_256/{mbp}"')
    mb_lines.append("namespace oasis::recompiler::ir {")
    mb_lines.append("template <typename Bus> inline bool run_exp256_mode_b_all(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {")
    for mb_idx in range(len(mb_parts)):
        mb_lines.append(f"  if (run_exp256_mode_b_chunk_{mb_idx}(bus, pc, out_pc)) return true;")
    mb_lines.append("  return false;")
    mb_lines.append("}")
    mb_lines.append("} // namespace oasis::recompiler::ir")
    (gen_dir / "exp256_dispatch_mode_b.hpp").write_text("\n".join(mb_lines) + "\n", encoding="utf-8")
    parts_written.append("exp256_dispatch_mode_b.hpp")

    pr_lines = [
        "#pragma once",
        '#include "tools/thor_ir/generated_m15_256/exp256_dispatch_mode_b.hpp"',
        '#include "tools/thor_ir/generated_m15_256/exp256_dispatch_router.hpp"',
        "namespace oasis::recompiler::ir {",
        "inline bool step_exp256_mb_block(PortableIrBus256& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {",
        "  (void)stop_pc;",
        "  if (run_exp256_mode_b_all(bus, pc, next_pc)) return true;",
        "  return step_one_exp256_dispatch_ir(bus, pc, next_pc);",
        "}",
        "inline PortableExecutionResult finalize_exp256_res(PortableIrBus256& bus, std::uint32_t stop_pc) {",
        "  PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;",
        "  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = bus.div_zero ? 5U : (bus.env.address_error ? 3U : 2U); }",
        "  else if (bus.env.yielded || (bus.env.pc != stop_pc && !is_compiled_exp256_dispatch_pc(bus.env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;",
        "  for (int i = 0; i < 8; ++i) { res.d[i] = bus.env.d_regs[i]; res.a[i] = bus.env.a_regs[i]; }",
        "  res.pc = bus.env.pc; res.ccr = bus.env.read_ccr(); res.cycles = bus.env.total_cycles; res.writes = bus.writes;",
        "  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;",
        "}",
        "inline PortableExecutionResult run_exp256_dispatch_mode_a(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  PortableIrBus256 bus{env}; env.pc = entry_pc & kBusAddressMask; std::uint32_t next_pc = env.pc; int guard = 0;",
        "  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {",
        "    if (!is_compiled_exp256_dispatch_pc(env.pc)) break;",
        "    if (!step_one_exp256_dispatch_ir(bus, env.pc, next_pc)) break;",
        "  }",
        "  return finalize_exp256_res(bus, stop_pc);",
        "}",
        "inline PortableExecutionResult run_exp256_dispatch_mode_b(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  PortableIrBus256 bus{env}; env.pc = entry_pc & kBusAddressMask; std::uint32_t next_pc = env.pc; int guard = 0;",
        "  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {",
        "    if (!is_compiled_exp256_dispatch_pc(env.pc)) break;",
        "    if (!step_exp256_mb_block(bus, env.pc, stop_pc, next_pc)) break;",
        "  }",
        "  return finalize_exp256_res(bus, stop_pc);",
        "}",
        "} // namespace oasis::recompiler::ir",
    ]
    (gen_dir / "exp256_dispatch_portable_routines.hpp").write_text("\n".join(pr_lines) + "\n", encoding="utf-8")
    parts_written.append("exp256_dispatch_portable_routines.hpp")

    return source_map, parts_written
