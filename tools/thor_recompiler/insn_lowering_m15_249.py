#!/usr/bin/env python3
"""
M15.249 Extended Instruction Lowering & Cycle Calibration.
Builds on M15.248 lowering and adds verified support for pc_index addressing mode
d8(PC, Xn) used by authentic enemy controller table lookups.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

from typing import List, Tuple

from tools.thor_recompiler.insn_lowering_m15_245 import (
    ea_cycles,
    emit_ea_addr as m245_ea_addr,
    exact_m68k_cycles as m245_cycles,
    sz_info,
)


def ea_cycles_249(rk: str, sz: str, is_dst: bool = False) -> int:
    if rk in ("disp_ax", "pc_index"):
        return 14 if sz == "l" else 10
    return ea_cycles(rk, sz, is_dst)


def exact_m68k_cycles_249(insn: dict) -> int:
    m = insn["mnemonic"]
    sz = insn["size"]
    ops = [o["raw_kind"] for o in insn["operands"]]
    if m == "MOVE":
        return 4 + ea_cycles_249(ops[0], sz) + ea_cycles_249(ops[1], sz, True)
    return m245_cycles(insn)
from tools.thor_recompiler.insn_lowering_m15_247 import emit_read_op_247
from tools.thor_recompiler.insn_lowering_m15_248 import emit_m15_248_insn_body


def emit_ea_addr_249(op: dict, var: str) -> Tuple[List[str], str]:
    rk = op["raw_kind"]
    if rk == "pc_index":
        ext = op.get("ext", 0)
        xr = (ext >> 12) & 7
        is_a = (ext & 0x8000) != 0
        is_l = (ext & 0x0800) != 0
        xsrc = f"bus.a({xr})" if is_a else f"bus.d({xr})"
        xoff = (f"static_cast<std::int32_t>({xsrc})" if is_l
                else f"static_cast<std::int32_t>(static_cast<std::int16_t>({xsrc} & 0xFFFFU))")
        base_addr = op["val"]
        return [
            f"const std::uint32_t {var} = (0x{base_addr & 0xFFFFFF:06X}U + static_cast<std::uint32_t>({xoff})) & kBusAddressMask;"
        ], var
    return m245_ea_addr(op, var)


def emit_read_op_249(op: dict, b: int, cty: str, ire: str, var: str) -> List[str]:
    rk = op["raw_kind"]
    if rk == "pc_index":
        ea_lines, avar = emit_ea_addr_249(op, f"addr_{var}")
        return ea_lines + [
            f"if (!bus.check_align({avar}, {ire})) return false;",
            f"const {cty} {var} = static_cast<{cty}>(bus.read_m({avar}, {ire}));",
            "if (bus.faulted()) return false;",
        ]
    return emit_read_op_247(op, b, cty, ire, var)


def emit_m15_249_insn_body(insn: dict) -> List[str]:
    pc = int(insn["pc"], 16)
    next_pc = pc + len(insn["raw_words"]) * 2
    w0 = insn["raw_words"][0]
    m = insn["mnemonic"]
    sz = insn["size"]
    b, cty, ire = sz_info(sz)
    cyc = exact_m68k_cycles_249(insn)
    ops = insn["operands"]

    if m == "MOVE" and ops[0]["raw_kind"] == "pc_index":
        src = ops[0]
        dst = ops[1]
        body = [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
        ]
        body += [f"  {l}" for l in emit_read_op_249(src, b, cty, ire, "val")]

        if dst["raw_kind"] == "reg_d":
            dn = dst["reg_index"]
            if sz == "b":
                body.append(f"  bus.set_d({dn}, (bus.d({dn}) & 0xFFFFFF00U) | (val & 0xFFU));")
            elif sz == "w":
                body.append(f"  bus.set_d({dn}, (bus.d({dn}) & 0xFFFF0000U) | (val & 0xFFFFU));")
            else:
                body.append(f"  bus.set_d({dn}, val);")
        elif dst["raw_kind"] == "disp_a":
            ea_lines, dav = emit_ea_addr_249(dst, "da")
            body += [f"  {l}" for l in ea_lines]
            body += [
                f"  if (!bus.check_align({dav}, {ire})) return false;",
                f"  bus.write_m({dav}, {ire}, val);",
                "  if (bus.faulted()) return false;",
            ]
        else:
            ea_lines, dav = emit_ea_addr_249(dst, "da")
            body += [f"  {l}" for l in ea_lines]
            body += [
                f"  if (!bus.check_align({dav}, {ire})) return false;",
                f"  bus.write_m({dav}, {ire}, val);",
                "  if (bus.faulted()) return false;",
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

    return emit_m15_248_insn_body(insn)
