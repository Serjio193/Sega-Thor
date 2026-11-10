#!/usr/bin/env python3
"""
M15.246 Extended Instruction Lowering & Cycle Calibration.
Reuses M15.245 lowering primitives for shared M68000 instructions and adds exact
lowering + Musashi cycle synchronization for M15.246 high-frequency callees.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

from typing import List

from tools.thor_recompiler.insn_lowering_m15_245 import (
    COND_MAP,
    ea_cycles,
    emit_ea_addr,
    emit_insn_body as emit_m245_insn_body,
    emit_read_op,
    exact_m68k_cycles as m245_cycles,
    sz_info,
)

SHARED_PASSTHROUGH = {
    "MOVEQ", "MOVE", "MOVEA", "CLR", "TST",
    "ADDQ", "SUBQ", "ADDI", "SUBI", "ADD", "SUB",
    "CMP", "CMPI", "AND", "ANDI", "OR", "ORI",
    "BTST", "BCLR", "BSET", "BSR", "JSR", "RTS", "JMP", "DBRA", "LEA",
} | set(COND_MAP.keys())


def _end_mc(next_pc: int, m68k_cyc: int, delta_mc: str, skip_ref: str) -> str:
    return (
        f"  out_pc = 0x{next_pc:06X}U; "
        f"return bus.end_insn_mc(out_pc, kOp, {m68k_cyc}, {delta_mc}, {skip_ref});"
    )


def _emit_movem(pc: int, next_pc: int, w0: int, sz: str, ops: list) -> List[str]:
    is_l = sz == "l"
    b = 4 if is_l else 2
    ire = "IrOpSize::Long" if is_l else "IrOpSize::Word"
    step_cyc = 8 if is_l else 4
    lines = [
        f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
        f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
    ]
    if ops[0]["raw_kind"] == "imm":
        # Regs -> -(An)
        mask = ops[0]["val"] & 0xFFFF
        an = ops[1]["reg_index"]
        nregs = bin(mask).count("1")
        cyc = 8 + step_cyc * nregs
        delta_mc = step_cyc * nregs * 7
        lines.append(f"  std::uint32_t sp = bus.a({an}) & kBusAddressMask;")
        lines.append(f"  if ((sp & 1U) != 0U) {{ bus.fault_addr(); return false; }}")
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        # bit 0..7 = A7..A0, bit 8..15 = D7..D0
        for i in range(16):
            if mask & (1 << i):
                val_expr = f"bus.a({7 - i})" if i < 8 else f"bus.d({15 - i})"
                lines.append(f"  sp = (sp - {b}U) & kBusAddressMask; bus.write_m(sp, {ire}, {val_expr}); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_a({an}, sp);")
        lines.append(_end_mc(next_pc, cyc, str(delta_mc), "true" if is_l else "false"))
    else:
        # (An)+ -> Regs
        an = ops[0]["reg_index"]
        mask = ops[1]["val"] & 0xFFFF
        nregs = bin(mask).count("1")
        cyc = 12 + step_cyc * nregs
        delta_mc = step_cyc * nregs * 7
        lines.append(f"  std::uint32_t sp = bus.a({an}) & kBusAddressMask;")
        lines.append(f"  if ((sp & 1U) != 0U) {{ bus.fault_addr(); return false; }}")
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        # bit 0..7 = D0..D7, bit 8..15 = A0..A7
        for i in range(16):
            if mask & (1 << i):
                raw_expr = f"bus.read_m(sp, {ire})"
                val_expr = raw_expr if is_l else f"static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>({raw_expr} & 0xFFFFU)))"
                lines.append(f"  const std::uint32_t mv_{i} = {val_expr}; if (bus.faulted()) return false; sp = (sp + {b}U) & kBusAddressMask;")
        for i in range(16):
            if mask & (1 << i):
                setter = f"bus.set_d({i}, mv_{i});" if i < 8 else f"bus.set_a({i - 8}, mv_{i});"
                lines.append(f"  {setter}")
        lines.append(f"  bus.set_a({an}, sp);")
        lines.append(_end_mc(next_pc, cyc, str(delta_mc), "true" if is_l else "false"))
    lines.append("}")
    return lines


def _emit_div_or_mul(pc: int, next_pc: int, w0: int, m: str, ops: list) -> List[str]:
    src = ops[0]
    dn = ops[1]["reg_index"]
    lines = [
        f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
        f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
    ]
    lines.extend("  " + l for l in emit_read_op(src, 2, "std::uint16_t", "IrOpSize::Word", "s16"))
    if m == "MULS":
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::int16_t s = static_cast<std::int16_t>(s16);")
        lines.append(f"  const std::int16_t d = static_cast<std::int16_t>(bus.d({dn}) & 0xFFFFU);")
        lines.append(f"  const std::int32_t prod = static_cast<std::int32_t>(s) * static_cast<std::int32_t>(d);")
        lines.append(f"  const std::uint32_t uprod = static_cast<std::uint32_t>(prod);")
        lines.append(f"  bus.set_d({dn}, uprod); bus.set_ccr(ccr_logic<std::uint32_t>(uprod, bus.ccr()));")
        lines.append("  const int mc = m15246_calc_muls_cycles(s);")
        ea_base = 28 if src["raw_kind"] == "imm" else 0
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn_mc(out_pc, kOp, (mc + {ea_base}) / 7, mc, false);")
    elif m == "DIVU":
        lines.append("  if (s16 == 0U) { bus.fault_div0(); return false; }")
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::uint32_t dvd = bus.d({dn}); const std::uint32_t q32 = dvd / s16;")
        lines.append("  const int mc = m15246_calc_divu_cycles(dvd, s16);")
        lines.append("  if (q32 > 0xFFFFU) { bus.set_ccr((bus.ccr() & kFlagX) | kFlagN | kFlagV); }")
        lines.append(f"  else {{ const std::uint16_t q = static_cast<std::uint16_t>(q32); const std::uint16_t r = static_cast<std::uint16_t>(dvd % s16);")
        lines.append(f"    bus.set_d({dn}, (static_cast<std::uint32_t>(r) << 16U) | q); bus.set_ccr(ccr_logic<std::uint16_t>(q, bus.ccr())); }}")
        ea_base = 28 if src["raw_kind"] == "imm" else 0
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn_mc(out_pc, kOp, (mc + {ea_base}) / 7, mc, true);")
    else:  # DIVS
        lines.append("  const std::int16_t s = static_cast<std::int16_t>(s16);")
        lines.append("  if (s == 0) { bus.fault_div0(); return false; }")
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::int32_t dvd = static_cast<std::int32_t>(bus.d({dn}));")
        lines.append("  const std::int64_t q64 = static_cast<std::int64_t>(dvd) / static_cast<std::int64_t>(s);")
        lines.append("  const int mc = m15246_calc_divs_cycles(dvd, s);")
        lines.append("  if (q64 < -32768 || q64 > 32767) { bus.set_ccr((bus.ccr() & kFlagX) | kFlagN | kFlagV); }")
        lines.append(f"  else {{ const std::uint16_t q = static_cast<std::uint16_t>(static_cast<std::int16_t>(q64));")
        lines.append(f"    const std::uint16_t r = static_cast<std::uint16_t>(static_cast<std::int16_t>(dvd % s));")
        lines.append(f"    bus.set_d({dn}, (static_cast<std::uint32_t>(r) << 16U) | q); bus.set_ccr(ccr_logic<std::uint16_t>(q, bus.ccr())); }}")
        ea_base = 28 if src["raw_kind"] == "imm" else 0
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn_mc(out_pc, kOp, (mc + {ea_base}) / 7, mc, true);")
    lines.append("}")
    return lines


def emit_m15_246_insn_body(insn: dict) -> List[str]:
    m = insn["mnemonic"]
    sz = insn["size"]
    ops = insn["operands"]
    pc = int(insn["pc"], 16)
    next_pc = pc + len(insn["raw_words"]) * 2
    w0 = insn["raw_words"][0]
    b, cty, ire = sz_info(sz)

    # Delegate unchanged common instructions, except special cases M15.245 skipped:
    if m == "SF" or m == "ST":
        if ops[0]["raw_kind"] == "reg_d":
            val_u = "0xFFU" if m == "ST" else "0U"
            cyc = 6 if m == "ST" else 4
            return [
                f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
                f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
                "  if (!bus.begin_insn(kPc, kOp)) return false;",
                f"  write_dn(bus, {ops[0]['reg_index']}, {val_u}, 1);",
                f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);",
                "}",
            ]
        return emit_m245_insn_body(insn)

    if m == "LEA" and ops[0]["raw_kind"] in ("disp_a", "disp_ax"):
        cyc = 12 if ops[0]["raw_kind"] == "disp_ax" else 8
        ea_lines, avar = emit_ea_addr(ops[0], "lea_a")
        return (
            [
                f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
                f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
                "  if (!bus.begin_insn(kPc, kOp)) return false;",
            ]
            + ["  " + l for l in ea_lines]
            + [
                f"  bus.set_a({ops[1]['reg_index']}, {avar}); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);",
                "}",
            ]
        )

    if m in ("BSR", "JSR") and ops and ops[0]["raw_kind"] == "indir_a":
        an = ops[0]["reg_index"]
        return [
            f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
            f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
            f"  const std::uint32_t tgt = bus.a({an}) & kBusAddressMask;",
            "  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;",
            "  if (((new_sp | tgt) & 1U) != 0U) { bus.fault_addr(); return false; }",
            "  if (!bus.begin_insn(kPc, kOp)) return false;",
            f"  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x{next_pc:06X}U); if (bus.faulted()) return false;",
            f"  out_pc = tgt; return bus.end_insn(out_pc, kOp, 16, 0);",
            "}",
        ]

    if m in ("ADD", "SUB", "AND", "OR") and sz == "l" and ops[1]["raw_kind"] == "reg_d" and ops[0]["raw_kind"] not in ("reg_d", "reg_a", "imm"):
        cyc_fix = 6 + ea_cycles(ops[0]["raw_kind"], "l")
        body = emit_m245_insn_body(insn)
        old_tail = body[-2]
        return body[:-2] + [old_tail.replace(f", {m245_cycles(insn)}, 0);", f", {cyc_fix}, 0);"), body[-1]]

    if m in SHARED_PASSTHROUGH:
        return emit_m245_insn_body(insn)

    if m == "MOVEM":
        return _emit_movem(pc, next_pc, w0, sz, ops)

    if m in ("DIVU", "DIVS", "MULS"):
        return _emit_div_or_mul(pc, next_pc, w0, m, ops)

    lines = [
        f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
        f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
    ]
    if m == "ADDA":
        cyc = 8 + ea_cycles(ops[0]["raw_kind"], sz)
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
        sext = "static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(s)))" if b == 2 else "s"
        ar = ops[1]["reg_index"]
        lines.append(f"  bus.set_a({ar}, bus.a({ar}) + {sext}); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "CMPA":
        cyc = 6 + ea_cycles(ops[0]["raw_kind"], sz)
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
        sext = "static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(s)))" if b == 2 else "s"
        ar = ops[1]["reg_index"]
        lines.append(f"  bus.set_ccr(ccr_sub<std::uint32_t>(bus.a({ar}), {sext}, bus.ccr(), false));")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "EXT":
        r = ops[0]["reg_index"]
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        if b == 2:
            lines.append(f"  const std::uint16_t v = static_cast<std::uint16_t>(static_cast<std::int16_t>(static_cast<std::int8_t>(bus.d({r}) & 0xFFU)));")
            lines.append(f"  write_dn(bus, {r}, v, 2); bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr()));")
        else:
            lines.append(f"  const std::uint32_t v = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.d({r}) & 0xFFFFU)));")
            lines.append(f"  bus.set_d({r}, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, 4, 0);")
    elif m == "EXG":
        rx, ry = ops[0]["reg_index"], ops[1]["reg_index"]
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::uint32_t vx = bus.d({rx}); bus.set_d({rx}, bus.d({ry})); bus.set_d({ry}, vx);")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, 6, 0);")
    elif m == "NEG":
        r = ops[0]["reg_index"]
        cyc = 6 if b == 4 else 4
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const {cty} s = static_cast<{cty}>(bus.d({r})); const {cty} v = static_cast<{cty}>(0U - s);")
        lines.append(f"  write_dn(bus, {r}, v, {b}); bus.set_ccr(ccr_sub<{cty}>(0U, s, bus.ccr(), true));")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "SWAP":
        r = ops[0]["reg_index"]
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::uint32_t o = bus.d({r}); const std::uint32_t v = (o >> 16U) | (o << 16U);")
        lines.append(f"  bus.set_d({r}, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, 4, 0);")
    elif m in ("ASL", "LSL", "LSR"):
        cnt = ops[0]["val"]
        r = ops[1]["reg_index"]
        cyc = (8 if b == 4 else 6) + 2 * cnt
        kind_tag = {"ASL": 0, "LSL": 1, "LSR": 2}[m]
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  m15246_shift_imm<{cty}, {b}, {kind_tag}>(bus, {r}, {cnt});")
        lines.append(_end_mc(next_pc, cyc, str(cnt * 14), "false"))
    else:
        raise NotImplementedError(f"Unsupported M15.246 mnemonic {m} at 0x{pc:06X}")
    lines.append("}")
    return lines
