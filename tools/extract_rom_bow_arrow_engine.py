#!/usr/bin/env python3
"""
extract_rom_bow_arrow_engine.py - M15.460 ROM Extractor
Extracts Player Ali Bow Arrow Projectile Spawner, Ground/Water Offsets
& Parabolic Launch Physics Subsystem from 0x00F396..0x00F476 (224 bytes).
"""

import json
import os
import struct
import sys
import capstone

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_bow_arrow_engine_m15_460.json"

START_ADDR = 0x00F396
END_ADDR = 0x00F476
TOTAL_BYTES = END_ADDR - START_ADDR

SUBROUTINES = [
    {
        "name": "BowArrow_GroundEntry",
        "start": 0x00F396,
        "end": 0x00F3A0,
        "size": 0x00F3A0 - 0x00F396,
        "description": "Durability check 0x008530, ground offset table pointer, branch to shared initializer"
    },
    {
        "name": "BowArrow_GroundOffsetsTable",
        "start": 0x00F3A0,
        "end": 0x00F3A8,
        "size": 0x00F3A8 - 0x00F3A0,
        "description": "4-entry (dx, dy) arrow launch offset table for ground standing bow shot"
    },
    {
        "name": "BowArrow_WaterEntry",
        "start": 0x00F3A8,
        "end": 0x00F3B2,
        "size": 0x00F3B2 - 0x00F3A8,
        "description": "Durability check 0x008530, water offset table pointer, branch to shared initializer"
    },
    {
        "name": "BowArrow_WaterOffsetsTable",
        "start": 0x00F3B2,
        "end": 0x00F3BA,
        "size": 0x00F3BA - 0x00F3B2,
        "description": "4-entry (dx, dy) arrow launch offset table for water swimming bow shot"
    },
    {
        "name": "BowArrow_SharedInitializer",
        "start": 0x00F3BA,
        "end": 0x00F476,
        "size": 0x00F476 - 0x00F3BA,
        "description": "Allocates slot $FF1B60, type #$3A, copies Ali sprite priority, calculates facing subpixel anchors, evaluates elevation threshold for high/low launch arcs, projects 8-way directional velocity via 0x00E2D4"
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
    ground_tbl = decode_offset_table(rom_data[0x00F3A0:0x00F3A8])
    # Water table
    water_tbl = decode_offset_table(rom_data[0x00F3B2:0x00F3BA])

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
        "milestone": "M15.460",
        "title": "ROM Player Ali Bow Arrow Projectile Spawner, Ground/Water Offsets & Parabolic Launch Physics",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": TOTAL_BYTES
        },
        "constants": {
            "projectile_slot_ram": "0x00FF1B60",
            "projectile_type": "0x003A",
            "damage_stat": "0x8005",
            "elevation_threshold": 16,
            "high_elevation_arc": {
                "speed_horizontal": "0x0300",
                "launch_z_velocity": "0x00020000"
            },
            "low_elevation_arc": {
                "speed_horizontal": "0x0400",
                "launch_z_velocity": "0x00028000"
            }
        },
        "ground_launch_offsets": ground_tbl,
        "water_launch_offsets": water_tbl,
        "subroutines": subroutine_map
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.460 evidence ({TOTAL_BYTES} bytes) to {OUTPUT_JSON}")

if __name__ == "__main__":
    extract()
