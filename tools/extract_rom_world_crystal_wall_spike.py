#!/usr/bin/env python3
"""
Extract ROM World Interactive Objects: Spirit Crystal Orb Switch, Breakable Wall Block
& Floor Spike/Saw Hazard (0x016CBE..0x016EBC, 510 bytes).

Disassembles 132 M68K instructions across 3 routines, validates opcodes and RAM/SFX
constants, and generates evidence JSON for Milestone M15.519.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_world_crystal_wall_spike_m15_519.json"

RANGE_START = 0x016CBE
RANGE_END = 0x016EBC
RANGE_SIZE = RANGE_END - RANGE_START  # 510 bytes

ROUTINE_BOUNDS = [
    ("AI_World_SpiritCrystalOrbSwitch", 0x016CBE, 0x016D5C, 41,
     "Spirit Crystal Orb Switch: sub-mode 0 initializes z=+$8C, checks persistent bit via 0x018030 (if set, anim 2, solid +$38:3, +$3C:1; else anim 0); sub-mode != 0 enforces 10-frame cooldown on +$82, incoming damage at +$86: if bit 13 set (spirit weapon), plays chime SFX #$9003A1 via 0x00E7FC, sets persistent bit via 0x018030, sets anim 2, +$3C:1, locks cooldown +$82=$FF; else plays clink SFX #$900238."),
    ("AI_World_BreakableWallBlock", 0x016D5C, 0x016DCE, 31,
     "Breakable Barrier / Wall Block: sub-mode 0 initializes HP +$88 and +$8A from parameter +$A0; sub-mode != 0 enforces 6-frame cooldown on +$82, incoming damage at +$86: if bit 13 set (spirit weapon), subtracts lower 12-bit damage from HP +$8A, if HP<=0 sets persistent bit via 0x018030, raises z+=16, triggers shatter debris via 0x00E96C(1); else plays clink SFX #$900238."),
    ("AI_World_FloorSpikeSawHazard", 0x016DCE, 0x016EBC, 60,
     "Floor Spike / Saw Hazard & Blinking Despawn: sub-mode 0: 52-frame sound timer +$AA plays blade buzz SFX #$600262 via 0x00E7E8; performs AABB collision test vs Ali via 0x00B922 [-14..+14, -8..+8, 0..40]; on contact checks Ali guard +$38:3, invulnerability +$82, hitstun +$83<2; inflicts damage $200F ($A00F if airborne), directional knockback via 0x00E338/0x00E2D4 ($400/$3C0 speeds), vz=+$00020000, hitstun +$83=10, impale SFX #$8004D9; sub-mode != 0: toggles visibility bit +$36:1 for 15 frames +$2, then despawns entity +$0=0 and sets persistent bit via 0x018030."),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, opcodes and table words."""
    # Routine 1: Spirit Crystal Orb Switch
    assert u16_be(rom, 0x016CD0) == 0x6100       # bsr.w $18030
    assert u32_be(rom, 0x016D2C) == 0x009003A1   # chime activation SFX ID
    assert u32_be(rom, 0x016D32) == 0x0000E7FC   # jsr $E7FC.l (play sound)
    assert u32_be(rom, 0x016D50) == 0x00900238   # clink SFX ID
    assert u32_be(rom, 0x016D56) == 0x0000E7FC   # jsr $E7FC.l

    # Routine 2: Breakable Wall Block
    assert u16_be(rom, 0x016D76) == 0x6100       # bsr.w $18030
    assert u32_be(rom, 0x016DBA) == 0x0000E96C   # jsr $E96C.l (shatter debris)
    assert u32_be(rom, 0x016DC2) == 0x00900238   # clink SFX ID
    assert u32_be(rom, 0x016DC8) == 0x0000E7FC   # jsr $E7FC.l

    # Routine 3: Floor Spike Saw Hazard
    assert u16_be(rom, 0x016DDE) == 0x0034       # 52 frames sound timer
    assert u32_be(rom, 0x016DE4) == 0x00600262   # blade buzz SFX ID
    assert u32_be(rom, 0x016DEA) == 0x0000E7E8   # jsr $E7E8.l
    assert u32_be(rom, 0x016DFE) == 0x0000B922   # jsr $B922.l (AABB test vs Ali)
    assert u16_be(rom, 0x016E2E) == 0x200F       # hazard damage word $200F
    assert u32_be(rom, 0x016E48) == 0x0000E338   # jsr $E338.l (angle from delta)
    assert u16_be(rom, 0x016E52) == 0x0400       # speed x magnitude $400
    assert u16_be(rom, 0x016E56) == 0x03C0       # speed y magnitude $3C0
    assert u32_be(rom, 0x016E5A) == 0x0000E2D4   # jsr $E2D4.l (directional velocity)
    assert u32_be(rom, 0x016E68) == 0x00020000   # launch vz +$00020000
    assert u32_be(rom, 0x016E8A) == 0x008004D9   # impale SFX ID
    assert u32_be(rom, 0x016E90) == 0x0000E7E8   # jsr $E7E8.l
    assert u16_be(rom, 0x016EB2) == 0x6100       # bsr.w $18030


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })

    routines = []
    for name, r_start, r_end, exp_insns, desc in ROUTINE_BOUNDS:
        r_bytes = rom[r_start:r_end]
        r_insns = list(md.disasm(r_bytes, r_start))
        assert len(r_insns) == exp_insns, f"Instruction count mismatch for {name}: {len(r_insns)} != {exp_insns}"
        routines.append({
            "name": name,
            "start": f"0x{r_start:06X}",
            "end": f"0x{r_end:06X}",
            "size_bytes": r_end - r_start,
            "instruction_count": len(r_insns),
            "description": desc,
        })

    evidence = {
        "metadata": {
            "milestone": "M15.519",
            "rom_file": "Beyond Oasis (USA).md",
            "range_start": f"0x{RANGE_START:06X}",
            "range_end": f"0x{RANGE_END:06X}",
            "range_size_bytes": RANGE_SIZE,
            "instruction_count": len(instructions),
            "routine_count": len(routines),
        },
        "routines": routines,
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(evidence, indent=2))
    print(f"Wrote evidence JSON to {OUTPUT_PATH}")
    print(f"Total instructions extracted: {len(instructions)}")
    print(f"Routines defined: {len(routines)}")


if __name__ == "__main__":
    extract()
