#!/usr/bin/env python3
"""
extract_rom_ali_command_special_motions.py — M15.384: ROM Ali Command Motions & Button-Mash Rapid Attack
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x007656..0x00780E:
  - 0x007656..0x0076C0: Special command motion dispatchers (0x8814, 0x86B4, 0x8778, 0x8910, 0x8950) & default combo hits
  - 0x0076C2..0x00770A: Button-mashing ring-buffer scanner (8 taps trigger frame 8 rapid-fire combo)
  - 0x00770C..0x007740: Slide attack state 0x40 / anim 0x46 (SFX 0x600343)
  - 0x007742..0x00776A: Somersault flip state 0x56 / anim 0x64 (vz = 0x84000, SFX 0x800220)
  - 0x00776C..0x0077AC: Dash leap state 0x62 / anim 0x70 (vz = 0x30000, speed 0x100, SFX 0x800220)
  - 0x0077AE..0x0077CC: Cartwheel state 0x72 / anim 0x7A (SFX 0x80025B)
  - 0x0077CE..0x00780E: Grand dash strike state 0x74 / anim 0x80 (speed 0x600, SFX 0x7004D3)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_command_special_motions_m15_384.json"


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

    # 0x007656: Button A & motion recognizers
    assert u16(rom, 0x007656) == 0x0839  # btst.b #4, $FF165E.l
    assert u16(rom, 0x007660) == 0x4EB9  # jsr $8814.l
    assert u32(rom, 0x007662) == 0x00008814
    assert u16(rom, 0x00766A) == 0x4EB9  # jsr $86B4.l
    assert u32(rom, 0x00766C) == 0x000086B4
    assert u16(rom, 0x007674) == 0x4EB9  # jsr $8778.l
    assert u32(rom, 0x007676) == 0x00008778
    assert u16(rom, 0x00767E) == 0x4EB9  # jsr $8910.l
    assert u32(rom, 0x007680) == 0x00008910
    assert u16(rom, 0x007688) == 0x4EB9  # jsr $8950.l
    assert u32(rom, 0x00768A) == 0x00008950

    # Default combo hits
    assert u16(rom, 0x007692) == 0x4279  # clr.w $FF16EA.l
    assert u32(rom, 0x007698) == 0x203C0070  # move.l #$7004d3, d0
    assert u16(rom, 0x0076AC) == 0x3D7C  # move.w #$8, $2a(a6) (hit 1)
    assert u16(rom, 0x0076B4) == 0x3D7C  # move.w #$3c, $2a(a6) (hit 2)
    assert u16(rom, 0x0076BA) == 0x3D7C  # move.w #$8, $4(a6)

    # 0x0076C2: Button-mash ring buffer scan
    assert u16(rom, 0x0076C4) == 0x41F9  # lea $FF1668.l, a0
    assert u16(rom, 0x0076F2) == 0x0C40  # cmpi.w #8, d0 (8 taps threshold)
    assert u16(rom, 0x007700) == 0x3D7C  # move.w #8, $30(a6)

    # 0x00770C: Slide attack state 0x40
    assert u16(rom, 0x00770C) == 0x3D7C  # move.w #$46, $2a(a6)
    assert u16(rom, 0x007712) == 0x3D7C  # move.w #$40, $4(a6)
    assert u32(rom, 0x007734) == 0x203C0060  # move.l #$600343, d0

    # 0x007742: Somersault flip state 0x56
    assert u16(rom, 0x007742) == 0x3D7C  # move.w #$64, $2a(a6)
    assert u16(rom, 0x007748) == 0x3D7C  # move.w #$56, $4(a6)
    assert u32(rom, 0x007764) == 0x00084000  # vz = 0x84000

    # 0x00776C: Dash leap state 0x62
    assert u16(rom, 0x00776C) == 0x3D7C  # move.w #$70, $2a(a6)
    assert u16(rom, 0x007772) == 0x3D7C  # move.w #$62, $4(a6)
    assert u32(rom, 0x007780) == 0x00030000  # vz = 0x30000
    assert u32(rom, 0x0077A0) == 0x203C0080  # move.l #$800220, d0

    # 0x0077AE: Cartwheel state 0x72
    assert u16(rom, 0x0077AE) == 0x3D7C  # move.w #$7a, $2a(a6)
    assert u16(rom, 0x0077B4) == 0x3D7C  # move.w #$72, $4(a6)
    assert u32(rom, 0x0077C0) == 0x203C0080  # move.l #$80025b, d0

    # 0x0077CE: Grand dash strike state 0x74
    assert u16(rom, 0x0077CE) == 0x3D7C  # move.w #$80, $2a(a6)
    assert u16(rom, 0x0077D4) == 0x3D7C  # move.w #$74, $4(a6)
    assert u16(rom, 0x0077DA) == 0x323C  # move.w #$600, d1
    assert u16(rom, 0x0077DC) == 0x0600
    assert u16(rom, 0x0077DE) == 0x343C  # move.w #$600, d2
    assert u16(rom, 0x0077E0) == 0x0600
    assert u16(rom, 0x00780C) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.384",
        "description": "ROM Ali Command Motions & Button-Mash Rapid Attack",
        "address_range": ["0x007656", "0x00780E"],
        "byte_count": 0x00780E - 0x007656,
        "routines": [
            {
                "name": "ali_motion_dispatcher",
                "start": "0x007656",
                "end": "0x0076C0",
                "purpose": "Dispatches special motions 0x8814, 0x86B4, 0x8778, 0x8910, 0x8950, or default hits"
            },
            {
                "name": "ali_button_mash_counter",
                "start": "0x0076C2",
                "end": "0x00770A",
                "purpose": "Scans 8 ring-buffer frames for Button A taps; >=8 taps forces rapid-fire combo at frame 8"
            },
            {
                "name": "ali_motion_slide_attack",
                "start": "0x00770C",
                "end": "0x007740",
                "purpose": "Ground slide state 0x40 / anim 0x46 with SFX 0x600343 and 18-frame invulnerability"
            },
            {
                "name": "ali_motion_somersault_flip",
                "start": "0x007742",
                "end": "0x00776A",
                "purpose": "Super somersault state 0x56 / anim 0x64 with vertical impulse 0x84000 and SFX 0x800220"
            },
            {
                "name": "ali_motion_dash_leap",
                "start": "0x00776C",
                "end": "0x0077AC",
                "purpose": "Dash leap state 0x62 / anim 0x70 with vertical impulse 0x30000, speed 0x100, SFX 0x800220"
            },
            {
                "name": "ali_motion_cartwheel",
                "start": "0x0077AE",
                "end": "0x0077CC",
                "purpose": "Cartwheel evasion state 0x72 / anim 0x7A with SFX 0x80025B"
            },
            {
                "name": "ali_motion_grand_dash_strike",
                "start": "0x0077CE",
                "end": "0x00780E",
                "purpose": "Grand dash strike state 0x74 / anim 0x80 with speed 0x600 and SFX 0x7004D3"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
