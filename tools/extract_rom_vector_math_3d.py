#!/usr/bin/env python3
"""
extract_rom_vector_math_3d.py - M15.457 ROM Extractor
Extracts 3D Vector Mathematics Engine: Euclidean Distance (2D/3D),
Direction Normalization, Sphere Interception Clamp & Projectile Launch Velocity
from 0x00F054..0x00F1B2 (350 bytes).
"""

import json
import os
import struct
import sys
import capstone

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_vector_math_3d_m15_457.json"

START_ADDR = 0x00F054
END_ADDR = 0x00F1B2
TOTAL_BYTES = END_ADDR - START_ADDR

SUBROUTINES = [
    {
        "name": "Boss3A_RockProjectileLaunchSetup",
        "start": 0x00F054,
        "end": 0x00F0C2,
        "size": 0x00F0C2 - 0x00F054,
        "description": "Calculates delta, checks 8px threshold, selects launch velocity, projects 8-way trajectory"
    },
    {
        "name": "Boss3A_RockVelocityTable",
        "start": 0x00F0C2,
        "end": 0x00F0D2,
        "size": 0x00F0D2 - 0x00F0C2,
        "description": "4-entry table of (speed_xy, launch_z_word) for projectile arcs"
    },
    {
        "name": "Math_EuclideanDistance2D",
        "start": 0x00F0D2,
        "end": 0x00F0EC,
        "size": 0x00F0EC - 0x00F0D2,
        "description": "2D integer Euclidean distance sqrt((x1-x0)^2 + (y1-y0)^2) using integer sqrt"
    },
    {
        "name": "Math_EuclideanDistance3D",
        "start": 0x00F0EC,
        "end": 0x00F10C,
        "size": 0x00F10C - 0x00F0EC,
        "description": "3D integer Euclidean distance sqrt(dx^2 + dy^2 + dz^2) using integer sqrt"
    },
    {
        "name": "Math_VectorNormalize3DVelocity",
        "start": 0x00F10C,
        "end": 0x00F160,
        "size": 0x00F160 - 0x00F10C,
        "description": "3D normalized vector scaled velocity (speed * delta) / dist in 16.16 fixed-point"
    },
    {
        "name": "Math_SphereInterceptionClamp3D",
        "start": 0x00F160,
        "end": 0x00F1B2,
        "size": 0x00F1B2 - 0x00F160,
        "description": "3D sphere ray-intersection & clamp P0 + (P1 - P0) * radius / dist with CCR Carry flag"
    }
]

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
    instructions = []
    for ins in md.disasm(chunk, START_ADDR):
        instructions.append({
            "address": f"0x{ins.address:06X}",
            "mnemonic": ins.mnemonic,
            "op_str": ins.op_str,
            "bytes": ins.bytes.hex()
        })

    # Decode table 0x00F0C2
    tbl_raw = rom_data[0x00F0C2:0x00F0D2]
    tbl_entries = []
    for i in range(4):
        s_xy, l_z = struct.unpack_from(">HH", tbl_raw, i * 4)
        elev_vel = (l_z << 8) if l_z < 0x8000 else ((l_z - 0x10000) << 8)
        tbl_entries.append({
            "index": i,
            "speed_xy": f"0x{s_xy:04X}",
            "launch_z_raw": f"0x{l_z:04X}",
            "launch_z_elev_velocity": f"0x{elev_vel & 0xFFFFFFFF:08X}"
        })

    subroutine_map = {}
    for sub in SUBROUTINES:
        sub_chunk = rom_data[sub["start"]:sub["end"]]
        sub_insns = []
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
        "milestone": "M15.457",
        "title": "ROM 3D Vector Mathematics Engine: Euclidean Distance, Normalization & Sphere Clamp",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": TOTAL_BYTES,
            "total_instructions": len(instructions)
        },
        "velocity_table_00F0C2": tbl_entries,
        "subroutines": subroutine_map,
        "all_instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({TOTAL_BYTES} bytes) to {OUTPUT_JSON}")

if __name__ == "__main__":
    extract()
