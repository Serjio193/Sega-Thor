#!/usr/bin/env python3
"""
Extract ROM Script Virtual Machine Spatial Positioning, Nearest Offscreen Target
& Edge Spawner Subsystem (0x0103B0..0x010594, 484 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.472.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_script_vm_spatial_spawn_m15_472.json")

RANGE_START = 0x0103B0
RANGE_END = 0x010594
RANGE_SIZE = RANGE_END - RANGE_START  # 484 bytes

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
        "milestone": "M15.472",
        "title": "ROM Script Virtual Machine Spatial Positioning, Nearest Offscreen Target & Edge Spawner Subsystem",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": [
            {
                "name": "ScriptSpawn_CopyFixedCoords",
                "start": "0x0103B0",
                "end": "0x0103BE",
                "size_bytes": 14,
                "description": "Copies immediate 16-bit (X, Y) coordinates from script stream (a0)+ into entity slot +$08 and +$0C, returning Carry clear."
            },
            {
                "name": "ScriptSpawn_SelectNearestOffscreenTargetPoint",
                "start": "0x0103BE",
                "end": "0x01046A",
                "size_bytes": 172,
                "description": "Scans 16 coordinate pairs in table $FF17E6, filters out on-screen points within camera viewport, computes Manhattan distance to Player Ali ($FF19F0/$FF19F4), and picks the closest offscreen point."
            },
            {
                "name": "ScriptSpawn_SelectEdgeSpawnPoint",
                "start": "0x01046A",
                "end": "0x010594",
                "size_bytes": 298,
                "description": "Picks random edge 0..3 (Top, Right, Bottom, Left) via PRNG 0x00E2A2, selects offscreen position along camera viewport boundary, and sets facing direction (2=Down, 3=Left, 0=Up, 1=Right)."
            }
        ],
        "jump_table_0x01049E": {
            "address": "0x01049E",
            "entries": [
                {"edge": 0, "name": "TopEdge_FacingDown", "target": "0x0104AC", "facing": 2},
                {"edge": 1, "name": "RightEdge_FacingLeft", "target": "0x0104E2", "facing": 3},
                {"edge": 2, "name": "BottomEdge_FacingUp", "target": "0x010520", "facing": 0},
                {"edge": 3, "name": "LeftEdge_FacingRight", "target": "0x01055E", "facing": 1}
            ]
        },
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")

if __name__ == "__main__":
    extract()
