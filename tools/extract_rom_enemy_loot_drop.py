#!/usr/bin/env python3
"""Extract and verify ROM enemy loot / 2-tier drop-table roller (0x00F476..0x00F53A)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_enemy_loot_drop_m15_348.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x00F47C) == 0x008C      # Fixed drop ID offset 0x8C(a6)
    assert u16(rom, 0x00F4C8) == 0x0008      # Pickup entity type 0x08
    assert u32(rom, 0x00F4FC) == 0x00080000  # Pickup pop Z velocity (+8.0 px/frame)
    assert u16(rom, 0x00F50E) == 0x003F      # Lifetime jitter mask (0..63)
    assert u32(rom, 0x00F51A) == 0x0005CE16  # Item flags table (bit 7 check)

    return {
        "milestone": "M15.348",
        "title": "ROM Enemy Loot / 2-Tier Drop-Table Roller",
        "routine": {"start": "0x00F476", "end": "0x00F53A"},
        "constants": {
            "fixed_drop_offset": "0x8C",
            "no_drop_sentinel": "0xFFFF",
            "pickup_entity_type": "0x0008",
            "pickup_pop_z_vel_16_16": "0x00080000",
            "pickup_timer_jitter_mask": "0x3F",
            "item_flags_table_rom": "0x0005CE16",
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
