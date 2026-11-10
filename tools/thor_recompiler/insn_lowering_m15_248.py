#!/usr/bin/env python3
"""
M15.248 Extended Instruction Lowering & Cycle Calibration.
Builds on M15.247 lowering and adds support for MOVEA with (An)+ post-increment
mode across authentic unowned hotpath callees.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

from typing import List

from tools.thor_recompiler.insn_lowering_m15_245 import (
    ea_cycles,
    emit_ea_addr,
    exact_m68k_cycles as m245_cycles,
    sz_info,
)
from tools.thor_recompiler.insn_lowering_m15_247 import (
    emit_m15_247_insn_body,
    emit_read_op_247,
)


def emit_m15_248_insn_body(insn: dict) -> List[str]:
    pc = int(insn["pc"], 16)
    next_pc = pc + len(insn["raw_words"]) * 2
    w0 = insn["raw_words"][0]
    m = insn["mnemonic"]
    sz = insn["size"]
    b, cty, ire = sz_info(sz)
    cyc = insn.get("cycles", 4)
    ops = insn["operands"]

    # General MOVE with post-increment source or destination
    if m == "MOVE" and (ops[0]["raw_kind"] == "postinc_a" or ops[1]["raw_kind"] == "postinc_a"):
        src = ops[0]
        dst = ops[1]
        body = [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
        ]
        body += [f"  {l}" for l in emit_read_op_247(src, b, cty, ire, "val")]

        if dst["raw_kind"] == "reg_d":
            dn = dst["reg_index"]
            if sz == "b":
                body.append(f"  bus.set_d({dn}, (bus.d({dn}) & 0xFFFFFF00U) | (val & 0xFFU));")
            elif sz == "w":
                body.append(f"  bus.set_d({dn}, (bus.d({dn}) & 0xFFFF0000U) | (val & 0xFFFFU));")
            else:
                body.append(f"  bus.set_d({dn}, val);")
        elif dst["raw_kind"] == "postinc_a":
            an = dst["reg_index"]
            body += [
                f"  const std::uint32_t addr_dst = bus.a({an}) & kBusAddressMask;",
                f"  if (!bus.check_align(addr_dst, {ire})) return false;",
                f"  bus.write_m(addr_dst, {ire}, val);",
                f"  bus.set_a({an}, (addr_dst + {b}U) & kBusAddressMask);",
                "  if (bus.faulted()) return false;",
            ]
        elif dst["raw_kind"] == "indir_a":
            an = dst["reg_index"]
            body += [
                f"  const std::uint32_t addr_dst = bus.a({an}) & kBusAddressMask;",
                f"  if (!bus.check_align(addr_dst, {ire})) return false;",
                f"  bus.write_m(addr_dst, {ire}, val);",
                "  if (bus.faulted()) return false;",
            ]
        else:
            ea_lines, dav = emit_ea_addr(dst, "da")
            body += [f"  {l}" for l in ea_lines]
            body += [
                f"  if (!bus.check_align({dav}, {ire})) return false; bus.write_m({dav}, {ire}, val); if (bus.faulted()) return false;",
            ]

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

    # MOVEA with post-increment source: MOVEA.W/L (An)+, Am
    if m == "MOVEA" and ops[0]["raw_kind"] == "postinc_a":
        dst_a = ops[1]["reg_index"]
        body = [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
        ] + [f"  {l}" for l in emit_read_op_247(ops[0], b, cty, ire, "v")]
        vexpr = "static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(v)))" if b == 2 else "v"
        body += [
            f"  bus.set_a({dst_a}, {vexpr});",
            f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);",
            "}",
        ]
        return body

    return emit_m15_247_insn_body(insn)
