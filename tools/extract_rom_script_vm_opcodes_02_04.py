#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Opcodes $02..$04: Parameter Initializers &
Relational Wait-Condition Engine (0x00F920..0x00FAAC, 396 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.464.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_opcodes_02_04_m15_464.json")

RANGE_START = 0x00F920
RANGE_END = 0x00FAAC
RANGE_SIZE = RANGE_END - RANGE_START  # 396 bytes

OPCODE4_OPERATOR_TABLE_BASE = 0x00F9EC
OPCODE4_OPERATOR_COUNT = 6

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    # Extract operator jump table at 0x00F9EC
    operators = []
    op_names = ["Equal", "NotEqual", "Greater", "GreaterEqual", "LessEqual", "Less"]
    for i in range(OPCODE4_OPERATOR_COUNT):
        entry_addr = OPCODE4_OPERATOR_TABLE_BASE + i * 2
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
        "milestone": "M15.464",
        "title": "ROM Script Virtual Machine Opcodes $02..$04: Parameter Initializers & Relational Wait-Condition Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "operator_table": {
            "base_address": f"0x{OPCODE4_OPERATOR_TABLE_BASE:06X}",
            "count": OPCODE4_OPERATOR_COUNT,
            "entries": operators
        },
        "opcodes": [
            {
                "opcode": "0x02",
                "name": "ScriptOp_SetParameterWordBuffer",
                "start": "0x00F920",
                "end": "0x00F942",
                "size_bytes": 34,
                "description": (
                    "Reads word stream terminated by $FFFF from script stream (a0)+ and writes up to 16 words "
                    "into parameter buffer at $FF17C6."
                ),
                "constants": {
                    "dest_buffer": "0xFF17C6",
                    "max_words": 16,
                    "terminator": "0xFFFF"
                }
            },
            {
                "opcode": "0x03",
                "name": "ScriptOp_SetTargetCoordinatePairTable",
                "start": "0x00F942",
                "end": "0x00F96A",
                "size_bytes": 40,
                "description": (
                    "Reads coordinate/target word pairs from script stream into $FF17E6 (up to 16 pairs / 32 bytes). "
                    "If $FFFF is encountered, fills remaining pairs with $FFFF."
                ),
                "constants": {
                    "dest_buffer": "0xFF17E6",
                    "max_pairs": 16,
                    "terminator": "0xFFFF"
                }
            },
            {
                "opcode": "0x04",
                "name": "ScriptOp_WaitUntilCondition",
                "start": "0x00F96A",
                "end": "0x00FAAC",
                "size_bytes": 322,
                "description": (
                    "Reads variable selector d0.b, comparison operator d1.b, and compare value / bit index d2.w. "
                    "Resolves variable value: d0=0 calls $0101EC (quest var), d0=1..3 tests bit d2 in persistent bitmask "
                    "banks $FF0DBC / $FF0E1C / $FF0E2C (with polarity d1), d0=4/5 reads Ali X ($FF19F0) / Y ($FF19F4), "
                    "d0=6/7 reads Spirit X ($FF1AAC) / Y ($FF1AB0) when active ($FF1AA4 == $1A). Evaluates relational "
                    "comparison (==, !=, >, >=, <=, <). If true, exits handler to advance script; if false, yields frame "
                    "via $F91C so condition is re-evaluated next frame."
                ),
                "constants": {
                    "bank1_bits": "0xFF0DBC",
                    "bank2_bits": "0xFF0E1C",
                    "bank3_bits": "0xFF0E2C",
                    "ali_x": "0xFF19F0",
                    "ali_y": "0xFF19F4",
                    "spirit_type": "0x001A",
                    "spirit_status": "0xFF1AA4",
                    "spirit_x": "0xFF1AAC",
                    "spirit_y": "0xFF1AB0"
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
