#!/usr/bin/env python3
"""
extract_rom_ali_spirit_power_inspector.py — M15.390: ROM Ali Spirit Power Invocation, Item Proximity Scanner, Object Inspector & 4-Direction Normalizer
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x00822C..0x0083F8:
  - 0x00822C..0x0082AC: Spirit Power Button A invocation (Air call state 0x16 / anim 0x16 vs Ground summon state 0x10 / anim 0x10 with vertical impulse vz = 0x70000)
  - 0x0082AE..0x0082F6: Item pickup proximity scanner (12x12 bounding box [-6..+6], entity type 8 absorption, coordinate & ID packing into $FF1976)
  - 0x0082F8..0x0083D2: Interactive object & NPC inspector (4-way directional probe box, angle check 0xE338, NPC 0x32, Save Book 0x20, Cutscene 0x94/0x96/0xBE, Chest 0xB0 with SFX 0xA00264)
  - 0x0083D4..0x0083F8: 4-direction rotation normalizer (maps facings 0->0, 1->3, 2->1, 3->2)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_spirit_power_inspector_m15_390.json"


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

    # 0x00822C: Spirit Power Button A
    assert u16(rom, 0x00822C) == 0x4A39  # tst.b $FF1985.l
    assert u32(rom, 0x00822E) == 0x00FF1985
    assert u16(rom, 0x00824E) == 0x2D7C  # move.l #$70000, $56(a6)
    assert u32(rom, 0x008250) == 0x00070000
    assert u16(rom, 0x00825C) == 0x3D7C  # move.w #$16, $2a(a6)
    assert u16(rom, 0x00825E) == 0x0016
    assert u16(rom, 0x008262) == 0x3D7C  # move.w #$16, 4(a6)
    assert u16(rom, 0x008264) == 0x0016
    assert u16(rom, 0x008280) == 0x2D7C  # move.l #$70000, $56(a6)
    assert u32(rom, 0x008282) == 0x00070000
    assert u16(rom, 0x00828E) == 0x3D7C  # move.w #$10, $2a(a6)
    assert u16(rom, 0x008290) == 0x0010
    assert u16(rom, 0x008294) == 0x3D7C  # move.w #$10, 4(a6)
    assert u16(rom, 0x008296) == 0x0010

    # 0x0082AE: Item pickup proximity scanner
    assert u16(rom, 0x0082AE) == 0x7206  # moveq #6, d1
    assert u16(rom, 0x0082C0) == 0x0C68  # cmpi.w #8, 0(a0) (pickup entity type)
    assert u16(rom, 0x0082C2) == 0x0008
    assert u16(rom, 0x0082C8) == 0x4268  # clr.w 0(a0) (absorb)
    assert u16(rom, 0x0082CA) == 0x0000
    assert u16(rom, 0x0082D6) == 0x33C0  # move.w d0, $FF1976.l
    assert u32(rom, 0x0082D8) == 0x00FF1976

    # 0x0082F8: Interactive object inspector
    assert u16(rom, 0x008352) == 0x3028  # move.w 0(a0), d0
    assert u16(rom, 0x008354) == 0x0000
    assert u16(rom, 0x008356) == 0x0C40  # cmpi.w #$32, d0 (NPC)
    assert u16(rom, 0x008358) == 0x0032
    assert u16(rom, 0x00835E) == 0x0C40  # cmpi.w #$20, d0 (Save book)
    assert u16(rom, 0x008360) == 0x0020
    assert u16(rom, 0x008366) == 0x0C40  # cmpi.w #$94, d0
    assert u16(rom, 0x00837E) == 0x0C40  # cmpi.w #$b0, d0 (Chest)
    assert u16(rom, 0x008380) == 0x00B0
    assert u32(rom, 0x00839E) == 0x203C00A0  # move.l #$a00264, d0 (chest SFX)
    assert u16(rom, 0x0083A2) == 0x0264
    assert u16(rom, 0x0083B0) == 0x3D7C  # move.w #$3f, $32(a6)
    assert u16(rom, 0x0083B2) == 0x003F

    # 0x0083D4: 4-direction normalizer
    assert u16(rom, 0x0083D4) == 0x322E  # move.w $16(a6), d1
    assert u16(rom, 0x0083D6) == 0x0016
    assert u16(rom, 0x0083F4) == 0x7201  # moveq #1, d1
    assert u16(rom, 0x0083F6) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.390",
        "description": "ROM Ali Spirit Power Invocation, Item Proximity Scanner, Object Inspector & 4-Direction Normalizer",
        "address_range": ["0x00822C", "0x0083F8"],
        "byte_count": 0x0083F8 - 0x00822C,
        "routines": [
            {
                "name": "ali_spirit_power_invoke",
                "start": "0x00822C",
                "end": "0x0082AC",
                "purpose": "Invokes Spirit Power: Air call (state 0x16, anim 0x16) vs Ground summon (state 0x10, anim 0x10) with vertical impulse vz = 0x70000"
            },
            {
                "name": "ali_item_proximity_scanner",
                "start": "0x0082AE",
                "end": "0x0082F6",
                "purpose": "Scans 12x12 area for type 8 pickup entities, absorbs target into $FF1976, and returns carry clear"
            },
            {
                "name": "ali_object_npc_inspector",
                "start": "0x0082F8",
                "end": "0x0083D2",
                "purpose": "Evaluates 4-way probe box in front of Ali, verifies angle via 0xE338, and routes NPC 0x32, Save Book 0x20, Cutscene 0x94/0x96/0xBE, or Chest 0xB0 with SFX 0xA00264"
            },
            {
                "name": "ali_facing_direction_normalizer",
                "start": "0x0083D4",
                "end": "0x0083F8",
                "purpose": "Normalizes 4-way facing direction indices (0->0, 1->3, 2->1, 3->2)"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
