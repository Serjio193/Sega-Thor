#!/usr/bin/env python3
"""
Extract ROM Multi-Part Boss/Golem (0x9A) Master Tick, 4-State Dispatcher,
5x8 PRNG Pattern Sequencer & Tentacle/Child Initializer (0x015316..0x0154E8, 466 bytes).

Disassembles code segments cleanly with Capstone, unpacks embedded jump and pattern tables,
validates opcodes/constants, and generates evidence JSON for Milestone M15.508.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_golem_boss_pattern_tentacles_m15_508.json"

RANGE_START = 0x015316
RANGE_END = 0x0154E8
RANGE_SIZE = RANGE_END - RANGE_START  # 466 bytes

CODE_SEGMENTS = [
    ("BossGolem_MasterTickAndRouter", 0x015316, 0x015360),
    ("BossGolem_State00_PatternSequencer", 0x015368, 0x0153AC),
    ("BossGolem_PatternActions", 0x0153E0, 0x01541A),
    ("BossGolem_TentacleArmActivator", 0x01541A, 0x0154A8),
    ("BossGolem_State06_RetractAndReset", 0x0154A8, 0x0154E8),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets and table words."""
    assert u32_be(rom, 0x01541C) == 0x00FF1E50  # Arm primary entity slot
    assert u32_be(rom, 0x01545E) == 0x00FF1F0C  # 7-slot tentacle chain start
    assert u16_be(rom, 0x015474) == 0x009A      # Golem boss entity type 0x9A
    assert u16_be(rom, 0x015480) == 0x0300      # Tentacle tile attribute base
    assert u32_be(rom, 0x015486) == 0x00FF17A6  # VRAM base dynamic pointer
    assert u32_be(rom, 0x01548C) == 0x000019A0  # VRAM offset


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # 1. Unpack 4-entry State Jump Table at 0x015360
    state_table_addr = 0x015360
    state_jump_table = []
    for state_idx in range(4):
        entry_addr = state_table_addr + state_idx * 2
        offset = s16_be(rom, entry_addr)
        target = entry_addr + offset
        state_jump_table.append({
            "state_id": state_idx * 2,
            "word_hex": f"0x{u16_be(rom, entry_addr):04X}",
            "relative_offset": offset,
            "target_address": f"0x{target:06X}",
        })

    # 2. Unpack 6-entry Pattern Action Jump Table at 0x0153AC
    action_table_addr = 0x0153AC
    action_jump_table = []
    for action_idx in range(6):
        offset = s16_be(rom, action_table_addr + action_idx * 2)
        target = (action_table_addr + action_idx * 2) + offset
        action_jump_table.append({
            "action_id": action_idx,
            "word_hex": f"0x{u16_be(rom, action_table_addr + action_idx * 2):04X}",
            "relative_offset": offset,
            "target_address": f"0x{target:06X}",
        })

    # 3. Unpack 5x8 Pattern Table at 0x0153B8..0x0153E0 (40 bytes)
    pattern_table_addr = 0x0153B8
    pattern_rows = []
    for row_idx in range(5):
        row_bytes = list(rom[pattern_table_addr + row_idx * 8 : pattern_table_addr + (row_idx + 1) * 8])
        pattern_rows.append({
            "pattern_index": row_idx,
            "sequence": row_bytes,
        })

    # 4. Disassemble Code Segments
    instructions = []
    routines_meta = []
    for name, seg_start, seg_end in CODE_SEGMENTS:
        seg_bytes = rom[seg_start:seg_end]
        seg_insns = []
        for insn in md.disasm(seg_bytes, seg_start):
            insn_dict = {
                "address": f"0x{insn.address:06X}",
                "mnemonic": insn.mnemonic,
                "op_str": insn.op_str,
                "bytes": insn.bytes.hex(),
                "size": insn.size,
            }
            instructions.append(insn_dict)
            seg_insns.append(insn_dict)

        routines_meta.append({
            "name": name,
            "start": f"0x{seg_start:06X}",
            "end": f"0x{seg_end:06X}",
            "size_bytes": seg_end - seg_start,
            "instruction_count": len(seg_insns),
        })

    evidence = {
        "milestone": "M15.508",
        "title": "ROM Multi-Part Boss/Golem (0x9A) Master Tick, 4-State Dispatcher, 5x8 PRNG Pattern Sequencer & Tentacle/Child Initializer",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "state_jump_table": {
            "address": "0x015360",
            "entry_count": 4,
            "entries": state_jump_table,
        },
        "action_jump_table": {
            "address": "0x0153AC",
            "entry_count": 6,
            "entries": action_jump_table,
        },
        "pattern_table_5x8": {
            "address": "0x0153B8",
            "rows": 5,
            "columns": 8,
            "total_bytes": 40,
            "patterns": pattern_rows,
        },
        "sub_mode_router": {
            "source_register": "+$A2(a6)",
            "branches": {
                "0": "0x015758",
                "1": "0x015338",
                "2": "0x0159E2",
                "3": "0x0159B4",
                "4": "0x01573C",
            },
        },
        "contact_box": {
            "test_function": "0x00B922",
            "dx_min": -56,
            "dx_max": 56,
            "dy_min": -120,
            "dy_max": 120,
            "callback_on_contact": "0x015648",
        },
        "tentacle_chain_config": {
            "primary_arm_slot": "0x00FF1E50",
            "child_slots_base": "0x00FF1F0C",
            "child_count": 7,
            "slot_stride": "0xBC",
            "entity_type": "0x009A",
            "tile_attribute": "0x0300",
            "vram_dynamic_offset": "0x000019A0",
            "child_sub_mode": 4,
            "child_substate": "0x0012",
        },
        "ram_addresses": {
            "arm_primary_slot": "0x00FF1E50",
            "tentacle_child_slots_start": "0x00FF1F0C",
            "vram_pointer_base": "0x00FF17A6",
        },
        "external_targets": {
            "contact_test_b922": "0x00B922",
            "contact_callback_15648": "0x015648",
            "prng_e268": "0x00E268",
            "entity_init_8d06": "0x008D06",
            "reset_idle_1563c": "0x01563C",
        },
        "code_segments": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": placeholder}, indent=2)
    insn_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{placeholder}"', f"[\n{insn_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8", newline="\n")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
