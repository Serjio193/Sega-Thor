#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Room Entry Presentation Manager,
Palette Crossfader & Footstep Audio Jingle (0x00CD64..0x00CEEA, 388 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_room_presentation_jingle_m15_434.json"

START_ADDR = 0x00CD64
END_ADDR = 0x00CEEA
ROUTINE_SIZE = END_ADDR - START_ADDR  # 388 bytes

TARGET_WHITE_PALETTE_BASE = 0x05D686
NUM_WHITE_PALETTE_WORDS = 64


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    # Read target white palette words at 0x05D686
    white_words = [
        struct.unpack_from(">H", rom, TARGET_WHITE_PALETTE_BASE + i * 2)[0]
        for i in range(NUM_WHITE_PALETTE_WORDS)
    ]

    evidence = {
        "routine": "room_entry_presentation_manager_and_jingle",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "room_entry_presentation_manager": f"0x{START_ADDR:06X}",
            "room_entry_whiteout_flash": "0x00CDEC",
            "room_entry_palette_crossfade": "0x00CDA8",
            "room_entry_standard_fade": "0x00CE22",
            "room_entry_instant_warp_steps": "0x00CD7E",
            "room_entry_footsteps_jingle": "0x00CE72",
            "cram_palette_buffer_copy": "0x00CECC",
        },
        "target_white_palette": {
            "address": f"0x{TARGET_WHITE_PALETTE_BASE:06X}",
            "count_words": NUM_WHITE_PALETTE_WORDS,
            "all_pure_white": all(w == 0x0EEE for w in white_words),
        },
        "audio_jingle_sequence": [
            {"step": 1, "sound_id": "0x0090038E", "delay_frames": 14},
            {"step": 2, "sound_id": "0x0090038F", "delay_frames": 14},
            {"step": 3, "sound_id": "0x0090038E", "delay_frames": 14},
            {"step": 4, "sound_id": "0x0090038F", "delay_frames": 14},
        ],
        "hardware_and_ram_flags": {
            "dma_fade_busy": "0xFF164E",
            "special_fade_mode": "0xFF17BF",
            "warp_mode_type": "0xFF17BE",
            "cram_dirty_flags": "0xFF164D",
            "room_custom_flag": "0xFF16F1",
            "room_flag_out": "0xFF164F",
            "warp_sfx_trigger": "0xFF17BB",
            "warp_sfx_suppress": "0xFF17BC",
            "saved_stack_pointer": "0xFF1930",
            "cram_buffer_base": "0xFF134C",
            "cram_palette_line1": "0xFF138C",
            "palette_scratch_buffer": "0xFF18A6",
            "source_palette_ptr": "0xFF179E",
            "gameplay_loop_dest": "0x008AA6",
        },
        "byte_hash": f"{hash(routine_bytes):016x}",
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {ROUTINE_SIZE} bytes from 0x{START_ADDR:06X} to {OUTPUT_JSON}")


if __name__ == "__main__":
    extract_evidence()
