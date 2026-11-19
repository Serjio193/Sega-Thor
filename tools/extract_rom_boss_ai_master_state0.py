#!/usr/bin/env python3
"""
Extract ROM Bow Spirit Spawn Initializer, Boss/Flying Enemy AI Master Tick,
Hit-Damage/Death Handler, 14-Entry State Jump Table, State 0 PRNG Action Selector,
and State 4 Wander Stepper (0x0147E0..0x014A72, 658 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.503.
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_ai_master_state0_m15_503.json"
)

RANGE_START = 0x0147E0
RANGE_END = 0x014A72
RANGE_SIZE = RANGE_END - RANGE_START  # 658 bytes

TABLE_START = 0x014914
TABLE_END = 0x014930  # 14 words = 28 bytes


def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # Part 1: 0x0147E0..0x014914 (308 bytes)
    insns_part1 = []
    for insn in md.disasm(rom[RANGE_START:TABLE_START], RANGE_START):
        insns_part1.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    # Embedded 14-word relative jump table: 0x014914..0x014930 (28 bytes)
    table_words = []
    jump_table_entries = []
    for i in range(14):
        entry_addr = TABLE_START + i * 2
        offset = struct.unpack_from(">h", rom, entry_addr)[0]
        target_addr = entry_addr + offset
        table_words.append(offset)
        jump_table_entries.append({
            "state_index": i * 2,
            "entry_address": f"0x{entry_addr:06X}",
            "relative_offset": offset,
            "target_address": f"0x{target_addr:06X}"
        })

    # Part 2: 0x014930..0x014A72 (322 bytes)
    insns_part2 = []
    for insn in md.disasm(rom[TABLE_END:RANGE_END], TABLE_END):
        insns_part2.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    all_instructions = insns_part1 + insns_part2

    routines = [
        {
            "name": "AI_BowSpirit_SpawnInit",
            "start": "0x0147E0",
            "end": "0x014848",
            "size_bytes": 0x014848 - 0x0147E0,
            "description": (
                "Bow Spirit (0x1E) spawn initializer. Configures entity type $1E, VRAM tile $4F8, "
                "HP 100, SP 10, position, facing, sprite $9D00, state 2, anim $14, flying bit 2 of "
                "+$38, clears $FF1C1C leash flag, and plays summon SFX $80037E via $E7FC."
            )
        },
        {
            "name": "AI_Boss_MasterTick_DamageDeathHandler",
            "start": "0x014848",
            "end": "0x014914",
            "size_bytes": 0x014914 - 0x014848,
            "description": (
                "Boss/Flying Enemy AI master tick, damage processing, and death handler. Ticks "
                "helpers $E682/$E36E, tests hitstun $A2 and flag $B4. Processes incoming damage "
                "from $86 (ignoring if bit 13 set), increments hit counter $B6, sets flinch anim $10 "
                "and state $0E, integrates knockback, subtracts 12-bit damage from HP $8A, plays "
                "hit SFX $A00250. If HP reaches 0: enters death state $1A, sets anim $18, invulnerable "
                "bit 3, flag $A2, spawns death particle $7F via $E90E, sets lock $FF1983, and calls "
                "$DAE6. Dispatches to 14-entry relative state jump table at 0x014914."
            )
        },
        {
            "name": "AI_Boss_State00_ActionSelector",
            "start": "0x014932",
            "end": "0x014A5C",
            "size_bytes": 0x014A5C - 0x014932,
            "description": (
                "State 0x00 PRNG action selector. Evaluates PRNG word d0 against thresholds $2000, "
                "$6000, $9000 (or if cutscene lock $FF1987 is active). Default action calculates midpoint "
                "between fixed origin (0x100, 0x140) and Player Ali ($FF19E8) with [-16..+15] PRNG jitter. "
                "Applies Chebyshev distance to select fixed speed: $20000 (>32px), $10000 (17..32px), "
                "or $8000 (<=16px), sets timer 10, and transitions to State 0x04 (Wander Stepper)."
            )
        },
        {
            "name": "AI_Boss_State04_WanderStep",
            "start": "0x014A5C",
            "end": "0x014A72",
            "size_bytes": 0x014A72 - 0x014A5C,
            "description": (
                "State 0x04 wander stepper. Invokes kinematic steering helper $150A0 toward wander "
                "target coordinates +$A6/+$AA. Decrements wander timer +$2(a6); when timer reaches 0, "
                "transitions directly back to State 0x00 (0x014932) to select next action."
            )
        }
    ]

    evidence = {
        "milestone": "M15.503",
        "title": (
            "ROM Bow Spirit Spawn Initializer, Boss/Flying Enemy AI Master Tick, Hit-Damage/Death "
            "Handler, 14-Entry State Jump Table, State 0 PRNG Action Selector & State 4 Wander Stepper"
        ),
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "jump_table": {
            "address": f"0x{TABLE_START:06X}",
            "size_bytes": TABLE_END - TABLE_START,
            "entry_count": len(jump_table_entries),
            "offsets": table_words,
            "entries": jump_table_entries
        },
        "routines": routines,
        "instruction_count": len(all_instructions),
        "instructions": all_instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(
        f"Extracted {len(all_instructions)} instructions ({RANGE_SIZE} bytes, "
        f"{len(jump_table_entries)}-entry jump table) across {len(routines)} routines to {OUTPUT_PATH}"
    )


if __name__ == "__main__":
    extract()
