#!/usr/bin/env python3
"""
Extract ROM Entity AI Trigonometric Tracking, Smooth Angle Turn-Rate Limiter
& 2D Sin/Cos Velocity Projection Engine (0x010594..0x01074A, 438 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.473.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_trig_tracking_m15_473.json")

RANGE_START = 0x010594
RANGE_END = 0x01074A
RANGE_SIZE = RANGE_END - RANGE_START  # 438 bytes

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
        "milestone": "M15.473",
        "title": "ROM Entity AI Trigonometric Tracking, Smooth Angle Turn-Rate Limiter & 2D Sin/Cos Velocity Projection Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": [
            {
                "name": "Math_TargetAngleAndVelocityProjector_TurnRate",
                "start": "0x010594",
                "end": "0x010660",
                "size_bytes": 204,
                "description": "Calculates delta vector to target (d0, d1), calculates arctangent angle using 0x010660 and ROM table $5D906, clamps angle change to turn rate limit 8, quantizes to 4 facings, and projects 2D velocity (vx at +$4E, vy at +$52) via ROM sin/cos table $5D706 scaled by speed."
            },
            {
                "name": "Math_ArctangentLookup_Octant1",
                "start": "0x010660",
                "end": "0x010684",
                "size_bytes": 36,
                "description": "Computes octant angle via ratio (d1 << 8) / d0 searched against 16-bit tangent threshold table at ROM $5D906."
            },
            {
                "name": "Math_TargetAngleAndVelocityProjector_Direct",
                "start": "0x010684",
                "end": "0x010726",
                "size_bytes": 162,
                "description": "Directly projects target angle and velocity vector without turn rate limiting, quantizing angle to 4 facings and computing 2D velocities from ROM sin/cos table $5D706."
            },
            {
                "name": "Math_ArctangentLookup_Octant2",
                "start": "0x010726",
                "end": "0x01074A",
                "size_bytes": 36,
                "description": "Computes octant angle via ratio (d4 << 8) / d0 searched against tangent table at ROM $5D906."
            }
        ],
        "tables": {
            "ROM_SIN_COS_TABLE": "0x05D706",
            "ROM_TANGENT_OCTANT_TABLE": "0x05D906"
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
