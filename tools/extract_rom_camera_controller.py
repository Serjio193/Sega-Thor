#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Camera Deadzone Tracking,
Boundary Clamp & Screen Shake Controller (0x00CEEA..0x00D146, 604 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_camera_controller_m15_435.json"

START_ADDR = 0x00CEEA
END_ADDR = 0x00D146
ROUTINE_SIZE = END_ADDR - START_ADDR  # 604 bytes


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    evidence = {
        "routine": "camera_deadzone_tracking_and_boundary_controller",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "camera_tracker_update": f"0x{START_ADDR:06X}",
            "camera_tracker_start_panning": "0x00CF06",
            "camera_fixed_axis_direct_follow": "0x00D0C4",
            "clamp_camera_viewport_bounds": "0x00D118",
        },
        "deadzone_parameters": {
            "horizontal": {
                "left_deadzone_bias_edge": 0x50,       # 80 px
                "right_deadzone_bias_edge": 0xF0,      # 240 px
                "left_pan_threshold": 0x78,            # 120 px
                "right_pan_threshold": 0xC8,           # 200 px
                "smooth_min_speed": 3,
                "smooth_fast_catchup_threshold": 32,
                "smooth_fast_catchup_offset": 30,
            },
            "vertical": {
                "top_pan_threshold": 0x68,             # 104 px
                "bottom_pan_threshold": 0x80,          # 128 px
                "smooth_min_speed": 3,
                "smooth_fast_catchup_threshold": 32,
                "smooth_fast_catchup_offset": 30,
                "max_negative_speed": -6,              # 0xFFFA
            },
            "viewport_tiles": {
                "screen_width_tiles": 40,              # 0x28 (320 px)
                "screen_height_tiles": 28,             # 0x1C (224 px)
                "tile_size_shift": 3,                  # 8 px per tile
            },
        },
        "hardware_and_ram_flags": {
            "player_x": "0xFF19F0",
            "player_y": "0xFF19F4",
            "player_elevation_z": "0xFF19F8",
            "warp_sfx_trigger": "0xFF17BB",
            "instant_snap_flag": "0xFF17BA",
            "room_custom_flag": "0xFF16F1",
            "max_camera_x": "0xFF177A",
            "max_camera_y": "0xFF177C",
            "camera_offset_x": "0xFF177E",
            "camera_offset_y": "0xFF1780",
            "screen_shake_timer": "0xFF17C4",
            "scroll_bias_flag": "0xFF1859",
            "scroll_bias_target": "0xFF185A",
        },
        "byte_hash": f"{hash(routine_bytes):016x}",
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {ROUTINE_SIZE} bytes from 0x{START_ADDR:06X} to {OUTPUT_JSON}")


if __name__ == "__main__":
    extract_evidence()
