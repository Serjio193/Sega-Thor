#!/usr/bin/env python3
"""
C++20 Code Generator for Thor Recompiler Prototype.
Translates M68000 CFG into verifiable C++20 native functions using basic block dispatch.
Guarantees <= 500 lines per file (AGENTS.md Rule 3) and zero cross-function goto errors.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
from pathlib import Path
from typing import Dict, List, Tuple
from tools.thor_recompiler.cfg import BasicBlock, FunctionCFG
from tools.thor_recompiler.decoder import DecodedInstruction, Operand

def reg_name(kind: str, num: int) -> str:
    return f"kReg{'A' if kind == 'reg_a' else 'D'}{num}"

def emit_operand_read(op: Operand, size: str) -> str:
    if op.kind in ("reg_d", "reg_a"):
        r = reg_name(op.kind, op.reg)
        if size == "b":
            return f"(api.read_register(api.context, {r}) & 0xFFU)"
        elif size == "w":
            return f"(api.read_register(api.context, {r}) & 0xFFFFU)"
        return f"api.read_register(api.context, {r})"
    elif op.kind == "imm":
        return f"0x{op.val:X}U"
    elif op.kind == "disp_a":
        r = reg_name("reg_a", op.reg)
        disp = op.val
        sign = "+" if disp >= 0 else "-"
        disp_abs = abs(disp)
        w = 1 if size == "b" else (2 if size == "w" else 4)
        return f"api.read(api.context, api.read_register(api.context, {r}) {sign} 0x{disp_abs:X}U, {w})"
    elif op.kind == "indir_a":
        r = reg_name("reg_a", op.reg)
        w = 2 if size == "w" else 4
        return f"api.read(api.context, api.read_register(api.context, {r}), {w})"
    elif op.kind == "postinc_a":
        r = reg_name("reg_a", op.reg)
        w = 2 if size == "w" else 4
        return f"api.read(api.context, api.read_register(api.context, {r}), {w})"
    raise NotImplementedError(f"Unsupported operand read kind {op.kind}")

def emit_block_body(block: BasicBlock) -> List[str]:
    lines = [f"    case 0x{block.start_pc:06X}U: {{"]
    for insn in block.instructions:
        raw_hex = "".join([f"{w:04X}" for w in insn.raw_words])
        lines.append(f"        // 0x{insn.pc:06X}: {insn.mnemonic}.{insn.size} [{raw_hex}]")
        if insn.mnemonic == "RTS":
            lines.extend([
                "        {",
                "            const auto sp = api.read_register(api.context, kRegA7);",
                "            resume_pc = api.read(api.context, sp, 4);",
                "            api.write_register(api.context, kRegA7, sp + 4U);",
                f"            api.add_cycles(api.context, {insn.base_cycles} * 7);",
                "            return 0U; // Terminal return",
                "        }"
            ])
        elif insn.mnemonic == "MOVEA":
            src_val = emit_operand_read(insn.ops[0], insn.size)
            dst_reg = reg_name(insn.ops[1].kind, insn.ops[1].reg)
            lines.append(f"        api.write_register(api.context, {dst_reg}, {src_val});")
            lines.append(f"        api.add_cycles(api.context, {insn.base_cycles} * 7);")
        elif insn.mnemonic == "LEA":
            src_op = insn.ops[0]
            dst_reg = reg_name(insn.ops[1].kind, insn.ops[1].reg)
            if src_op.kind == "abs_l":
                lines.append(f"        api.write_register(api.context, {dst_reg}, 0x{src_op.val:08X}U);")
            elif src_op.kind == "disp_a":
                r = reg_name("reg_a", src_op.reg)
                disp = src_op.val
                sign = "+" if disp >= 0 else "-"
                lines.append(f"        api.write_register(api.context, {dst_reg}, api.read_register(api.context, {r}) {sign} 0x{abs(disp):X}U);")
            lines.append(f"        api.add_cycles(api.context, {insn.base_cycles} * 7);")
        elif insn.mnemonic == "CLR":
            dst_reg = reg_name(insn.ops[0].kind, insn.ops[0].reg)
            lines.extend([
                "        {",
                f"            const auto old_d = api.read_register(api.context, {dst_reg});",
                f"            api.write_register(api.context, {dst_reg}, old_d & 0xFFFF0000U);",
                "            update_logic_ccr16(api, 0U);",
                "        }",
                f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
            ])
        elif insn.mnemonic == "NEG":
            dst_reg = reg_name(insn.ops[0].kind, insn.ops[0].reg)
            lines.extend([
                "        {",
                f"            const auto old_d = api.read_register(api.context, {dst_reg});",
                "            const std::uint32_t a = old_d & 0xFFFFU;",
                "            const std::uint32_t res = (0U - a) & 0xFFFFU;",
                f"            api.write_register(api.context, {dst_reg}, (old_d & 0xFFFF0000U) | res);",
                "            const bool c = (a != 0U);",
                "            const bool v = (a == 0x8000U);",
                "            const bool n = (res & 0x8000U) != 0;",
                "            const bool z = (res == 0U);",
                "            set_ccr(api, (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));",
                "        }",
                f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
            ])
        elif insn.mnemonic in ("BTST", "BSET"):
            bit_num = insn.ops[0].val
            dst_op = insn.ops[1]
            if dst_op.kind == "disp_a":
                r = reg_name("reg_a", dst_op.reg)
                disp = dst_op.val
                sign = "+" if disp >= 0 else "-"
                lines.extend([
                    "        {",
                    f"            const auto addr = api.read_register(api.context, {r}) {sign} 0x{abs(disp):X}U;",
                    "            const auto byte_val = api.read(api.context, addr, 1);",
                    f"            const bool z = ((byte_val >> ({bit_num % 8}U)) & 1U) == 0;",
                    "            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));",
                    "        }",
                    f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
                ])
            elif dst_op.kind == "reg_d":
                dst_reg = reg_name(dst_op.kind, dst_op.reg)
                is_set = (insn.mnemonic == "BSET")
                set_code = f"api.write_register(api.context, {dst_reg}, old_d | (1U << {bit_num % 32}U));" if is_set else ""
                lines.extend([
                    "        {",
                    f"            const auto old_d = api.read_register(api.context, {dst_reg});",
                    f"            const bool z = ((old_d >> ({bit_num % 32}U)) & 1U) == 0;",
                    f"            {set_code}",
                    "            set_ccr(api, (get_ccr(api) & ~0x04U) | (z ? 0x04U : 0x00U));",
                    "        }",
                    f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
                ])
        elif insn.mnemonic == "MOVE":
            src_val = emit_operand_read(insn.ops[0], insn.size)
            dst_op = insn.ops[1]
            if dst_op.kind == "reg_d":
                dst_reg = reg_name(dst_op.kind, dst_op.reg)
                if insn.size == "b":
                    lines.extend([
                        "        {",
                        f"            const auto val = static_cast<std::uint8_t>({src_val});",
                        f"            const auto old = api.read_register(api.context, {dst_reg});",
                        f"            api.write_register(api.context, {dst_reg}, (old & 0xFFFFFF00U) | val);",
                        "            const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U;",
                        "            const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;",
                        "            set_ccr(api, (get_ccr(api) & 0x10U) | n | z);",
                        "        }"
                    ])
                else:
                    lines.extend([
                        "        {",
                        f"            const auto val = static_cast<std::uint16_t>({src_val});",
                        f"            const auto old = api.read_register(api.context, {dst_reg});",
                        f"            api.write_register(api.context, {dst_reg}, (old & 0xFFFF0000U) | val);",
                        "            update_logic_ccr16(api, val);",
                        "        }"
                    ])
            elif dst_op.kind == "disp_a":
                r = reg_name("reg_a", dst_op.reg)
                disp = dst_op.val
                sign = "+" if disp >= 0 else "-"
                w = 1 if insn.size == "b" else 2
                val_cast = "std::uint8_t" if insn.size == "b" else "std::uint16_t"
                lines.extend([
                    "        {",
                    f"            const auto val = static_cast<{val_cast}>({src_val});",
                    f"            api.write(api.context, api.read_register(api.context, {r}) {sign} 0x{abs(disp):X}U, {w}, val);",
                    f"            {'const std::uint16_t n = (val & 0x80U) ? 0x08U : 0x00U; const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U; set_ccr(api, (get_ccr(api) & 0x10U) | n | z);' if insn.size == 'b' else 'update_logic_ccr16(api, val);'}",
                    "        }"
                ])
            elif dst_op.kind == "indir_a":
                r = reg_name("reg_a", dst_op.reg)
                lines.extend([
                    "        {",
                    f"            const auto val = static_cast<std::uint16_t>({src_val});",
                    f"            api.write(api.context, api.read_register(api.context, {r}), 2, val);",
                    "            update_logic_ccr16(api, val);",
                    "        }"
                ])
            if insn.ops[0].kind == "postinc_a":
                r_src = reg_name("reg_a", insn.ops[0].reg)
                lines.append(f"        api.write_register(api.context, {r_src}, api.read_register(api.context, {r_src}) + 2U);")
            lines.append(f"        api.add_cycles(api.context, {insn.base_cycles} * 7);")
        elif insn.mnemonic in ("ADD", "SUB", "ADDI", "SUBI", "ADDQ", "SUBQ"):
            is_sub = "SUB" in insn.mnemonic
            src_val = emit_operand_read(insn.ops[0], insn.size)
            dst_op = insn.ops[1]
            dst_reg = reg_name(dst_op.kind, dst_op.reg)
            op_sign = "-" if is_sub else "+"
            lines.extend([
                "        {",
                f"            const auto old_d = api.read_register(api.context, {dst_reg});",
                f"            const std::uint32_t a = old_d & 0xFFFFU;",
                f"            const std::uint32_t b = static_cast<std::uint32_t>({src_val}) & 0xFFFFU;",
                f"            const std::uint32_t res = (a {op_sign} b) & 0xFFFFU;",
                f"            api.write_register(api.context, {dst_reg}, (old_d & 0xFFFF0000U) | res);",
                f"            const bool c = {'a < b' if is_sub else '(a + b) > 0xFFFFU'};",
                f"            const bool v = {'((a ^ b) & (a ^ res) & 0x8000U) != 0' if is_sub else '(~(a ^ b) & (a ^ res) & 0x8000U) != 0'};",
                "            const bool n = (res & 0x8000U) != 0;",
                "            const bool z = (res == 0U);",
                "            const std::uint16_t ccr = (c ? 0x11U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U);",
                "            set_ccr(api, ccr);",
                "        }",
                f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
            ])
        elif insn.mnemonic in ("ADDA", "SUBA"):
            is_sub = "SUB" in insn.mnemonic
            src_val = emit_operand_read(insn.ops[0], insn.size)
            dst_reg = reg_name(insn.ops[1].kind, insn.ops[1].reg)
            op_sign = "-" if is_sub else "+"
            lines.extend([
                "        {",
                f"            const auto old_a = api.read_register(api.context, {dst_reg});",
                f"            const std::int32_t diff = static_cast<std::int16_t>({src_val});",
                f"            api.write_register(api.context, {dst_reg}, old_a {op_sign} diff);",
                "        }",
                f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
            ])
        elif insn.mnemonic in ("ANDI", "ORI"):
            is_or = (insn.mnemonic == "ORI")
            src_val = emit_operand_read(insn.ops[0], insn.size)
            dst_reg = reg_name(insn.ops[1].kind, insn.ops[1].reg)
            op_char = "|" if is_or else "&"
            lines.extend([
                "        {",
                f"            const auto old_d = api.read_register(api.context, {dst_reg});",
                f"            const auto res = static_cast<std::uint16_t>((old_d & 0xFFFFU) {op_char} {src_val});",
                f"            api.write_register(api.context, {dst_reg}, (old_d & 0xFFFF0000U) | res);",
                "            update_logic_ccr16(api, res);",
                "        }",
                f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
            ])
        elif insn.mnemonic in ("CMP", "CMPI"):
            src_val = emit_operand_read(insn.ops[0], insn.size)
            dst_reg = reg_name(insn.ops[1].kind, insn.ops[1].reg)
            lines.extend([
                "        {",
                f"            const std::uint32_t a = api.read_register(api.context, {dst_reg}) & 0xFFFFU;",
                f"            const std::uint32_t b = static_cast<std::uint32_t>({src_val}) & 0xFFFFU;",
                "            const std::uint32_t res = (a - b) & 0xFFFFU;",
                "            const bool c = a < b;",
                "            const bool v = ((a ^ b) & (a ^ res) & 0x8000U) != 0;",
                "            const bool n = (res & 0x8000U) != 0;",
                "            const bool z = (res == 0U);",
                "            const std::uint16_t old_x = get_ccr(api) & 0x10U;",
                "            set_ccr(api, old_x | (c ? 0x01U : 0x00U) | (v ? 0x02U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));",
                "        }",
                f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
            ])
        elif insn.mnemonic in ("LSL", "LSR", "ASR", "ROL"):
            dst_reg = reg_name("reg_d", insn.ops[1].reg)
            if insn.ops[0].kind == "imm":
                shift_cnt = insn.ops[0].val
                lines.append("        {")
                lines.append(f"            const auto old_d = api.read_register(api.context, {dst_reg});")
                lines.append("            const std::uint32_t val = old_d & 0xFFFFU;")
                if insn.mnemonic == "LSL":
                    lines.append(f"            const std::uint32_t res = (val << {shift_cnt}U) & 0xFFFFU;")
                    lines.append(f"            const bool c = ((val << ({shift_cnt}U - 1U)) & 0x8000U) != 0;")
                elif insn.mnemonic == "LSR":
                    lines.append(f"            const std::uint32_t res = (val >> {shift_cnt}U) & 0xFFFFU;")
                    lines.append(f"            const bool c = ((val >> ({shift_cnt}U - 1U)) & 1U) != 0;")
                elif insn.mnemonic == "ASR":
                    lines.append(f"            const std::int16_t sval = static_cast<std::int16_t>(val);")
                    lines.append(f"            const std::int16_t sres = sval >> {shift_cnt}U;")
                    lines.append(f"            const std::uint32_t res = static_cast<std::uint16_t>(sres);")
                    lines.append(f"            const bool c = ((sval >> ({shift_cnt}U - 1U)) & 1U) != 0;")
                else:
                    lines.append(f"            const std::uint32_t res = ((val << {shift_cnt}U) | (val >> (16U - {shift_cnt}U))) & 0xFFFFU;")
                    lines.append(f"            const bool c = ((val >> (16U - {shift_cnt}U)) & 1U) != 0;")
                lines.append(f"            api.write_register(api.context, {dst_reg}, (old_d & 0xFFFF0000U) | res);")
                lines.append("            const bool n = (res & 0x8000U) != 0;")
                lines.append("            const bool z = (res == 0U);")
                x_flag = "(c ? 0x10U : 0x00U)" if insn.mnemonic != "ROL" else "(get_ccr(api) & 0x10U)"
                lines.append(f"            set_ccr(api, {x_flag} | (c ? 0x01U : 0x00U) | (z ? 0x04U : 0x00U) | (n ? 0x08U : 0x00U));")
                lines.append("        }")
                lines.append(f"        api.add_cycles(api.context, {insn.base_cycles + insn.variable_cycles});")
            else:
                cnt_reg = reg_name("reg_d", insn.ops[0].reg)
                lines.extend([
                    "        {",
                    f"            const auto shift = api.read_register(api.context, {cnt_reg}) & 0x3FU;",
                    f"            const auto old_d = api.read_register(api.context, {dst_reg});",
                    "            const std::uint32_t res = ((old_d & 0xFFFFU) << (shift % 64U)) & 0xFFFFU;",
                    f"            api.write_register(api.context, {dst_reg}, (old_d & 0xFFFF0000U) | res);",
                    "            update_logic_ccr16(api, static_cast<std::uint16_t>(res));",
                    "            if (shift != 0) api.add_cycles(api.context, static_cast<int>(shift) * 14);",
                    "            api.skip_bus_refresh(api.context);",
                    "        }",
                    f"        api.add_cycles(api.context, {insn.base_cycles});"
                ])
        elif insn.mnemonic == "SWAP":
            dst_reg = reg_name("reg_d", insn.ops[0].reg)
            lines.extend([
                "        {",
                f"            const auto v = api.read_register(api.context, {dst_reg});",
                "            const auto swapped = (v >> 16U) | (v << 16U);",
                f"            api.write_register(api.context, {dst_reg}, swapped);",
                "            update_logic_ccr16(api, static_cast<std::uint16_t>(swapped & 0xFFFFU));",
                "        }",
                f"        api.add_cycles(api.context, {insn.base_cycles} * 7);"
            ])
        elif insn.mnemonic == "BRA":
            lines.append(f"        return 0x{insn.branch_target:06X}U;")
        elif insn.mnemonic in ("BMI", "BPL", "BLT", "BGE", "BNE", "BEQ", "BGT", "BLE"):
            conds = {
                "BMI": "(get_ccr(api) & 0x08U) != 0",
                "BPL": "(get_ccr(api) & 0x08U) == 0",
                "BEQ": "(get_ccr(api) & 0x04U) != 0",
                "BNE": "(get_ccr(api) & 0x04U) == 0",
                "BLT": "((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0)",
                "BGE": "((get_ccr(api) & 0x08U) != 0) == ((get_ccr(api) & 0x02U) != 0)",
                "BGT": "(get_ccr(api) & 0x04U) == 0 && (((get_ccr(api) & 0x08U) != 0) == ((get_ccr(api) & 0x02U) != 0))",
                "BLE": "(get_ccr(api) & 0x04U) != 0 || (((get_ccr(api) & 0x08U) != 0) != ((get_ccr(api) & 0x02U) != 0))",
            }
            lines.append(f"        if ({conds[insn.mnemonic]}) return 0x{insn.branch_target:06X}U;")
            lines.append(f"        return 0x{block.end_pc:06X}U;")
        elif insn.mnemonic == "DBRA":
            dst_reg = reg_name("reg_d", insn.ops[0].reg)
            lines.extend([
                "        {",
                f"            const auto old_d = api.read_register(api.context, {dst_reg});",
                "            const auto count = static_cast<std::int16_t>(old_d & 0xFFFFU) - 1;",
                f"            api.write_register(api.context, {dst_reg}, (old_d & 0xFFFF0000U) | static_cast<std::uint16_t>(count));",
                f"            api.add_cycles(api.context, {insn.base_cycles} * 7);",
                f"            if (count != -1) return 0x{insn.branch_target:06X}U;",
                f"            return 0x{block.end_pc:06X}U;",
                "        }"
            ])

    last_insn = block.instructions[-1]
    if last_insn.mnemonic not in ("BRA", "RTS", "BMI", "BPL", "BLT", "BGE", "BNE", "BEQ", "BGT", "BLE", "DBRA"):
        lines.append(f"        return 0x{block.end_pc:06X}U;")
    lines.append("    }")
    return lines

def generate_sub_function(name: str, blocks: List[BasicBlock]) -> str:
    lines = [
        f'#include "tools/hybrid/generated/{name}.hpp"',
        "",
        "namespace oasis::hybrid::generated {",
        "",
        "namespace {",
        "constexpr unsigned kRegD0 = 0U;",
        "constexpr unsigned kRegD1 = 1U;",
        "constexpr unsigned kRegD2 = 2U;",
        "constexpr unsigned kRegD3 = 3U;",
        "constexpr unsigned kRegD4 = 4U;",
        "constexpr unsigned kRegD5 = 5U;",
        "constexpr unsigned kRegD6 = 6U;",
        "constexpr unsigned kRegD7 = 7U;",
        "constexpr unsigned kRegA0 = 8U;",
        "constexpr unsigned kRegA1 = 9U;",
        "constexpr unsigned kRegA2 = 10U;",
        "constexpr unsigned kRegA3 = 11U;",
        "constexpr unsigned kRegA4 = 12U;",
        "constexpr unsigned kRegA5 = 13U;",
        "constexpr unsigned kRegA6 = 14U;",
        "constexpr unsigned kRegA7 = 15U;",
        "constexpr unsigned kRegSR = 16U;",
        "",
        "inline std::uint16_t get_ccr(const M68k9D70StepApi& api) noexcept {",
        "    return static_cast<std::uint16_t>(api.read_register(api.context, kRegSR) & 0xFFU);",
        "}",
        "inline void set_ccr(const M68k9D70StepApi& api, std::uint16_t ccr) noexcept {",
        "    const auto sr = api.read_register(api.context, kRegSR);",
        "    api.write_register(api.context, kRegSR, (sr & 0xFF00U) | (ccr & 0xFFU));",
        "}",
        "inline void update_logic_ccr16(const M68k9D70StepApi& api, std::uint16_t val) noexcept {",
        "    const std::uint16_t n = (val & 0x8000U) ? 0x08U : 0x00U;",
        "    const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;",
        "    set_ccr(api, (get_ccr(api) & 0x10U) | n | z);",
        "}",
        "} // namespace",
        "",
        f"unsigned {name}(const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc) noexcept {{",
        "    switch (block_pc) {",
    ]
    for b in blocks:
        lines.extend(emit_block_body(b))
    lines.extend([
        "    default:",
        "        return 0U;",
        "    }",
        "}",
        "} // namespace oasis::hybrid::generated",
        ""
    ])
    return "\n".join(lines)

def generate_cpp_header(function_name: str, is_orchestrator: bool = False) -> str:
    guard = f"OASIS_HYBRID_GENERATED_{function_name.upper()}_HPP_"
    ret_type = "M68k9D70StepResult" if is_orchestrator else "unsigned"
    arg_list = "const M68k9D70StepApi& api, unsigned& resume_pc" if is_orchestrator else "const M68k9D70StepApi& api, unsigned block_pc, unsigned& resume_pc"
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        '#include "tools/hybrid/m68k_9d70_instruction_step.hpp"',
        "",
        "namespace oasis::hybrid::generated {",
        "",
        f"{ret_type} {function_name}({arg_list}) noexcept;",
        "",
        "} // namespace oasis::hybrid::generated",
        "",
        f"#endif // {guard}",
    ]
    return "\n".join(lines) + "\n"
