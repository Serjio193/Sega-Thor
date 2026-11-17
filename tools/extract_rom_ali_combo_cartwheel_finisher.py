#!/usr/bin/env python3
"""
extract_rom_ali_combo_cartwheel_finisher.py — M15.385: ROM Ali Combat Combo Chains, 6-Cartwheel Super Finisher & Ripple Stun Callback
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x00780E..0x007A28:
  - 0x00780E..0x007850: Ali rapid spin 360-degree facing rotation (frames 4, 8, 12, 16)
  - 0x007852..0x0078F6: Ali combat combo chain evaluator (anim 0x0A / state 0x08 vs anim 0x3A / state 0x0A)
  - 0x0078F8..0x007928: Ali combo flurry enemy stun cleanup ($FF1B60 == 0x3A)
  - 0x00792A..0x0079FA: Ali cartwheel chain counter ($FF1991), 0x7C/0x7E alternation & 6-cartwheel super finisher
  - 0x0079FC..0x007A0E: Landing friction / ground stop callback ($FF1940)
  - 0x007A10..0x007A28: Air dive landing recovery & shockwave callback at frame 24
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_combo_cartwheel_finisher_m15_385.json"


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

    # 0x00780E: Rapid spin facing direction rotation
    assert u16(rom, 0x00780E) == 0x7203  # moveq #3, d1
    assert u16(rom, 0x007814) == 0x0C40  # cmpi.w #4, d0
    assert u16(rom, 0x007816) == 0x0004
    assert u16(rom, 0x00781C) == 0x0C40  # cmpi.w #8, d0
    assert u16(rom, 0x00781E) == 0x0008
    assert u16(rom, 0x007824) == 0x0C40  # cmpi.w #$c, d0
    assert u16(rom, 0x007826) == 0x000C
    assert u16(rom, 0x00782C) == 0x3D41  # move.w d1, $16(a6)
    assert u16(rom, 0x007836) == 0x3D7C  # move.w #2, $16(a6)

    # 0x007852: Combo chain evaluator & motion check
    assert u16(rom, 0x007878) == 0x4EB9  # jsr $8814.l (Dash Leap)
    assert u32(rom, 0x00787A) == 0x00008814
    assert u16(rom, 0x007882) == 0x4EB9  # jsr $86B4.l (Slide)
    assert u32(rom, 0x007884) == 0x000086B4
    assert u16(rom, 0x00788C) == 0x4EB9  # jsr $8778.l (Somersault)
    assert u32(rom, 0x00788E) == 0x00008778
    assert u16(rom, 0x007896) == 0x4EB9  # jsr $8910.l (Cartwheel)
    assert u32(rom, 0x007898) == 0x00008910
    assert u16(rom, 0x0078A0) == 0x4EB9  # jsr $8950.l (Grand Dash Strike)
    assert u32(rom, 0x0078A2) == 0x00008950
    assert u16(rom, 0x0078AA) == 0x0839  # btst.b #4, $FF165E.l
    assert u16(rom, 0x0078B6) == 0x3D7C  # move.w #$a, $2a(a6)
    assert u16(rom, 0x0078BC) == 0x3D7C  # move.w #$8, $4(a6)
    assert u32(rom, 0x0078C2) == 0x203C0070  # move.l #$7004d3, d0
    assert u16(rom, 0x0078EA) == 0x3D7C  # move.w #$3a, $2a(a6)
    assert u16(rom, 0x0078F0) == 0x3D7C  # move.w #$a, $4(a6)

    # 0x0078F8: Flurry cleanup
    assert u32(rom, 0x0078F8) == 0x41F900FF  # lea $FF1B60.l, a0
    assert u16(rom, 0x0078FE) == 0x0C68  # cmpi.w #$3a, 0(a0)

    # 0x00792A: Cartwheel chain counter & super finisher
    assert u16(rom, 0x00794C) == 0x23FC  # move.l #$79fc, $FF1940.l
    assert u32(rom, 0x00794E) == 0x000079FC
    assert u16(rom, 0x007962) == 0x4EB9  # jsr $8910.l
    assert u32(rom, 0x007964) == 0x00008910
    assert u16(rom, 0x007972) == 0x5239  # addq.b #1, $FF1991.l
    assert u32(rom, 0x007974) == 0x00FF1991
    assert u16(rom, 0x007978) == 0x0C39  # cmpi.b #6, $FF1991.l
    assert u16(rom, 0x00797A) == 0x0006
    assert u32(rom, 0x00797C) == 0x00FF1991
    assert u32(rom, 0x007982) == 0x203C0080  # move.l #$80025b, d0
    assert u16(rom, 0x00798E) == 0x3D7C  # move.w #$72, $4(a6)
    assert u16(rom, 0x007994) == 0x0C6E  # cmpi.w #$7a, $2a(a6)
    assert u16(rom, 0x00799C) == 0x3D7C  # move.w #$7c, $2a(a6)
    assert u16(rom, 0x0079A4) == 0x3D7C  # move.w #$7e, $2a(a6)

    # 0x0079AC: 6-Cartwheel Super Finisher
    assert u16(rom, 0x0079B6) == 0x2D7C  # move.l #$40000, $56(a6) (vz = +4.0 px/frame)
    assert u32(rom, 0x0079B8) == 0x00040000
    assert u16(rom, 0x0079C2) == 0x3D7C  # move.w #$f, $10(a6) (elevation 15)
    assert u16(rom, 0x0079C4) == 0x000F
    assert u16(rom, 0x0079C8) == 0x323C  # move.w #$800, d1
    assert u16(rom, 0x0079CC) == 0x343C  # move.w #$800, d2
    assert u16(rom, 0x0079D4) == 0x4EB9  # jsr $E2D4.l (vector table)
    assert u32(rom, 0x0079D6) == 0x0000E2D4
    assert u16(rom, 0x0079E2) == 0x3D7C  # move.w #$32, $2a(a6) (anim 0x32 = Super Jump Kick)
    assert u16(rom, 0x0079E8) == 0x3D7C  # move.w #$26, $4(a6) (state 0x26 = Air Finisher)

    # 0x0079FC: Landing ground stop callback
    assert u16(rom, 0x007A00) == 0x4AA8  # tst.l $56(a0)
    assert u16(rom, 0x007A02) == 0x0056
    assert u16(rom, 0x007A06) == 0x42A8  # clr.l $4E(a0)
    assert u16(rom, 0x007A0A) == 0x42A8  # clr.l $52(a0)

    # 0x007A10: Air dive recovery
    assert u16(rom, 0x007A1A) == 0x0C6E  # cmpi.w #$18, $30(a6)
    assert u16(rom, 0x007A1C) == 0x0018
    assert u16(rom, 0x007A26) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.385",
        "description": "ROM Ali Combat Combo Chains, 6-Cartwheel Super Finisher & Ripple Stun Callback",
        "address_range": ["0x00780E", "0x007A28"],
        "byte_count": 0x007A28 - 0x00780E,
        "routines": [
            {
                "name": "ali_rapid_spin_facing_rotation",
                "start": "0x00780E",
                "end": "0x007850",
                "purpose": "Calculates Ali 360-degree facing direction (3, 2, 1, 0) at frames 4, 8, 12, 16"
            },
            {
                "name": "ali_combo_chain_evaluator",
                "start": "0x007852",
                "end": "0x0078F6",
                "purpose": "Evaluates command motions and button transitions between Combo Hit 1 (anim 0x0A, state 8) and Combo Hit 2 (anim 0x3A, state 10)"
            },
            {
                "name": "ali_combo_flurry_cleanup",
                "start": "0x0078F8",
                "end": "0x007928",
                "purpose": "Cleans up target enemy stun callback at 0xFF1B60 when anim matches 0x3A"
            },
            {
                "name": "ali_cartwheel_chain_finisher",
                "start": "0x00792A",
                "end": "0x0079FA",
                "purpose": "Tracks cartwheel counter $FF1991 (alternates anim 0x7C/0x7E); on 6th cartwheel triggers Super Finisher (vz 0x40000, elevation 15, state 0x26, anim 0x32, SFX 0x80025B)"
            },
            {
                "name": "ali_cartwheel_landing_callback",
                "start": "0x0079FC",
                "end": "0x007A0E",
                "purpose": "Ground stop callback: clears horizontal speed when vz reaches 0"
            },
            {
                "name": "ali_air_dive_recovery",
                "start": "0x007A10",
                "end": "0x007A28",
                "purpose": "Air dive recovery and frame 24 ground impact shockwave invocation"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
