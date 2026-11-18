#!/usr/bin/env python3
"""
Extract ROM Entity AI Ali Facing Tracker, Elevation Gate & 4-Way Directional
Melee Attack Hitbox Collision Engine (0x01074A..0x010916, 460 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.474.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_combat_facing_m15_474.json")

RANGE_START = 0x01074A
RANGE_END = 0x010916
RANGE_SIZE = RANGE_END - RANGE_START  # 460 bytes

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    # Unpack 4-direction angle table at 0x01074A
    dir_angles = list(struct.unpack(">4H", rom[0x01074A:0x010752]))
    # Unpack 4-entry jump table at 0x0107D0
    jump_table = list(struct.unpack(">4h", rom[0x0107D0:0x0107D8]))

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    # Disassemble code segment 1: 0x010752..0x0107D0
    for insn in md.disasm(rom[0x010752:0x0107D0], 0x010752):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Disassemble code segment 2: 0x0107D8..0x010916
    for insn in md.disasm(rom[0x0107D8:0x010916], 0x0107D8):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "milestone": "M15.474",
        "title": "ROM Entity AI Ali Facing Tracker, Elevation Gate & 4-Way Directional Melee Attack Hitbox Collision Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "direction_angle_table": {
            "address": "0x01074A",
            "values": [f"0x{v:04X}" for v in dir_angles],
            "descriptions": ["Up (0x0040)", "Right (0x0000)", "Down (0x00C0)", "Left (0x0080)"]
        },
        "jump_table_0x0107D0": {
            "address": "0x0107D0",
            "entries": [
                {"facing": 0, "name": "FacingUp", "offset": jump_table[0], "target": f"0x{0x0107D0 + jump_table[0]:06X}"},
                {"facing": 1, "name": "FacingRight", "offset": jump_table[1], "target": f"0x{0x0107D2 + jump_table[1]:06X}"},
                {"facing": 2, "name": "FacingDown", "offset": jump_table[2], "target": f"0x{0x0107D4 + jump_table[2]:06X}"},
                {"facing": 3, "name": "FacingLeft", "offset": jump_table[3], "target": f"0x{0x0107D6 + jump_table[3]:06X}"}
            ]
        },
        "routines": [
            {
                "name": "AI_FaceTowardsAliAndGetDistance",
                "start": "0x010752",
                "end": "0x0107AA",
                "size_bytes": 88,
                "description": "Reads Ali coordinates ($FF19F0/$FF19F4), determines dominant axis (horizontal vs vertical), sets facing +$16(a6) towards Ali (0=Up, 1=Right, 2=Down, 3=Left), and returns primary distance in d5."
            },
            {
                "name": "AI_CheckMeleeAttackBoxHit",
                "start": "0x0107AA",
                "end": "0x010916",
                "size_bytes": 364,
                "description": "Checks elevation difference abs(ali_z - entity_z) against 24px threshold, dispatches via 4-way facing jump table at 0x0107D0, tests forward reach d2 and lateral spans d0/d1 against Ali hitbox dimensions ($FF1A2A/$FF1A2C). Returns Carry set on hit, Carry clear on miss."
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
