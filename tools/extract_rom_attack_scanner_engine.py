#!/usr/bin/env python3
"""
extract_rom_attack_scanner_engine.py — ROM Universal Attack Frame Scanner
& Hitbox Intersection Engine (0x00BC8E..0x00BE98, 522 bytes)
"""

import json
import os
import struct

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00BC8E
END_ADDR = 0x00BE98
SIZE = END_ADDR - START_ADDR  # 522 bytes

ENTRY_POINTS = {
    "scan_all_entities": {
        "address": 0x00BC8E,
        "callback": 0x00BEBA,
        "pool_base": 0x00FF19E8,
        "slot_count": 21,
        "slot_stride": 188,
    },
    "scan_group3_a": {
        "address": 0x00BCA4,
        "callback": 0x00BE98,
        "pool_base": 0x00FF1CD8,
        "slot_count": 17,
        "slot_stride": 188,
    },
    "scan_group3_b": {
        "address": 0x00BCBA,
        "callback": 0x00BE98,
        "pool_base": 0x00FF1CD8,
        "slot_count": 17,
        "slot_stride": 188,
        "mode_flag_198b": 2,
    },
    "scan_single": {
        "address": 0x00BCD8,
        "callback": 0x00BEBA,
        "pool_base": 0x00FF19E8,
        "slot_count": 1,
        "slot_stride": 188,
    },
}

STRENGTH_THRESHOLDS = [
    (10, 1),
    (20, 1),
    (30, 1),
    (40, 1),
    (50, 1),
    (100, 2),
    (200, 4),
]

def calculate_ali_strength_bonus(stat_96: int) -> int:
    bonus = 0
    for threshold, val in STRENGTH_THRESHOLDS:
        if stat_96 >= threshold:
            bonus += val
    return bonus

def pack_attack_damage(base_dmg: int, element_raw: int, stat_96: int,
                       weapon_bonus_90: int, is_ali: bool) -> tuple[int, int]:
    dmg = base_dmg
    if is_ali:
        dmg += calculate_ali_strength_bonus(stat_96)
    stun_duration = (element_raw & 0x0F) + 1
    element_flag = (element_raw & 0xF0) << 8
    total_dmg = (dmg | element_flag) + weapon_bonus_90
    packed_a4 = ((total_dmg & 0xFFFF) << 16) | (stun_duration & 0xFF)
    return total_dmg, packed_a4

def extract():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == SIZE, f"Expected {SIZE} bytes, got {len(routine_bytes)}"

    # Verify key opcodes
    assert struct.unpack_from(">H", rom, 0x00BC8E)[0] == 0x23FC, "move.l #$beba, $ff1940.l"
    assert struct.unpack_from(">I", rom, 0x00BC90)[0] == 0x0000BEBA
    assert struct.unpack_from(">I", rom, 0x00BC94)[0] == 0x00FF1940
    assert struct.unpack_from(">H", rom, 0x00BC98)[0] == 0x41F9, "lea $ff19e8.l, a0"
    assert struct.unpack_from(">H", rom, 0x00BCA4)[0] == 0x23FC, "move.l #$be98, $ff1940.l"
    assert struct.unpack_from(">I", rom, 0x00BCA6)[0] == 0x0000BE98
    assert struct.unpack_from(">H", rom, 0x00BCE8)[0] == 0x7E00, "moveq #0, d7"
    assert struct.unpack_from(">H", rom, 0x00BCEA)[0] == 0x082E, "btst #3, $37(a6)"
    assert struct.unpack_from(">H", rom, 0x00BE96)[0] == 0x4E75, "rts"

    data = {
        "routine": "Universal Attack Frame Scanner & Hitbox Intersection Engine",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": SIZE,
        "entry_points": ENTRY_POINTS,
        "strength_thresholds": STRENGTH_THRESHOLDS,
        "sha256": None,
    }

    import hashlib
    data["sha256"] = hashlib.sha256(routine_bytes).hexdigest()

    out_path = "data/thor-evidence/rom_attack_scanner_engine_m15_422.json"
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"Extracted {SIZE} bytes, saved evidence to {out_path}")

if __name__ == "__main__":
    extract()
