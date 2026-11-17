#!/usr/bin/env python3
"""
extract_rom_combat_hit_response.py — ROM Entity Combat Hit Response,
Knockback & Trajectory Physics Calculator (0x00BE98..0x00C012, 378 bytes)
"""

import hashlib
import json
import os
import struct

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00BE98
END_ADDR = 0x00C012
SIZE = END_ADDR - START_ADDR  # 378 bytes

KNOCKBACK_TRAJECTORIES = {
    0: {
        "name": "light_knockback",
        "grounded": {"h_speed_x": 0x0300, "h_speed_y": 0x02C0, "v_impulse": 0},
        "airborne": {"h_speed_x": 0x0400, "h_speed_y": 0x03C0, "v_impulse": 0x00028000, "set_air_flag": True},
    },
    1: {
        "name": "none",
        "grounded": {"h_speed_x": 0, "h_speed_y": 0, "v_impulse": 0},
        "airborne": {"h_speed_x": 0, "h_speed_y": 0, "v_impulse": 0},
    },
    2: {
        "name": "heavy_knockback",
        "speed": {"h_speed_x": 0x0400, "h_speed_y": 0x03C0, "v_impulse": 0x00028000},
    },
    3: {
        "name": "severe_launch",
        "speed": {"h_speed_x": 0x0700, "h_speed_y": 0x0680, "v_impulse": 0x00030000},
    },
    "popup": {
        "name": "vertical_popup",
        "speed": {"h_speed_x": 0, "h_speed_y": 0, "v_impulse": 0x00070000},
    },
}

def is_shield_facing(attacker_facing: int, target_facing: int) -> bool:
    """bchg.b #1, d5; cmp.w target_facing, d5"""
    opp_facing = attacker_facing ^ 2
    return opp_facing == target_facing

def resolve_knockback_mode(packed_a4: int) -> int:
    """d6 = a4; swap d6; rol.w #3, d6; andi.w #6, d6"""
    dmg_word = (packed_a4 >> 16) & 0xFFFF
    rol3 = ((dmg_word << 3) | (dmg_word >> 13)) & 0xFFFF
    return (rol3 & 6) >> 1

def extract():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == SIZE, f"Expected {SIZE} bytes, got {len(routine_bytes)}"

    # Check key opcodes
    assert struct.unpack_from(">H", rom, 0x00BE98)[0] == 0xB1FC, "cmpa.l #$ff19e8, a0"
    assert struct.unpack_from(">H", rom, 0x00BEBA)[0] == 0x4A28, "tst.b $6d(a0)"
    assert struct.unpack_from(">H", rom, 0x00BED4)[0] == 0x48E7, "movem.l d0-d3, -(a7)"
    assert struct.unpack_from(">H", rom, 0x00BF26)[0] == 0x0008, "table entry 0"
    assert struct.unpack_from(">H", rom, 0x00BF28)[0] == 0x0070, "table entry 1"
    assert struct.unpack_from(">H", rom, 0x00BF2A)[0] == 0x002E, "table entry 2"
    assert struct.unpack_from(">H", rom, 0x00BF2C)[0] == 0x004C, "table entry 3"
    assert struct.unpack_from(">H", rom, 0x00BFAC)[0] == 0x4E75, "rts 1"
    assert struct.unpack_from(">H", rom, 0x00BFAE)[0] == 0x48E7, "movem.l d0-d3, -(a7) (blocked)"
    assert struct.unpack_from(">H", rom, 0x00C010)[0] == 0x4E75, "rts 2"

    data = {
        "routine": "Entity Combat Hit Response, Knockback & Trajectory Physics Calculator",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": SIZE,
        "knockback_trajectories": KNOCKBACK_TRAJECTORIES,
        "shield_block_sfx": "0x00900202",
        "sha256": hashlib.sha256(routine_bytes).hexdigest(),
    }

    out_path = "data/thor-evidence/rom_combat_hit_response_m15_423.json"
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"Extracted {SIZE} bytes, saved evidence to {out_path}")

if __name__ == "__main__":
    extract()
