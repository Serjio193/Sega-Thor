#!/usr/bin/env python3
"""
Extract ROM Spirit Entity Orb Kinematics, Trail Table & Diamond Vertex Generator
(0x0115C8..0x01171C, 340 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.483.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_spirit_orb_kinematics_m15_483.json")

RANGE_START = 0x0115C8
RANGE_END = 0x01171C
RANGE_SIZE = RANGE_END - RANGE_START  # 340 bytes

TABLE_START = 0x0116D2
TABLE_END = 0x0116E4

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # Part 1 code: 0x0115C8..0x0116D2
    insns_part1 = []
    for insn in md.disasm(rom[RANGE_START:TABLE_START], RANGE_START):
        insns_part1.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Embedded data table: 0x0116D2..0x0116E4 (18 bytes)
    table_bytes = list(rom[TABLE_START:TABLE_END])

    # Part 2 code: 0x0116E4..0x01171C
    insns_part2 = []
    for insn in md.disasm(rom[TABLE_END:RANGE_END], TABLE_END):
        insns_part2.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    all_instructions = insns_part1 + insns_part2

    evidence = {
        "milestone": "M15.483",
        "title": "ROM Spirit Entity Orb Kinematics, Trail Table & Diamond Vertex Generator",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "table": {
            "address": f"0x{TABLE_START:06X}",
            "size_bytes": len(table_bytes),
            "values": table_bytes
        },
        "routines": [
            {
                "name": "AI_SpiritOrb_State2_FrictionKinematics",
                "start": "0x0115D8",
                "end": "0x011646",
                "size_bytes": 110,
                "description": "State 2: Decelerates 32-bit fixed velocities +$4E/+$52 by $18000 towards zero, integrates position into +$72/+$76, advances timer +$2(a6), looks up scale table 0x0116D2, and invokes diamond vertex builder 0x0116E4."
            },
            {
                "name": "AI_SpiritOrb_State1_OrbAnimationAndTargetSnap",
                "start": "0x011646",
                "end": "0x0116D2",
                "size_bytes": 140,
                "description": "State 1: Advances animation timer +$2(a6); for frames 0..8 uses forward table indexing; for frames 9..26 uses reverse indexing (26 - frame); at frame 27+, checks $FF19A6==3, snaps to target $FF19E8, and plays warp SFX $80022F."
            },
            {
                "name": "AI_SpiritOrb_BuildDiamondVertexBuffer",
                "start": "0x0116E4",
                "end": "0x01171C",
                "size_bytes": 56,
                "description": "Generates 12-byte 4-point diamond vertex buffer at (a2) using (frame % 3) + 11 and scale parameter d1: writes (d0, d1), (d1, d0), (d1, -d1), (d0, -d1), etc.; sets vertex count 3 at (a1), sprite animation $16 at +$6(a6), and stores buffer pointer in +$62(a6)."
            }
        ],
        "instruction_count": len(all_instructions),
        "instructions": all_instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(all_instructions)} instructions ({RANGE_SIZE} bytes, 18-byte table) to {OUTPUT_PATH}")

if __name__ == "__main__":
    extract()
