#!/usr/bin/env python3
"""
extract_rom_ali_sword_spin_bow_fire.py — M15.382: ROM Ali Sword Spin, Elemental Blade & Bow Arrow Dispatcher
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x007262..0x0074A4:
  - 0x007262..0x0072E8: Ground Sword 360-spin attack, button-A steering, SFX 0x600379, elemental blade 0x0000F258
  - 0x0072EA..0x007362: Water Sword 360-spin attack, stroke counter increment, water blade 0x0000F26A
  - 0x007364..0x0073EA: Ground Bow fire, button-A steering, SFX 0x7004D3, arrow spawner 0x0000F396
  - 0x0073EC..0x00743A: Water Bow fire, water arrow spawner 0x0000F3A8
  - 0x00743C..0x0074A4: Water Dagger flurry, SFX 0x800222, and shoreline exit state 0x54
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_sword_spin_bow_fire_m15_382.json"


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

    # 0x007262: Ground Sword 360-Spin
    assert u16(rom, 0x007262) == 0x302E  # move.w $30(a6), d0
    assert u16(rom, 0x007266) == 0x0C40  # cmpi.w #$c, d0
    assert u16(rom, 0x00726E) == 0x0C40  # cmpi.w #$10, d0
    assert u16(rom, 0x007274) == 0x0839  # btst.b #4, $FF165E.l
    assert u32(rom, 0x007282) == 0x203C0060  # move.l #$600379, d0
    assert u16(rom, 0x007286) == 0x0379
    assert u16(rom, 0x00728E) == 0x1039  # move.b $FF0DB1.l, d0
    assert u32(rom, 0x0072AE) == 0x203C8000  # move.l #$8000001c, d0 (fire sword)
    assert u16(rom, 0x0072B2) == 0x001C
    assert u32(rom, 0x0072B6) == 0x203C4000  # move.l #$4000001c, d0 (omega sword)
    assert u16(rom, 0x0072BA) == 0x001C
    assert u32(rom, 0x0072BE) == 0x203C2000  # move.l #$2000001c, d0 (hyper sword)
    assert u16(rom, 0x0072C2) == 0x001C
    assert u16(rom, 0x0072DC) == 0xE481  # asr.l #2, d1
    assert u16(rom, 0x0072DE) == 0xE482  # asr.l #2, d2

    # 0x0072EA: Water Sword 360-Spin
    assert u16(rom, 0x0072EA) == 0x3D7C  # move.w #$14, $4a(a6)
    assert u16(rom, 0x0072F0) == 0x5279  # addq.w #1, $FF1980.l
    assert u32(rom, 0x007306) == 0x203C0060  # move.l #$600379, d0
    assert u16(rom, 0x00730A) == 0x0379
    assert u32(rom, 0x007332) == 0x203C8000  # move.l #$8000000d, d0
    assert u16(rom, 0x007336) == 0x000D
    assert u16(rom, 0x007356) == 0x3D7C  # move.w #$24, $2a(a6)
    assert u16(rom, 0x00735C) == 0x3D7C  # move.w #$10, $4(a6)

    # 0x007364: Ground Bow Fire
    assert u16(rom, 0x007364) == 0x302E  # move.w $30(a6), d0
    assert u16(rom, 0x007368) == 0x0C40  # cmpi.w #4, d0
    assert u16(rom, 0x007370) == 0x0C40  # cmpi.w #8, d0
    assert u32(rom, 0x007384) == 0x203C0070  # move.l #$7004d3, d0 (bow release SFX)
    assert u16(rom, 0x007388) == 0x04D3
    assert u16(rom, 0x007390) == 0x700C  # moveq #$c, d0
    assert u16(rom, 0x007398) == 0x6702  # beq.b $739c (facing 2 down fires at frame 12)
    assert u16(rom, 0x00739A) == 0x5840  # addq.w #4, d0 (other facings fire at frame 16)
    assert u16(rom, 0x0073B0) == 0x0400  # subi.b #9, d0
    assert u16(rom, 0x0073C6) == 0x3D7C  # move.w #$ffff, $5c(a6)

    # 0x0073EC: Water Bow Fire
    assert u16(rom, 0x0073EC) == 0x3D7C  # move.w #$14, $4a(a6)
    assert u16(rom, 0x00740E) == 0x0400  # subi.b #9, d0
    assert u16(rom, 0x007420) == 0x3D7C  # move.w #$ffff, $5c(a6)

    # 0x00743C: Water Dagger Flurry
    assert u32(rom, 0x007452) == 0x203C0080  # move.l #$800222, d0
    assert u16(rom, 0x007456) == 0x0222
    assert u16(rom, 0x007496) == 0x3D7C  # move.w #$60, $2a(a6)
    assert u16(rom, 0x00749C) == 0x3D7C  # move.w #$54, $4(a6)
    assert u16(rom, 0x0074A2) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.382",
        "description": "ROM Ali Sword Spin, Elemental Blade & Bow Arrow Dispatcher",
        "address_range": ["0x007262", "0x0074A4"],
        "byte_count": 0x0074A4 - 0x007262,
        "routines": [
            {
                "name": "ali_ground_sword_spin",
                "start": "0x007262",
                "end": "0x0072E8",
                "purpose": "Ground 360-spin attack, 25% steering, sword spin SFX 0x600379, elemental blade 0xF258"
            },
            {
                "name": "ali_water_sword_spin",
                "start": "0x0072EA",
                "end": "0x007362",
                "purpose": "Water 360-spin attack at frame 12, elemental payload, water blade 0xF26A"
            },
            {
                "name": "ali_ground_bow_fire",
                "start": "0x007364",
                "end": "0x0073EA",
                "purpose": "Ground bow release SFX 0x7004D3, facing 2 fire at frame 12 / others at frame 16, arrow spawner 0xF396"
            },
            {
                "name": "ali_water_bow_fire",
                "start": "0x0073EC",
                "end": "0x00743A",
                "purpose": "Water bow fire at frame 12, water arrow spawner 0xF3A8"
            },
            {
                "name": "ali_water_dagger_flurry",
                "start": "0x00743C",
                "end": "0x0074A4",
                "purpose": "Water dagger strike SFX 0x800222, shoreline transition to state 0x54 on land probe"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
