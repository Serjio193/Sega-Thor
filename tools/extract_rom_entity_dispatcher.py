#!/usr/bin/env python3
"""
extract_rom_entity_dispatcher.py — Extract ROM Master Group 2 Entity Tick Dispatcher,
Post-Tick Isometric Bounding Box Calculator & 89-Entry Action Function Pointer Table
(0x00DEEC..0x00E0BA, 462 bytes).
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_dispatcher_m15_446.json")

START_ADDR = 0x00DEEC
CODE_END_ADDR = 0x00DF54
TABLE_START_ADDR = 0x00DF54
TABLE_END_ADDR = 0x00E0B8
DUMMY_HANDLER_ADDR = 0x00E0B8
TOTAL_END_ADDR = 0x00E0BA


def extract_instructions_and_table():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom_data = f.read()

    # Disassemble code part
    chunk = rom_data[START_ADDR:CODE_END_ADDR]
    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(chunk, START_ADDR):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Disassemble dummy handler at 0x00E0B8
    dummy_chunk = rom_data[DUMMY_HANDLER_ADDR:TOTAL_END_ADDR]
    for insn in md.disasm(dummy_chunk, DUMMY_HANDLER_ADDR):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Extract 89 jump table entries
    table_entries = []
    num_entries = (TABLE_END_ADDR - TABLE_START_ADDR) // 4
    for i in range(num_entries):
        addr = TABLE_START_ADDR + i * 4
        target_fn = struct.unpack_from(">I", rom_data, addr)[0]
        entity_type = 0x16 + i * 2
        table_entries.append({
            "index": i,
            "entity_type": f"0x{entity_type:02X}",
            "table_addr": f"0x{addr:06X}",
            "handler_addr": f"0x{target_fn:06X}",
            "is_dummy": (target_fn == DUMMY_HANDLER_ADDR)
        })

    evidence = {
        "routine": "master_group2_entity_tick_dispatcher",
        "description": (
            "ROM Master Group 2 Entity Tick Dispatcher, Post-Tick Isometric "
            "Bounding Box Calculator & 89-Entry Action Function Pointer Table"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{TOTAL_END_ADDR:06X}",
        "byte_count": TOTAL_END_ADDR - START_ADDR,
        "code_bytes": CODE_END_ADDR - START_ADDR,
        "table_bytes": TABLE_END_ADDR - TABLE_START_ADDR,
        "table_entry_count": len(table_entries),
        "table_min_type": "0x16",
        "table_max_type": f"0x{0x16 + (len(table_entries) - 1) * 2:02X}",
        "pool": {
            "base_address": "$FF1AA4",
            "entity_count": 20,
            "stride": 188,
            "pause_flag": "$FF17B8 (bit 1)"
        },
        "post_tick_calc": {
            "source_raw_extents": "+$48 (X) and +$49 (Y)",
            "swap_flag": "+$17 bit 0",
            "effective_half_width": "+$42",
            "effective_half_height": "+$44",
            "formula_height": "d2 = (raw_y + 1) / 2; effective_y = d2 + (d2 / 2)"
        },
        "instructions": instructions,
        "jump_table": table_entries
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions and {len(table_entries)} table entries ({TOTAL_END_ADDR - START_ADDR} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract_instructions_and_table()
