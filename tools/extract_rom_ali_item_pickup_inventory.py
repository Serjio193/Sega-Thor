#!/usr/bin/env python3
"""
extract_rom_ali_item_pickup_inventory.py — M15.386: ROM Ali Item Pickup Dispatcher, Inventory Storage & Upgrades
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x007A28..0x007C3C:
  - 0x007A28..0x007A68: Item ID branch table ($FF1976: weapons 0x01..0x0C, jewels 0x0D..0x10, rank 0x11, spirit jewels 0x12..0x15, keys 0x16..0x19, food 0x1A..0x3F)
  - 0x007A6C..0x007AF0: Item consumption classification table 0x05CE56 & SFX triggers (0x508, 0x50D, 0xC0035F, 0xC00360, 0xC00361)
  - 0x007AF0..0x007B28: Region chest collection bitmask recorder ($FF0DBC, $FF0E1C, $FF0E2C)
  - 0x007B64..0x007B82: 8-slot weapon inventory scanner ($FF0D80)
  - 0x007B84..0x007BA2: 16-slot consumable/food inventory scanner ($FF0D90)
  - 0x007BA4..0x007BD2: Spirit jewel level tracker ($FF0DB6) & SP capacity boost (+5)
  - 0x007BD4..0x007BE6: Gem counter increments ($FF0DB2)
  - 0x007BE8..0x007BF4: Magic key / medal bitmask setter ($FF0DBA)
  - 0x007BF6..0x007C1E: Ali rank upgrade ($FF1A7E < 200, HP = level*2 + 0xC6, SP += 2)
  - 0x007C20..0x007C3C: Full bag reject buzzer (anim 0x44, state 0x3C, SFX 0xC00255)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_item_pickup_inventory_m15_386.json"


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

    # 0x007A28: Item ID branch table
    assert u16(rom, 0x007A28) == 0x082E  # btst.b #1, $37(a6)
    assert u16(rom, 0x007A32) == 0x1039  # move.b $FF1976.l, d0
    assert u32(rom, 0x007A34) == 0x00FF1976
    assert u16(rom, 0x007A3C) == 0x0C00  # cmpi.b #$c, d0
    assert u16(rom, 0x007A3E) == 0x000C
    assert u16(rom, 0x007A44) == 0x0C00  # cmpi.b #$10, d0
    assert u16(rom, 0x007A46) == 0x0010
    assert u16(rom, 0x007A4C) == 0x0C00  # cmpi.b #$11, d0
    assert u16(rom, 0x007A4E) == 0x0011
    assert u16(rom, 0x007A54) == 0x0C00  # cmpi.b #$15, d0
    assert u16(rom, 0x007A56) == 0x0015
    assert u16(rom, 0x007A5C) == 0x0C00  # cmpi.b #$19, d0
    assert u16(rom, 0x007A5E) == 0x0019
    assert u16(rom, 0x007A64) == 0x0C00  # cmpi.b #$3f, d0
    assert u16(rom, 0x007A66) == 0x003F

    # 0x007A6C: Classification table 0x05CE56 & SFX
    assert u16(rom, 0x007A6C) == 0x43F9  # lea $5CE56.l, a1
    assert u32(rom, 0x007A6E) == 0x0005CE56
    assert u32(rom, 0x007A8A) == 0x203C00C0  # move.l #$c0035f, d0
    assert u32(rom, 0x007A94) == 0x203C00C0  # move.l #$c00360, d0
    assert u32(rom, 0x007AA0) == 0x203C00C0  # move.l #$c00361, d0
    assert u32(rom, 0x007ACA) == 0x203C0000  # move.l #$50d, d0
    assert u16(rom, 0x007ACE) == 0x050D
    assert u32(rom, 0x007AD8) == 0x203C0000  # move.l #$508, d0
    assert u16(rom, 0x007ADC) == 0x0508
    assert u16(rom, 0x007AE4) == 0x3D7C  # move.w #$44, $4(a6)
    assert u16(rom, 0x007AEA) == 0x3D7C  # move.w #$4a, $2a(a6)

    # 0x007AF0: Region chest collection bits
    assert u32(rom, 0x007AF8) == 0x43F900FF  # lea $FF0DBC.l, a1
    assert u16(rom, 0x007AFC) == 0x0DBC
    assert u16(rom, 0x007B24) == 0x03F1  # bset.b d1, (a1, d0.w)

    # 0x007B64: Weapon inventory scan
    assert u32(rom, 0x007B64) == 0x41F900FF  # lea $FF0D80.l, a0
    assert u16(rom, 0x007B68) == 0x0D80
    assert u16(rom, 0x007B6A) == 0x7007  # moveq #7, d0 (8 slots)

    # 0x007B84: Food inventory scan
    assert u32(rom, 0x007B84) == 0x41F900FF  # lea $FF0D90.l, a0
    assert u16(rom, 0x007B88) == 0x0D90
    assert u16(rom, 0x007B8A) == 0x700F  # moveq #$f, d0 (16 slots)

    # 0x007BA4: Spirit jewel
    assert u16(rom, 0x007BA4) == 0x0400  # subi.b #$12, d0
    assert u16(rom, 0x007BA6) == 0x0012
    assert u32(rom, 0x007BAA) == 0x41F900FF  # lea $FF0DB6.l, a0
    assert u16(rom, 0x007BAE) == 0x0DB6
    assert u16(rom, 0x007BC4) == 0x5A79  # addq.w #5, $FF1A74.l
    assert u16(rom, 0x007BCA) == 0x5A79  # addq.w #5, $FF1A76.l

    # 0x007BD4: Gem counter
    assert u16(rom, 0x007BD4) == 0x0400  # subi.b #$d, d0
    assert u16(rom, 0x007BD6) == 0x000D
    assert u32(rom, 0x007BDA) == 0x41F900FF  # lea $FF0DB2.l, a0
    assert u16(rom, 0x007BDE) == 0x0DB2

    # 0x007BE8: Key bits
    assert u16(rom, 0x007BE8) == 0x0400  # subi.b #$16, d0
    assert u16(rom, 0x007BEA) == 0x0016
    assert u16(rom, 0x007BEC) == 0x01F9  # bset.b d0, $FF0DBA.l
    assert u32(rom, 0x007BEE) == 0x00FF0DBA

    # 0x007BF6: Rank upgrade
    assert u16(rom, 0x007BF6) == 0x3039  # move.w $FF1A7E.l, d0
    assert u32(rom, 0x007BF8) == 0x00FF1A7E
    assert u16(rom, 0x007BFC) == 0x0C40  # cmpi.w #$c8, d0 (cap 200)
    assert u16(rom, 0x007BFE) == 0x00C8
    assert u16(rom, 0x007C0C) == 0x0640  # addi.w #$c6, d0
    assert u16(rom, 0x007C0E) == 0x00C6
    assert u16(rom, 0x007C16) == 0x5479  # addq.w #2, $FF1A72.l
    assert u32(rom, 0x007C18) == 0x00FF1A72

    # 0x007C20: Full bag reject
    assert u16(rom, 0x007C20) == 0x3D7C  # move.w #$44, $2a(a6)
    assert u16(rom, 0x007C26) == 0x3D7C  # move.w #$3c, $4(a6)
    assert u32(rom, 0x007C2C) == 0x203C00C0  # move.l #$c00255, d0 (full bag buzzer SFX)
    assert u16(rom, 0x007C30) == 0x0255

    evidence = {
        "milestone": "M15.386",
        "description": "ROM Ali Item Pickup Dispatcher, Inventory Storage, Jewel Upgrades & Full-Bag Audio Rejection",
        "address_range": ["0x007A28", "0x007C3C"],
        "byte_count": 0x007C3C - 0x007A28,
        "routines": [
            {
                "name": "ali_item_pickup_dispatcher",
                "start": "0x007A28",
                "end": "0x007A68",
                "purpose": "Evaluates item category for $FF1976 (weapons <= 0xC, gems <= 0x10, rank 0x11, spirit jewels <= 0x15, keys <= 0x19, food <= 0x3F)"
            },
            {
                "name": "ali_item_consume_sfx_classifier",
                "start": "0x007A6C",
                "end": "0x007AF0",
                "purpose": "Reads classification table 0x05CE56 and dispatches SFX 0x508, 0x50D, 0xC0035F, 0xC00360, 0xC00361"
            },
            {
                "name": "ali_chest_collection_bit_recorder",
                "start": "0x007AF0",
                "end": "0x007B28",
                "purpose": "Sets persistent chest/item collected bit in region bitmasks ($FF0DBC, $FF0E1C, $FF0E2C)"
            },
            {
                "name": "ali_weapon_inventory_storage",
                "start": "0x007B64",
                "end": "0x007B82",
                "purpose": "Searches 8 weapon slots at $FF0D80; stores item into first free slot or branches to full-bag reject"
            },
            {
                "name": "ali_food_inventory_storage",
                "start": "0x007B84",
                "end": "0x007BA2",
                "purpose": "Searches 16 food/consumable slots at $FF0D90; stores item into first free slot or branches to full-bag reject"
            },
            {
                "name": "ali_spirit_jewel_tracker",
                "start": "0x007BA4",
                "end": "0x007BD2",
                "purpose": "Increments spirit jewel level in $FF0DB6 (max 15) and boosts SP capacities by +5"
            },
            {
                "name": "ali_gem_key_rank_upgrades",
                "start": "0x007BD4",
                "end": "0x007C1E",
                "purpose": "Increments gem counts in $FF0DB2, sets key bits in $FF0DBA, and computes Ali rank level up (HP = level*2 + 0xC6, SP += 2)"
            },
            {
                "name": "ali_full_bag_audio_reject",
                "start": "0x007C20",
                "end": "0x007C3C",
                "purpose": "Plays full bag error buzzer SFX 0xC00255 with rejection state 0x3C and anim 0x44"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
