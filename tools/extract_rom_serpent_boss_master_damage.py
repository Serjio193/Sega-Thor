#!/usr/bin/env python3
"""
Extract ROM Composite Serpent/Multi-Segment Boss Sub-Modes 2/3, Hit-Flash Palette
Toggle, Defense-Mitigated HP Subtraction, Knockback Trigger & 12-Entry State Jump Table
(0x015C4A..0x015E0E, 452 bytes).

Disassembles code cleanly with Capstone, unrolls 5-segment linked palette updates,
unpacks 13-entry relative jump table, asserts ROM constants, and generates evidence JSON
for Milestone M15.512.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_serpent_boss_master_damage_m15_512.json"

RANGE_START = 0x015C4A
RANGE_END = 0x015E0E
RANGE_SIZE = RANGE_END - RANGE_START  # 452 bytes

CODE_START = 0x015C4A
CODE_END = 0x015DF4
CODE_SIZE = CODE_END - CODE_START  # 426 bytes

TABLE_START = 0x015DF4
TABLE_END = 0x015E0E
TABLE_SIZE = TABLE_END - TABLE_START  # 26 bytes (13 entries)


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, opcodes and table words."""
    assert u16_be(rom, 0x015C50) == 0x3D7C       # move.w #$10, $2a(a6)
    assert u16_be(rom, 0x015C52) == 0x0010
    assert u16_be(rom, 0x015C78) == 0x3D7C       # move.w #$12, $2a(a6)
    assert u16_be(rom, 0x015C7A) == 0x0012
    assert u16_be(rom, 0x015CC4) == 0x7002       # moveq #$2, d0
    assert u32_be(rom, 0x015CCE) == 0x00FF0E1D   # Game active / pause flag
    assert u16_be(rom, 0x015CFA) == 0x323C       # move.w #$200, d1
    assert u16_be(rom, 0x015CFC) == 0x0200
    assert u16_be(rom, 0x015D2A) == 0x323C       # move.w #$4200, d1
    assert u16_be(rom, 0x015D2C) == 0x4200
    assert u16_be(rom, 0x015D68) == 0x3D7C       # move.w #$34, $2a(a6)
    assert u16_be(rom, 0x015D6A) == 0x0034
    assert u32_be(rom, 0x015DAE) == 0x00020000   # Z upward impulse +$20000
    assert u32_be(rom, 0x015DBC) == 0x00FF1982   # Cutscene flag $FF1982


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # 1. Disassemble code segment (0x015C4A..0x015DF4, 426 bytes)
    code_bytes = rom[CODE_START:CODE_END]
    instructions = []
    for insn in md.disasm(code_bytes, CODE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })

    # 2. Unpack Relative State Jump Table (0x015DF4..0x015E0E, 26 bytes, 13 entries)
    jump_table_entries = []
    for idx in range(13):
        entry_addr = TABLE_START + idx * 2
        raw_word = u16_be(rom, entry_addr)
        rel_offset = s16_be(rom, entry_addr)
        target_addr = entry_addr + rel_offset
        state_id = idx * 2
        jump_table_entries.append({
            "state_id": f"0x{state_id:02X}",
            "state_index": idx,
            "address": f"0x{entry_addr:06X}",
            "raw_word_hex": f"0x{raw_word:04X}",
            "relative_offset": rel_offset,
            "target_address": f"0x{target_addr:06X}",
        })

    routines = [
        {
            "name": "AI_SerpentBoss_SubMode3_Activation",
            "start": "0x015C4A",
            "end": "0x015CB6",
            "size_bytes": 0x015CB6 - 0x015C4A,
            "instruction_count": 25,
            "description": (
                "Sub-Mode 3 Cutscene/Intro Activation. Iterates across 9 linked entity slots (stride $BC): "
                "sets anim $10 on slots 1..4, anim $12 on slot 5 (head), clears +$36 on slots 5..9 and +$38 on slot 5. "
                "Restores root entity pointer a6, clears root +$36, +$38, and timer +$2."
            ),
        },
        {
            "name": "AI_SerpentBoss_SubMode2_Countdown",
            "start": "0x015CB6",
            "end": "0x015CCA",
            "size_bytes": 0x015CCA - 0x015CB6,
            "instruction_count": 6,
            "description": (
                "Sub-Mode 2 Countdown & Death Trigger. Decrements timer +$2; upon expiration (timer == 0) "
                "invokes transition routine 0x00E96C with mode d0 = 2 to initiate boss death."
            ),
        },
        {
            "name": "AI_SerpentBoss_SubMode2_MasterTick",
            "start": "0x015CCA",
            "end": "0x015DF4",
            "size_bytes": 0x015DF4 - 0x015CCA,
            "instruction_count": 75,
            "description": (
                "Sub-Mode 2 Master Tick & Combat Handler. Checks game active bit btst #0, $FF0E1D (branches to 0x015AFC if unpaused). "
                "Clears +$83, +$82, +$9C. If State == $12, jumps directly to 0x0160E2. "
                "Decrements hit-flash timer +$AF; upon reaching 0, restores normal palette $0200 across root and 4 body slots (stride $BC). "
                "On incoming damage +$86 != 0: sets flash palette $4200 across 5 linked slots, sets +$AF = 2; "
                "if in State 2, transitions to State 4, anim $34, mirrors timer (17 - +$2 clamped >= 1), and negates velocity vectors +$A6/+$AA; "
                "mitigates damage by defense +$92 (clamped >= 1) and subtracts from HP +$8A; "
                "on HP = 0, transitions to State $10, vz = +$20000, sets +$38:3, and if cutscene flag $FF1982 != 0, "
                "targets Ali via 0x015F7C setting damage $800A, knockback vy = +$30000, reaction mode 2, anim $34. "
                "Dispatches via relative jump table 0x015DF4."
            ),
        },
        {
            "name": "AI_SerpentBoss_StateJumpTable",
            "start": "0x015DF4",
            "end": "0x015E0E",
            "size_bytes": 26,
            "entry_count": 13,
            "description": (
                "13-entry relative state jump table for States $00..$18 (12 primary states $00..$16 + State $18 at 0x015E0C): "
                "targets 0x15E0E, 0x15E56, 0x15EEA, 0x15F04, 0x15E3C, 0x15F34, 0x15F52, 0x15FC4, 0x1608C, 0x160E2, 0x15FE0, 0x15FFE, 0x16070."
            ),
        },
    ]

    evidence = {
        "metadata": {
            "milestone": "M15.512",
            "rom_file": "Beyond Oasis (USA).md",
            "range_start": f"0x{RANGE_START:06X}",
            "range_end": f"0x{RANGE_END:06X}",
            "range_size_bytes": RANGE_SIZE,
            "code_size_bytes": CODE_SIZE,
            "table_size_bytes": TABLE_SIZE,
            "instruction_count": len(instructions),
            "jump_table_entry_count": len(jump_table_entries),
        },
        "jump_table": jump_table_entries,
        "instructions": instructions,
        "routines": routines,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(evidence, indent=2))
    print(f"Wrote evidence JSON to {OUTPUT_PATH}")
    print(f"Total instructions extracted: {len(instructions)}")
    print(f"Jump table entries unpacked: {len(jump_table_entries)}")


if __name__ == "__main__":
    extract()
