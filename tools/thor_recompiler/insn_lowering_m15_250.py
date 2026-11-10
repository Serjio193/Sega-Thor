#!/usr/bin/env python3
"""
M15.250 Instruction Lowering & Cycle Calibration.
Builds on M15.249 instruction lowering with verified cycle accuracy.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

from typing import List
from tools.thor_recompiler.insn_lowering_m15_245 import (
    ea_cycles,
    emit_read_op,
    sz_info,
)
from tools.thor_recompiler.insn_lowering_m15_249 import (
    ea_cycles_249,
    emit_ea_addr_249,
    emit_m15_249_insn_body,
    emit_read_op_249,
    exact_m68k_cycles_249,
)


def exact_m68k_cycles_250(insn: dict) -> int:
    m = insn["mnemonic"]
    sz = insn["size"]
    ops = [o["raw_kind"] for o in insn["operands"]]
    if m in ("ADDQ", "SUBQ"):
        if ops[1] == "abs_l":
            return 24 if sz == "l" else 20
        if ops[1] == "disp_a":
            return 20 if sz == "l" else 16
    if m in ("BSET", "BCLR"):
        if ops[1] == "reg_d":
            return (12 if m == "BSET" else 14) if ops[0] == "imm" else (8 if m == "BSET" else 10)
    return exact_m68k_cycles_249(insn)


def emit_m15_250_insn_body(insn: dict) -> List[str]:
    m = insn["mnemonic"]
    sz = insn["size"]
    ops = insn["operands"]
    if m in ("ADDQ", "SUBQ") and ops[1]["raw_kind"] in ("abs_l", "disp_a"):
        pc = int(insn["pc"], 16)
        next_pc = pc + len(insn["raw_words"]) * 2
        w0 = insn["raw_words"][0]
        b, cty, ire = sz_info(sz)
        cyc = exact_m68k_cycles_250(insn)
        is_sub = (m == "SUBQ")
        lines = [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
        ]
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
        lines.extend("  " + l for l in emit_read_op(ops[1], b, cty, ire, "d"))
        expr = f"static_cast<{cty}>(d - s)" if is_sub else f"static_cast<{cty}>(d + s)"
        ccr_fn = "ccr_sub" if is_sub else "ccr_add"
        lines.append(f"  const {cty} r = {expr};")
        lines.append(f"  bus.write_m(addr_d, {ire}, r); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr({ccr_fn}<{cty}>(d, s, bus.ccr(), true)); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
        lines.append("}")
        return lines

    if m in ("BSET", "BCLR") and ops[1]["raw_kind"] == "reg_d":
        pc = int(insn["pc"], 16)
        next_pc = pc + len(insn["raw_words"]) * 2
        w0 = insn["raw_words"][0]
        cyc = exact_m68k_cycles_250(insn)
        dn = ops[1]["reg_index"]
        lines = [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
        ]
        if ops[0]["raw_kind"] == "imm":
            imm = ops[0]["val"] & 31
            lines.append(f"  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x{imm:02X}U);")
        else:
            lines.append(f"  const std::uint8_t bit_s = static_cast<std::uint8_t>(bus.d({ops[0]['reg_index']}) & 31U);")
        lines.append(f"  const std::uint32_t cur = bus.d({dn});")
        lines.append("  const std::uint32_t mask = 1U << (bit_s & 31U); const bool z = (cur & mask) == 0U;")
        if m == "BSET":
            lines.append(f"  bus.set_d({dn}, cur | mask);")
        else:
            lines.append(f"  bus.set_d({dn}, cur & ~mask);")
        if m == "BSET" and ops[0]["raw_kind"] == "reg_d":
            lines.append("  const int mc = ((bit_s & 31U) >= 16U) ? 14 : 0;")
            lines.append(f"  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x{next_pc:06X}U; return bus.end_insn_mc(out_pc, kOp, 6 + (mc / 7), mc, false);")
        else:
            lines.append(f"  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
        lines.append("}")
        return lines

    if m == "DBRA":
        pc = int(insn["pc"], 16)
        next_pc = pc + len(insn["raw_words"]) * 2
        w0 = insn["raw_words"][0]
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
            "  return bus.end_insn_mc(out_pc, kOp, taken ? 10 : 14, taken ? -14 : 14, false);",
            "}",
        ]

    return emit_m15_249_insn_body(insn)
