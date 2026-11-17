#!/usr/bin/env python3
"""
extract_rom_ali_jump_weapon_swap.py — M15.388: ROM Ali Jump Initiation, Weapon Hotkey Swap & Dagger Attack Dispatcher
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x007E46..0x00803C:
  - 0x007E46..0x007EC2: Jump button C initiation & ledge grab state 0x14 / anim 0x14 with SFX 0x900359 and stack unroll
  - 0x007EC4..0x007F80: Weapon quick-swap manager (empty slot SFX 0xA00253; equipped weapon index 0x0A Dagger, 0x0C Sword, 0x0E Bow, 0x10 Bomb; HUD toast 60f 0x3C; equip SFX 0xC0025D)
  - 0x007F82..0x00803C: Attack button B dispatcher & Dagger strike initialization (anim 0x3A, state 0x0A, combo timer $FF198B = 4, stack unroll)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_jump_weapon_swap_m15_388.json"


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

    # 0x007E46: Jump initiation
    assert u16(rom, 0x007E46) == 0x4A39  # tst.b $FF1985.l
    assert u32(rom, 0x007E48) == 0x00FF1985
    assert u16(rom, 0x007E50) == 0x13FC  # move.b #1, $FF1985.l
    assert u16(rom, 0x007E5C) == 0x0C6E  # cmpi.w #4, 4(a6)
    assert u16(rom, 0x007E5E) == 0x0004
    assert u16(rom, 0x007E78) == 0x3D7C  # move.w #$14, $2a(a6)
    assert u16(rom, 0x007E7A) == 0x0014
    assert u16(rom, 0x007E7E) == 0x3D7C  # move.w #$14, 4(a6)
    assert u16(rom, 0x007E80) == 0x0014
    assert u16(rom, 0x007E84) == 0x51F9  # sf.b $FF1990.l
    assert u32(rom, 0x007E8A) == 0x203C0090  # move.l #$900359, d0
    assert u16(rom, 0x007E8E) == 0x0359
    assert u16(rom, 0x007E96) == 0x2E79  # movea.l $FF1934.l, a7 (stack unroll)
    assert u32(rom, 0x007E98) == 0x00FF1934

    # 0x007EC4: Weapon quick swap
    assert u16(rom, 0x007EC4) == 0x0839  # btst.b #4, $FF165F.l
    assert u32(rom, 0x007EC8) == 0x00FF165F
    assert u16(rom, 0x007EE6) == 0x41F9  # lea $FF0D80.l, a0
    assert u32(rom, 0x007EE8) == 0x00FF0D80
    assert u32(rom, 0x007EF2) == 0x203C00A0  # move.l #$a00253, d0 (empty buzzer)
    assert u16(rom, 0x007EF6) == 0x0253
    assert u16(rom, 0x007F00) == 0x13C0  # move.b d0, $FF0DB0.l
    assert u32(rom, 0x007F02) == 0x00FF0DB0
    assert u16(rom, 0x007F0A) == 0x13C0  # move.b d0, $FF0DB1.l
    assert u32(rom, 0x007F0C) == 0x00FF0DB1
    assert u16(rom, 0x007F10) == 0x0C00  # cmpi.b #1, d0
    assert u16(rom, 0x007F16) == 0x0C00  # cmpi.b #5, d0
    assert u16(rom, 0x007F1C) == 0x0C00  # cmpi.b #8, d0
    assert u16(rom, 0x007F22) == 0x0C00  # cmpi.b #$b, d0
    assert u16(rom, 0x007F32) == 0x33C0  # move.w d0, $FF0D7E.l
    assert u32(rom, 0x007F34) == 0x00FF0D7E
    assert u16(rom, 0x007F38) == 0x4279  # clr.w $FF19EE.l
    assert u16(rom, 0x007F4E) == 0x33FC  # move.w #$3c, $FF1856.l
    assert u16(rom, 0x007F56) == 0x50F9  # st.b $FF198F.l
    assert u32(rom, 0x007F5C) == 0x203C00C0  # move.l #$c0025d, d0 (equip chime)
    assert u16(rom, 0x007F60) == 0x025D

    # 0x007F82: Attack router & Dagger strike
    assert u16(rom, 0x007F82) == 0x4A39  # tst.b $FF1985.l
    assert u16(rom, 0x007F8C) == 0x0839  # btst.b #3, $FF1660.l
    assert u16(rom, 0x008002) == 0x3039  # move.w $FF0D7E.l, d0
    assert u16(rom, 0x008008) == 0x0C40  # cmpi.w #$c, d0 (Sword)
    assert u16(rom, 0x00800A) == 0x000C
    assert u16(rom, 0x00800E) == 0x0C40  # cmpi.w #$e, d0 (Bow)
    assert u16(rom, 0x008010) == 0x000E
    assert u16(rom, 0x008014) == 0x0C40  # cmpi.w #$10, d0 (Bomb)
    assert u16(rom, 0x008016) == 0x0010
    assert u16(rom, 0x00801C) == 0x303C  # move.w #$3a, d0
    assert u16(rom, 0x00801E) == 0x003A
    assert u16(rom, 0x008020) == 0x323C  # move.w #$a, d1
    assert u16(rom, 0x008022) == 0x000A
    assert u16(rom, 0x008024) == 0x3D40  # move.w d0, $2a(a6)
    assert u16(rom, 0x008028) == 0x3D41  # move.w d1, 4(a6)
    assert u16(rom, 0x00802C) == 0x13FC  # move.b #4, $FF198B.l
    assert u16(rom, 0x008034) == 0x2E79  # movea.l $FF1934.l, a7
    assert u16(rom, 0x00803A) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.388",
        "description": "ROM Ali Jump Initiation, Weapon Hotkey Swap & Dagger Attack Dispatcher",
        "address_range": ["0x007E46", "0x00803C"],
        "byte_count": 0x00803C - 0x007E46,
        "routines": [
            {
                "name": "ali_jump_initiation",
                "start": "0x007E46",
                "end": "0x007EC2",
                "purpose": "Jump Button C entry: verifies ground state, sets ledge/jump state 0x14 / anim 0x14, plays SFX 0x900359, unrolls stack"
            },
            {
                "name": "ali_weapon_hotkey_swap",
                "start": "0x007EC4",
                "end": "0x007F80",
                "purpose": "Weapon quick-swap: scans slot at $FF0D80, dispatches weapon type (0x0A Dagger, 0x0C Sword, 0x0E Bow, 0x10 Bomb), HUD toast 60f, SFX 0xC0025D or 0xA00253"
            },
            {
                "name": "ali_attack_weapon_router",
                "start": "0x007F82",
                "end": "0x00803C",
                "purpose": "Evaluates weapon index at $FF0D7E (routes 0x0C Sword, 0x0E Bow, 0x10 Bomb) and initializes Dagger strike (anim 0x3A, state 0x0A, combo timer $FF198B = 4)"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
