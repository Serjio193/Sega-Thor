#!/usr/bin/env python3
"""
extract_rom_ali_weapon_attacks.py — M15.389: ROM Ali Weapon Combat Attack Initializers (Sword, Bow, Bomb & Sprint Strikes)
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x00803C..0x00822C:
  - 0x00803C..0x00808E: Sword attack variations (stationary slash 0x4E/0x46, running slash 0x60/0x54, leap slash 0x66/0x5C)
  - 0x008090..0x0080D6: Bow draw (SFX 0x600377, anim 0x54/0x4C, Fire Bow 0x5C/0x4C) & Bomb placement (anim 0x58/0x50)
  - 0x0080D8..0x00814C: Running sprint tackle (anim 0x40/0x38, timer 0x15) & dagger dash leap/somersault motion routes
  - 0x00814E..0x00820C: Mount / grabbed entity combat strikes (anim 0x42/0x3A and 0x3E/0x36)
  - 0x00820E..0x00822C: Attack commit finalizer ($FF198B = 4, $2A = d0, 4 = d1, clear $FF16EA, unroll stack, rts)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_weapon_attacks_m15_389.json"


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

    # 0x00803C: Sword attack variations
    assert u16(rom, 0x00803C) == 0x4EB9  # jsr $8814.l
    assert u32(rom, 0x00803E) == 0x00008814
    assert u16(rom, 0x00806C) == 0x303C  # move.w #$60, d0 (running slash)
    assert u16(rom, 0x00806E) == 0x0060
    assert u16(rom, 0x008070) == 0x323C  # move.w #$54, d1
    assert u16(rom, 0x008072) == 0x0054
    assert u16(rom, 0x008078) == 0x303C  # move.w #$4e, d0 (stationary slash)
    assert u16(rom, 0x00807A) == 0x004E
    assert u16(rom, 0x00807C) == 0x323C  # move.w #$46, d1
    assert u16(rom, 0x00807E) == 0x0046
    assert u16(rom, 0x008084) == 0x303C  # move.w #$66, d0 (leap slash)
    assert u16(rom, 0x008086) == 0x0066
    assert u16(rom, 0x008088) == 0x323C  # move.w #$5c, d1
    assert u16(rom, 0x00808A) == 0x005C

    # 0x008090: Bow & Bomb attacks
    assert u32(rom, 0x00809A) == 0x203C0060  # move.l #$600377, d0 (bow draw SFX)
    assert u16(rom, 0x00809E) == 0x0377
    assert u16(rom, 0x0080A6) == 0x303C  # move.w #$54, d0
    assert u16(rom, 0x0080A8) == 0x0054
    assert u16(rom, 0x0080AA) == 0x323C  # move.w #$4c, d1
    assert u16(rom, 0x0080AC) == 0x004C
    assert u16(rom, 0x0080AE) == 0x0C39  # cmpi.b #8, $FF0DB1.l (Fire Bow)
    assert u16(rom, 0x0080B0) == 0x0008
    assert u32(rom, 0x0080B2) == 0x00FF0DB1
    assert u16(rom, 0x0080BA) == 0x303C  # move.w #$5c, d0 (Fire Bow anim)
    assert u16(rom, 0x0080BC) == 0x005C
    assert u16(rom, 0x0080CC) == 0x303C  # move.w #$58, d0 (Bomb placement)
    assert u16(rom, 0x0080CE) == 0x0058
    assert u16(rom, 0x0080D0) == 0x323C  # move.w #$50, d1
    assert u16(rom, 0x0080D2) == 0x0050

    # 0x0080D8: Sprint dash attacks
    assert u16(rom, 0x0080D8) == 0x0C79  # cmpi.w #$c, $FF0D7E.l
    assert u16(rom, 0x0080DA) == 0x000C
    assert u32(rom, 0x0080DC) == 0x00FF0D7E
    assert u16(rom, 0x0080E4) == 0x0C79  # cmpi.w #$a, $FF0D7E.l
    assert u16(rom, 0x0080E6) == 0x000A
    assert u32(rom, 0x0080E8) == 0x00FF0D7E
    assert u16(rom, 0x00810E) == 0x303C  # move.w #$40, d0 (Sprint tackle)
    assert u16(rom, 0x008110) == 0x0040
    assert u16(rom, 0x008112) == 0x323C  # move.w #$38, d1
    assert u16(rom, 0x008114) == 0x0038
    assert u16(rom, 0x008116) == 0x08EE  # bset.b #2, $39(a6)
    assert u16(rom, 0x008118) == 0x0002
    assert u16(rom, 0x00811A) == 0x0039
    assert u16(rom, 0x00811C) == 0x1D7C  # move.b #$15, $9a(a6) (21 frames)
    assert u16(rom, 0x00811E) == 0x0015
    assert u16(rom, 0x008120) == 0x009A

    # 0x00814E: Mount grab strikes
    assert u16(rom, 0x008176) == 0x303C  # move.w #$42, d0
    assert u16(rom, 0x008178) == 0x0042
    assert u16(rom, 0x00817A) == 0x323C  # move.w #$3a, d1
    assert u16(rom, 0x00817C) == 0x003A
    assert u16(rom, 0x008206) == 0x303C  # move.w #$3e, d0
    assert u16(rom, 0x008208) == 0x003E
    assert u16(rom, 0x00820A) == 0x323C  # move.w #$36, d1
    assert u16(rom, 0x00820C) == 0x0036

    # 0x00820E: Commit finalizer
    assert u16(rom, 0x00820E) == 0x13FC  # move.b #4, $FF198B.l
    assert u16(rom, 0x008210) == 0x0004
    assert u32(rom, 0x008212) == 0x00FF198B
    assert u16(rom, 0x008216) == 0x3D40  # move.w d0, $2a(a6)
    assert u16(rom, 0x00821A) == 0x3D41  # move.w d1, 4(a6)
    assert u16(rom, 0x00821E) == 0x4279  # clr.w $FF16EA.l
    assert u32(rom, 0x008220) == 0x00FF16EA
    assert u16(rom, 0x008224) == 0x2E79  # movea.l $FF1934.l, a7
    assert u32(rom, 0x008226) == 0x00FF1934
    assert u16(rom, 0x00822A) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.389",
        "description": "ROM Ali Weapon Combat Attack Initializers (Sword, Bow, Bomb & Sprint Strikes)",
        "address_range": ["0x00803C", "0x00822C"],
        "byte_count": 0x00803C - 0x00822C,
        "routines": [
            {
                "name": "ali_sword_attack_dispatch",
                "start": "0x00803C",
                "end": "0x00808E",
                "purpose": "Initializes Sword attacks: leap slash (anim 0x66, state 0x5C), running slash (anim 0x60, state 0x54), stationary slash (anim 0x4E, state 0x46)"
            },
            {
                "name": "ali_bow_bomb_attack_dispatch",
                "start": "0x008090",
                "end": "0x0080D6",
                "purpose": "Initializes Bow draw with SFX 0x600377 (standard 0x54/0x4C, Fire Bow 0x5C/0x4C) and Bomb placement (0x58/0x50)"
            },
            {
                "name": "ali_sprint_dash_attacks",
                "start": "0x0080D8",
                "end": "0x00814C",
                "purpose": "Initializes running sprint tackle (anim 0x40, state 0x38, timer 0x15) and evaluates special motion cascades"
            },
            {
                "name": "ali_mount_grab_attacks",
                "start": "0x00814E",
                "end": "0x00820C",
                "purpose": "Initializes combat strikes while mounting/grabbing entities (anims 0x42/0x3A and 0x3E/0x36)"
            },
            {
                "name": "ali_attack_commit_finalize",
                "start": "0x00820E",
                "end": "0x00822C",
                "purpose": "Commits animation, state, recovery timer $FF198B = 4, clears input ring buffer count, and unrolls stack"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
