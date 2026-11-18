#!/usr/bin/env python3
"""
Extract ROM Entity AI Enemy Projectile Spawner Subsystem: Elemental Slash Waves,
Fast Arrows & High-Arc Mortar Lob Trajectories (0x010CFE..0x010EEA, 492 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.478.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_projectile_spawner_m15_478.json")

RANGE_START = 0x010CFE
RANGE_END = 0x010EEA
RANGE_SIZE = RANGE_END - RANGE_START  # 492 bytes

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
        "milestone": "M15.478",
        "title": "ROM Entity AI Enemy Projectile Spawner Subsystem: Elemental Slash Waves, Fast Arrows & High-Arc Mortar Lob Trajectories",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": [
            {
                "name": "AI_SpawnEnemySwordSlashWaveProjectile",
                "start": "0x010CFE",
                "end": "0x010DB2",
                "size_bytes": 180,
                "description": "Allocates slot, initializes Type $34, priority $55E, dimensions 6x6x6 (h=8), clones parent coords, sets anchor offsets from table (a4), reads parameters from (a3), projects directional velocities, and sets upward float velocity $C000."
            },
            {
                "name": "AI_SpawnEnemyBowArrow_Fast",
                "start": "0x010DB2",
                "end": "0x010E4E",
                "size_bytes": 156,
                "description": "Allocates slot, initializes Type $3A, sets anchor offsets from (a4), reads 6-byte parameter record from table (a3) indexed by +$96(a5)*6, sets horizontal speed $0400/$0400 and launch velocity $28000."
            },
            {
                "name": "AI_SpawnEnemyBowArrow_HighArc",
                "start": "0x010E4E",
                "end": "0x010EEA",
                "size_bytes": 156,
                "description": "Allocates slot, initializes Type $3A, reads 6-byte parameter record, sets low horizontal speed $0100/$0100 and high upward launch velocity $40000 for steep mortar lob trajectory."
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
