#!/usr/bin/env python3
"""Extract and verify ROM consumable item validation & HP/SP/spirit restore (0x005236..0x005472)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_consumable_effects_m15_330.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00523E) == 0x00FF0E40  # Area/room ID (blocked if area == 0x10)
    assert u32(rom, 0x0052B2) == 0x00FF1A70  # MaxHP
    assert u32(rom, 0x0052B8) == 0x00FF1A72  # CurHP
    assert u32(rom, 0x0052C6) == 0x00FF1A74  # MaxSP
    assert u32(rom, 0x0052CC) == 0x00FF1A76  # CurSP
    assert u32(rom, 0x005310) == 0x00FF1864  # Pending consumed item ID
    assert u32(rom, 0x0053EE) == 0x00A00234  # Heal SFX
    assert u32(rom, 0x005446) == 0x00A00234  # Heal SFX

    jump_table = [f"0x{0x00532C + s16(rom, 0x00532C + i * 2):06X}" for i in range(17)]

    return {
        "milestone": "M15.330",
        "title": "ROM Consumable Item Validation & HP/SP/Spirit Restore Engine",
        "routines": {
            "can_use_consumable": {"start": "0x005236", "end": "0x00530A"},
            "apply_consumable": {"start": "0x00530C", "end": "0x005472"},
        },
        "jump_table_0x00532C": jump_table,
        "constants": {
            "blocked_area_id": "0x10",
            "item_id_min": "0x1A",
            "item_id_max": "0x2A",
            "spirit_gem_range": ["0x1A", "0x1D"],
            "hp_food_range": ["0x1E", "0x21"],
            "sp_food_range": ["0x22", "0x25"],
            "elixir_range": ["0x26", "0x2A"],
            "heal_sfx_cmd": "0x00A00234",
            "summon_routine_rom": "0x00007E9E",
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
