#!/usr/bin/env python3
"""
extract_rom_item_drop_engine.py - M15.461 ROM Extractor
Extracts Enemy Defeat Loot Drop & Chest Item Spawner Subsystem
from 0x00F476..0x00F64C (470 bytes).
"""

import json
import os
import struct
import sys
import capstone

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_item_drop_engine_m15_461.json"

START_ADDR = 0x00F476
END_ADDR = 0x00F64C
TOTAL_BYTES = END_ADDR - START_ADDR

SUBROUTINES = [
    {
        "name": "ItemDrop_EnemyDefeatDropEvaluator",
        "start": 0x00F476,
        "end": 0x00F53C,
        "size": 0x00F53C - 0x00F476,
        "description": "Evaluates 2-tier enemy drop table (a1), rolls PRNG, allocates Pool 2 slot, spawns Type 8 item entity with +0x00080000 launch velocity"
    },
    {
        "name": "ItemDrop_FixedChestDropSpawner",
        "start": 0x00F53C,
        "end": 0x00F64C,
        "size": 0x00F64C - 0x00F53C,
        "description": "Decodes fixed drop ID from +$8C(a0); for >= 0x8000 reads 6-byte chest record at $43830, tests persistent collected bitmask ($FF0DBC/$FF0E1C/$FF0E2C), spawns Type 8 item or aborts"
    }
]

def extract():
    if not os.path.isfile(ROM_PATH):
        print(f"Error: ROM file not found at {ROM_PATH}")
        sys.exit(1)

    with open(ROM_PATH, "rb") as f:
        rom_data = f.read()

    chunk = rom_data[START_ADDR:END_ADDR]
    if len(chunk) != TOTAL_BYTES:
        print(f"Error: Expected {TOTAL_BYTES} bytes, got {len(chunk)}")
        sys.exit(1)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    instructions = []
    for ins in md.disasm(chunk, START_ADDR):
        instructions.append({
            "address": f"0x{ins.address:06X}",
            "mnemonic": ins.mnemonic,
            "op_str": ins.op_str,
            "bytes": ins.bytes.hex()
        })

    # Decode first 20 records of chest loot table $43830
    tbl_addr = 0x043830
    chest_records = []
    for i in range(20):
        rec = rom_data[tbl_addr + i * 6 : tbl_addr + (i + 1) * 6]
        cat, item_id, pad, bank = rec[0], rec[1], rec[2], rec[3]
        bit_idx = struct.unpack_from(">H", rec, 4)[0]
        chest_records.append({
            "record_index": i,
            "category": f"0x{cat:02X}",
            "item_id": f"0x{item_id:02X}",
            "persistent_bank": bank,
            "persistent_bit_index": bit_idx,
            "raw_hex": rec.hex()
        })

    subroutine_map = {}
    for sub in SUBROUTINES:
        sub_chunk = rom_data[sub["start"]:sub["end"]]
        sub_insns = []
        for ins in md.disasm(sub_chunk, sub["start"]):
            sub_insns.append({
                "address": f"0x{ins.address:06X}",
                "mnemonic": ins.mnemonic,
                "op_str": ins.op_str,
                "bytes": ins.bytes.hex()
            })
        subroutine_map[sub["name"]] = {
            "meta": sub,
            "instruction_count": len(sub_insns),
            "instructions": sub_insns
        }

    evidence = {
        "milestone": "M15.461",
        "title": "ROM Enemy Defeat Loot Drop & Chest Item Spawner Subsystem",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": TOTAL_BYTES,
            "total_instructions": len(instructions)
        },
        "constants": {
            "item_entity_type": 8,
            "elevation_launch_velocity": "0x00080000",
            "chest_table_addr": "0x043830",
            "chest_table_record_size": 6,
            "persistent_banks": {
                "1": "0x00FF0DBC",
                "2": "0x00FF0E1C",
                "3": "0x00FF0E2C"
            },
            "item_attribute_table": "0x05CE16"
        },
        "sample_chest_records": chest_records,
        "subroutines": subroutine_map,
        "all_instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.461 evidence ({TOTAL_BYTES} bytes, {len(instructions)} insns) to {OUTPUT_JSON}")

if __name__ == "__main__":
    extract()
