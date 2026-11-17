#!/usr/bin/env python3
"""Extract and verify ROM inventory/menu sprite & font DMA strip emitter (0x004000..0x00417A)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_menu_sprite_dma_m15_322.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x004006) == 0x86DD      # Default icon tile attr
    assert u16(rom, 0x004056) == 0x85D0      # Digit '0' tile attr
    assert u16(rom, 0x004068) == 0x0050      # Max 80 SAT sprites
    lut = list(rom[0x0040CE:0x0040DE])
    assert lut == [1, 2, 3, 4, 2, 4, 6, 8, 3, 6, 9, 12, 4, 8, 12, 16]

    assert u32(rom, 0x0040E4) == 0x0015375A  # Alt font ROM base
    assert u32(rom, 0x0040F0) == 0x00152F5A  # Primary font ROM base
    assert u32(rom, 0x00415C) == 0x00FF164E  # V-Int sync flag
    assert u32(rom, 0x004164) == 0x00FF13CC  # SAT buffer base

    return {
        "milestone": "M15.322",
        "title": "ROM Inventory/Menu Sprite & Font DMA Strip Emitter",
        "routines": {
            "emit_item_and_digit_pair": {"start": "0x004026", "end": "0x00405C"},
            "append_sat_sprite": {"start": "0x004066", "end": "0x0040A4"},
            "queue_sprite_tile_dma": {"start": "0x0040A6", "end": "0x0040CC"},
            "emit_font_strip_sprites": {"start": "0x0040E2", "end": "0x004156"},
            "acquire_sat_and_dma_cursors": {"start": "0x004158", "end": "0x00417A"},
        },
        "sprite_tile_count_lut": {
            "address": "0x0040CE",
            "values": lut,
        },
        "constants": {
            "icon_default_tile_attr": "0x86DD",
            "digit_base_tile_attr": "0x85D0",
            "max_digit_value": 9,
            "sat_max_sprites": 80,
            "font_primary_rom": "0x00152F5A",
            "font_alt_rom": "0x0015375A",
            "ascii_remap_min": "0x2B",
            "ascii_remap_max": "0x2F",
            "ascii_remap_bias": "0x30",
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
