#!/usr/bin/env python3
"""
Extracts and analyzes ROM Projectile/Effect Entity Pool Allocators & Sound Driver Command Dispatchers
(0x00D9A4..0x00DB1A, 374 bytes) from Beyond Oasis (USA).md into structured JSON evidence for milestone M15.442.
"""

import json
from pathlib import Path
import capstone

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUTPUT_PATH = Path("data/thor-evidence/rom_entity_alloc_sound_dispatch_m15_442.json")

START_ADDR = 0x00D9A4
END_ADDR = 0x00DB1A
EXPECTED_LEN = END_ADDR - START_ADDR

def main():
    if not ROM_PATH.exists():
        raise FileNotFoundError(f"ROM file not found: {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom_bytes = f.read()

    slice_bytes = rom_bytes[START_ADDR:END_ADDR]
    assert len(slice_bytes) == EXPECTED_LEN, f"Expected {EXPECTED_LEN} bytes, got {len(slice_bytes)}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    instructions = []
    for ins in md.disasm(slice_bytes, START_ADDR):
        instructions.append({
            "address": f"0x{ins.address:06X}",
            "mnemonic": ins.mnemonic,
            "op_str": ins.op_str,
            "bytes": ins.bytes.hex()
        })

    metadata = {
        "milestone": "M15.442",
        "description": "ROM Projectile/Effect Entity Pool Allocators & Sound Driver Command Dispatchers",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": EXPECTED_LEN,
        "first_bytes": slice_bytes[:16].hex(),
        "last_bytes": slice_bytes[-16:].hex(),
        "routines": {
            "decompress_palette_block": "0x00D9A4",
            "alloc_projectile_entity_slot": "0x00D9BE",
            "alloc_spark_effect_slot": "0x00D9F0",
            "find_free_general_entity_slot_18": "0x00DA1A",
            "find_free_general_entity_slot_17": "0x00DA2A",
            "alloc_spawn_npc_entity_slot_18": "0x00DA62",
            "alloc_spawn_npc_entity_slot_17": "0x00DA72",
            "sound_play_music_track": "0x00DAC2",
            "sound_resume_music_track": "0x00DACE",
            "sound_pause_music_track": "0x00DAE6",
            "sound_stop_music_track": "0x00DB00"
        },
        "sound_driver_entry": "0x060004",
        "sound_active_track_register": "0xFF16F2",
        "projectile_pool_base": "0xFF2ABC",
        "projectile_pool_slots": 8,
        "projectile_pool_stride": 90,
        "spark_pool_base": "0xFF2D8C",
        "spark_pool_slots": 6,
        "spark_pool_stride": 90,
        "general_pool_base_18": "0xFF1C1C",
        "general_pool_base_17": "0xFF1CD8",
        "general_pool_stride": 188,
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(metadata, f, indent=2)

    print(f"Wrote {len(instructions)} instructions to {OUTPUT_PATH}")

if __name__ == "__main__":
    main()
