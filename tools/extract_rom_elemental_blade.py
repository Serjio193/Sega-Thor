#!/usr/bin/env python3
"""
extract_rom_elemental_blade.py - M15.459 ROM Extractor
Extracts Elemental Sword Wave Projectile Spawner, Parameter Evaluator
& 8-Way Directional Trajectory Subsystem from 0x00F258..0x00F396 (318 bytes).
"""

import json
import os
import struct
import sys
import capstone

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_elemental_blade_m15_459.json"

START_ADDR = 0x00F258
END_ADDR = 0x00F396
TOTAL_BYTES = END_ADDR - START_ADDR

SUBROUTINES = [
    {
        "name": "ElementalBlade_GroundEntry",
        "start": 0x00F258,
        "end": 0x00F262,
        "size": 0x00F262 - 0x00F258,
        "description": "Durability tick 0x008530, ground offset table pointer, jump to shared initializer"
    },
    {
        "name": "ElementalBlade_GroundFacingOffsetsTable",
        "start": 0x00F262,
        "end": 0x00F26A,
        "size": 0x00F26A - 0x00F262,
        "description": "4-entry (dx, dy) sword-tip offset table for ground 360 spin"
    },
    {
        "name": "ElementalBlade_WaterEntry",
        "start": 0x00F26A,
        "end": 0x00F274,
        "size": 0x00F274 - 0x00F26A,
        "description": "Durability tick 0x008530, water offset table pointer, jump to shared initializer"
    },
    {
        "name": "ElementalBlade_WaterFacingOffsetsTable",
        "start": 0x00F274,
        "end": 0x00F27C,
        "size": 0x00F27C - 0x00F274,
        "description": "4-entry (dx, dy) sword-tip offset table for water 360 spin"
    },
    {
        "name": "ElementalBlade_SharedInitializer",
        "start": 0x00F27C,
        "end": 0x00F396,
        "size": 0x00F396 - 0x00F27C,
        "description": "Allocates slot $FF1B60, type #$34, decodes sword type bits (Fire/Omega/Hyper), 8-way velocity projection 0x00E2D4, anchor calculation, 240-frame lifetime"
    }
]

def decode_offset_table(raw_bytes):
    entries = []
    for i in range(4):
        w = struct.unpack_from(">h", raw_bytes, i * 2)[0]
        dx = (w >> 8) if (w >> 8) < 128 else ((w >> 8) - 256)
        dy = (w & 0xFF) if (w & 0xFF) < 128 else ((w & 0xFF) - 256)
        entries.append({
            "facing": i,
            "facing_name": ["Up", "Right", "Down", "Left"][i],
            "raw_word": f"0x{w & 0xFFFF:04X}",
            "offset_dx": dx,
            "offset_dy": dy
        })
    return entries

def extract():
    if not os.path.isfile(ROM_PATH):
        print(f"Error: ROM file not found at {ROM_PATH}")
        sys.exit(1)

    with open(ROM_PATH, "rb") as f:
        rom_data = f.read()

    chunk = rom_data[START_ADDR:END_ADDR]
    if len(chunk) != TOTAL_BYTES:
        print(f"Error: Expected {TOTAL_BYTES} bytes, got {len(chunk)}")
        sys.exit(1)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    
    # Ground table
    ground_tbl = decode_offset_table(rom_data[0x00F262:0x00F26A])
    # Water table
    water_tbl = decode_offset_table(rom_data[0x00F274:0x00F27C])

    subroutine_map = {}
    for sub in SUBROUTINES:
        sub_chunk = rom_data[sub["start"]:sub["end"]]
        sub_insns = []
        if "Table" not in sub["name"]:
            for ins in md.disasm(sub_chunk, sub["start"]):
                sub_insns.append({
                    "address": f"0x{ins.address:06X}",
                    "mnemonic": ins.mnemonic,
                    "op_str": ins.op_str,
                    "bytes": ins.bytes.hex()
                })
        subroutine_map[sub["name"]] = {
            "meta": sub,
            "instruction_count": len(sub_insns),
            "instructions": sub_insns
        }

    evidence = {
        "milestone": "M15.459",
        "title": "ROM Elemental Sword Wave Projectile Spawner, Parameter Evaluator & Trajectory Engine",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": TOTAL_BYTES
        },
        "constants": {
            "projectile_slot_ram": "0x00FF1B60",
            "projectile_type": "0x0034",
            "sprite_priority": "0x055E",
            "hitbox_radii": {"rx": 6, "ry": 6, "rz": 6, "height": 8},
            "elevation_float_velocity": "0x0000C000",
            "lifetime_frames": 240,
            "sword_types": {
                "fire_sword": {
                    "bit_mask": "0x80000000",
                    "damage": "0x2004",
                    "anim_id": 4,
                    "speed_cardinal": "0x0C00",
                    "speed_diagonal": "0x0BC0"
                },
                "omega_sword": {
                    "bit_mask": "0x40000000",
                    "damage": "0x8005",
                    "piercing_flag": "0xFF",
                    "speed_cardinal": "0x1000",
                    "speed_diagonal": "0x0F00"
                },
                "hyper_sword": {
                    "bit_mask": "0x20000000",
                    "damage": "0x0000",
                    "anim_id": 4,
                    "speed_cardinal": "0x0E00",
                    "speed_diagonal": "0x0D00"
                }
            }
        },
        "ground_facing_offsets": ground_tbl,
        "water_facing_offsets": water_tbl,
        "subroutines": subroutine_map
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.459 evidence ({TOTAL_BYTES} bytes) to {OUTPUT_JSON}")

if __name__ == "__main__":
    extract()
