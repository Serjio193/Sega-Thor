#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Map & Scene Script Dispatcher,
21-Area Script Table & State Manager routines (0x00C88C..0x00C9B2, 294 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_map_scene_dispatcher_m15_431.json"

START_ADDR = 0x00C88C
END_ADDR = 0x00C9B2
ROUTINE_SIZE = END_ADDR - START_ADDR  # 294 bytes

AREA_TABLE_ADDR = 0x00C92C
NUM_AREAS = 21


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    # Extract 21 area pointers
    areas = []
    for i in range(NUM_AREAS):
        ptr = struct.unpack_from(">I", rom, AREA_TABLE_ADDR + i * 4)[0]
        areas.append({
            "area_id": i,
            "address": f"0x{ptr:06X}",
            "ptr_val": ptr,
        })

    evidence = {
        "routine": "map_scene_script_dispatcher_and_state_manager",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "dialogue_update_caller_anim": f"0x{START_ADDR:06X}",
            "load_map_scene_script": "0x00C8AA",
            "get_map_scene_script_pointer": "0x00C90E",
            "exec_map_script_tick": "0x00C980",
            "exec_map_script_subtick": "0x00C998",
        },
        "area_table": {
            "address": f"0x{AREA_TABLE_ADDR:06X}",
            "count": NUM_AREAS,
            "areas": areas,
        },
        "script_header_layout": {
            "offset_0x00": "relative_offset_to_tick_callback",
            "offset_0x02": "relative_offset_to_subtick_callback",
            "offset_0x1A": "script_initialization_entrypoint",
        },
        "memory_map": {
            "FF0E40": "current_packed_area_scene_id",
            "FF0E2C": "area_state_flags_16_bytes",
            "FF0E1C": "map_state_flags_16_bytes",
            "FF0DB3": "area_trigger_flags_3_bytes",
            "FF16EC": "active_scene_script_pointer",
            "FF17B8": "script_pause_flags",
            "FF1882": "dialogue_caller_entity_ptr",
        },
        "code_hex": routine_bytes.hex(),
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted evidence: {ROUTINE_SIZE} bytes, {NUM_AREAS} areas -> {OUTPUT_JSON}")
    return evidence


if __name__ == "__main__":
    extract_evidence()
