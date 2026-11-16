#!/usr/bin/env python3
"""
extract_rom_item_attribute_tables.py — Milestone M15.272

Extracts the two 64-byte ROM Item Attribute & Pickup Fanfare Tables
(`0x05CE16` and `0x05CE56`, IDs `0x00..0x3F`) and Pickup Handler (`0x007A6C`):
1. `0x05CE16` (64 bytes — Item Category & Key-Item Bitmask, read at `0x0048C8`):
   - Bit 5 (`0x20`): non-discardable / key item guard.
   - `0x01`: weapon, `0x00`: consumable/ring (`0x1A..0x2A`), `0xA0..0xA4`: key items.
2. `0x05CE56` (64 bytes — Pickup SFX / Fanfare Tier, read at `0x007A6C`):
   - Tier `0`: SFX `0xC0035F`
   - Tier `1`: SFX `0xC00360`
   - Tier `2`: SFX `0xC00361`
   - Tier `3`: Fanfare `0x0508` + Player State `0x44` / Action `0x4A`
   - Tier `4`: Fanfare `0x050D` + Player State `0x44` / Action `0x4A`
3. Persistent Flag Setter (`0x007AF0..0x007B28`):
   - Bank `$FF197A` (`1=$FF0DBC`, `2=$FF0E1C`, `3=$FF0E2C`), bit `$FF197C`.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_item_attribute_tables_m15_272.json"

ATTR_TABLE_ADDR = 0x05CE16
PICKUP_TIER_TABLE_ADDR = 0x05CE56
ITEM_ID_COUNT = 64


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    # Verify table pointers in ROM instructions
    assert read_u16_be(rom, 0x0048C8) == 0x41F9 and read_u32_be(rom, 0x0048CA) == ATTR_TABLE_ADDR
    assert read_u16_be(rom, 0x007A6C) == 0x43F9 and read_u32_be(rom, 0x007A6E) == PICKUP_TIER_TABLE_ADDR

    attrs = list(rom[ATTR_TABLE_ADDR : ATTR_TABLE_ADDR + ITEM_ID_COUNT])
    tiers = list(rom[PICKUP_TIER_TABLE_ADDR : PICKUP_TIER_TABLE_ADDR + ITEM_ID_COUNT])

    items = []
    for item_id in range(ITEM_ID_COUNT):
        items.append({
            "item_id": f"0x{item_id:02X}",
            "attr_byte": f"0x{attrs[item_id]:02X}",
            "non_discardable": (attrs[item_id] & 0x20) != 0,
            "pickup_tier": tiers[item_id],
        })

    evidence = {
        "milestone": "M15.272",
        "description": "ROM 64-Byte Item Attribute Table (0x05CE16), Pickup Fanfare Table (0x05CE56), and Pickup Flag Setter (0x007A6C)",
        "attr_table_addr": f"0x{ATTR_TABLE_ADDR:06X}",
        "pickup_tier_table_addr": f"0x{PICKUP_TIER_TABLE_ADDR:06X}",
        "pickup_handler_addr": "0x007A6C",
        "tier_contracts": {
            "0": {"sfx": "0xC0035F", "fanfare": False},
            "1": {"sfx": "0xC00360", "fanfare": False},
            "2": {"sfx": "0xC00361", "fanfare": False},
            "3": {"cmd": "0x0508", "fanfare": True, "player_state": "0x44", "player_action": "0x4A"},
            "4": {"cmd": "0x050D", "fanfare": True, "player_state": "0x44", "player_action": "0x4A"},
        },
        "items": items,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(items)} items)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
