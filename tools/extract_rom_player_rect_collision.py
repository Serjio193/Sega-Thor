#!/usr/bin/env python3
"""
extract_rom_player_rect_collision.py — ROM Tile Rect vs Player Ali AABB
Containment & Overlap Collision Testers (0x00C17A..0x00C222, 168 bytes)
"""

import hashlib
import json
import os
import struct

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C17A
END_ADDR = 0x00C222
SIZE = END_ADDR - START_ADDR  # 168 bytes

PLAYER_ADDR = 0x00FF19E8

def unpack_tile_rect(d1: int, d2: int) -> tuple[tuple[int, int], tuple[int, int]]:
    """
    Unpacks packed tile rect registers:
    d1: (x_min_tile << 16) | y_min_tile
    d2: (x_max_tile << 16) | y_max_tile
    Scaled by 8 with +7 pixel extent on max bounds.
    Returns ((rect_x_min, rect_x_max), (rect_y_min, rect_y_max)) in pixels.
    """
    # asl.l #3, d1
    d1_scaled = (d1 << 3) & 0xFFFFFFFF
    # asl.l #3, d2; addi.l #$70007, d2
    d2_scaled = ((d2 << 3) + 0x00070007) & 0xFFFFFFFF

    rect_y_min = d1_scaled & 0xFFFF
    rect_y_max = d2_scaled & 0xFFFF
    rect_x_min = (d1_scaled >> 16) & 0xFFFF
    rect_x_max = (d2_scaled >> 16) & 0xFFFF
    return (rect_x_min, rect_x_max), (rect_y_min, rect_y_max)

def player_inside_rect(d1: int, d2: int, player_x: int, player_y: int, player_rad: int) -> bool:
    """Emulates 0x00C17A: True if player AABB is completely inside tile rect."""
    (rx0, rx1), (ry0, ry1) = unpack_tile_rect(d1, d2)
    px0 = player_x - player_rad
    px1 = player_x + player_rad
    py0 = player_y - player_rad
    py1 = player_y + player_rad

    if px0 < rx0 or px1 > rx1:
        return False
    if py0 < ry0 or py1 > ry1:
        return False
    return True

def player_overlaps_rect(d1: int, d2: int, player_x: int, player_y: int, player_rad: int) -> bool:
    """Emulates 0x00C1CE: True if player AABB overlaps tile rect."""
    (rx0, rx1), (ry0, ry1) = unpack_tile_rect(d1, d2)
    px0 = player_x - player_rad
    px1 = player_x + player_rad
    py0 = player_y - player_rad
    py1 = player_y + player_rad

    if px0 > rx1 or px1 < rx0:
        return False
    if py0 > ry1 or py1 < ry0:
        return False
    return True

def extract():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == SIZE, f"Expected {SIZE} bytes, got {len(routine_bytes)}"

    # Check key opcodes
    assert struct.unpack_from(">H", rom, 0x00C17A)[0] == 0x48E7, "movem.l d0-d6/a0, -(a7) (inside)"
    assert struct.unpack_from(">H", rom, 0x00C17E)[0] == 0xE781, "asl.l #3, d1"
    assert struct.unpack_from(">H", rom, 0x00C182)[0] == 0x0682, "addi.l #$70007, d2"
    assert struct.unpack_from(">I", rom, 0x00C184)[0] == 0x00070007
    assert struct.unpack_from(">I", rom, 0x00C192)[0] == PLAYER_ADDR
    assert struct.unpack_from(">H", rom, 0x00C1BE)[0] == 0x003C, "ori.b #1, ccr"
    assert struct.unpack_from(">H", rom, 0x00C1CC)[0] == 0x4E75, "rts 1"

    assert struct.unpack_from(">H", rom, 0x00C1CE)[0] == 0x48E7, "movem.l d0-d6/a0, -(a7) (overlap)"
    assert struct.unpack_from(">H", rom, 0x00C212)[0] == 0x003C, "ori.b #1, ccr"
    assert struct.unpack_from(">H", rom, 0x00C220)[0] == 0x4E75, "rts 2"

    data = {
        "routine": "Tile Rect vs Player Ali AABB Containment & Overlap Collision Testers",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": SIZE,
        "player_address": f"0x{PLAYER_ADDR:08X}",
        "entry_points": {
            "player_inside_rect": "0x00C17A",
            "player_overlaps_rect": "0x00C1CE",
        },
        "tile_scale_shift": 3,
        "max_extent_bias": "0x00070007",
        "sha256": hashlib.sha256(routine_bytes).hexdigest(),
    }

    out_path = "data/thor-evidence/rom_player_rect_collision_m15_426.json"
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"Extracted {SIZE} bytes, saved evidence to {out_path}")

if __name__ == "__main__":
    extract()
