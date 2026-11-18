#!/usr/bin/env python3
"""
extract_rom_prng_vector_engine.py — Extract ROM Core PRNG Subsystem,
8-Way Directional Velocity Projector & Cardinal Angle Resolver
(0x00E25C..0x00E398, 316 bytes).
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_prng_vector_engine_m15_448.json")

START_ADDR = 0x00E25C
END_ADDR = 0x00E398


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
        "routine": "rom_prng_vector_engine",
        "description": (
            "ROM Core PRNG Subsystem, 8-Way Directional Velocity Projector "
            "& Cardinal Angle Resolver"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "entity_lifecycle_timer_countdown": {
                "start": "0x00E25C",
                "end": "0x00E268",
                "notes": "Decrements +$02(a6). When 0, zeroes +$00(a6) to despawn entity."
            },
            "prng_step_16bit_lfsr": {
                "start": "0x00E268",
                "end": "0x00E2A2",
                "callers_count": 206,
                "notes": "Reads seed from $FF1928, executes 4 Galois/Fibonacci XOR-shift rounds, stores back to $FF1928, returns 16-bit random word in d0."
            },
            "prng_range_word": {
                "start": "0x00E2A2",
                "end": "0x00E2BA",
                "callers_count": 63,
                "notes": "Generates random word, computes (rand * N) >> 16, returning integer in [0..N-1]."
            },
            "prng_range_byte": {
                "start": "0x00E2BA",
                "end": "0x00E2D4",
                "notes": "Masks N to byte (N & 0xFF), returns integer in [0..(N&0xFF)-1]."
            },
            "vector_directional_projection_8way": {
                "start": "0x00E2D4",
                "end": "0x00E338",
                "callers_count": 104,
                "notes": "Projects (d1, d2) according to 8-way direction d3 & 7 using jump table at 0x00E2F2. Diagonals scaled by 0.75x ((x>>1) + (x>>2))."
            },
            "vector_to_cardinal_direction_4way": {
                "start": "0x00E338",
                "end": "0x00E36E",
                "callers_count": 16,
                "notes": "Compares |dx| and |dy| to determine dominant 4-way direction d3."
            },
            "entity_ground_water_surface_probe": {
                "start": "0x00E36E",
                "end": "0x00E398",
                "callers_count": 30,
                "notes": "Probes terrain attribute at entity pos via $9C40/$9C7C. If water surface ($80), sets bit 3 of +$36(a6)."
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
