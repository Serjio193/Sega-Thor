#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Dialogue Entity Sprite Compositor,
Prompt Arrow Indicator & Frame Sync Engine (0x00C756..0x00C88C, 310 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_dialogue_task_compositor_m15_430.json"

START_ADDR = 0x00C756
END_ADDR = 0x00C88C
ROUTINE_SIZE = END_ADDR - START_ADDR  # 310 bytes


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    evidence = {
        "routine": "dialogue_task_sprite_compositor_and_frame_sync",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "dialogue_task_sprite_compositor": f"0x{START_ADDR:06X}",
            "dialogue_emit_line_sprites": "0x00C81C",
            "dialogue_emit_sat_piece": "0x00C838",
            "dialogue_frame_sync_and_render": "0x00C84E",
        },
        "prompt_arrow": {
            "tile_continue": "0x86DC",
            "tile_dismiss": "0x96DC",  # bit 12 set
            "x_pos": 284,  # 0x11C
            "y_pos_base": 332,  # 0x14C
            "y_step_decrement": 12,  # 0x0C per line < 3
        },
        "line_sprites": {
            "pieces_per_row": 8,
            "size_code": "0x0C00",  # 4x4 tiles (32x32 px)
            "x_start": 160,  # 0x00A0
            "x_step": 32,  # 0x20
            "tile_step": 4,
            "y_bias": 272,  # 0x110
        },
        "hardware_sprite_limit": 80,  # 0x50
        "memory_map": {
            "FF13CC": "sat_buffer_base",
            "FF188C": "sat_buffer_offset",
            "FF188A": "sat_link_counter",
            "FF1886": "prompt_icon_active",
            "FF1887": "prompt_dismiss_flag",
            "FF1657": "animation_timer",
            "FF1880": "line_counter",
            "FF1872": "plane_top_base",
            "FF1874": "plane_bottom_base",
            "FF1876": "line1_row_offset",
            "FF1878": "line2_row_offset",
            "FF187A": "line3_row_offset",
            "FF187C": "line4_row_offset",
            "FF185D": "render_in_progress_flag",
            "FF1654": "frame_sync_cadence",
            "FF164E": "vint_status_flags",
        },
        "code_hex": routine_bytes.hex(),
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted evidence: {ROUTINE_SIZE} bytes -> {OUTPUT_JSON}")
    return evidence


if __name__ == "__main__":
    extract_evidence()
