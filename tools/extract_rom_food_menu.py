#!/usr/bin/env python3
"""Extract and verify ROM 4x4 food/consumable menu use & drop (0x00474E..0x00494A)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_food_menu_m15_329.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x004750) == 0x00FF0E51  # 4x4 food cursor
    assert u32(rom, 0x004838) == 0x00FF0D90  # 16-slot food inventory
    assert u16(rom, 0x004852) == 0x6100
    assert 0x004854 + struct.unpack_from(">h", rom, 0x004854)[0] == 0x00005236  # Consume check/effect routine
    assert u32(rom, 0x004864) == 0x00A00256  # Consume SFX
    assert u16(rom, 0x004880) == 0x005A      # 90-frame consume timer ($FF1856)
    assert u32(rom, 0x0048CA) == 0x0005CE16  # Item flags table (bit 5 = undroppable)
    assert u16(rom, 0x004916) == 0x0003      # col mask (4 cols)
    assert u16(rom, 0x00491A) == 0x0003      # row mask (4 rows)

    return {
        "milestone": "M15.329",
        "title": "ROM 4x4 Food/Consumable Menu Use & Drop",
        "routines": {
            "food_menu_loop": {"start": "0x00474E", "end": "0x004828"},
            "use_food_item": {"start": "0x00482A", "end": "0x004894"},
            "drop_food_item": {"start": "0x004896", "end": "0x004904"},
            "step_cursor_4x4": {"start": "0x004906", "end": "0x00494A"},
        },
        "constants": {
            "cursor_ram": "0x00FF0E51",
            "food_inventory_ram": "0x00FF0D90",
            "item_flags_table_rom": "0x0005CE16",
            "undroppable_flag_bit": 5,
            "consume_check_routine": "0x00005236",
            "consume_sfx": "0x00A00256",
            "consume_timer_frames": 90,
            "col_mask": 3,
            "row_mask": 3,
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
