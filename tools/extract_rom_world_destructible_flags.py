#!/usr/bin/env python3
"""
Extract ROM Interactive/Destructible Enemy/Object & Core Persistent Flag Bitfield
Helpers (0x017E28..0x018072, 586 bytes).

Disassembles 146 M68K instructions across 3 routines, records the embedded state
jump table (0x017EE6) and loot drop table (0x017F6C), validates ROM constants,
and generates evidence JSON for Milestone M15.525.
"""

from __future__ import annotations

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_world_destructible_flags_m15_525.json"
)

RANGE_START = 0x017E28
RANGE_END = 0x018072
RANGE_SIZE = RANGE_END - RANGE_START  # 586 bytes

ROUTINE_DEFS = [
    (
        "AI_WorldObject_DestructibleEnemyObject",
        0x017E28,
        0x017FF2,
        [
            (0x017E28, 0x017EE6),
            (0x017EF0, 0x017F6C),
            (0x017F74, 0x017FF2),
        ],
        107,
        (
            "Interactive/destructible object & enemy controller with relative jump table at 0x017EE6 "
            "(states 0/2/4/6/8 via +$4). Clears +$16/+$66 on first tick, calls 0x00E682. On incoming damage "
            "+$86 != 0: bit 13 sets +$39:3 and plays SFX #$8004D9; subtracts defense +$92 from 12-bit damage "
            "(clamped >= 1) and deducts from HP +$8A. On 0 HP: enters Death state 2 (anim 2 via 0x008EF8, "
            "timer +$2 = 16, invuln +$38:3, SFX #$900232); else enters Flinch state 8 (anim 10 via 0x008EF8). "
            "Applies gravity -$C000 when z > 0. Action selector 0x017EF0 checks target distance 0x00E398 < 48 "
            "-> State 6 (anim 6), else screen shake $FF17C4 != 0 or PRNG 0x00E268 < 0x1A00 -> State 4 (anim 4), "
            "else State 0 (timer +$2 = 4, anim 0). State 2 blinks +$36:1 for 16 frames then calls loot drop "
            "0x00F476 (table 0x017F6C) and despawns (+$0 = 0). State 4 waits for +$37:1. State 6 plays SFX "
            "#$800202 on frames 12/20 and runs hitbox check 0x00BCE2 with callback 0x017FC0 (0x00BEBA, "
            "+$84 = 2, neg.l +$52). State 8 holds +$36:6 until +$37:1 then clears +$36:6 and +$39:3."
        ),
    ),
    (
        "PersistentFlag_SetBit",
        0x017FF2,
        0x018030,
        [
            (0x017FF2, 0x018030),
        ],
        19,
        (
            "Core persistent flag bit setter. Maps bank selector +$A2(a0) (1 -> $FF0DBC, 2 -> $FF0E1C, "
            "3 -> $FF0E2C; 0 or >= 4 -> no-op) and bit index +$A4(a0) (byte offset = +$A4 >> 3, "
            "bit = +$A4 & 7) to execute bset.b d1, (a1, d0.w)."
        ),
    ),
    (
        "PersistentFlag_ResolveBit",
        0x018030,
        0x018072,
        [
            (0x018030, 0x018072),
        ],
        20,
        (
            "Core persistent flag bit resolver. Maps bank selector +$A2(a6) (1 -> $FF0DBC, 2 -> $FF0E1C, "
            "3 -> $FF0E2C) and bit index +$A4(a6) to return byte pointer in a0 (base + (+$A4 >> 3)), "
            "bit index in d0 (+$A4 & 7), and Carry clear (C = 0). For bank 0 or >= 4, returns Carry set (C = 1)."
        ),
    ),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract() -> None:
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    instructions = []
    routines = []

    for name, r_start, r_end, code_ranges, exp_insns, desc in ROUTINE_DEFS:
        r_insns = []
        for c_start, c_end in code_ranges:
            for insn in md.disasm(rom[c_start:c_end], c_start):
                entry = {
                    "address": f"0x{insn.address:06X}",
                    "mnemonic": insn.mnemonic,
                    "op_str": insn.op_str,
                    "bytes": insn.bytes.hex(),
                }
                r_insns.append(entry)
                instructions.append(entry)

        assert len(r_insns) == exp_insns, f"Instruction count mismatch for {name}: {len(r_insns)} != {exp_insns}"
        routines.append({
            "name": name,
            "start": f"0x{r_start:06X}",
            "end": f"0x{r_end:06X}",
            "size_bytes": r_end - r_start,
            "instruction_count": len(r_insns),
            "description": desc,
        })

    jump_table_0x017ee6 = []
    for idx in range(5):
        addr = 0x017EE6 + idx * 2
        off = s16_be(rom, addr)
        jump_table_0x017ee6.append({
            "state_value": idx * 2,
            "entry_address": f"0x{addr:06X}",
            "offset_word": f"0x{u16_be(rom, addr):04X}",
            "target_address": f"0x{addr + off:06X}",
        })

    evidence = {
        "milestone": "M15.525",
        "title": "ROM Interactive/Destructible Enemy/Object & Core Persistent Flag Bitfield Helpers",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "embedded_tables": {
            "state_jump_table_0x017ee6": jump_table_0x017ee6,
            "loot_drop_table_0x017f6c": [f"0x{b:02X}" for b in rom[0x017F6C:0x017F74]],
            "persistent_flag_banks": {
                "1": "0xFF0DBC",
                "2": "0xFF0E1C",
                "3": "0xFF0E2C",
            },
        },
        "routines": routines,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
