#!/usr/bin/env python3
"""
extract_rom_spatial_audio_pan.py — Extract ROM Positional Spatial Audio Dispatcher,
Distance Attenuation & Stereo Pan Subsystem
(0x00E78E..0x00E90E, 384 bytes).
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_spatial_audio_pan_m15_452.json")

START_ADDR = 0x00E78E
END_ADDR = 0x00E90E


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
        "routine": "rom_spatial_audio_pan",
        "description": (
            "ROM Positional Spatial Audio Dispatcher, Distance Attenuation "
            "& Stereo Pan Subsystem"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "spatial_sound_coords_flag_set": {
                "start": "0x00E78E",
                "end": "0x00E7A6",
                "callers_count": 12,
                "notes": "Plays spatial sound at (d1, d2) with high bit 31 set."
            },
            "spatial_sound_coords_flag_clear": {
                "start": "0x00E7A6",
                "end": "0x00E7E8",
                "callers_count": 25,
                "notes": "Plays spatial sound at (d1, d2) with high bit 31 clear."
            },
            "spatial_sound_entity_flag_set": {
                "start": "0x00E7E8",
                "end": "0x00E7FC",
                "callers_count": 34,
                "notes": "Plays spatial sound at entity position (+$8, +$C) with bit 31 set."
            },
            "spatial_sound_entity_flag_clear": {
                "start": "0x00E7FC",
                "end": "0x00E834",
                "callers_count": 78,
                "notes": "Plays spatial sound at entity position (+$8, +$C) with bit 31 clear."
            },
            "spatial_audio_distance_attenuation_and_pan": {
                "start": "0x00E834",
                "end": "0x00E90E",
                "callers_count": 4,
                "notes": (
                    "Calculates isometric distance max(|dx|, 1.25 * |dy|). "
                    "For ID >= 0x300: near radius 0x58 (no attenuation), 32px step buckets, "
                    "culls if distance > 312px, applies stereo pan (+0x42 left, +0x82 right) "
                    "if |dx| >= 0x40. For 0x200 <= ID < 0x300: culls if distance >= 0x110 (272px), "
                    "stepped volume reduction at 0xB0 and 0xE0, stereo pan (|dx| >= 0x60). "
                    "Dispatches to sound driver $60004."
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
