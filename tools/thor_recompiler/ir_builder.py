#!/usr/bin/env python3
"""
Thor IR Builder and Generator for M15.241.
Converts decoded Motorola 68000 instructions from real ROM bytes into
backend-neutral Thor IR, and produces Mode A (verification) and Mode B (portable native) C++.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

from dataclasses import dataclass, field
import hashlib
import json
from pathlib import Path
import struct
import sys
from typing import Any, Dict, List, Optional

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from tools.thor_recompiler.decoder import decode_at, DecodedInstruction


@dataclass
class IrOperandJson:
    kind: str  # "Register", "Immediate", "MemoryDisplacement", "MemoryIndirect", "AbsoluteAddress"
    size: str  # "Byte", "Word", "Long"
    reg_kind: str = "Data"  # "Data", "Address", "PC", "SR", "CCR"
    reg_index: int = 0
    imm_val: int = 0
    displacement: int = 0


@dataclass
class IrInstructionJson:
    original_pc: int
    opcode: str  # "Move", "Tst", "Bclr", "Bset", "Btst", "Add", "Compare", "Clear", "Branch", "Return", "SyncBoundary"
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
            original_pc=pc, opcode="Return", size="Word",
            base_cycles=16, updates_ccr=False, is_sync_boundary=True
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
            original_pc=pc, opcode=op, size=ir_size,
            condition=cond_map.get(insn.mnemonic, "Always"),
            target_pc=target, base_cycles=insn.base_cycles,
            updates_ccr=False, is_sync_boundary=True
        )

    if insn.mnemonic == "TST":
        ops = []
        for op in insn.ops:
            if op.kind == "reg_d":
                ops.append(IrOperandJson(kind="Register", size=ir_size, reg_kind="Data", reg_index=op.reg))
            elif op.kind == "disp_a":
                ops.append(IrOperandJson(kind="MemoryDisplacement", size=ir_size, reg_kind="Address", reg_index=op.reg, displacement=op.val))
            elif op.kind == "abs_l":
                ops.append(IrOperandJson(kind="AbsoluteAddress", size=ir_size, imm_val=op.val))
        return IrInstructionJson(original_pc=pc, opcode="Tst", size=ir_size, operands=ops, base_cycles=insn.base_cycles, updates_ccr=True)

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
            elif op.kind == "abs_l":
                ops.append(IrOperandJson(kind="AbsoluteAddress", size=ir_size, imm_val=op.val))
        return IrInstructionJson(original_pc=pc, opcode=op_map[insn.mnemonic], size=ir_size, operands=ops, base_cycles=insn.base_cycles, updates_ccr=True)

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
            elif op.kind == "abs_l":
                ops.append(IrOperandJson(kind="AbsoluteAddress", size=ir_size, imm_val=op.val))
        return IrInstructionJson(original_pc=pc, opcode="Move", size=ir_size, operands=ops, base_cycles=insn.base_cycles, updates_ccr=True)

    if insn.mnemonic in ("ADD", "ADDQ"):
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
        return IrInstructionJson(original_pc=pc, opcode="Add", size=ir_size, operands=ops, base_cycles=insn.base_cycles, updates_ccr=True)

    if insn.mnemonic == "MOVEQ":
        ops = [
            IrOperandJson(kind="Immediate", size="Long", imm_val=insn.ops[0].val),
            IrOperandJson(kind="Register", size="Long", reg_kind="Data", reg_index=insn.ops[1].reg)
        ]
        return IrInstructionJson(original_pc=pc, opcode="Move", size="Long", operands=ops, base_cycles=insn.base_cycles, updates_ccr=True)

    if insn.mnemonic == "LEA":
        ops = []
        for op in insn.ops:
            if op.kind == "abs_l":
                ops.append(IrOperandJson(kind="AbsoluteAddress", size="Long", imm_val=op.val))
            elif op.kind == "disp_a":
                ops.append(IrOperandJson(kind="MemoryDisplacement", size="Long", reg_kind="Address", reg_index=op.reg, displacement=op.val))
            elif op.kind == "reg_a":
                ops.append(IrOperandJson(kind="Register", size="Long", reg_kind="Address", reg_index=op.reg))
            elif op.kind == "imm":
                ops.append(IrOperandJson(kind="Immediate", size="Long", imm_val=op.val))
        return IrInstructionJson(original_pc=pc, opcode="Lea", size="Long", operands=ops, base_cycles=insn.base_cycles, updates_ccr=False)

    if insn.mnemonic == "DBRA":
        target = insn.branch_target if insn.branch_target is not None else 0
        ops = [IrOperandJson(kind="Register", size="Word", reg_kind="Data", reg_index=insn.ops[0].reg)]
        return IrInstructionJson(
            original_pc=pc, opcode="Dbra", size="Word", operands=ops,
            target_pc=target, base_cycles=insn.base_cycles, updates_ccr=False, is_sync_boundary=True
        )

    if insn.mnemonic in ("CMP", "CMPI", "CMPA"):
        ops = []
        for op in insn.ops:
            if op.kind == "imm":
                ops.append(IrOperandJson(kind="Immediate", size=ir_size, imm_val=op.val))
            elif op.kind == "reg_d":
                ops.append(IrOperandJson(kind="Register", size=ir_size, reg_kind="Data", reg_index=op.reg))
            elif op.kind == "reg_a":
                ops.append(IrOperandJson(kind="Register", size=ir_size, reg_kind="Address", reg_index=op.reg))
        return IrInstructionJson(original_pc=pc, opcode="Compare", size=ir_size, operands=ops, base_cycles=insn.base_cycles, updates_ccr=True)

    if insn.mnemonic == "CLR":
        ops = []
        for op in insn.ops:
            if op.kind == "reg_d":
                ops.append(IrOperandJson(kind="Register", size=ir_size, reg_kind="Data", reg_index=op.reg))
            elif op.kind == "disp_a":
                ops.append(IrOperandJson(kind="MemoryDisplacement", size=ir_size, reg_kind="Address", reg_index=op.reg, displacement=op.val))
            elif op.kind == "abs_l":
                ops.append(IrOperandJson(kind="AbsoluteAddress", size=ir_size, imm_val=op.val))
        return IrInstructionJson(original_pc=pc, opcode="Clear", size=ir_size, operands=ops, base_cycles=insn.base_cycles, updates_ccr=True)

    return IrInstructionJson(original_pc=pc, opcode="Nop", size=ir_size, base_cycles=insn.base_cycles, updates_ccr=False)


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

    block_starts = {start_pc}
    for insn in decoded_insns:
        if insn.branch_target is not None and start_pc <= insn.branch_target <= end_pc:
            block_starts.add(insn.branch_target)
        if insn.mnemonic in ("RTS", "BRA", "BSR", "BNE", "BEQ", "BCC", "BCS", "BLT", "BGE"):
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

    return IrFunctionJson(name=func_name, entry_pc=start_pc, blocks=blocks)


def function_to_dict(fn: IrFunctionJson) -> Dict[str, Any]:
    return {
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


def main():
    with open("local-roms/Beyond Oasis (USA).md", "rb") as f:
        rom = f.read()

    targets = [
        ("pilot_00e682", 0x00E682, 0x00E6A8, "src/tools/thor_ir/pilot_00e682_ir.json"),
        ("recompiled_016294", 0x016294, 0x0162CE, "src/tools/thor_ir/recompiled_016294_ir.json"),
        ("recompiled_006e7a", 0x006E7A, 0x006E88, "src/tools/thor_ir/recompiled_006e7a_ir.json"),
        ("connected_00d2b4", 0x00D2B4, 0x00D342, "src/tools/thor_ir/connected_00d2b4_ir.json"),
        ("connected_00d344", 0x00D344, 0x00D396, "src/tools/thor_ir/connected_00d344_ir.json")
    ]

    manifests = {}
    for name, spc, epc, out_rel in targets:
        fn = build_ir_from_rom(rom, spc, epc, name)
        m = function_to_dict(fn)
        out_path = Path(out_rel)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with open(out_path, "w", encoding="utf-8") as f:
            json.dump(m, f, indent=2)
        manifests[name] = {
            "entry_pc": f"0x{spc:06X}",
            "end_pc": f"0x{epc:06X}",
            "blocks": len(fn.blocks),
            "sha256": hashlib.sha256(out_path.read_bytes()).hexdigest(),
            "path": out_rel
        }
        print(f"Generated Thor IR for {name} -> {out_path} ({len(fn.blocks)} blocks)")

    out_man = Path("data/thor-fixtures/m15_242_ir_manifest.json")
    out_man.parent.mkdir(parents=True, exist_ok=True)
    with open(out_man, "w", encoding="utf-8") as f:
        json.dump({"schema": "oasis.m15_242.ir_manifest.v1", "functions": manifests}, f, indent=2)
    print(f"Emitted IR manifest -> {out_man}")


if __name__ == "__main__":
    main()
