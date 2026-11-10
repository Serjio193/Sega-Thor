#!/usr/bin/env python3
"""
Thor IR to Auto-Split Portable C++20 Generator (M15.244 Phase 2 & Phase 10).
Generates Mode A instruction-stepper and Mode B basic-block C++20 directly from decoded Thor IR JSON,
auto-splitting generated headers to <= 350 lines and recording exact 1:1 ROM_PC -> C++ source mappings.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from pathlib import Path
from typing import Dict, List, Tuple


COMMON_HPP = """#pragma once
#include "tools/thor_ir/portable_guest_env.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace oasis::recompiler::ir {

constexpr std::uint32_t kJumpTableBase = 0x0059BAU;

template <typename T>
inline std::uint16_t ccr_logic(T val, std::uint16_t cur) noexcept {
    const std::uint64_t msb = 1ULL << (sizeof(T) * 8U - 1U);
    std::uint16_t r = cur & kFlagX;
    if (val == 0) r |= kFlagZ;
    if (static_cast<std::uint64_t>(val) & msb) r |= kFlagN;
    return r;
}

template <typename T>
inline std::uint16_t ccr_add(T d, T s, std::uint16_t cur, bool upd_x) noexcept {
    const std::uint64_t mask = (sizeof(T) == 1) ? 0xFFULL : ((sizeof(T) == 2) ? 0xFFFFULL : 0xFFFFFFFFULL);
    const std::uint64_t msb = 1ULL << (sizeof(T) * 8U - 1U);
    const std::uint64_t sum = static_cast<std::uint64_t>(d) + static_cast<std::uint64_t>(s);
    const std::uint64_t res = sum & mask;
    std::uint16_t r = upd_x ? 0U : (cur & kFlagX);
    if (res == 0) r |= kFlagZ;
    if (res & msb) r |= kFlagN;
    if (sum > mask) { r |= kFlagC; if (upd_x) r |= kFlagX; }
    if (((static_cast<std::uint64_t>(d) ^ res) & (static_cast<std::uint64_t>(s) ^ res) & msb) != 0ULL) r |= kFlagV;
    return r;
}

template <typename T>
inline std::uint16_t ccr_sub(T d, T s, std::uint16_t cur, bool upd_x) noexcept {
    const std::uint64_t mask = (sizeof(T) == 1) ? 0xFFULL : ((sizeof(T) == 2) ? 0xFFFFULL : 0xFFFFFFFFULL);
    const std::uint64_t msb = 1ULL << (sizeof(T) * 8U - 1U);
    const std::uint64_t diff = static_cast<std::uint64_t>(d) - static_cast<std::uint64_t>(s);
    const std::uint64_t res = diff & mask;
    std::uint16_t r = upd_x ? 0U : (cur & kFlagX);
    if (res == 0) r |= kFlagZ;
    if (res & msb) r |= kFlagN;
    if (static_cast<std::uint64_t>(s) > static_cast<std::uint64_t>(d)) { r |= kFlagC; if (upd_x) r |= kFlagX; }
    if (((static_cast<std::uint64_t>(d) ^ static_cast<std::uint64_t>(s)) & (static_cast<std::uint64_t>(d) ^ res) & msb) != 0ULL) r |= kFlagV;
    return r;
}

inline bool eval_cond(std::uint16_t ccr, int cond) noexcept {
    const bool c = (ccr & kFlagC) != 0, v = (ccr & kFlagV) != 0;
    const bool z = (ccr & kFlagZ) != 0, n = (ccr & kFlagN) != 0;
    switch (cond) {
    case 0: return true; case 2: return !c && !z; case 3: return c || z;
    case 4: return !c; case 5: return c; case 6: return !z; case 7: return z;
    case 10: return !n; case 11: return n; case 12: return n == v;
    case 13: return n != v; case 14: return !z && (n == v); case 15: return z || (n != v);
    default: return false;
    }
}

struct PortableIrBus {
    PortableGuestEnvironment& env;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    std::uint32_t d(int i) const noexcept { return env.d_regs[i]; }
    void set_d(int i, std::uint32_t v) noexcept { env.d_regs[i] = v; }
    std::uint32_t a(int i) const noexcept { return env.a_regs[i]; }
    void set_a(int i, std::uint32_t v) noexcept {
        if (i == 7) env.a_regs[7] = v & kBusAddressMask; else env.a_regs[i] = v;
    }
    std::uint16_t ccr() const noexcept { return env.read_ccr(); }
    void set_ccr(std::uint16_t c) noexcept { env.write_ccr(c); }
    bool check_align(std::uint32_t addr, IrOpSize sz) noexcept {
        if (sz != IrOpSize::Byte && (addr & 1U) != 0U) { env.address_error = true; return false; }
        return true;
    }
    std::uint32_t read_m(std::uint32_t addr, IrOpSize sz) noexcept { return env.read_memory(addr & kBusAddressMask, sz); }
    void write_m(std::uint32_t addr, IrOpSize sz, std::uint32_t val) noexcept {
        const auto m = addr & kBusAddressMask;
        env.write_memory(m, sz, val);
        if (env.address_error || env.bus_error) return;
        if (sz == IrOpSize::Byte) writes.emplace_back(m, val & 0xFF);
        else if (sz == IrOpSize::Word) { writes.emplace_back(m, (val >> 8) & 0xFF); writes.emplace_back(m + 1, val & 0xFF); }
        else for (int k = 0; k < 4; ++k) writes.emplace_back(m + k, (val >> (24 - k * 8)) & 0xFF);
    }
    bool faulted() const noexcept { return env.address_error || env.bus_error; }
    void fault_addr() noexcept { env.address_error = true; }
    bool begin_insn(std::uint32_t, std::uint16_t) noexcept { return true; }
    bool end_insn(std::uint32_t next_pc, std::uint16_t, int m68k_cyc, int branch_adj_sign) noexcept {
        env.pc = next_pc & kBusAddressMask;
        env.add_cycles(m68k_cyc + branch_adj_sign * 2);
        return true;
    }
};

template <typename Bus>
inline void write_dn(Bus& bus, int r, std::uint32_t val, int bytes) noexcept {
    if (bytes == 1) bus.set_d(r, (bus.d(r) & 0xFFFFFF00U) | (val & 0xFFU));
    else if (bytes == 2) bus.set_d(r, (bus.d(r) & 0xFFFF0000U) | (val & 0xFFFFU));
    else bus.set_d(r, val);
}

inline constexpr IrOpSize sz_enum(int b) noexcept {
    return b == 1 ? IrOpSize::Byte : (b == 2 ? IrOpSize::Word : IrOpSize::Long);
}
} // namespace oasis::recompiler::ir
"""


def exact_m68k_cycles(insn: dict) -> int:
    m = insn["mnemonic"]
    sz = insn["size"]
    ops = [o["raw_kind"] for o in insn["operands"]]
    if m == "RTS":
        return 16
    if m == "JMP":
        return 8
    if m == "MOVEQ":
        return 4
    if m in ("BSR", "JSR"):
        return 18 if m == "BSR" else 20
    if m == "BRA" or m in ("BNE", "BEQ", "BCC", "BCS", "BPL", "BMI", "BGT", "BLE", "BLT", "BGE", "BHI", "BLS"):
        return 10
    if m == "LEA":
        return 12 if "pc_index" in ops else 8
    if m == "ADDA":
        return 12 if ops[0] == "indir_a" else 8
    if m == "MOVEA":
        return 20 if ops[0] == "abs_l" and sz == "l" else 12
    if m == "MOVE":
        ea_in = {"reg_d": 0, "imm": 4 if sz != "l" else 8, "disp_a": 8 if sz != "l" else 12, "abs_l": 12 if sz != "l" else 16}
        ea_out = {"reg_d": 0, "disp_a": 8 if sz != "l" else 12, "abs_l": 12 if sz != "l" else 16}
        return 4 + ea_in.get(ops[0], 4) + ea_out.get(ops[1], 4)
    if m == "CLR":
        if ops[0] == "disp_a": return 24 if sz == "l" else 16
        if ops[0] == "abs_l": return 24 if sz == "l" else 16
        return 6 if sz == "l" else 4
    if m == "TST":
        if ops[0] == "abs_l": return 16
        return 8 if ops[0] == "disp_a" else 4
    if m == "SF":
        return 20 if ops[0] == "abs_l" else 16
    if m in ("ADDQ", "SUBQ"):
        return 4 if ops[1] == "reg_d" else (20 if sz == "l" else 16)
    if m in ("ADDI", "SUBI", "ORI"):
        if ops[1] == "reg_d": return 16 if sz == "l" else 8
        return 28 if sz == "l" else 20
    if m == "CMPI":
        if ops[1] == "reg_d": return 14 if sz == "l" else 8
        return 16
    if m == "ADD":
        if ops[0] == "reg_d" and ops[1] == "disp_a": return 24 if sz == "l" else 16
        return 12
    if m in ("BCLR", "BSET"):
        return 20 if ops[1] == "disp_a" else 12
    if m == "BTST":
        return 20 if (ops[0] == "imm" and ops[1] == "abs_l") else 16
    return insn["cycles"]


def sz_info(sz: str) -> Tuple[int, str, str, str]:
    if sz == "b": return 1, "std::uint8_t", "IrOpSize::Byte", "0xFFU"
    if sz == "l": return 4, "std::uint32_t", "IrOpSize::Long", "0xFFFFFFFFU"
    return 2, "std::uint16_t", "IrOpSize::Word", "0xFFFFU"


def emit_read_op(op: dict, b: int, cty: str, ire: str, var: str) -> List[str]:
    rk = op["raw_kind"]
    if rk == "imm":
        return [f"const {cty} {var} = static_cast<{cty}>(0x{op['val'] & 0xFFFFFFFF:X}U);"]
    if rk == "reg_d":
        return [f"const {cty} {var} = static_cast<{cty}>(bus.d({op['reg_index']}));"]
    if rk == "disp_a":
        d = op["val"]
        sign = f"+ 0x{d:X}U" if d >= 0 else f"- 0x{-d:X}U"
        return [
            f"const std::uint32_t addr_{var} = (bus.a({op['reg_index']}) {sign}) & kBusAddressMask;",
            f"if (!bus.check_align(addr_{var}, {ire})) return false;",
            f"const {cty} {var} = static_cast<{cty}>(bus.read_m(addr_{var}, {ire}));",
            f"if (bus.faulted()) return false;",
        ]
    if rk == "abs_l":
        return [
            f"const std::uint32_t addr_{var} = 0x{op['val'] & 0xFFFFFF:06X}U;",
            f"if (!bus.check_align(addr_{var}, {ire})) return false;",
            f"const {cty} {var} = static_cast<{cty}>(bus.read_m(addr_{var}, {ire}));",
            f"if (bus.faulted()) return false;",
        ]
    raise NotImplementedError(f"Unsupported read op {rk}")


def emit_insn_body(insn: dict) -> List[str]:
    pc = int(insn["pc"], 16)
    next_pc = pc + len(insn["raw_words"]) * 2
    w0 = insn["raw_words"][0]
    m = insn["mnemonic"]
    sz = insn["size"]
    b, cty, ire, mask = sz_info(sz)
    cyc = exact_m68k_cycles(insn)
    ops = insn["operands"]

    lines = [
        f"template <typename Bus> inline bool step_ir_0x{pc:06X}(Bus& bus, std::uint32_t& out_pc) noexcept {{",
        f"  constexpr std::uint16_t kOp = 0x{w0:04X}U; constexpr std::uint32_t kPc = 0x{pc:06X}U;"
    ]
    cond_map = {"BRA": 0, "BHI": 2, "BLS": 3, "BCC": 4, "BCS": 5, "BNE": 6, "BEQ": 7, "BPL": 10, "BMI": 11, "BGE": 12, "BLT": 13, "BGT": 14, "BLE": 15}

    if m == "MOVEQ":
        imm = ops[0]["val"]
        uval = imm & 0xFFFFFFFF
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::uint32_t v = 0x{uval:08X}U;")
        lines.append(f"  bus.set_d({ops[1]['reg_index']}, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "MOVE":
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "v"))
        dst = ops[1]
        if dst["raw_kind"] == "reg_d":
            lines.append(f"  write_dn(bus, {dst['reg_index']}, v, {b});")
        elif dst["raw_kind"] == "disp_a":
            d = dst["val"]
            lines.append(f"  const std::uint32_t da = (bus.a({dst['reg_index']}) + static_cast<std::uint32_t>(static_cast<std::int32_t>({d}))) & kBusAddressMask;")
            lines.append(f"  if (!bus.check_align(da, {ire})) return false; bus.write_m(da, {ire}, v); if (bus.faulted()) return false;")
        elif dst["raw_kind"] == "abs_l":
            lines.append(f"  const std::uint32_t da = 0x{dst['val'] & 0xFFFFFF:06X}U;")
            lines.append(f"  if (!bus.check_align(da, {ire})) return false; bus.write_m(da, {ire}, v); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr(ccr_logic<{cty}>(v, bus.ccr())); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "MOVEA":
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "v"))
        lines.append(f"  bus.set_a({ops[1]['reg_index']}, v); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "CLR":
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        dst = ops[0]
        if dst["raw_kind"] == "disp_a":
            lines.append(f"  const std::uint32_t da = (bus.a({dst['reg_index']}) + static_cast<std::uint32_t>(static_cast<std::int32_t>({dst['val']}))) & kBusAddressMask;")
            lines.append(f"  if (!bus.check_align(da, {ire})) return false; bus.write_m(da, {ire}, 0U); if (bus.faulted()) return false;")
        elif dst["raw_kind"] == "abs_l":
            lines.append(f"  const std::uint32_t da = 0x{dst['val'] & 0xFFFFFF:06X}U;")
            lines.append(f"  if (!bus.check_align(da, {ire})) return false; bus.write_m(da, {ire}, 0U); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "SF":
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        if ops[0]["raw_kind"] == "abs_l":
            lines.append(f"  bus.write_m(0x{ops[0]['val'] & 0xFFFFFF:06X}U, IrOpSize::Byte, 0U); if (bus.faulted()) return false;")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "TST":
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "v"))
        lines.append(f"  bus.set_ccr(ccr_logic<{cty}>(v, bus.ccr())); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in ("ADDQ", "SUBQ", "ADDI", "SUBI", "ADD"):
        is_sub = m in ("SUBQ", "SUBI")
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
        lines.extend("  " + l for l in emit_read_op(ops[1], b, cty, ire, "d"))
        expr = f"static_cast<{cty}>(d - s)" if is_sub else f"static_cast<{cty}>(d + s)"
        ccr_fn = "ccr_sub" if is_sub else "ccr_add"
        lines.append(f"  const {cty} r = {expr};")
        dst = ops[1]
        if dst["raw_kind"] == "reg_d":
            lines.append(f"  write_dn(bus, {dst['reg_index']}, r, {b});")
        else:
            lines.append(f"  bus.write_m(addr_d, {ire}, r); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr({ccr_fn}<{cty}>(d, s, bus.ccr(), true)); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "CMPI":
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
        lines.extend("  " + l for l in emit_read_op(ops[1], b, cty, ire, "d"))
        lines.append(f"  bus.set_ccr(ccr_sub<{cty}>(d, s, bus.ccr(), false)); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "ORI":
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], b, cty, ire, "s"))
        lines.extend("  " + l for l in emit_read_op(ops[1], b, cty, ire, "d"))
        lines.append(f"  const {cty} r = static_cast<{cty}>(d | s); bus.write_m(addr_d, {ire}, r); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr(ccr_logic<{cty}>(r, bus.ccr())); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in ("BTST", "BCLR", "BSET"):
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.extend("  " + l for l in emit_read_op(ops[0], 1, "std::uint8_t", "IrOpSize::Byte", "bit_s"))
        lines.extend("  " + l for l in emit_read_op(ops[1], 1, "std::uint8_t", "IrOpSize::Byte", "cur"))
        lines.append(f"  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U));")
        lines.append(f"  const bool z = (cur & mask) == 0U;")
        if m == "BCLR":
            lines.append(f"  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur & ~mask)); if (bus.faulted()) return false;")
        elif m == "BSET":
            lines.append(f"  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;")
        lines.append(f"  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "LEA":
        base_addr = ops[0]["val"]
        idx_r = ops[0]["reg_index"]
        dst_r = ops[1]["reg_index"]
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::int32_t idx = static_cast<std::int16_t>(bus.d({idx_r}) & 0xFFFFU);")
        lines.append(f"  bus.set_a({dst_r}, (0x{base_addr:06X}U + static_cast<std::uint32_t>(idx)) & kBusAddressMask);")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "ADDA":
        s_r = ops[0]["reg_index"]
        d_r = ops[1]["reg_index"]
        lines.append(f"  const std::uint32_t ptr = bus.a({s_r}) & kBusAddressMask;")
        lines.append(f"  if (!bus.check_align(ptr, IrOpSize::Word)) return false;")
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::int32_t disp = static_cast<std::int16_t>(bus.read_m(ptr, IrOpSize::Word));")
        lines.append(f"  if (bus.faulted()) return false;")
        lines.append(f"  bus.set_a({d_r}, (bus.a({d_r}) + static_cast<std::uint32_t>(disp)) & kBusAddressMask);")
        lines.append(f"  out_pc = 0x{next_pc:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "JMP":
        lines.append(f"  const std::uint32_t tgt = bus.a({ops[0]['reg_index']}) & kBusAddressMask;")
        lines.append(f"  if ((tgt & 1U) != 0U) {{ bus.fault_addr(); return false; }}")
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  out_pc = tgt; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in ("BSR", "JSR"):
        tgt = insn["branch_target"]
        lines.append(f"  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;")
        lines.append(f"  if ((new_sp & 1U) != 0U) {{ bus.fault_addr(); return false; }}")
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x{next_pc:06X}U); if (bus.faulted()) return false;")
        lines.append(f"  out_pc = 0x{tgt:06X}U; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m == "RTS":
        lines.append(f"  const std::uint32_t sp = bus.a(7) & kBusAddressMask;")
        lines.append(f"  if ((sp & 1U) != 0U) {{ bus.fault_addr(); return false; }}")
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
        lines.append(f"  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;")
        lines.append(f"  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, {cyc}, 0);")
    elif m in cond_map:
        tgt = insn["branch_target"]
        ccode = cond_map[m]
        adj_nt = -1 if (m != "BRA" and sz == "b") else (1 if (m != "BRA") else 0)
        lines.append(f"  if (!bus.begin_insn(kPc, kOp)) return false;")
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


def emit_cpp_files(ir_doc: dict, gen_dir: Path) -> Tuple[dict, List[str]]:
    """Emit split .hpp files (<=350 lines each) and record 1:1 ROM_PC -> generated line mapping."""
    gen_dir.mkdir(parents=True, exist_ok=True)
    common_path = gen_dir / "entity_dispatch_common.hpp"
    common_path.write_text(COMMON_HPP, encoding="utf-8")

    # Collect all instructions in sorted PC order
    all_insns = []
    for b in ir_doc["blocks"]:
        all_insns.extend(b["instructions"])
    all_insns.sort(key=lambda x: int(x["pc"], 16))
    compiled_pcs = [int(x["pc"], 16) for x in all_insns]
    compiled_set = set(compiled_pcs)

    parts_written: List[str] = ["entity_dispatch_common.hpp"]
    source_map: Dict[str, dict] = {}
    max_lines_per_part = 260

    cur_lines = [
        "#pragma once",
        '#include "tools/thor_ir/generated/entity_dispatch_common.hpp"',
        "namespace oasis::recompiler::ir {",
    ]
    part_idx = 0

    def flush_part():
        nonlocal cur_lines, part_idx
        if len(cur_lines) <= 3:
            return
        cur_lines.append("} // namespace oasis::recompiler::ir")
        fname = f"entity_dispatch_part{part_idx}.hpp"
        (gen_dir / fname).write_text("\n".join(cur_lines) + "\n", encoding="utf-8")
        parts_written.append(fname)
        part_idx += 1
        cur_lines = [
            "#pragma once",
            '#include "tools/thor_ir/generated/entity_dispatch_common.hpp"',
            "namespace oasis::recompiler::ir {",
        ]

    for insn in all_insns:
        body = emit_insn_body(insn)
        if len(cur_lines) + len(body) + 2 > max_lines_per_part:
            flush_part()
        start_ln = len(cur_lines) + 1
        cur_lines.extend(body)
        end_ln = len(cur_lines)
        fname = f"entity_dispatch_part{part_idx}.hpp"
        source_map[insn["pc"]] = {
            "ir_op_id": insn["ir_op_id"],
            "cpp_file": f"src/tools/thor_ir/generated/{fname}",
            "cpp_lines": f"{start_ln}..{end_ln}",
        }
    flush_part()

    # Emit Mode A step router & Mode B block executor in part_router.hpp (auto-split if needed)
    router_lines = ["#pragma once"]
    for p in parts_written:
        if p.startswith("entity_dispatch_part"):
            router_lines.append(f'#include "tools/thor_ir/generated/{p}"')
    router_lines.append("namespace oasis::recompiler::ir {")
    router_lines.append("inline bool is_compiled_entity_dispatch_pc(std::uint32_t pc) noexcept {")
    router_lines.append("  switch (pc & kBusAddressMask) {")
    for i in range(0, len(compiled_pcs), 8):
        cases = " ".join(f"case 0x{p:06X}U:" for p in compiled_pcs[i:i+8])
        router_lines.append(f"  {cases} return true;")
    router_lines.append("  default: return false; }")
    router_lines.append("}")
    router_lines.append("template <typename Bus> inline bool step_one_entity_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {")
    router_lines.append("  switch (pc & kBusAddressMask) {")
    for p in compiled_pcs:
        router_lines.append(f"  case 0x{p:06X}U: return step_ir_0x{p:06X}(bus, out_pc);")
    router_lines.append("  default: return false; }")
    router_lines.append("}")
    router_lines.append("} // namespace oasis::recompiler::ir")
    (gen_dir / "entity_dispatch_router.hpp").write_text("\n".join(router_lines) + "\n", encoding="utf-8")
    parts_written.append("entity_dispatch_router.hpp")

    # Emit Mode B basic block executor
    mb_lines = [
        "#pragma once",
        '#include "tools/thor_ir/generated/entity_dispatch_router.hpp"',
        "namespace oasis::recompiler::ir {",
        "inline PortableExecutionResult run_entity_dispatch_mode_b_ir(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  PortableIrBus bus{env}; env.pc = entry_pc & kBusAddressMask; PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;",
        "  std::uint32_t next_pc = env.pc; int guard = 0;",
        "  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 4096) {",
        "    if (!is_compiled_entity_dispatch_pc(env.pc)) { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }",
        "    switch (env.pc) {",
    ]
    for b in ir_doc["blocks"]:
        spc = int(b["start_pc"], 16)
        mb_lines.append(f"    case 0x{spc:06X}U: {{")
        for insn in b["instructions"]:
            ipc = int(insn["pc"], 16)
            mb_lines.append(f"      if (!step_ir_0x{ipc:06X}(bus, next_pc)) break; if (next_pc == stop_pc) break;")
        mb_lines.append("      break; }")
    mb_lines.extend([
        "    default: if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break; break; }",
        "  }",
        "  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }",
        "  else if (env.yielded || (env.pc != stop_pc && !is_compiled_entity_dispatch_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;",
        "  for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }",
        "  res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;",
        "  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;",
        "}",
        "} // namespace oasis::recompiler::ir",
    ])
    (gen_dir / "entity_dispatch_mode_b.hpp").write_text("\n".join(mb_lines) + "\n", encoding="utf-8")
    parts_written.append("entity_dispatch_mode_b.hpp")

    # Umbrella header preserving Mode A & Mode B public API
    umb = [
        "#pragma once",
        '#include "tools/thor_ir/generated/entity_dispatch_mode_b.hpp"',
        "namespace oasis::recompiler::ir {",
        "inline PortableExecutionResult run_entity_dispatch_mode_a(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  PortableIrBus bus{env}; env.pc = entry_pc & kBusAddressMask; PortableExecutionResult res{}; res.outcome = PortableContinuationOutcome::CompiledNextPc;",
        "  std::uint32_t next_pc = env.pc; int guard = 0;",
        "  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 4096) {",
        "    if (!is_compiled_entity_dispatch_pc(env.pc)) { res.outcome = PortableContinuationOutcome::ValidUncompiledPc; break; }",
        "    if (!step_one_entity_dispatch_ir(bus, env.pc, next_pc)) break;",
        "  }",
        "  if (bus.faulted()) { res.outcome = PortableContinuationOutcome::OriginalCpuException; res.exception_vector = env.address_error ? 3U : 2U; }",
        "  else if (env.yielded || (env.pc != stop_pc && !is_compiled_entity_dispatch_pc(env.pc))) res.outcome = PortableContinuationOutcome::ValidUncompiledPc;",
        "  for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }",
        "  res.pc = env.pc; res.ccr = env.read_ccr(); res.cycles = env.total_cycles; res.writes = bus.writes;",
        "  res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc); return res;",
        "}",
        "inline PortableExecutionResult run_entity_dispatch_mode_b(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {",
        "  return run_entity_dispatch_mode_b_ir(env, entry_pc, stop_pc);",
        "}",
        "} // namespace oasis::recompiler::ir",
    ]
    (gen_dir / "entity_dispatch_portable_routines.hpp").write_text("\n".join(umb) + "\n", encoding="utf-8")
    parts_written.append("entity_dispatch_portable_routines.hpp")

    return source_map, parts_written
