#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Room Warp Controller,
Camera Centering, Cutscene Locks & Player Ali Spawn Reset Engine (0x00C9B2..0x00CC12, 608 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_room_warp_engine_m15_432.json"

START_ADDR = 0x00C9B2
END_ADDR = 0x00CC12
ROUTINE_SIZE = END_ADDR - START_ADDR  # 608 bytes


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    evidence = {
        "routine": "room_warp_camera_centering_and_player_spawn_engine",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "check_cutscene_script_lock": f"0x{START_ADDR:06X}",
            "warp_room_entry_immediate": "0x00C9EC",
            "warp_room_entry_fade": "0x00CA10",
            "room_transition_execute": "0x00CA3E",
            "spawn_player_ali_in_room": "0x00CB64",
        },
        "camera_midpoint": {
            "offset_x": -160,  # 0x00A0
            "offset_y": -112,  # 0x0070
        },
        "special_overrides": {
            "room_4_to_0611": {
                "source": "0x0004",
                "replacement": "0x0611",
            }
        },
        "ali_spawn_defaults": {
            "entity_slot": "0xFF19E8",
            "idle_frame": "0x003F",
            "visible_flag": "0x01",
            "tangible_layer": "0x10",
            "falling_pose_elevation": 240,  # 0x00F0
            "falling_pose_mode": 6,
        },
        "memory_map": {
            "FF17B9": "cutscene_active_flag",
            "FF1987": "transition_busy_flag",
            "FF1985": "pause_menu_flag",
            "FF17B8": "script_pause_and_lock_flags",
            "FF1983": "secondary_lock_flag",
            "FF1982": "tertiary_lock_flag",
            "FF19FE": "warp_transition_mode",
            "FF17BE": "immediate_warp_flag",
            "FF17BC": "fade_flag",
            "FF17BB": "blackout_flag",
            "FF17BA": "room_loaded_flag",
            "FF178E": "camera_spawn_coords_storage",
            "FF1972": "player_spawn_coord_cache",
            "FF1984": "spawn_lock_flag",
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
