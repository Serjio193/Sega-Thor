#!/usr/bin/env python3
"""Extract and verify ROM debug stage/room warp menu & spawn table (0x004C9C..0x004F20, 0x005D918)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_debug_warp_menu_m15_337.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x004D5A) == 0x0005D918  # Debug warp spawn table
    assert u32(rom, 0x004DCC) == 0x0005D918
    assert u16(rom, 0x004ECA) == 0x001F      # Max area index (31)
    assert u16(rom, 0x004E76) == 0x000A      # Initial repeat delay (10f)
    assert u16(rom, 0x004E90) == 0x0003      # Held repeat delay (3f)

    areas = []
    base = 0x0005D918
    for area_id in range(32):
        rel = s16(rom, base + area_id * 2)
        if rel == 0:
            continue
        area_addr = base + area_id * 2 + rel
        max_room = u16(rom, area_addr)
        rooms = []
        for room_id in range(max_room + 1):
            r_rel = s16(rom, area_addr + 2 + room_id * 2)
            if r_rel == 0:
                continue
            r_addr = area_addr + 2 + room_id * 2 + r_rel
            tx = rom[r_addr]
            ty = rom[r_addr + 1]
            rooms.append({"room_id": room_id, "spawn_x_px": tx << 3, "spawn_y_px": ty << 3})
        areas.append({"area_id": area_id, "max_room_id": max_room, "valid_rooms": len(rooms), "first_room": rooms[0] if rooms else None})

    return {
        "milestone": "M15.337",
        "title": "ROM Debug Stage/Room Warp Menu & Spawn Table",
        "routine": {"start": "0x004C9C", "end": "0x004F20"},
        "spawn_table_rom": "0x0005D918",
        "constants": {
            "max_area_index": 31,
            "repeat_initial_frames": 10,
            "repeat_held_frames": 3,
            "spawn_tile_shift": 3,
            "confirm_sfx": "0x00A00256",
            "cancel_sfx": "0x00A00347",
        },
        "active_area_count": len(areas),
        "areas": areas,
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
