#!/usr/bin/env python3
"""
extract_rom_math_sqrt_atan.py — Extract ROM Newton-Raphson Integer Square Root
& 256-Step Circular Arctan Engine
(0x00E6BA..0x00E78E, 212 bytes).
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_math_sqrt_atan_m15_451.json")

START_ADDR = 0x00E6BA
END_ADDR = 0x00E78E
TABLE_ADDR = 0x05D906


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

    # Read tangent sector threshold table
    tangent_table = []
    for i in range(9):
        val = struct.unpack_from(">H", rom_data, TABLE_ADDR + i * 2)[0]
        tangent_table.append(val)

    evidence = {
        "routine": "rom_math_sqrt_atan",
        "description": (
            "ROM Newton-Raphson Integer Square Root "
            "& 256-Step Circular Arctan Engine"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "tangent_table_address": f"0x{TABLE_ADDR:06X}",
        "tangent_table": tangent_table,
        "subroutines": {
            "integer_sqrt_divu_newton_raphson": {
                "start": "0x00E6BA",
                "end": "0x00E6F0",
                "callers_count": 48,
                "notes": (
                    "Computes integer sqrt(d0) using Newton-Raphson division refinement "
                    "via divu.w. Returns floor(sqrt(d0)) in d0."
                )
            },
            "trig_atan2_target_coords": {
                "start": "0x00E6F0",
                "end": "0x00E706",
                "callers_count": 14,
                "notes": (
                    "Resolves 256-step angle from entity position (+$8, +$C) "
                    "to arbitrary target (d1, d2)."
                )
            },
            "trig_atan2_player_ali": {
                "start": "0x00E706",
                "end": "0x00E768",
                "callers_count": 28,
                "notes": (
                    "Resolves 256-step angle from entity position to Player Ali ($FF19E8), "
                    "mapping 4 quadrants with sign negation and +$80 offsets."
                )
            },
            "trig_atan_first_octant_table_search": {
                "start": "0x00E768",
                "end": "0x00E78E",
                "callers_count": 4,
                "notes": (
                    "Computes fixed-point slope (d2 << 8) / d0 via divu.w, "
                    "searches 9-entry tangent table at 0x05D906, advancing angle "
                    "by +8 for each sector. If d0 == 0, returns #$40 (64 = 90 deg)."
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
