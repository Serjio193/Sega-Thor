#!/usr/bin/env python3
"""
M15.245 Instruction Cycle Calibration and C++20 Body Lowering Helpers.
Cleanly handles all 508 expanded M15.245 entity handler and helper instructions.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from typing import List, Tuple

COND_MAP = {
    "BRA": 0, "BHI": 2, "BLS": 3, "BCC": 4, "BCS": 5, "BNE": 6,
    "BEQ": 7, "BPL": 10, "BMI": 11, "BGE": 12, "BLT": 13, "BGT": 14, "BLE": 15,
}


def sz_info(sz: str) -> Tuple[int, str, str]:
    if sz == "b":
        return 1, "std::uint8_t", "IrOpSize::Byte"
    if sz == "l":
        return 4, "std::uint32_t", "IrOpSize::Long"
    return 2, "std::uint16_t", "IrOpSize::Word"


def ea_cycles(rk: str, sz: str, is_dst: bool = False) -> int:
    is_l = (sz == "l")
    if rk in ("reg_d", "reg_a"):
        return 0
    if rk in ("imm", "indir_a", "postinc_a", "predec_a"):
        return 8 if is_l else 4
    if rk == "disp_a":
        return 12 if is_l else 8
    if rk == "disp_ax":
        return 14 if is_l else 10
    if rk == "abs_l":
        return 16 if is_l else 12
    return 4


def exact_m68k_cycles(insn: dict) -> int:
    m = insn["mnemonic"]
    sz = insn["size"]
    ops = [o["raw_kind"] for o in insn["operands"]]
    if m == "RTS":
        return 16
    if m == "JMP":
        return 12 if "abs_l" in ops else 8
    if m == "MOVEQ":
        return 4
    if m in ("BSR", "JSR"):
        return 18 if m == "BSR" else 20
    if m == "BRA" or m in COND_MAP:
        return 10
    if m == "DBRA":
        return 10
    if m == "LEA":
        return 12 if ("pc_index" in ops or "abs_l" in ops) else 8
    if m == "MOVEA":
        if ops[0] == "abs_l":
            return 20 if sz == "l" else 16
        if ops[0] == "postinc_a":
            return 12 if sz == "l" else 8
        return 12 if ops[0] == "disp_a" else 4
    if m == "MOVE":
        return 4 + ea_cycles(ops[0], sz) + ea_cycles(ops[1], sz, True)
    if m == "CLR":
        if ops[0] == "reg_d":
            return 6 if sz == "l" else 4
        if ops[0] == "abs_l":
            return 28 if sz == "l" else 20
        return (24 if sz == "l" else 16) if ops[0] == "disp_a" else 12
    if m == "TST":
        if ops[0] == "abs_l":
            return 20 if sz == "l" else 16
        return (12 + (4 if sz == "l" else 0)) if ops[0] == "disp_a" else (8 + (4 if sz == "l" else 0) if ops[0] == "disp_a" else 4)
    if m in ("SF", "ST"):
        return 20 if ops[0] == "abs_l" else 16
    if m in ("ADDQ", "SUBQ"):
        if ops[1] in ("reg_d", "reg_a"):
            return 8 if sz == "l" else 4
        return (20 if sz == "l" else 16)
    if m in ("ADDI", "SUBI", "ORI", "ANDI"):
        if ops[1] == "reg_d":
            return 16 if sz == "l" else 8
        return 28 if sz == "l" else (20 if ops[1] == "disp_a" else 24)
    if m == "CMPI":
        if ops[1] == "reg_d":
            return 14 if sz == "l" else 8
        return 16
    if m in ("ADD", "SUB", "AND", "OR", "CMP"):
        base = 6 if (sz == "l" and m == "CMP" and ops[1] == "reg_d") else (8 if (sz == "l" and ops[1] == "reg_d") else 4)
        if ops[1] == "reg_d":
            return base + ea_cycles(ops[0], sz)
        return (12 if sz == "l" else 8) + ea_cycles(ops[1], sz, True)
    if m in ("ASR", "ASL", "LSR", "LSL"):
        cnt = insn["operands"][0]["val"]
        return (8 if sz == "l" else 6) + 2 * cnt
    if m == "BTST":
        is_mem = ops[1] != "reg_d"
        if is_mem:
            return 8 + ea_cycles(ops[1], "b")
        return 10 if ops[0] == "imm" else 6
    if m in ("BCLR", "BSET"):
        if ops[1] == "reg_d":
            return 6 if m == "BSET" else 10
        return (12 if ops[0] == "imm" else 8) + ea_cycles(ops[1], "b")
    return insn["cycles"]


def emit_ea_addr(op: dict, var: str) -> Tuple[List[str], str]:
    rk = op["raw_kind"]
    if rk == "disp_a":
        d = op["val"]
        sign = f"+ static_cast<std::uint32_t>(static_cast<std::int32_t>({d}))"
        return [f"const std::uint32_t {var} = (bus.a({op['reg_index']}) {sign}) & kBusAddressMask;"], var
    if rk == "indir_a":
        return [f"const std::uint32_t {var} = bus.a({op['reg_index']}) & kBusAddressMask;"], var
    if rk == "abs_l":
        return [f"const std::uint32_t {var} = 0x{op['val'] & 0xFFFFFF:06X}U;"], var
    if rk == "disp_ax":
        ext = op.get("ext", 0)
        xr = (ext >> 12) & 7
        is_a = (ext & 0x8000) != 0
        is_l = (ext & 0x0800) != 0
        xsrc = f"bus.a({xr})" if is_a else f"bus.d({xr})"
        xoff = (f"static_cast<std::int32_t>({xsrc})" if is_l
                else f"static_cast<std::int32_t>(static_cast<std::int16_t>({xsrc} & 0xFFFFU))")
        d8 = op["val"]
        return [
            f"const std::uint32_t {var} = (bus.a({op['reg_index']}) + static_cast<std::uint32_t>(static_cast<std::int32_t>({d8})) + static_cast<std::uint32_t>({xoff})) & kBusAddressMask;"
        ], var
    raise NotImplementedError(f"Unsupported EA kind {rk}")


def emit_read_op(op: dict, b: int, cty: str, ire: str, var: str) -> List[str]:
    rk = op["raw_kind"]
    if rk == "imm":
        return [f"const {cty} {var} = static_cast<{cty}>(0x{op['val'] & 0xFFFFFFFF:X}U);"]
    if rk == "reg_d":
        return [f"const {cty} {var} = static_cast<{cty}>(bus.d({op['reg_index']}));"]
    if rk == "reg_a":
        if b == 2:
            return [f"const {cty} {var} = static_cast<{cty}>(bus.a({op['reg_index']}) & 0xFFFFU);"]
        return [f"const {cty} {var} = static_cast<{cty}>(bus.a({op['reg_index']}));"]
    ea_lines, avar = emit_ea_addr(op, f"addr_{var}")
    return ea_lines + [
        f"if (!bus.check_align({avar}, {ire})) return false;",
        f"const {cty} {var} = static_cast<{cty}>(bus.read_m({avar}, {ire}));",
        "if (bus.faulted()) return false;",
    ]


def emit_insn_body(insn: dict) -> List[str]:
    pc = int(insn["pc"], 16)
    next_pc = pc + len(insn["raw_words"]) * 2
    w0 = insn["raw_words"][0]
    m = insn["mnemonic"]
    sz = insn["size"]
    b, cty, ire = sz_info(sz)
    cyc = exact_m68k_cycles(insn)
    ops = insn["operands"]

    lines = [
        f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
        f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;",
    ]

    if m == "MOVEQ":
        uval = ops[0]["val"] & 0xFFFFFFFF
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::uint32_t v = 0x{uval:08X}U;")
        lines.append(f"  bus.set_d({ops[1]['reg_index']}, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "MOVE":
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "v"))
        dst = ops[1]
        if dst["raw_kind"] == "reg_d":
            lines.append(f"  write_dn(bus, {dst['reg_index']}, v, {b});")
        else:
            ea_lines, dav = emit_ea_addr(dst, "da")
            lines.extend("  " + l for l in ea_lines)
            lines.append(f"  if (!bus.check_align({dav}, {ire})) return false; bus.write_m({dav}, {ire}, v); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr(ccr_logic<{cty}>(v, bus.ccr())); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "MOVEA":
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "v"))
        vexpr = "static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(v)))" if b == 2 else "v"
        lines.append(f"  bus.set_a({ops[1]['reg_index']}, {vexpr}); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "CLR":
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        dst = ops[0]
        if dst["raw_kind"] == "reg_d":
            lines.append(f"  write_dn(bus, {dst['reg_index']}, 0U, {b});")
        else:
            ea_lines, dav = emit_ea_addr(dst, "da")
            lines.extend("  " + l for l in ea_lines)
            lines.append(f"  if (!bus.check_align({dav}, {ire})) return false; bus.write_m({dav}, {ire}, 0U); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in ("SF", "ST"):
        val_u = "0xFFU" if m == "ST" else "0U"
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        ea_lines, dav = emit_ea_addr(ops[0], "da")
        lines.extend("  " + l for l in ea_lines)
        lines.append(f"  bus.write_m({dav}, IrOpSize::Byte, {val_u}); if (bus.faulted()) return false;")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "TST":
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "v"))
        lines.append(f"  bus.set_ccr(ccr_logic<{cty}>(v, bus.ccr())); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in ("ADDQ", "SUBQ", "ADDI", "SUBI", "ADD", "SUB"):
        is_sub = m in ("SUBQ", "SUBI", "SUB")
        dst = ops[1]
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        if dst["raw_kind"] == "reg_a":
            imm = ops[0]["val"]
            op_s = "-" if is_sub else "+"
            lines.append(f"  bus.set_a({dst['reg_index']}, bus.a({dst['reg_index']}) {op_s} 0x{imm & 0xFFFFFFFF:X}U);")
            lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
        else:
            lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
            lines.extend("  " + l for l in emit_read_op(dst, b, cty, ire, "d"))
            expr = f"static_cast<{cty}>(d - s)" if is_sub else f"static_cast<{cty}>(d + s)"
            ccr_fn = "ccr_sub" if is_sub else "ccr_add"
            lines.append(f"  const {cty} r = {expr};")
            if dst["raw_kind"] == "reg_d":
                lines.append(f"  write_dn(bus, {dst['reg_index']}, r, {b});")
            else:
                lines.append(f"  bus.write_m(addr_d, {ire}, r); if (bus.faulted()) return false;")
            lines.append(f"  bus.set_ccr({ccr_fn}<{cty}>(d, s, bus.ccr(), true)); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in ("CMP", "CMPI"):
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
        lines.extend("  " + l for l in emit_read_op(ops[1], b, cty, ire, "d"))
        lines.append(f"  bus.set_ccr(ccr_sub<{cty}>(d, s, bus.ccr(), false)); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in ("AND", "ANDI", "OR", "ORI"):
        is_and = m in ("AND", "ANDI")
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
        lines.extend("  " + l for l in emit_read_op(ops[1], b, cty, ire, "d"))
        op_c = "&" if is_and else "|"
        lines.append(f"  const {cty} r = static_cast<{cty}>(d {op_c} s);")
        dst = ops[1]
        if dst["raw_kind"] == "reg_d":
            lines.append(f"  write_dn(bus, {dst['reg_index']}, r, {b});")
        else:
            lines.append(f"  bus.write_m(addr_d, {ire}, r); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr(ccr_logic<{cty}>(r, bus.ccr())); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "ASR":
        cnt = ops[0]["val"]
        sty = "std::int8_t" if b == 1 else ("std::int16_t" if b == 2 else "std::int32_t")
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const {sty} sv = static_cast<{sty}>(static_cast<{cty}>(bus.d({ops[1]['reg_index']})));")
        lines.append(f"  const {sty} res = static_cast<{sty}>(sv >> {cnt});")
        lines.append(f"  const bool carry = ((static_cast<{cty}>(sv) >> ({cnt} - 1)) & 1U) != 0U;")
        lines.append(f"  write_dn(bus, {ops[1]['reg_index']}, static_cast<{cty}>(res), {b});")
        lines.append("  std::uint16_t nc = 0U; if (res == 0) nc |= kFlagZ; if (res < 0) nc |= kFlagN; if (carry) nc |= (kFlagC | kFlagX);")
        adj_sh = 1 if cnt == 1 else 0
        cyc_sh = (cyc - 2) if adj_sh else cyc
        lines.append(f"  bus.set_ccr(nc); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc_sh}, {adj_sh});")
    elif m in ("BTST", "BCLR", "BSET"):
        dst = ops[1]
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], 1, "std::uint8_t", "IrOpSize::Byte", "bit_s"))
        if dst["raw_kind"] == "reg_d":
            lines.append(f"  const std::uint32_t cur = bus.d({dst['reg_index']});")
            lines.append("  const std::uint32_t mask = 1U << (bit_s & 31U); const bool z = (cur & mask) == 0U;")
            if m == "BCLR":
                lines.append(f"  bus.set_d({dst['reg_index']}, cur & ~mask);")
            elif m == "BSET":
                lines.append(f"  bus.set_d({dst['reg_index']}, cur | mask);")
        else:
            lines.extend("  " + l for l in emit_read_op(dst, 1, "std::uint8_t", "IrOpSize::Byte", "cur"))
            lines.append("  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;")
            if m == "BCLR":
                lines.append("  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur & ~mask)); if (bus.faulted()) return false;")
            elif m == "BSET":
                lines.append("  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "LEA":
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        dst_r = ops[1]["reg_index"]
        if ops[0]["raw_kind"] == "pc_index":
            lines.append(f"  const std::int32_t idx = static_cast<std::int16_t>(bus.d({ops[0]['reg_index']}) & 0xFFFFU);")
            lines.append(f"  bus.set_a({dst_r}, (0x{ops[0]['val']:06X}U + static_cast<std::uint32_t>(idx)) & kBusAddressMask);")
        else:
            lines.append(f"  bus.set_a({dst_r}, 0x{ops[0]['val'] & 0xFFFFFF:06X}U);")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in ("BSR", "JSR"):
        bt = insn["branch_target"]
        tgt = int(bt, 16) if isinstance(bt, str) else bt
        lines.append("  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;")
        lines.append("  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }")
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x{next_pc:06X}U); if (bus.faulted()) return false;")
        lines.append(f"  out_pc = 0x{tgt:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "RTS":
        lines.append("  const std::uint32_t sp = bus.a(7) & kBusAddressMask;")
        lines.append("  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }")
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append("  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;")
        lines.append(f"  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "JMP":
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        if ops[0]["raw_kind"] == "abs_l":
            lines.append(f"  out_pc = 0x{ops[0]['val'] & 0xFFFFFF:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
        else:
            lines.append(f"  const std::uint32_t tgt = bus.a({ops[0]['reg_index']}) & kBusAddressMask; if ((tgt & 1U) != 0U) {{ bus.fault_addr(); return false; }}")
            lines.append(f"  out_pc = tgt; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "DBRA":
        bt = insn["branch_target"]
        tgt = int(bt, 16) if isinstance(bt, str) else bt
        r = ops[0]["reg_index"]
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d({r}) & 0xFFFFU) - 1U);")
        lines.append(f"  write_dn(bus, {r}, nw, 2);")
        lines.append(f"  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x{tgt:06X}U : 0x{next_pc:06X}U;")
        lines.append("  return bus.end_insn(out_pc, kOp, 12, taken ? -1 : 1);")
    elif m in COND_MAP:
        bt = insn["branch_target"]
        tgt = int(bt, 16) if isinstance(bt, str) else bt
        ccode = COND_MAP[m]
        adj_nt = -1 if (m != "BRA" and sz == "b") else (1 if m != "BRA" else 0)
        lines.append("  if (!bus.begin_insn(kPc, kOp)) return false;")
        if m == "BRA":
            lines.append(f"  out_pc = 0x{tgt:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
        else:
            lines.append(f"  const bool taken = eval_cond(bus.ccr(), {ccode});")
            lines.append(f"  out_pc = taken ? 0x{tgt:06X}U : 0x{next_pc:06X}U;")
            lines.append(f"  return bus.end_insn(out_pc, kOp, {cyc}, taken ? 0 : {adj_nt});")
    else:
        raise NotImplementedError(f"Unsupported mnemonic {m} at 0x{pc:06X}")

    lines.append("}")
    return lines
