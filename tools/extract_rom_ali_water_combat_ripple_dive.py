#!/usr/bin/env python3
"""
extract_rom_ali_water_combat_ripple_dive.py — M15.383: ROM Ali Water Combat Ripple Physics & Air Dive
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x0074A4..0x007656:
  - 0x0074A4..0x00751E: Water ripple directional impulse callback 0x74A4 (isometric speeds 0x800/0x780, vz = 0x20000)
  - 0x007520..0x00754E: Weapon element / durability table 0x7548 (random 1..6 on 0xFFFF)
  - 0x007550..0x007582: Water dagger strike at frame 8 with SFX 0x800223 and ripple mode 6
  - 0x007584..0x007630: Dagger air somersault, high dive impulse 0x80000 at frame 36, SFX 0x500341, 0xC000 gravity
  - 0x007632..0x007656: Multi-hit audio table 0x7632 and target stun callback 0x763C (12-frame hitstun)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_water_combat_ripple_dive_m15_383.json"


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

    # 0x0074A4: Water ripple directional impulse callback
    assert u16(rom, 0x0074A4) == 0x6100  # bsr $BE98
    assert u16(rom, 0x0074AC) == 0x362E  # move.w $16(a6), d3
    assert u16(rom, 0x0074FC) == 0x323C  # move.w #$800, d1
    assert u16(rom, 0x0074FE) == 0x0800
    assert u16(rom, 0x007500) == 0x343C  # move.w #$780, d2
    assert u16(rom, 0x007502) == 0x0780
    assert u16(rom, 0x007512) == 0x217C  # move.l #$20000, $56(a0)
    assert u32(rom, 0x007514) == 0x00020000
    assert u16(rom, 0x00751E) == 0x4E75  # rts

    # 0x007520: Weapon durability table & PRNG roll
    assert u16(rom, 0x007520) == 0x4240  # clr.w d0
    assert u16(rom, 0x007522) == 0x1039  # move.b $FF0DB1.l, d0
    assert u16(rom, 0x007528) == 0x5500  # subq.b #2, d0
    assert u16(rom, 0x00753A) == 0x7005  # moveq #5, d0
    assert u16(rom, 0x007546) == 0x4E75  # rts
    assert u16(rom, 0x007548) == 0x0000  # table word 0
    assert u16(rom, 0x00754A) == 0x0001  # table word 1
    assert u16(rom, 0x00754C) == 0xFFFF  # table word 2 (PRNG trigger)
    assert u16(rom, 0x00754E) == 0x2003  # table word 3

    # 0x007550: Water dagger strike
    assert u16(rom, 0x007550) == 0x0C6E  # cmpi.w #8, $30(a6)
    assert u32(rom, 0x00755E) == 0x203C0080  # move.l #$800223, d0
    assert u16(rom, 0x007562) == 0x0223

    # 0x007584: Dagger air somersault & high dive
    assert u16(rom, 0x0075AC) == 0x0C42  # cmpi.w #$24, d2 (frame 36)
    assert u16(rom, 0x0075AE) == 0x0024
    assert u16(rom, 0x0075B2) == 0x2D7C  # move.l #$80000, $56(a6) (vz = +8.0 px/frame)
    assert u32(rom, 0x0075B4) == 0x00080000
    assert u32(rom, 0x0075BA) == 0x203C0050  # move.l #$500341, d0
    assert u16(rom, 0x0075BE) == 0x0341
    assert u16(rom, 0x007612) == 0x0480  # subi.l #$c000, d0 (gravity)
    assert u32(rom, 0x007614) == 0x0000C000
    assert u16(rom, 0x007628) == 0x3D7C  # move.w #$48, $30(a6) (landing timer 72)
    assert u16(rom, 0x00762A) == 0x0048

    # 0x00763C: Target stun callback
    assert u16(rom, 0x00763C) == 0x6100  # bsr $BE98
    assert u16(rom, 0x007640) == 0x4AA8  # tst.l $56(a0)
    assert u16(rom, 0x00764E) == 0x117C  # move.b #$c, $83(a0)
    assert u16(rom, 0x007650) == 0x000C
    assert u16(rom, 0x007654) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.383",
        "description": "ROM Ali Water Combat Ripple Physics & Air Dive",
        "address_range": ["0x0074A4", "0x007656"],
        "byte_count": 0x007656 - 0x0074A4,
        "routines": [
            {
                "name": "ali_ripple_impulse_callback",
                "start": "0x0074A4",
                "end": "0x00751E",
                "purpose": "Directional water ripple velocity calculation (speeds 0x800/0x780, vz = 0x20000)"
            },
            {
                "name": "ali_weapon_durability_table",
                "start": "0x007520",
                "end": "0x00754E",
                "purpose": "Weapon element / durability index lookup [0, 1, 0xFFFF, 0x2003] with random 1..6"
            },
            {
                "name": "ali_water_dagger_strike_frame8",
                "start": "0x007550",
                "end": "0x007582",
                "purpose": "Water dagger strike at frame 8 with SFX 0x800223 and ripple mode 6"
            },
            {
                "name": "ali_dagger_somersault_high_dive",
                "start": "0x007584",
                "end": "0x007630",
                "purpose": "High dive impulse 0x80000 at frame 36, SFX 0x500341, 0xC000 gravity, landing timer 0x48"
            },
            {
                "name": "ali_target_hitstun_callback",
                "start": "0x00763C",
                "end": "0x007656",
                "purpose": "Clears target horizontal speed and applies 12-frame hitstun timer +0x83 = 0x0C"
            }
        ],
        "durability_table_7548": ["0x0000", "0x0001", "0xFFFF", "0x2003"]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
