#!/usr/bin/env python3
"""
Extract ROM Entity AI Player Elemental Blade Attack Parry & Shield Interception Engine
(0x010A74..0x010BF0, 380 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.476.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_blade_parry_m15_476.json")

RANGE_START = 0x010A74
RANGE_END = 0x010BF0
RANGE_SIZE = RANGE_END - RANGE_START  # 380 bytes

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    # Unpack 4-entry jump table at 0x010ADA
    jump_table = list(struct.unpack(">4h", rom[0x010ADA:0x010AE2]))

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    # Segment 1: 0x010A74..0x010ADA
    for insn in md.disasm(rom[0x010A74:0x010ADA], 0x010A74):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Segment 2: 0x010AE2..0x010BF0
    for insn in md.disasm(rom[0x010AE2:0x010BF0], 0x010AE2):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "milestone": "M15.476",
        "title": "ROM Entity AI Player Elemental Blade Attack Parry & Shield Interception Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "jump_table_0x010ADA": {
            "address": "0x010ADA",
            "entries": [
                {"facing": 0, "name": "FacingUp", "offset": jump_table[0], "target": f"0x{0x010ADA + jump_table[0]:06X}"},
                {"facing": 1, "name": "FacingRight", "offset": jump_table[1], "target": f"0x{0x010ADC + jump_table[1]:06X}"},
                {"facing": 2, "name": "FacingDown", "offset": jump_table[2], "target": f"0x{0x010ADE + jump_table[2]:06X}"},
                {"facing": 3, "name": "FacingLeft", "offset": jump_table[3], "target": f"0x{0x010AE0 + jump_table[3]:06X}"}
            ]
        },
        "routines": [
            {
                "name": "AI_CheckBladeParryShieldHit",
                "start": "0x010A74",
                "end": "0x010BF0",
                "size_bytes": 380,
                "description": "Checks if projectile slot $FF1B60 has active Elemental Blade wave (Type $34 in state 2 or 4), opposite facing direction, vertical 3D elevation overlap, and 4-way lateral span/forward reach bounding box intersection. Returns Carry set if blade is parried/intercepted by shield, Carry clear on miss."
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
