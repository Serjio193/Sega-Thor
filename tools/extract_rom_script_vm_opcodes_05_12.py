#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Opcodes $05 & $12: Unconditional Jump &
Conditional Branch Engine (0x00FAAC..0x00FC6C, 448 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.465.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_opcodes_05_12_m15_465.json")

RANGE_START = 0x00FAAC
RANGE_END = 0x00FC6C
RANGE_SIZE = RANGE_END - RANGE_START  # 448 bytes

MODE_TABLE_BASE = 0x00FAC2
MODE_COUNT = 12

OPERATOR_TABLE_BASE = 0x00FB64
OPERATOR_COUNT = 6

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    # Extract 12-mode jump table at 0x00FAC2
    modes = []
    mode_names = [
        "QuestVar", "Bank1Bits", "Bank2Bits", "Bank3Bits",
        "ItemPossession", "Sub10254", "Sub10268", "SpiritStatus",
        "AliX", "AliY", "SpiritX", "SpiritY"
    ]
    for i in range(MODE_COUNT):
        entry_addr = MODE_TABLE_BASE + i * 2
        rel_offset = struct.unpack(">h", rom[entry_addr:entry_addr + 2])[0]
        target_addr = entry_addr + rel_offset
        modes.append({
            "mode_id": i,
            "name": mode_names[i],
            "table_addr": f"0x{entry_addr:06X}",
            "rel_offset": rel_offset,
            "target_addr": f"0x{target_addr:06X}"
        })

    # Extract 6-operator jump table at 0x00FB64
    operators = []
    op_names = ["Equal", "NotEqual", "Greater", "GreaterEqual", "LessEqual", "Less"]
    for i in range(OPERATOR_COUNT):
        entry_addr = OPERATOR_TABLE_BASE + i * 2
        rel_offset = struct.unpack(">h", rom[entry_addr:entry_addr + 2])[0]
        target_addr = entry_addr + rel_offset
        operators.append({
            "operator_id": i,
            "name": op_names[i],
            "table_addr": f"0x{entry_addr:06X}",
            "rel_offset": rel_offset,
            "target_addr": f"0x{target_addr:06X}"
        })

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "milestone": "M15.465",
        "title": "ROM Script Virtual Machine Opcodes $05 & $12: Unconditional Jump & Conditional Branch Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "mode_table": {
            "base_address": f"0x{MODE_TABLE_BASE:06X}",
            "count": MODE_COUNT,
            "entries": modes
        },
        "operator_table": {
            "base_address": f"0x{OPERATOR_TABLE_BASE:06X}",
            "count": OPERATOR_COUNT,
            "entries": operators
        },
        "opcodes": [
            {
                "opcode": "0x05",
                "name": "ScriptOp_UnconditionalBranch",
                "start": "0x00FAAC",
                "end": "0x00FAB2",
                "size_bytes": 6,
                "description": "Adds 16-bit word displacement at +2(a0) directly to script PC a0 (adda.w $2(a0), a0; rts)."
            },
            {
                "opcode": "0x12",
                "name": "ScriptOp_ConditionalBranch",
                "start": "0x00FAB2",
                "end": "0x00FC6C",
                "size_bytes": 442,
                "description": (
                    "Reads mode selector +2(a0), operator +3(a0), compare value / bit index +4(a0), and branch displacement +6(a0). "
                    "Dispatches through 12-entry mode table at $FAC2. Evaluates relational comparison (table at $FB64) or bit tests. "
                    "If condition is true, branches by +6(a0) (adda.w $6(a0), a0; rts). If false, advances past instruction "
                    "(lea.l $8(a0), a0; rts)."
                ),
                "constants": {
                    "taken_target_offset": "0x00FBA0",
                    "not_taken_target_offset": "0x00FBA6",
                    "instruction_size_bytes": 8
                }
            }
        ],
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")

if __name__ == "__main__":
    extract()
