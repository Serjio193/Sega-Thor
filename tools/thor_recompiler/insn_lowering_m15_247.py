#!/usr/bin/env python3
"""
M15.247 Extended Instruction Lowering & Cycle Calibration.
Builds on M15.246 lowering and adds verified MULU.W and post-increment (An)+
addressing mode for M15.247 palette / entity routines.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

from typing import List

from tools.thor_recompiler.insn_lowering_m15_245 import (
    ea_cycles,
    emit_read_op as m245_read_op,
    exact_m68k_cycles as m245_cycles,
    sz_info,
)
from tools.thor_recompiler.insn_lowering_m15_246 import (
    _end_mc,
    emit_m15_246_insn_body,
)


def emit_read_op_247(op: dict, b: int, cty: str, ire: str, var: str) -> List[str]:
    rk = op["raw_kind"]
    if rk == "postinc_a":
        an = op["reg_index"]
        return [
            f"const std::uint32_t addr_{var} = bus.a({an}) & kBusAddressMask;",
            f"if (!bus.check_align(addr_{var}, {ire})) return false;",
            f"const {cty} {var} = static_cast<{cty}>(bus.read_m(addr_{var}, {ire}));",
            f"bus.set_a({an}, (addr_{var} + {b}U) & kBusAddressMask);",
            "if (bus.faulted()) return false;",
        ]
    return m245_read_op(op, b, cty, ire, var)


def emit_m15_247_insn_body(insn: dict) -> List[str]:
    pc = int(insn["pc"], 16)
    next_pc = pc + len(insn["raw_words"]) * 2
    w0 = insn["raw_words"][0]
    m = insn["mnemonic"]
    sz = insn["size"]
    b, cty, ire = sz_info(sz)
    cyc = insn.get("cycles", 4)
    ops = insn["operands"]

    if m == "MULU":
        dn = ops[1]["reg_index"]
        read_lines = emit_read_op_247(ops[0], 2, "std::uint16_t", "IrOpSize::Word", "src")
        # Musashi MULU.W base: 38 + 2*popcount(src). In hybrid delta:
        ea_cyc = ea_cycles(ops[0], "w")
        ea_mc = ea_cyc * 7
        body = [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
        ] + [f"  {l}" for l in read_lines] + [
            f"  const std::uint32_t dst_low = bus.d({dn}) & 0xFFFFU;",
            "  const std::uint32_t res = dst_low * static_cast<std::uint32_t>(src);",
            f"  bus.set_d({dn}, res);",
            "  const std::uint16_t old_ccr = bus.ccr();",
            "  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);",
            "  if ((res & 0x80000000U) != 0U) new_ccr |= 0x08U;",
            "  if (res == 0U) new_ccr |= 0x04U;",
            "  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);",
            "  bus.set_ccr(new_ccr);",
            f"  int mc = 38 * 7 + {ea_mc}; std::uint16_t tmp = src;",
            "  while (tmp != 0U) { if ((tmp & 1U) != 0U) mc += 14; tmp >>= 1U; }",
            f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn_mc(out_pc, kOp, (mc + 28) / 7, mc, false);",
            "}",
        ]
        return body

    if m == "MOVE" and (ops[0]["raw_kind"] == "postinc_a" or ops[1]["raw_kind"] == "postinc_a"):
        body = [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
        ]
        if ops[0]["raw_kind"] == "postinc_a":
            body += [f"  {l}" for l in emit_read_op_247(ops[0], b, cty, ire, "val")]
            dn = ops[1]["reg_index"]
            if sz == "b":
                body.append(f"  bus.set_d({dn}, (bus.d({dn}) & 0xFFFFFF00U) | (val & 0xFFU));")
            elif sz == "w":
                body.append(f"  bus.set_d({dn}, (bus.d({dn}) & 0xFFFF0000U) | (val & 0xFFFFU));")
            else:
                body.append(f"  bus.set_d({dn}, val);")
        elif ops[1]["raw_kind"] == "postinc_a":
            body += [f"  {l}" for l in emit_read_op_247(ops[0], b, cty, ire, "val")]
            an = ops[1]["reg_index"]
            body += [
                f"  const std::uint32_t addr_dst = bus.a({an}) & kBusAddressMask;",
                f"  if (!bus.check_align(addr_dst, {ire})) return false;",
                f"  bus.write_m(addr_dst, {ire}, val);",
                f"  bus.set_a({an}, (addr_dst + {b}U) & kBusAddressMask);",
                "  if (bus.faulted()) return false;",
            ]
        # CCR update for MOVE: N and Z set according to val, V=0, C=0, X unchanged
        body += [
            "  const std::uint16_t old_ccr = bus.ccr();",
            "  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);",
            f"  if ((val & static_cast<{cty}>(1ULL << ({b * 8 - 1}))) != 0U) new_ccr |= 0x08U;",
            "  if (val == 0U) new_ccr |= 0x04U;",
            "  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);",
            "  bus.set_ccr(new_ccr);",
            f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);",
            "}",
        ]
        return body

    if m == "DBRA":
        bt = insn["branch_target"]
        tgt = int(bt, 16) if isinstance(bt, str) else bt
        r = ops[0]["reg_index"]
        return [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
            f"  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d({r}) & 0xFFFFU) - 1U);",
            f"  write_dn(bus, {r}, nw, 2);",
            f"  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x{tgt:06X}U : 0x{next_pc:06X}U;",
            "  return bus.end_insn_mc(out_pc, kOp, taken ? 10 : 14, taken ? 0 : 28, false);",
            "}",
        ]

    return emit_m15_246_insn_body(insn)
