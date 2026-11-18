#!/usr/bin/env python3
"""
extract_rom_player_targeting.py — Extract ROM Player Ali Targeting,
Normalized Aim Vector & Environmental Velocity Engine
(0x00E398..0x00E502, 362 bytes).
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_player_targeting_m15_449.json")

START_ADDR = 0x00E398
END_ADDR = 0x00E502


def extract_instructions_and_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom_data = f.read()

    chunk = rom_data[START_ADDR:END_ADDR]
    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(chunk, START_ADDR):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "routine": "rom_player_targeting",
        "description": (
            "ROM Player Ali Targeting, Normalized Aim Vector "
            "& Environmental Velocity Engine"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "target_player_ali_quadrant_angle": {
                "start": "0x00E398",
                "end": "0x00E3E8",
                "callers_count": 8,
                "notes": (
                    "Computes delta to Player Ali ($FF19F0, $FF19F4), "
                    "evaluates Manhattan distance (|dx| + |dy| into d5), "
                    "and resolves dominant cardinal direction into d0: "
                    "0=Up, 1=Right, 2=Down, 3=Left."
                )
            },
            "target_player_ali_and_load_motion_table": {
                "start": "0x00E3E8",
                "end": "0x00E45E",
                "callers_count": 14,
                "notes": (
                    "Resolves dominant cardinal direction to Ali into d0, "
                    "stores direction into +$16(a6), loads velocity vector "
                    "from table at (a0, d0*8) into +$4E(a6) and +$52(a6), "
                    "and clamps dominant axis distance to minimum 16px into +$02(a6)."
                )
            },
            "normalize_target_aim_vector_divs": {
                "start": "0x00E45E",
                "end": "0x00E4D0",
                "callers_count": 12,
                "notes": (
                    "Takes target (d1, d2), subtracts entity position (+$8, +$C), "
                    "determines half dominant distance d4, performs fixed-point "
                    "division divs.w on d1 and d2 (scaled << 8, shifted to 16.16), "
                    "and returns normalized velocity (d1, d2) and quadrant d3."
                )
            },
            "launch_directional_projectile_physics": {
                "start": "0x00E4D0",
                "end": "0x00E502",
                "callers_count": 6,
                "notes": (
                    "Checks mask d7 & 0xC000. If 0, loads speeds d1=0x0300, d2=0x02E0, "
                    "direction d3 from +$84(a6), invokes 0x00E2D4 to project velocities, "
                    "stores into +$4E(a6), +$52(a6), and initializes +$56(a6) to 0x00060000."
                )
            }
        },
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({END_ADDR - START_ADDR} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract_instructions_and_evidence()
