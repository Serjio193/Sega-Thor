#!/usr/bin/env python3
"""
Extract ROM Entity AI Isometric Projected Melee Hitbox & Reverse-Facing
Collision Detection Engine (0x010916..0x010A74, 350 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.475.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_isometric_hitbox_m15_475.json")

RANGE_START = 0x010916
RANGE_END = 0x010A74
RANGE_SIZE = RANGE_END - RANGE_START  # 350 bytes

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    # Unpack 4-entry jump table at 0x010956
    jump_table = list(struct.unpack(">4h", rom[0x010956:0x01095E]))

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    # Segment 1: 0x010916..0x010956
    for insn in md.disasm(rom[0x010916:0x010956], 0x010916):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Segment 2: 0x01095E..0x010A74
    for insn in md.disasm(rom[0x01095E:0x010A74], 0x01095E):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "milestone": "M15.475",
        "title": "ROM Entity AI Isometric Projected Melee Hitbox & Reverse-Facing Collision Detection Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "jump_table_0x010956": {
            "address": "0x010956",
            "entries": [
                {"facing": 0, "name": "FacingUp", "offset": jump_table[0], "target": f"0x{0x010956 + jump_table[0]:06X}"},
                {"facing": 1, "name": "FacingRight", "offset": jump_table[1], "target": f"0x{0x010958 + jump_table[1]:06X}"},
                {"facing": 2, "name": "FacingDown", "offset": jump_table[2], "target": f"0x{0x01095A + jump_table[2]:06X}"},
                {"facing": 3, "name": "FacingLeft", "offset": jump_table[3], "target": f"0x{0x01095C + jump_table[3]:06X}"}
            ]
        },
        "entrypoints": [
            {
                "address": "0x010916",
                "name": "CheckHit_ReverseFacing_NoElevGate",
                "description": "Inverts facing bit 1 and checks isometric attack box directly without elevation gate."
            },
            {
                "address": "0x010920",
                "name": "CheckHit_NormalFacing_NoElevGate",
                "description": "Checks isometric attack box directly without elevation gate."
            },
            {
                "address": "0x010926",
                "name": "CheckHit_ReverseFacing_WithElevGate",
                "description": "Inverts facing bit 1, tests elevation diff < 24px, and checks isometric attack box."
            },
            {
                "address": "0x010930",
                "name": "CheckHit_NormalFacing_WithElevGate",
                "description": "Tests elevation diff < 24px and checks isometric attack box."
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
