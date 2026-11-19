#!/usr/bin/env python3
"""
Extract ROM Serpent Boss Phase 1 Intro Cutscene, Palette Crossfades, Active Master Tick,
5-Segment Hit-Flash, Death Transition & 13-Entry State Jump Table (0x016294..0x0164E2, 590 bytes).

Disassembles 142 M68K instructions across 2 routines, unpacks 13-entry relative jump table,
validates opcodes and CRAM/table addresses, and generates evidence JSON for Milestone M15.515.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_serpent_boss_phase1_intro_damage_m15_515.json"

RANGE_START = 0x016294
RANGE_END = 0x0164E2
RANGE_SIZE = RANGE_END - RANGE_START  # 590 bytes

CODE_START = 0x016294
CODE_END = 0x0164C8
CODE_SIZE = CODE_END - CODE_START  # 564 bytes

TABLE_START = 0x0164C8
TABLE_END = 0x0164E2
TABLE_SIZE = TABLE_END - TABLE_START  # 26 bytes (13 entries)


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, opcodes and table words."""
    assert u32_be(rom, 0x016298) == 0x00FF0E1D   # Combat active flag
    assert u16_be(rom, 0x0162AC) == 0x005A       # Cutscene stage 1 timer (90)
    assert u16_be(rom, 0x0162B4) == 0x007B       # Cutscene stage 2 timer (123)
    assert u16_be(rom, 0x0162BC) == 0x0096       # Cutscene stage 3 timer (150)
    assert u16_be(rom, 0x0162C4) == 0x00B7       # Cutscene stage 4 timer (183)
    assert u16_be(rom, 0x0162CC) == 0x00D3       # Cutscene stage 5 timer (211)
    assert u32_be(rom, 0x01630E) == 0x00FF136C   # CRAM palette buffer 3
    assert u32_be(rom, 0x016314) == 0x0005D0E6   # ROM palette 1
    assert u32_be(rom, 0x01631A) == 0x0005D106   # ROM palette 2
    assert u32_be(rom, 0x01632A) == 0x00700219   # Rumble SFX ID
    assert u32_be(rom, 0x016352) == 0x00FF13AC   # CRAM palette buffer 4
    assert u16_be(rom, 0x0163DC) == 0x0200       # Normal palette attribute
    assert u16_be(rom, 0x016412) == 0x4200       # Hit flash palette attribute
    assert u16_be(rom, 0x016472) == 0x0016       # Death state 0x16
    assert u32_be(rom, 0x01647E) == 0x00020000   # Z upward impulse +$20000
    assert u32_be(rom, 0x01648E) == 0x00FF210A   # Boss defeat flag $FF210A


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # 1. Disassemble code segment (0x016294..0x0164C8, 564 bytes, 142 instructions)
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

    # 2. Unpack Relative State Jump Table (0x0164C8..0x0164E2, 26 bytes, 13 entries)
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
            "name": "AI_SerpentBoss_Phase1IntroCutscene",
            "start": "0x016294",
            "end": "0x0163A4",
            "size_bytes": 0x0163A4 - 0x016294,
            "instruction_count": 62,
            "description": (
                "Phase 1 Intro Cutscene Sequencer. If active combat flag $FF0E1D:0 is set, jumps directly to active tick 0x0163A4. "
                "Otherwise increments cutscene timer +$B0(a6): "
                "timer < 90 calls 0x015AFC; timer 90..122 fades CRAM line 3 $FF136C ($5D0E6->$5D106) via 0x00302E, triggers SFX $700219 at step 0 and $FF20AE=$12 via 0x00DAE6 at step 32; "
                "timer 123..149 calls 0x015AFC; timer 150..182 fades CRAM line 4 $FF13AC, sets $0200 across 5 linked segments and SFX $700219 at step 32; "
                "timer 183..210 calls 0x015AFC; timer >= 211 calls Sub-Mode 3 init 0x015C4A, copies 32B CRAM from $5D126 to $FF13AC, asserts $FF164D:0, calls $DAC2($102), and activates combat $FF0E1D:0."
            ),
        },
        {
            "name": "AI_SerpentBoss_Phase1ActiveMasterTick",
            "start": "0x0163A4",
            "end": "0x0164C8",
            "size_bytes": 0x0164C8 - 0x0163A4,
            "instruction_count": 80,
            "description": (
                "Phase 1 Active Combat Master Tick. Clears +$72, +$76, +$83, +$9C. "
                "Decrements invulnerability timer +$AE, asserting invuln +$82=$FF while > 0. "
                "Decrements hit-flash timer +$AF, restoring normal palette $0200 across 5 linked segments upon reaching 0 (if HP != 0). "
                "On incoming damage +$86 != 0: sets flash palette $4200 across 5 segments, sets +$AF=2; if heavy damage (bit 14 or 15 set), sets +$AE=2 and +$82=$FF; "
                "subtracts defense +$92 from 12-bit damage (clamped >= 1) and applies to HP +$8A; "
                "on HP = 0, initiates death: sets State $16, anim 8, vz = +$20000, +$38 |= $C, $FF210A=$0FFF, calls 0x00E96C (d0=10) across 4 child slots (stride $BC). "
                "Dispatches +$04 via jump table 0x0164C8."
            ),
        },
        {
            "name": "AI_SerpentBoss_Phase1StateJumpTable",
            "start": "0x0164C8",
            "end": "0x0164E2",
            "size_bytes": 26,
            "entry_count": 13,
            "description": (
                "13-entry relative state jump table for States $00..$18: "
                "targets 0x164E2, 0x1652E, 0x1658A, 0x165AE, 0x165C4, 0x165D0, 0x165E2, 0x1663A, 0x16690, 0x166D2, 0x166F6, 0x164F2, 0x1651C."
            ),
        },
    ]

    evidence = {
        "metadata": {
            "milestone": "M15.515",
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
