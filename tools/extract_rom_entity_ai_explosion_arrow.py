#!/usr/bin/env python3
"""
Extract ROM Entity AI Environmental Explosion Waves, Screen Shake
& Arrow Proximity AABB Interceptor (0x0114AE..0x0115C8, 282 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.482.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_explosion_arrow_m15_482.json")

RANGE_START = 0x0114AE
RANGE_END = 0x0115C8
RANGE_SIZE = RANGE_END - RANGE_START  # 282 bytes

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

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
        "milestone": "M15.482",
        "title": "ROM Entity AI Environmental Explosion Waves, Screen Shake & Arrow Proximity AABB Interceptor",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": [
            {
                "name": "AI_SpawnEnvironmentalExplosionWave",
                "start": "0x0114AE",
                "end": "0x011508",
                "size_bytes": 90,
                "description": "Computes centered explosion coordinates from Plane A ($FF1716) and Plane B ($FF173E), adds random scatter via 0x00E268, spawns particle wave 0x00E90E with radius $14, and sets screen shake $FF17C4 = (frame & 1) + 2."
            },
            {
                "name": "AI_SpawnEntityGroundPoundImpactWave",
                "start": "0x011508",
                "end": "0x011554",
                "size_bytes": 76,
                "description": "Takes entity world coordinates +$8/+$C(a6), elevation d3=$20, scatters (x, y, z) by (0x00E268 >> 2), spawns particle wave 0x00E90E, and sets screen shake $FF17C4 = (frame & 1) + 2."
            },
            {
                "name": "AI_CheckBowArrowProximityHitbox",
                "start": "0x011554",
                "end": "0x0115B6",
                "size_bytes": 98,
                "description": "Inspects projectile slot $FF1B60 for Type $3A (Bow Arrow), active attack +$2A!=0, and matching elevation +$14(a0)==+$14(a6). Tests 2D AABB bounding box radius d0 around entity (+-d0 on X and Y), returning Carry set on hit or Carry clear on miss."
            },
            {
                "name": "AI_JumpToProjectileTrigVelocity",
                "start": "0x0115B6",
                "end": "0x0115C8",
                "size_bytes": 18,
                "description": "Fetches projectile coordinates from $FF1B68 and $FF1B6C into d1/d2 and jumps to trigonometric velocity resolver 0x00E3A4."
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
