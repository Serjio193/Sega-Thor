#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Companion Spirit Warp Repositioning,
Room Collision Table Loader & BGM Engine Synchronizer (0x00CC12..0x00CD64, 338 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_spirit_warp_bgm_m15_433.json"

START_ADDR = 0x00CC12
END_ADDR = 0x00CD64
ROUTINE_SIZE = END_ADDR - START_ADDR  # 338 bytes

COLLISION_TABLE_BASE = 0x04371E
NUM_COLLISION_AREAS = 32


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    # Extract 32 collision area pointers
    collision_ptrs = []
    for i in range(NUM_COLLISION_AREAS):
        ptr = struct.unpack_from(">I", rom, COLLISION_TABLE_BASE + i * 4)[0]
        collision_ptrs.append({
            "area_id": i,
            "address": f"0x{ptr:06X}",
            "ptr_val": ptr,
        })

    evidence = {
        "routine": "companion_spirit_warp_repositioning_and_bgm_sync",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "reposition_spirit_on_room_warp": f"0x{START_ADDR:06X}",
            "load_room_collision_table": "0x00CCA4",
            "init_room_environment_and_sync_bgm": "0x00CCE6",
            "latch_all_entity_ground_surfaces": "0x00CD4A",
        },
        "collision_tables": {
            "address": f"0x{COLLISION_TABLE_BASE:06X}",
            "count": NUM_COLLISION_AREAS,
            "areas": collision_ptrs,
        },
        "spirit_mechanics": {
            "slot": "0xFF1AA4",
            "y_offset_from_player": -8,
            "type_unsummon_bow": "0x0016",
            "type_dytto_efreet": "0x001C",
            "type_shade": "0x001E",
            "type_reset_state": "0x001A",
        },
        "bgm_sync": {
            "target_track_reg": "0xFF16F4",
            "current_track_reg": "0xFF16F2",
            "continue_command": 3,
        },
        "memory_map": {
            "FF1AA4": "spirit_entity_slot",
            "FF19E8": "player_ali_slot",
            "FF17B2": "active_collision_table_ptr",
            "FF17B6": "collision_tick_counter",
            "FF17B7": "collision_tick_cadence",
            "FF16F1": "room_spirit_forbid_flags",
            "FF184F": "stale_enemy_hud_tracker",
            "FF188A": "sat_link_counter",
            "FF13CC": "sat_buffer_base",
            "FF164E": "vint_busy_flags",
        },
        "code_hex": routine_bytes.hex(),
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted evidence: {ROUTINE_SIZE} bytes, {NUM_COLLISION_AREAS} collision areas -> {OUTPUT_JSON}")
    return evidence


if __name__ == "__main__":
    extract_evidence()
