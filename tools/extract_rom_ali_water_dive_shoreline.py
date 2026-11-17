#!/usr/bin/env python3
"""
extract_rom_ali_water_dive_shoreline.py — M15.380: ROM Ali Water Dive, Air Combat, Splash & Shoreline Exit
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x006D7A..0x007074:
  - 0x006D7A..0x006DD0: Water stroke paddle & coast transitions
  - 0x006DD2..0x006E78: Water dive jump & airborne gravity arc (0xC000 gravity)
  - 0x006E7A..0x006ED4: Mid-air attack dispatch from water jump
  - 0x006EDA..0x006F8A: Water jump slash and dagger air dive steppers
  - 0x006F8C..0x006FE2: Water surface splash landing & SFX 0x600343
  - 0x006FE4..0x007074: Shoreline boundary exit jump triggers (impulse 0x40000 / 0x50000, SFX 0x500341)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_water_dive_shoreline_m15_380.json"


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

    # 0x006D7A: Water stroke paddle & coast
    assert u16(rom, 0x006D7A) == 0x3D7C  # move.w #$14, $4a(a6)
    assert u16(rom, 0x006D80) == 0x5279  # addq.w #1, $FF1980.l
    assert u32(rom, 0x006D82) == 0x00FF1980
    assert u16(rom, 0x006D94) == 0x3D7C  # move.w #$24, $2a(a6)
    assert u16(rom, 0x006D9A) == 0x3D7C  # move.w #$10, $4(a6)
    assert u16(rom, 0x006DA0) == 0x4E75  # rts

    # 0x006DD2: Water dive jump airborne gravity arc
    assert u16(rom, 0x006DFE) == 0x0480  # subi.l #$c000, d0 (gravity)
    assert u32(rom, 0x006E00) == 0x0000C000
    assert u16(rom, 0x006E2A) == 0x0480  # subi.l #$c000, d0
    assert u16(rom, 0x006E40) == 0x3D7C  # move.w #$c, $30(a6) (landing timer)
    assert u16(rom, 0x006E6C) == 0x3D7C  # move.w #$4, $2a(a6)
    assert u16(rom, 0x006E72) == 0x3D7C  # move.w #$4, $4(a6)

    # 0x006E7A: Mid-air attack dispatch
    assert u16(rom, 0x006E7A) == 0x0839  # btst.b #4, $FF165F.l
    assert u16(rom, 0x006E8C) == 0x0C79  # cmpi.w #$c, $FF0D7E.l (dagger)
    assert u16(rom, 0x006E96) == 0x3D7C  # move.w #$32, $2a(a6) (sword slash)
    assert u16(rom, 0x006E9C) == 0x3D7C  # move.w #$26, $4(a6)
    assert u32(rom, 0x006EA2) == 0x203C0070  # move.l #$700209, d0
    assert u16(rom, 0x006EA6) == 0x0209
    assert u32(rom, 0x006EB2) == 0x203C0080  # move.l #$800223, d0
    assert u16(rom, 0x006EB6) == 0x0223
    assert u16(rom, 0x006EC0) == 0x323C  # move.w #$62, d1
    assert u32(rom, 0x006EC4) == 0x203C0080  # move.l #$800222, d0
    assert u16(rom, 0x006EC8) == 0x0222

    # 0x006F8C: Water surface splash landing
    assert u16(rom, 0x006F8C) == 0x3D7C  # move.w #0, $4a(a6)
    assert u16(rom, 0x006FC4) == 0x42AE  # clr.l $10(a6)
    assert u16(rom, 0x006FC8) == 0x42AE  # clr.l $56(a6)
    assert u32(rom, 0x006FCC) == 0x203C0060  # move.l #$600343, d0
    assert u16(rom, 0x006FD0) == 0x0343
    assert u16(rom, 0x006FD8) == 0x082E  # btst.b #1, $37(a6)

    # 0x006FE4: Shoreline boundary exit jump triggers
    assert u16(rom, 0x006FE4) == 0x6100  # bsr $7074
    assert u16(rom, 0x006FF8) == 0x3D7C  # move.w #$72, $2a(a6)
    assert u16(rom, 0x006FFE) == 0x3D7C  # move.w #$64, $4(a6)
    assert u16(rom, 0x007004) == 0x2D7C  # move.l #$40000, $56(a6) (jump impulse 0x40000)
    assert u32(rom, 0x007006) == 0x00040000
    assert u32(rom, 0x00700C) == 0x203C0050  # move.l #$500341, d0
    assert u16(rom, 0x007010) == 0x0341
    assert u16(rom, 0x00702E) == 0x3D7C  # move.w #$74, $2a(a6)
    assert u16(rom, 0x007034) == 0x3D7C  # move.w #$66, $4(a6)
    assert u32(rom, 0x00703C) == 0x00050000  # jump impulse 0x50000
    assert u16(rom, 0x007066) == 0x3D7C  # move.w #$76, $2a(a6)
    assert u16(rom, 0x00706C) == 0x3D7C  # move.w #$68, $4(a6)
    assert u16(rom, 0x007072) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.380",
        "description": "ROM Ali Water Dive, Air Combat, Splash & Shoreline Exit",
        "address_range": ["0x006D7A", "0x007074"],
        "byte_count": 0x007074 - 0x006D7A,
        "routines": [
            {
                "name": "ali_water_paddle_coast",
                "start": "0x006D7A",
                "end": "0x006DD0",
                "purpose": "Advance stroke counter $FF1980 and transition to anim 0x24 / state 0x10 on anim end"
            },
            {
                "name": "ali_water_dive_jump_arc",
                "start": "0x006DD2",
                "end": "0x006E78",
                "purpose": "Airborne gravity arc (subi.l #0xC000) for water dive jumps and landing recovery"
            },
            {
                "name": "ali_air_attack_dispatch_from_water",
                "start": "0x006E7A",
                "end": "0x006ED4",
                "purpose": "In-air weapon dispatch on Button A (sword anim 0x32, dagger anim 0x50/0x62)"
            },
            {
                "name": "ali_water_splash_landing",
                "start": "0x006F8C",
                "end": "0x006FE2",
                "purpose": "Water surface landing reset, height clear, and splash SFX 0x600343"
            },
            {
                "name": "ali_shoreline_boundary_exit_triggers",
                "start": "0x006FE4",
                "end": "0x007074",
                "purpose": "Shoreline climb-out jump impulse (0x40000/0x50000), splash SFX 0x500341, anims 0x72/0x74/0x76"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
