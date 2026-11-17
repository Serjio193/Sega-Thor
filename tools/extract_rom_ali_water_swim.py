#!/usr/bin/env python3
"""
extract_rom_ali_water_swim.py вЂ” M15.379: ROM Ali Pit Respawn Interpolation & Water Swimming Combat
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x006B2A..0x006D7A:
  - 0x006B2A..0x006B46: State 0x58 recovery / transition to State 0x5A
  - 0x006B48..0x006C38: Pit-Fall Respawn Interpolation Step (State 0x38)
  - 0x006C3A..0x006C44: Post-respawn idle animation gate
  - 0x006C46..0x006CCC: Water swimming stroke, speed reduction & idle tread
  - 0x006CCE..0x006D7A: Water weapon attack triggers (unarmed, dagger, sword, bow)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_water_swim_m15_379.json"


def u16(buf, offset):
    return struct.unpack_from(">H", buf, offset)[0]


def u32(buf, offset):
    return struct.unpack_from(">I", buf, offset)[0]


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        print(f"Error: ROM not found at {ROM_PATH}")
        sys.exit(1)

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    # Validate opcodes at key addresses
    assert u16(rom, 0x006B2A) == 0x42AE  # clr.l $72(a6)
    assert u16(rom, 0x006B2C) == 0x0072
    assert u16(rom, 0x006B2E) == 0x42AE  # clr.l $76(a6)
    assert u16(rom, 0x006B32) == 0x082E  # btst.b #1, $37(a6)
    assert u16(rom, 0x006B3A) == 0x3D7C  # move.w #$10, $2a(a6)
    assert u16(rom, 0x006B3C) == 0x0010
    assert u16(rom, 0x006B40) == 0x3D7C  # move.w #$5a, $4(a6)
    assert u16(rom, 0x006B42) == 0x005A
    assert u16(rom, 0x006B46) == 0x4E75  # rts

    # 0x006B48: Pit respawn interpolation step
    assert u16(rom, 0x006B48) == 0x362E  # move.w $10(a6), d3
    assert u16(rom, 0x006B4C) == 0x0C43  # cmpi.w #8, d3
    assert u16(rom, 0x006B58) == 0x41F9  # lea $FF1972.l, a0
    assert u32(rom, 0x006B5A) == 0x00FF1972
    assert u16(rom, 0x006B66) == 0x6100  # bsr $6B12
    assert u16(rom, 0x006B84) == 0x4A05  # tst.b d5
    assert u16(rom, 0x006BB6) == 0x3D7C  # move.w #2, $16(a6)
    assert u16(rom, 0x006BDA) == 0x41F9  # lea $3F306.l, a0
    assert u32(rom, 0x006BDC) == 0x0003F306
    assert u16(rom, 0x006C08) == 0x3D7C  # move.w #$3F, $32(a6)
    assert u16(rom, 0x006C16) == 0x4BF9  # lea $FF1AA4.l, a5
    assert u32(rom, 0x006C18) == 0x00FF1AA4
    assert u16(rom, 0x006C30) == 0x08B9  # bclr.b #2, $FF17B8.l

    # 0x006C46: Water swimming & combat
    assert u16(rom, 0x006C46) == 0x3D7C  # move.w #$14, $4a(a6)
    assert u16(rom, 0x006C4C) == 0x4A39  # tst.b $FF1988.l
    assert u32(rom, 0x006C4E) == 0x00FF1988
    assert u16(rom, 0x006C62) == 0x0C79  # cmpi.w #$10, $FF1980.l
    assert u32(rom, 0x006C64) == 0x001000FF
    assert u32(rom, 0x006C80) == 0x203C0050  # move.l #$500341, d0
    assert u16(rom, 0x006C84) == 0x0341
    assert u16(rom, 0x006C9C) == 0xE281  # asr.l #1, d1
    assert u16(rom, 0x006C9E) == 0xE282  # asr.l #1, d2
    assert u16(rom, 0x006CB6) == 0x3D7C  # move.w #$20, $2a(a6)
    assert u16(rom, 0x006CC6) == 0x3D7C  # move.w #$24, $2a(a6)

    # 0x006CCE: Weapon attack triggers in water
    assert u16(rom, 0x006CCE) == 0x3039  # move.w $FF0D7E.l, d0
    assert u16(rom, 0x006CE6) == 0x3D7C  # move.w #$48, $2a(a6) (unarmed)
    assert u16(rom, 0x006CEC) == 0x3D7C  # move.w #$42, $4(a6)
    assert u16(rom, 0x006CFA) == 0x3D7C  # move.w #$52, $2a(a6) (dagger)
    assert u16(rom, 0x006D00) == 0x3D7C  # move.w #$4a, $4(a6)
    assert u16(rom, 0x006D18) == 0x303C  # move.w #$56, d0 (sword)
    assert u16(rom, 0x006D2E) == 0x3D7C  # move.w #$4E, $4(a6)
    assert u32(rom, 0x006D34) == 0x203C0060  # move.l #$600377, d0
    assert u16(rom, 0x006D38) == 0x0377
    assert u16(rom, 0x006D52) == 0x3D7C  # move.w #$5A, $2a(a6) (bow)
    assert u16(rom, 0x006D58) == 0x3D7C  # move.w #$52, $4(a6)
    assert u16(rom, 0x006D78) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.379",
        "description": "ROM Ali Pit Respawn Interpolation & Water Swimming Combat",
        "address_range": ["0x006B2A", "0x006D7A"],
        "byte_count": 0x006D7A - 0x006B2A,
        "routines": [
            {
                "name": "ali_state58_recovery",
                "start": "0x006B2A",
                "end": "0x006B46",
                "purpose": "Clear velocity and transition to State 0x5A upon anim completion"
            },
            {
                "name": "ali_pit_respawn_interpolation_step",
                "start": "0x006B48",
                "end": "0x006C38",
                "purpose": "Step 2x interpolation towards safe anchor $FF1972, notify companion and exit state 0x38"
            },
            {
                "name": "ali_water_swimming_locomotion",
                "start": "0x006C46",
                "end": "0x006CCC",
                "purpose": "Water locomotion with 25% speed scaling, tread water idle, splash SFX 0x500341"
            },
            {
                "name": "ali_water_weapon_attacks",
                "start": "0x006CCE",
                "end": "0x006D7A",
                "purpose": "Water attack triggers: unarmed (0x42), dagger (0x4A), sword (0x4E/0x600377), bow (0x52)"
            }
        ],
        "default_animation_descriptors_3f306": [
            f"0x{u32(rom, 0x03F306):08X}",
            f"0x{u32(rom, 0x03F30A):08X}",
            f"0x{u32(rom, 0x03F30E):08X}"
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
