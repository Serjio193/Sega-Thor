#!/usr/bin/env python3
"""
C++20 Scheduler-Safe Instruction Stepper Generator for Thor Recompiler.
Translates M68000 instructions into individual compiled case-statements,
calling prepare_step, complete_step, step_rts and yielding to GPGX scheduler per instruction.
Guarantees <= 500 lines per file (AGENTS.md Rule 3).
"""
import struct
from pathlib import Path
from typing import Dict, List, Optional
from tools.thor_recompiler.decoder import DecodedInstruction, Operand

def reg_const(kind: str, num: int) -> str:
    return f"kReg{'A' if kind == 'reg_a' else 'D'}{num}"

def emit_operand_read_expr(op: Operand, size: str) -> str:
    if op.kind in ("reg_d", "reg_a"):
        r = reg_const(op.kind, op.reg)
        if size == "b": return f"(api.read_register(api.context, {r}) & 0xFFU)"
        if size == "w": return f"(api.read_register(api.context, {r}) & 0xFFFFU)"
        return f"api.read_register(api.context, {r})"
    elif op.kind == "imm":
        return f"0x{op.val:X}U"
    elif op.kind == "disp_a":
        r = reg_const("reg_a", op.reg)
        disp = op.val
        sign = "+" if disp >= 0 else "-"
        w = 1 if size == "b" else (2 if size == "w" else 4)
        return f"static_cast<std::uint{8 if size=='b' else (16 if size=='w' else 32)}_t>(api.read(api.context, (api.read_register(api.context, {r}) {sign} 0x{abs(disp):X}U) & kMask24, {w}))"
    elif op.kind in ("indir_a", "postinc_a"):
        r = reg_const("reg_a", op.reg)
        w = 1 if size == "b" else (2 if size == "w" else 4)
        return f"static_cast<std::uint{8 if size=='b' else (16 if size=='w' else 32)}_t>(api.read(api.context, api.read_register(api.context, {r}) & kMask24, {w}))"
    elif op.kind == "abs_l":
        w = 1 if size == "b" else (2 if size == "w" else 4)
        return f"static_cast<std::uint{8 if size=='b' else (16 if size=='w' else 32)}_t>(api.read(api.context, 0x{op.val:06X}U, {w}))"
    raise NotImplementedError(f"Unsupported read operand {op.kind}")

def emit_operand_write_stmts(op: Operand, size: str, val_expr: str) -> List[str]:
    if op.kind == "reg_d":
        r = reg_const("reg_d", op.reg)
        if size == "b":
            return [f"api.write_register(api.context, {r}, (api.read_register(api.context, {r}) & 0xFFFFFF00U) | ({val_expr} & 0xFFU));"]
        elif size == "w":
            return [f"api.write_register(api.context, {r}, (api.read_register(api.context, {r}) & 0xFFFF0000U) | ({val_expr} & 0xFFFFU));"]
        return [f"api.write_register(api.context, {r}, {val_expr});"]
    elif op.kind == "reg_a":
        r = reg_const("reg_a", op.reg)
        if size == "w":
            return [f"api.write_register(api.context, {r}, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>({val_expr}))));"]
        return [f"api.write_register(api.context, {r}, {val_expr});"]
    elif op.kind == "disp_a":
        r = reg_const("reg_a", op.reg)
        disp = op.val
        sign = "+" if disp >= 0 else "-"
        w = 1 if size == "b" else (2 if size == "w" else 4)
        return [f"api.write(api.context, (api.read_register(api.context, {r}) {sign} 0x{abs(disp):X}U) & kMask24, {w}, {val_expr});"]
    elif op.kind in ("indir_a", "postinc_a"):
        r = reg_const("reg_a", op.reg)
        w = 1 if size == "b" else (2 if size == "w" else 4)
        stmts = [f"api.write(api.context, api.read_register(api.context, {r}) & kMask24, {w}, {val_expr});"]
        if op.kind == "postinc_a":
            stmts.append(f"api.write_register(api.context, {r}, api.read_register(api.context, {r}) + {w}U);")
        return stmts
    elif op.kind == "abs_l":
        w = 1 if size == "b" else (2 if size == "w" else 4)
        return [f"api.write(api.context, 0x{op.val:06X}U, {w}, {val_expr});"]
    raise NotImplementedError(f"Unsupported write operand {op.kind}")

def get_cond_expr(mnemonic: str) -> str:
    conds = {
        "BEQ": "(get_ccr(api) & 0x04U) != 0U",
        "BNE": "(get_ccr(api) & 0x04U) == 0U",
        "BPL": "(get_ccr(api) & 0x08U) == 0U",
        "BMI": "(get_ccr(api) & 0x08U) != 0U",
        "BCS": "(get_ccr(api) & 0x01U) != 0U",
        "BCC": "(get_ccr(api) & 0x01U) == 0U",
        "BGT": "((get_ccr(api) & 0x04U) == 0U) && (((get_ccr(api) & 0x08U) != 0U) == ((get_ccr(api) & 0x02U) != 0U))",
        "BLT": "(((get_ccr(api) & 0x08U) != 0U) != ((get_ccr(api) & 0x02U) != 0U))",
        "BGE": "(((get_ccr(api) & 0x08U) != 0U) == ((get_ccr(api) & 0x02U) != 0U))",
        "BLE": "((get_ccr(api) & 0x04U) != 0U) || (((get_ccr(api) & 0x08U) != 0U) != ((get_ccr(api) & 0x02U) != 0U))",
    }
    return conds.get(mnemonic, "true")

def emit_instruction_case(insn: DecodedInstruction, rom: bytes) -> List[str]:
    lines = [f"    case 0x{insn.pc:06X}U: {{"]
    opcode_hex = f"0x{insn.raw_words[0]:04X}U"

    if insn.mnemonic == "RTS":
        lines.extend([
            "        const auto res = step_rts(api, pc, resume_pc);",
            "        if (res == M68k9D70StepResult::InstructionStepped) {",
            "            metrics.stepped_instructions++;",
            "            metrics.completed_calls++;",
            "            active = false;",
            "            interrupted = false;",
            "        }",
            "        return res;",
            "    }"
        ])
        return lines

    lines.extend([
        f"        const std::uint16_t opcode = {opcode_hex};",
        "        prepare_step(api, pc, opcode);",
    ])
    for _ in range(len(insn.raw_words) - 1):
        lines.append("        (void)api.fetch16(api.context);")

    insn_bytes = len(insn.raw_words) * 2
    fallthrough_pc = insn.pc + insn_bytes

    if insn.mnemonic == "BRA" and insn.branch_target is not None:
        target_pc = insn.branch_target
        lines.extend([
            f"        complete_step(api, 0x{target_pc:06X}U, opcode, 0, resume_pc);",
            "        metrics.stepped_instructions++;",
            "        return M68k9D70StepResult::InstructionStepped;",
            "    }"
        ])
        return lines

    if insn.mnemonic == "BSR" and insn.branch_target is not None:
        target_pc = insn.branch_target
        lines.extend([
            "        const unsigned sp = (api.read_register(api.context, kRegA7) - 4U) & kMask24;",
            "        api.write_register(api.context, kRegA7, sp);",
            f"        api.write(api.context, sp, 4, 0x{fallthrough_pc:06X}U);",
            "        unsigned dummy_resume = 0;",
            f"        complete_step(api, 0x{target_pc:06X}U, opcode, 0, dummy_resume);",
            f"        resume_pc = 0x{fallthrough_pc:06X}U;",
            "        metrics.stepped_instructions++;",
            "        return M68k9D70StepResult::InstructionStepped;",
            "    }"
        ])
        return lines

    if insn.mnemonic == "DBRA" and insn.branch_target is not None:
        target_pc = insn.branch_target
        reg_c = reg_const("reg_d", insn.ops[0].reg)
        lines.extend([
            f"        const auto old_d = api.read_register(api.context, {reg_c});",
            "        const auto count = static_cast<std::int16_t>(old_d & 0xFFFFU) - 1;",
            f"        api.write_register(api.context, {reg_c}, (old_d & 0xFFFF0000U) | static_cast<std::uint16_t>(count));",
            "        const bool taken = (count != -1);",
            f"        const unsigned target = taken ? 0x{target_pc:06X}U : 0x{fallthrough_pc:06X}U;",
            "        const int adj = taken ? 0 : 4;",
            "        complete_step(api, target, opcode, adj, resume_pc);",
            "        metrics.stepped_instructions++;",
            "        return M68k9D70StepResult::InstructionStepped;",
            "    }"
        ])
        return lines

    if insn.mnemonic == "JMP" and len(insn.ops) > 0 and insn.ops[0].kind == "indir_a":
        reg_c = reg_const("reg_a", insn.ops[0].reg)
        lines.extend([
            f"        const auto target = api.read_register(api.context, {reg_c}) & kMask24;",
            "        if ((target & 1U) != 0U || target < 0x000200U || target >= 0x300000U) {",
            "            active = false; interrupted = false; return M68k9D70StepResult::ContractFault;",
            "        }",
            "        complete_step(api, target, opcode, 0, resume_pc);",
            "        metrics.stepped_instructions++;",
            "        metrics.completed_calls++;",
            "        active = false;",
            "        interrupted = false;",
            "        return M68k9D70StepResult::InstructionStepped;",
            "    }"
        ])
        return lines

    if insn.mnemonic in ("BEQ", "BNE", "BPL", "BMI", "BCS", "BCC", "BGT", "BLT", "BGE", "BLE") and insn.branch_target is not None:
        target_pc = insn.branch_target
        cond = get_cond_expr(insn.mnemonic)
        adj_func = "word_not_taken_adjustment" if len(insn.raw_words) > 1 else "byte_not_taken_adjustment"
        lines.extend([
            f"        const bool taken = {cond};",
            f"        const unsigned target = taken ? 0x{target_pc:06X}U : 0x{fallthrough_pc:06X}U;",
            f"        const int adj = taken ? 0 : (api.{adj_func} ? api.{adj_func}(api.context) : 0);",
            "        complete_step(api, target, opcode, adj, resume_pc);",
            "        metrics.stepped_instructions++;",
            "        return M68k9D70StepResult::InstructionStepped;",
            "    }"
        ])
        return lines

    # ALU / Data movement
    if insn.mnemonic in ("MOVE", "MOVEA", "MOVEQ"):
        src = emit_operand_read_expr(insn.ops[0], insn.size)
        lines.extend([f"        {s}" for s in emit_operand_write_stmts(insn.ops[1], insn.size, src)])
        if insn.mnemonic != "MOVEA":
            lines.append(f"        update_logic_ccr_{insn.size}(api, {src});")
    elif insn.mnemonic == "LEA":
        src_op = insn.ops[0]
        dst_r = reg_const(insn.ops[1].kind, insn.ops[1].reg)
        if src_op.kind == "abs_l":
            lines.append(f"        api.write_register(api.context, {dst_r}, 0x{src_op.val:08X}U);")
        elif src_op.kind == "disp_a":
            r = reg_const("reg_a", src_op.reg)
            sign = "+" if src_op.val >= 0 else "-"
            lines.append(f"        api.write_register(api.context, {dst_r}, (api.read_register(api.context, {r}) {sign} 0x{abs(src_op.val):X}U) & kMask24);")
        elif src_op.kind == "pc_index":
            idx_r = reg_const("reg_d", src_op.reg)
            lines.extend([
                f"        const auto idx = static_cast<std::int16_t>(api.read_register(api.context, {idx_r}) & 0xFFFFU);",
                f"        api.write_register(api.context, {dst_r}, (0x{src_op.val:06X}U + static_cast<std::uint32_t>(static_cast<std::int32_t>(idx))) & kMask24);",
            ])
    elif insn.mnemonic in ("ADD", "ADDI", "ADDQ", "ADDA"):
        src = emit_operand_read_expr(insn.ops[0], insn.size)
        dst = emit_operand_read_expr(insn.ops[1], "l" if insn.ops[1].kind == "reg_a" else insn.size)
        if insn.ops[1].kind == "reg_a" and insn.size == "w":
            src_expr = f"static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>({src})))"
            lines.extend([
                f"        const auto src = {src_expr};",
                f"        const auto dst = {dst};",
                "        const auto res = (dst + src) & kMask24;",
            ])
            lines.extend([f"        {s}" for s in emit_operand_write_stmts(insn.ops[1], "l", "res")])
        else:
            lines.extend([
                f"        const auto src = {src};",
                f"        const auto dst = {dst};",
                f"        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);",
            ])
            mask = '0xFF' if insn.size == 'b' else ('0xFFFF' if insn.size == 'w' else '0xFFFFFFFF')
            lines.extend([f"        {s}" for s in emit_operand_write_stmts(insn.ops[1], insn.size, f"(res & {mask}U)")])
            if insn.ops[1].kind != "reg_a":
                lines.append(f"        update_add_ccr_{insn.size}(api, dst, src, res);")
    elif insn.mnemonic in ("SUB", "SUBI", "SUBQ", "SUBA"):
        src = emit_operand_read_expr(insn.ops[0], insn.size)
        dst = emit_operand_read_expr(insn.ops[1], insn.size)
        lines.extend([
            f"        const auto src = {src};",
            f"        const auto dst = {dst};",
            f"        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);",
        ])
        mask = '0xFF' if insn.size == 'b' else ('0xFFFF' if insn.size == 'w' else '0xFFFFFFFF')
        lines.extend([f"        {s}" for s in emit_operand_write_stmts(insn.ops[1], insn.size, f"(res & {mask}U)")])
        if insn.mnemonic != "SUBA" and insn.ops[1].kind != "reg_a":
            lines.append(f"        update_sub_ccr_{insn.size}(api, dst, src, res);")
    elif insn.mnemonic in ("CMP", "CMPI", "CMPA"):
        src = emit_operand_read_expr(insn.ops[0], insn.size)
        dst = emit_operand_read_expr(insn.ops[1], insn.size)
        lines.extend([
            f"        const auto src = {src};",
            f"        const auto dst = {dst};",
            f"        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);",
            f"        update_cmp_ccr_{insn.size}(api, dst, src, res);",
        ])
    elif insn.mnemonic == "TST":
        val = emit_operand_read_expr(insn.ops[0], insn.size)
        lines.extend([
            f"        const auto val = {val};",
            f"        update_logic_ccr_{insn.size}(api, val);",
        ])
    elif insn.mnemonic in ("OR", "ORI"):
        src = emit_operand_read_expr(insn.ops[0], insn.size)
        dst = emit_operand_read_expr(insn.ops[1], insn.size)
        lines.extend([
            f"        const auto val = ({dst}) | ({src});",
        ])
        lines.extend([f"        {s}" for s in emit_operand_write_stmts(insn.ops[1], insn.size, "val")])
        lines.append(f"        update_logic_ccr_{insn.size}(api, val);")
    elif insn.mnemonic in ("AND", "ANDI"):
        src = emit_operand_read_expr(insn.ops[0], insn.size)
        dst = emit_operand_read_expr(insn.ops[1], insn.size)
        lines.extend([
            f"        const auto val = ({dst}) & ({src});",
        ])
        lines.extend([f"        {s}" for s in emit_operand_write_stmts(insn.ops[1], insn.size, "val")])
        lines.append(f"        update_logic_ccr_{insn.size}(api, val);")
    elif insn.mnemonic in ("EOR", "EORI"):
        src = emit_operand_read_expr(insn.ops[0], insn.size)
        dst = emit_operand_read_expr(insn.ops[1], insn.size)
        lines.extend([
            f"        const auto val = ({dst}) ^ ({src});",
        ])
        lines.extend([f"        {s}" for s in emit_operand_write_stmts(insn.ops[1], insn.size, "val")])
        lines.append(f"        update_logic_ccr_{insn.size}(api, val);")
    elif insn.mnemonic in ("BTST", "BSET", "BCLR"):
        bit_num = insn.ops[0].val
        dst_op = insn.ops[1]
        val_expr = emit_operand_read_expr(dst_op, insn.size)
        bit_mask = 7 if insn.size == "b" else 31
        lines.extend([
            f"        const auto val = {val_expr};",
            f"        const bool is_zero = (val & (1U << ({bit_num}U & {bit_mask}U))) == 0U;",
            "        update_btst_ccr(api, is_zero);",
        ])
        if insn.mnemonic in ("BSET", "BCLR"):
            set_or_clr = f"val | (1U << ({bit_num}U & {bit_mask}U))" if insn.mnemonic == "BSET" else f"val & ~(1U << ({bit_num}U & {bit_mask}U))"
            lines.append(f"        const auto new_val = static_cast<std::uint8_t>({set_or_clr});")
            lines.extend([f"        {s}" for s in emit_operand_write_stmts(dst_op, insn.size, "new_val")])
    elif insn.mnemonic in ("LSL", "LSR", "ASR"):
        shift_val = insn.ops[0].val if insn.ops[0].kind == "imm" else 1
        dst = emit_operand_read_expr(insn.ops[1], insn.size)
        shift_op = "<<" if insn.mnemonic == "LSL" else ">>"
        lines.extend([
            f"        const auto old_d = {dst};",
            f"        const auto shift = {shift_val}U;",
            f"        const auto val = static_cast<std::uint16_t>((old_d {shift_op} shift) & 0xFFFFU);",
        ])
        lines.extend([f"        {s}" for s in emit_operand_write_stmts(insn.ops[1], insn.size, "val")])
        lines.append(f"        update_shift_ccr_{insn.mnemonic.lower()}_{insn.size}(api, old_d, shift, val);")
        lines.extend([
            f"        complete_step(api, 0x{fallthrough_pc:06X}U, opcode, {shift_val * 14}, resume_pc);",
            "        metrics.stepped_instructions++;",
            "        return M68k9D70StepResult::InstructionStepped;",
            "    }"
        ])
        return lines
    else:
        raise NotImplementedError(f"Instruction {insn.mnemonic} not supported in scheduler-safe codegen")

    lines.extend([
        f"        complete_step(api, 0x{fallthrough_pc:06X}U, opcode, 0, resume_pc);",
        "        metrics.stepped_instructions++;",
        "        return M68k9D70StepResult::InstructionStepped;",
        "    }"
    ])
    return lines

def generate_scheduler_safe_cpp(name: str, entry_pc: int, insns: List[DecodedInstruction], rom: bytes) -> str:
    suffix = name.split('_')[-1]
    lines = [
        f'#include "tools/hybrid/generated/{name}.hpp"',
        '#include "tools/hybrid/generated/m68k_step_helpers.hpp"',
        "",
        "namespace oasis::hybrid::generated {",
        "namespace {",
        "",
        f"constexpr unsigned kStart = 0x{entry_pc:06X}U;",
        "bool active = false;",
        "bool interrupted = false;",
        "unsigned resume_pc = 0U;",
        f"StepMetrics_{suffix} metrics{{}};",
        "",
        "} // namespace",
        "",
        f"M68k9D70StepResult step_m68k_{suffix}(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {{",
        "    const unsigned pc = pc_in & kMask24;",
        "    if (pc == kStart) {",
        "        metrics.target_entries++;",
        "        if (!enabled) return M68k9D70StepResult::NotHandled;",
        "        if (active) {",
        "            metrics.active_at_entry++;",
        "            active = false;",
        "            interrupted = false;",
        "            return M68k9D70StepResult::ContractFault;",
        "        }",
        "        active = true;",
        "        interrupted = false;",
        "        metrics.native_calls++;",
        "    }",
        "    if (!active) return M68k9D70StepResult::NotHandled;",
        "    if (active && pc != resume_pc && pc != kStart) {",
        "        interrupted = true;",
        "        return M68k9D70StepResult::NotHandled;",
        "    }",
        "    if (interrupted && pc == resume_pc) {",
        "        interrupted = false;",
        "    }",
        "    switch (pc) {",
    ]
    for insn in insns:
        lines.extend(emit_instruction_case(insn, rom))
    lines.extend([
        "    default:",
        "        active = false;",
        "        interrupted = false;",
        "        return M68k9D70StepResult::NotHandled;",
        "    }",
        "}",
        "",
        f"const StepMetrics_{suffix}& get_metrics_{suffix}() noexcept {{ return metrics; }}",
        f"void reset_metrics_{suffix}() noexcept {{ metrics = {{}}; active = false; interrupted = false; resume_pc = 0; }}",
        "",
        "} // namespace oasis::hybrid::generated",
        ""
    ])
    return "\n".join(lines)

def generate_scheduler_safe_hpp(name: str) -> str:
    suffix = name.split('_')[-1]
    guard = f"OASIS_HYBRID_GENERATED_M68K_RECOMPILED_{suffix.upper()}_HPP_"
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        '#include "tools/hybrid/m68k_9d70_instruction_step.hpp"',
        "#include <cstdint>",
        "",
        "namespace oasis::hybrid::generated {",
        "",
        f"struct StepMetrics_{suffix} {{",
        "    unsigned target_entries{};",
        "    unsigned native_calls{};",
        "    unsigned completed_calls{};",
        "    unsigned stepped_instructions{};",
        "    unsigned active_at_entry{};",
        "};",
        "",
        f"M68k9D70StepResult step_m68k_{suffix}(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;",
        f"const StepMetrics_{suffix}& get_metrics_{suffix}() noexcept;",
        f"void reset_metrics_{suffix}() noexcept;",
        "",
        "} // namespace oasis::hybrid::generated",
        "",
        f"#endif // {guard}",
        ""
    ]
    return "\n".join(lines)
