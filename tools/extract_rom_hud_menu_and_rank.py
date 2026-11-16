#!/usr/bin/env python3
"""
extract_rom_hud_menu_and_rank.py — Milestone M15.268

Extracts ROM semantic knowledge for:
1. Pause / Ring Menu Icon Rendering (`0x0042E8..0x0043AC`):
   - Icon tileset ROM address `0x00155D76` (`3x3 = 9` tiles per icon).
   - 8 Weapon slots at `$FF0D80` (cursor `$FF0E50`), 24px (`0x18`) spacing.
   - 16 Item slots at `$FF0D90` (cursor `$FF0E51`).
2. Player Rank & MaxHP Scaling Formula (`0x007BF6..0x007C1C`):
   - Rank `$FF1A7E` capped at `0xC8` (200).
   - `MaxHP ($FF1A70) = 0xC6 + (Rank * 2)` (`198 + 2 * Rank`), `+2` HP on rank-up.
   - SP pickup `+5` (`0x007BCA` -> `$FF1A76`), Gem counters `$FF0DB2` (`0x007BD4`).
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_hud_menu_and_rank_m15_268.json"


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_contracts(rom: bytes) -> dict:
    """Verify M68k opcodes for ring menu icon tileset and rank/MaxHP formula."""
    # 1. 0x0042FC: lea $155d76.l, a3 (47F9 0015 5D76)
    assert read_u16_be(rom, 0x0042FC) == 0x47F9
    icon_tileset = read_u32_be(rom, 0x0042FE)
    assert icon_tileset == 0x00155D76

    # 2. 0x004320: mulu.w #$9, d3 (C6FC 0009) -> 9 tiles (3x3) per icon
    assert read_u16_be(rom, 0x004320) == 0xC6FC
    tiles_per_icon = read_u16_be(rom, 0x004322)
    assert tiles_per_icon == 9

    # 3. 0x007BFC: cmpi.w #$c8, d0 (0C40 00C8) -> Max Rank = 200
    assert read_u16_be(rom, 0x007BFC) == 0x0C40
    max_rank = read_u16_be(rom, 0x007BFE)
    assert max_rank == 200

    # 4. 0x007C0C: addi.w #$c6, d0 (0640 00C6) -> Base MaxHP = 198 + 2*Rank
    assert read_u16_be(rom, 0x007C0C) == 0x0640
    base_max_hp = read_u16_be(rom, 0x007C0E)
    assert base_max_hp == 0x00C6

    return {
        "icon_tileset_rom_addr": f"0x{icon_tileset:06X}",
        "tiles_per_icon": tiles_per_icon,
        "max_rank": max_rank,
        "base_max_hp": base_max_hp,
    }


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()
    contracts = verify_rom_contracts(rom)

    evidence = {
        "milestone": "M15.268",
        "description": "ROM Pause Ring Menu Icon Renderer (0x0042E8) and Player Rank / MaxHP Scaling Formula (0x007BF6)",
        "ring_menu": {
            "weapon_ring_renderer": "0x0042E8",
            "item_ring_renderer": "0x004388",
            "icon_tileset_rom_addr": contracts["icon_tileset_rom_addr"],
            "tiles_per_icon": contracts["tiles_per_icon"],
            "weapon_slots_ram": "0x00FF0D80",
            "weapon_slot_count": 8,
            "weapon_cursor_ram": "0x00FF0E50",
            "item_slots_ram": "0x00FF0D90",
            "item_slot_count": 16,
            "item_cursor_ram": "0x00FF0E51",
            "slot_spacing_px": 24,
        },
        "rank_and_stats": {
            "rank_up_routine": "0x007BF6",
            "rank_ram": "0x00FF1A7E",
            "max_hp_ram": "0x00FF1A70",
            "current_hp_ram": "0x00FF1A72",
            "max_sp_ram": "0x00FF1A74",
            "current_sp_ram": "0x00FF1A76",
            "max_rank": contracts["max_rank"],
            "base_max_hp": contracts["base_max_hp"],
            "max_hp_formula": "198 + 2 * rank",
            "sp_pickup_step": 5,
            "gem_counters_ram": "0x00FF0DB2",
        },
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(json.dumps(evidence))} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
