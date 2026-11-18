#!/usr/bin/env python3
"""
Extract ROM Drop Item Slot Allocation Eviction Probe, Ground Shadow Spawner
and Ali Kill Counter Engine (0x00F64C..0x00F788, 316 bytes).

Disassembles the exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.462.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_drop_probe_shadow_m15_462.json")

RANGE_START = 0x00F64C
RANGE_END = 0x00F788
RANGE_SIZE = RANGE_END - RANGE_START  # 316 bytes

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "milestone": "M15.462",
        "title": "ROM Drop Item Slot Allocation Eviction Probe, Ground Shadow Spawner & Ali Kill Counter Engine",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "subroutines": [
            {
                "name": "DropItem_AllocateSlot_EvictionProbe",
                "start": "0x00F64C",
                "end": "0x00F6E2",
                "size_bytes": 150,
                "description": (
                    "Scans 8-slot drop item pool at $FF2ABC (stride $5A = 90 bytes). "
                    "Pass 1 checks distance threshold $10 via $E5AE for slots with item attribute bit 7 clear; "
                    "if Carry set, evicts and jumps to $F550. Pass 2 evicts first slot with attribute bit 7 clear. "
                    "If all 8 slots protected, checks candidate item attribute bit 5 (key quest item); if 0, "
                    "aborts drop at $F644; if 1, triggers panic trap at $000206."
                ),
                "constants": {
                    "item_pool_base": "0xFF2ABC",
                    "slot_stride": 90,
                    "slot_count": 8,
                    "base_tile_index": "0x0694",
                    "tile_stride": 9,
                    "item_attrib_table": "0x05CE16",
                    "dist_threshold": 16,
                    "panic_trap": "0x000206"
                }
            },
            {
                "name": "Item_SpawnGroundShadowParticle",
                "start": "0x00F6E2",
                "end": "0x00F740",
                "size_bytes": 94,
                "description": (
                    "Allocates particle entity slot via $DA2A. If allocation succeeds, clears 94 words (188 bytes), "
                    "copies 45 words (90 bytes) from parent item entity a4 to child shadow entity a6. Marks parent "
                    "+$36 bit 1 and elevation priority +$0C = $7D00. Configures child entity type = $6A, flags "
                    "+$37 bits 5 & 6 set, +$38 bit 2 clear, and links parent pointer at +$A6."
                ),
                "constants": {
                    "shadow_entity_type": "0x006A",
                    "parent_priority_word": "0x7D00",
                    "shadow_parent_offset": "0x00A6",
                    "clear_word_count": 94,
                    "copy_word_count": 45
                }
            },
            {
                "name": "Enemy_CreditAliKillAndSpecialDrop",
                "start": "0x00F740",
                "end": "0x00F788",
                "size_bytes": 72,
                "description": (
                    "If enemy drop field +$8C is 0 and global flag $FF0DBB is set, clears $FF0DBB and sets "
                    "+$8C = $1101 (special forced drop). If attacker entity +$7E is Player Ali ($FF19E8) and "
                    "+$3A bit 0 is 0, increments Ali's kill counter at $FF1A7C with saturating clamp ($FFFF)."
                ),
                "constants": {
                    "ali_entity_address": "0xFF19E8",
                    "ali_kill_counter_address": "0xFF1A7C",
                    "global_special_drop_flag": "0xFF0DBB",
                    "special_drop_id": "0x1101"
                }
            }
        ],
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")

if __name__ == "__main__":
    extract()
