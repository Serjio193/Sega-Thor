#!/usr/bin/env python3
"""
Thor IR Builder and Generator.
Converts decoded Motorola 68000 instructions from real ROM bytes into
backend-neutral Thor IR, and produces Mode A (verification) and Mode B (portable native) C++ code.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from dataclasses import dataclass, field
import json
from pathlib import Path
import struct
import sys
from typing import Any, Dict, List, Optional

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from tools.thor_recompiler.decoder import decode_at, DecodedInstruction


@dataclass
class IrOperandJson:
    kind: str  # "Register", "Immediate", "MemoryDisplacement", "MemoryIndirect"
    size: str  # "Byte", "Word", "Long"
    reg_kind: str = "Data"  # "Data", "Address", "PC", "SR", "CCR"
    reg_index: int = 0
    imm_val: int = 0
    displacement: int = 0


@dataclass
class IrInstructionJson:
    original_pc: int
    opcode: str  # "Move", "Tst", "Bclr", "Bset", "Branch", "Return", "SyncBoundary"
    size: str
    operands: List[IrOperandJson] = field(default_factory=list)
    condition: str = "Always"
    target_pc: int = 0
    base_cycles: int = 4
    updates_ccr: bool = True
    is_sync_boundary: bool = False


@dataclass
class IrBasicBlockJson:
    start_pc: int
    end_pc: int
    instructions: List[IrInstructionJson] = field(default_factory=list)
    successors: List[int] = field(default_factory=list)


@dataclass
class IrFunctionJson:
    name: str
    entry_pc: int
    blocks: List[IrBasicBlockJson] = field(default_factory=list)


def translate_instruction_to_ir(insn: DecodedInstruction) -> IrInstructionJson:
    size_map = {"b": "Byte", "w": "Word", "l": "Long", "": "Word"}
    ir_size = size_map.get(insn.size, "Word")
    pc = insn.pc

    if insn.mnemonic == "RTS":
        return IrInstructionJson(
            original_pc=pc,
            opcode="Return",
            size="Word",
            base_cycles=16,
            updates_ccr=False,
            is_sync_boundary=True
        )

    if insn.mnemonic in ("BRA", "BSR", "BNE", "BEQ", "BCC", "BCS", "BPL", "BMI", "BGT", "BLE", "BLT", "BGE"):
        cond_map = {
            "BRA": "Always", "BSR": "Always", "BNE": "NotEqual", "BEQ": "Equal",
            "BCC": "CarryClear", "BCS": "CarrySet", "BPL": "Plus", "BMI": "Minus",
            "BGT": "GreaterThan", "BLE": "LessEqual", "BLT": "LessThan", "BGE": "GreaterEqual"
        }
        op = "Call" if insn.mnemonic == "BSR" else "Branch"
        target = insn.branch_target if insn.branch_target is not None else 0
        return IrInstructionJson(
            original_pc=pc,
            opcode=op,
            size=ir_size,
            condition=cond_map.get(insn.mnemonic, "Always"),
            target_pc=target,
            base_cycles=insn.base_cycles,
            updates_ccr=False,
            is_sync_boundary=True
        )

    if insn.mnemonic == "TST":
        ops = []
        for op in insn.ops:
            if op.kind == "reg_d":
                ops.append(IrOperandJson(kind="Register", size=ir_size, reg_kind="Data", reg_index=op.reg))
            elif op.kind == "disp_a":
                ops.append(IrOperandJson(kind="MemoryDisplacement", size=ir_size, reg_kind="Address", reg_index=op.reg, displacement=op.val))
        return IrInstructionJson(
            original_pc=pc,
            opcode="Tst",
            size=ir_size,
            operands=ops,
            base_cycles=insn.base_cycles,
            updates_ccr=True
        )

    if insn.mnemonic in ("BCLR", "BSET", "BTST"):
        op_map = {"BCLR": "Bclr", "BSET": "Bset", "BTST": "Btst"}
        ops = []
        for op in insn.ops:
            if op.kind == "imm":
                ops.append(IrOperandJson(kind="Immediate", size=ir_size, imm_val=op.val))
            elif op.kind == "reg_d":
                ops.append(IrOperandJson(kind="Register", size=ir_size, reg_kind="Data", reg_index=op.reg))
            elif op.kind == "disp_a":
                ops.append(IrOperandJson(kind="MemoryDisplacement", size=ir_size, reg_kind="Address", reg_index=op.reg, displacement=op.val))
        return IrInstructionJson(
            original_pc=pc,
            opcode=op_map[insn.mnemonic],
            size=ir_size,
            operands=ops,
            base_cycles=insn.base_cycles,
            updates_ccr=True
        )

    if insn.mnemonic == "MOVE":
        ops = []
        for op in insn.ops:
            if op.kind == "imm":
                ops.append(IrOperandJson(kind="Immediate", size=ir_size, imm_val=op.val))
            elif op.kind == "reg_d":
                ops.append(IrOperandJson(kind="Register", size=ir_size, reg_kind="Data", reg_index=op.reg))
            elif op.kind == "reg_a":
                ops.append(IrOperandJson(kind="Register", size=ir_size, reg_kind="Address", reg_index=op.reg))
            elif op.kind == "disp_a":
                ops.append(IrOperandJson(kind="MemoryDisplacement", size=ir_size, reg_kind="Address", reg_index=op.reg, displacement=op.val))
            elif op.kind == "indir_a":
                ops.append(IrOperandJson(kind="MemoryIndirect", size=ir_size, reg_kind="Address", reg_index=op.reg))
        return IrInstructionJson(
            original_pc=pc,
            opcode="Move",
            size=ir_size,
            operands=ops,
            base_cycles=insn.base_cycles,
            updates_ccr=True
        )

    # Fallback to Nop / generic
    return IrInstructionJson(
        original_pc=pc,
        opcode="Nop",
        size=ir_size,
        base_cycles=insn.base_cycles,
        updates_ccr=False
    )


def build_ir_from_rom(rom: bytes, start_pc: int, end_pc: int, func_name: str) -> IrFunctionJson:
    pc = start_pc
    decoded_insns: List[DecodedInstruction] = []
    while pc <= end_pc:
        insn = decode_at(rom, pc)
        decoded_insns.append(insn)
        sz = len(insn.raw_words) * 2
        pc += sz
        if insn.mnemonic == "RTS" and pc > end_pc:
            break

    # Build basic blocks
    block_starts = {start_pc}
    for insn in decoded_insns:
        if insn.branch_target is not None and start_pc <= insn.branch_target <= end_pc:
            block_starts.add(insn.branch_target)
        if insn.mnemonic in ("RTS", "BRA", "BSR", "BNE", "BEQ", "BCC", "BCS"):
            next_pc = insn.pc + len(insn.raw_words) * 2
            if next_pc <= end_pc:
                block_starts.add(next_pc)

    sorted_starts = sorted(list(block_starts))
    blocks: List[IrBasicBlockJson] = []

    for i, bstart in enumerate(sorted_starts):
        bend = sorted_starts[i + 1] if i + 1 < len(sorted_starts) else end_pc + 2
        block_insns: List[IrInstructionJson] = []
        for insn in decoded_insns:
            if bstart <= insn.pc < bend:
                block_insns.append(translate_instruction_to_ir(insn))

        succs = []
        if block_insns:
            last = block_insns[-1]
            if last.target_pc and start_pc <= last.target_pc <= end_pc:
                succs.append(last.target_pc)
            if last.opcode != "Branch" or last.condition != "Always":
                if i + 1 < len(sorted_starts):
                    succs.append(sorted_starts[i + 1])

        blocks.append(IrBasicBlockJson(
            start_pc=bstart,
            end_pc=bend - 2,
            instructions=block_insns,
            successors=succs
        ))

    return IrFunctionJson(
        name=func_name,
        entry_pc=start_pc,
        blocks=blocks
    )


if __name__ == "__main__":
    with open("local-roms/Beyond Oasis (USA).md", "rb") as f:
        rom = f.read()
    fn = build_ir_from_rom(rom, 0x00E682, 0x00E6A8, "pilot_00e682")
    manifest = {
        "schema": "oasis.thor-ir.function.v1",
        "name": fn.name,
        "entry_pc": f"0x{fn.entry_pc:06X}",
        "block_count": len(fn.blocks),
        "blocks": [
            {
                "start_pc": f"0x{b.start_pc:06X}",
                "end_pc": f"0x{b.end_pc:06X}",
                "instruction_count": len(b.instructions),
                "successors": [f"0x{s:06X}" for s in b.successors],
                "instructions": [
                    {
                        "pc": f"0x{ins.original_pc:06X}",
                        "opcode": ins.opcode,
                        "size": ins.size,
                        "condition": ins.condition,
                        "target_pc": f"0x{ins.target_pc:06X}" if ins.target_pc else "0x000000",
                        "cycles": ins.base_cycles,
                        "operands": [
                            {
                                "kind": op.kind,
                                "size": op.size,
                                "reg": f"{op.reg_kind}{op.reg_index}",
                                "imm": hex(op.imm_val),
                                "disp": hex(op.displacement)
                            } for op in ins.operands
                        ]
                    } for ins in b.instructions
                ]
            } for b in fn.blocks
        ]
    }
    out_path = Path("src/tools/thor_ir/pilot_00e682_ir.json")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)
    print(f"Generated Thor IR for pilot 0x00E682 -> {out_path} ({len(fn.blocks)} blocks)")
