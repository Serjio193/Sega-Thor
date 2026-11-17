#!/usr/bin/env python3
"""Extract and verify ROM room & area transition loader (0x00C8AA..0x00C96B)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_room_area_loader_m15_316.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def resolve_room_header(rom: bytes, area_ptrs: list[int], room_id: int) -> int:
    area = (room_id >> 8) & 0x0F
    room = room_id & 0xFF
    table_base = area_ptrs[area]
    entry_addr = table_base + (room * 2)
    return entry_addr + s16(rom, entry_addr)


def extract_evidence(rom: bytes) -> dict:
    # Verify RAM addresses in 0x00C8AA..0x00C90C
    assert u32(rom, 0x00C8AE) == 0x00FF0E40
    assert u32(rom, 0x00C8B8) == 0x00FF0E2C
    assert u32(rom, 0x00C8CA) == 0x00FF0E40
    assert u32(rom, 0x00C8D2) == 0x00FF0E1C
    assert u32(rom, 0x00C8E0) == 0x00FF0DB3
    assert u32(rom, 0x00C8EA) == 0x00FF0E40
    assert u32(rom, 0x00C904) == 0x00FF16EC

    # Verify entry offset +0x1A at 0x00C908 (lea $1a(a1), a0)
    assert u16(rom, 0x00C908) == 0x41E9
    assert u16(rom, 0x00C90A) == 0x001A

    area_ptrs = [u32(rom, 0x00C92C + i * 4) for i in range(16)]
    for ptr in area_ptrs:
        assert 0x00020000 <= ptr < len(rom)

    new_game_room_hdr = resolve_room_header(rom, area_ptrs, 0x001B)

    return {
        "milestone": "M15.316",
        "title": "ROM Room & Area Transition Loader",
        "routines": {
            "load_room_and_jump": {"start": "0x00C8AA", "end": "0x00C90C"},
            "get_room_header": {"start": "0x00C90E", "end": "0x00C92A"},
        },
        "area_table": {
            "address": "0x00C92C",
            "count": 16,
            "pointers": [f"0x{p:08X}" for p in area_ptrs],
        },
        "ram_state": {
            "current_room_id": "0x00FF0E40",
            "room_flags_16b": "0x00FF0E2C",
            "area_flags_16b": "0x00FF0E1C",
            "area_extra_3b": "0x00FF0DB3",
            "current_room_header_ptr": "0x00FF16EC",
        },
        "constants": {
            "room_entry_offset": "0x001A",
            "new_game_room_id": "0x001B",
            "new_game_room_header": f"0x{new_game_room_hdr:08X}",
        },
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
