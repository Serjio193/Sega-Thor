#!/usr/bin/env python3
"""Extract and verify ROM ring menu 4x2 cursor wrap & item drop handler (0x00462E..0x0046E2)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ring_menu_drop_m15_328.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x00463E) == 0x0003      # col mask (4 cols)
    assert u16(rom, 0x004642) == 0x0001      # row mask (2 rows)
    assert u32(rom, 0x00464A) == 0x00FF0E50  # cursor byte RAM
    assert u32(rom, 0x004650) == 0x006003A6  # cursor move SFX
    assert u32(rom, 0x00467C) == 0x00FF0DB0  # equipped/protected slot
    assert u32(rom, 0x00468C) == 0x00009C40  # terrain drop-check routine
    assert u32(rom, 0x0046A0) == 0x00FF0D80  # 8-slot inventory RAM
    assert u32(rom, 0x0046BA) == 0x00FF1864  # dropped item ID (| 0x80)
    assert u32(rom, 0x0046C2) == 0x00FF1865  # dropped item qty

    return {
        "milestone": "M15.328",
        "title": "ROM Ring Menu 4x2 Cursor Wrap & Item Drop Handler",
        "routines": {
            "step_cursor_4x2": {"start": "0x00462E", "end": "0x004670"},
            "try_drop_inventory_item": {"start": "0x004674", "end": "0x0046E2"},
        },
        "constants": {
            "col_mask": 3,
            "row_mask": 1,
            "cursor_move_sfx": "0x006003A6",
            "min_droppable_item_id": 2,
            "drop_flag_bit": 7,
            "inventory_ram": "0x00FF0D80",
            "protected_slot_ram": "0x00FF0DB0",
            "drop_item_id_ram": "0x00FF1864",
            "drop_item_qty_ram": "0x00FF1865",
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
