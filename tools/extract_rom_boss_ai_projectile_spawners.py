#!/usr/bin/env python3
"""
Extract ROM Boss AI Feather/Spore & 3-Way Fireball Spread Projectile Spawners
(0x015136..0x015316, 480 bytes).

Disassembles exact M68K instructions, unpacks embedded 24-byte spawn offset tables,
structures metadata, and generates evidence JSON for Milestone M15.507.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_ai_projectile_spawners_m15_507.json"
)

RANGE_START = 0x015136
RANGE_END = 0x015316
RANGE_SIZE = RANGE_END - RANGE_START  # 480 bytes

TABLE1_START = 0x0151FC
TABLE1_END = 0x015214  # 24 bytes (3 entries * 8 bytes)

CODE2_START = 0x015214
TABLE2_START = 0x0152FE
TABLE2_END = 0x015316  # 24 bytes (3 entries * 8 bytes)


def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # Disassemble Code 1: 0x015136..0x0151FC (198 bytes)
    insns_part1 = []
    for insn in md.disasm(rom[RANGE_START:TABLE1_START], RANGE_START):
        insns_part1.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Unpack Table 1: 0x0151FC..0x015214 (3 entries of 8 bytes: dx, dy, vx_hi, vy_hi)
    table1_entries = []
    for i in range(3):
        entry_addr = TABLE1_START + i * 8
        dx, dy, vx_hi, vy_hi = struct.unpack_from(">hhhh", rom, entry_addr)
        table1_entries.append({
            "facing": i + 1,
            "address": f"0x{entry_addr:06X}",
            "dx": dx,
            "dy": dy,
            "vx_hi": vx_hi,
            "vy_hi": vy_hi
        })

    # Disassemble Code 2: 0x015214..0x0152FE (234 bytes)
    insns_part2 = []
    for insn in md.disasm(rom[CODE2_START:TABLE2_START], CODE2_START):
        insns_part2.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Unpack Table 2: 0x0152FE..0x015316 (3 entries of 8 bytes: dx, dy, base_angle, pad)
    table2_entries = []
    for i in range(3):
        entry_addr = TABLE2_START + i * 8
        dx, dy, base_angle, pad = struct.unpack_from(">hhhh", rom, entry_addr)
        table2_entries.append({
            "facing": i + 1,
            "address": f"0x{entry_addr:06X}",
            "dx": dx,
            "dy": dy,
            "base_angle": base_angle,
            "pad": pad
        })

    all_instructions = insns_part1 + insns_part2

    routines = [
        {
            "name": "AI_Boss_SpawnFeatherSporeProjectile",
            "start": "0x015136",
            "end": "0x0151FC",
            "size_bytes": TABLE1_START - RANGE_START,
            "description": (
                "Spawns single feather/spore projectile (type $24) into Pool 2 via 0x00DA2A and 0x008D06. "
                "Sets tile $200, 8px bounds extents, size 8, flying/invulnerable flags +$38:3/:2, "
                "sorting layer 2, anim 6, State $0C. Launches upward with vz = -$50000. Indexes "
                "3-entry 8B table at 0x0151FC by (parent_facing - 1) * 8 to obtain (dx, dy, vx_hi, vy_hi), "
                "adding to parent XY position and velocity. Sets z = parent_z + 0x10 and calls 0x009D00."
            )
        },
        {
            "name": "AI_Boss_Spawn3WayFireballSpread",
            "start": "0x015214",
            "end": "0x0152FE",
            "size_bytes": TABLE2_START - CODE2_START,
            "description": (
                "Spawns 3-way spread fireball projectiles (type $24) with angle offsets d6 = 0 (center), "
                "+$1C (right), -$1C (left). Allocates slot via 0x00DA2A, sets tile $200, bounds 5px, "
                "height 8px, anim $14, State $12. Indexes 3-entry 8B table at 0x0152FE for (dx, dy, base_angle), "
                "evaluates sin/cos of (base_angle + d6) from ROM $5D706 scaled by $600 into +$4E/+$52. "
                "Sets vz = -$C000 and calls 0x009D00."
            )
        }
    ]

    evidence = {
        "milestone": "M15.507",
        "title": "ROM Boss AI Feather/Spore & 3-Way Fireball Spread Projectile Spawners",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "tables": {
            "feather_spawn_table": {
                "address": f"0x{TABLE1_START:06X}",
                "size_bytes": TABLE1_END - TABLE1_START,
                "entries": table1_entries
            },
            "fireball_spawn_table": {
                "address": f"0x{TABLE2_START:06X}",
                "size_bytes": TABLE2_END - TABLE2_START,
                "entries": table2_entries
            }
        },
        "routines": routines,
        "instruction_count": len(all_instructions),
        "instructions": all_instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(
        f"Extracted {len(all_instructions)} instructions ({RANGE_SIZE} bytes, "
        f"2 embedded tables) across {len(routines)} routines to {OUTPUT_PATH}"
    )


if __name__ == "__main__":
    extract()
