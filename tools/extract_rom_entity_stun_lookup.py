#!/usr/bin/env python3
"""
extract_rom_entity_stun_lookup.py - M15.458 ROM Extractor
Extracts Pool 2 Entity Knockback / Stun Wave Broadcast & Script Trigger
and Global Entity Type ID Search Scanner from 0x00F1B2..0x00F258 (166 bytes).
"""

import json
import os
import sys
import capstone

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_entity_stun_lookup_m15_458.json"

START_ADDR = 0x00F1B2
END_ADDR = 0x00F258
TOTAL_BYTES = END_ADDR - START_ADDR

SUBROUTINES = [
    {
        "name": "Entity_Pool2_KnockbackStunWaveBroadcast",
        "start": 0x00F1B2,
        "end": 0x00F234,
        "size": 0x00F234 - 0x00F1B2,
        "description": "Scans Pool 2 (17 slots at $FF1CD8), applies knockback, facing flip, heavy damage $CFFF, and triggers script event $0004382E"
    },
    {
        "name": "Entity_FindByTypeIdGlobal",
        "start": 0x00F234,
        "end": 0x00F258,
        "size": 0x00F258 - 0x00F234,
        "description": "Scans 21 slots starting at $FF19E8 for matching entity type ID, returns pointer in a0 and sets Carry flag"
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
        "milestone": "M15.458",
        "title": "ROM Entity Pool 2 Knockback Stun Broadcast & Global Type Scanner",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": TOTAL_BYTES,
            "total_instructions": len(instructions)
        },
        "constants": {
            "pool2_base_ram": "0x00FF1CD8",
            "pool2_slot_count": 17,
            "entity_slot_stride": 188,
            "pool1_base_ram": "0x00FF19E8",
            "pool1_slot_count": 21,
            "knockback_damage": "0xCFFF",
            "script_event_addr": "0x00FF17B2",
            "script_event_id": "0x0004382E"
        },
        "subroutines": subroutine_map,
        "all_instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({TOTAL_BYTES} bytes) to {OUTPUT_JSON}")

if __name__ == "__main__":
    extract()
