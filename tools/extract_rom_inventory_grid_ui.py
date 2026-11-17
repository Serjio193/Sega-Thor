#!/usr/bin/env python3
"""Extract and verify ROM inventory/ring 4x4 grid UI layout (0x0042A6..0x004360)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_inventory_grid_ui_m15_324.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x0042AE) == 0x0003      # col mask
    assert u16(rom, 0x0042B2) == 0x000C      # row mask (shifted << 2)
    assert u16(rom, 0x0042C6) == 0x0070      # grid origin X (112)
    assert u16(rom, 0x0042CA) == 0x0040      # grid origin Y (64)
    assert u32(rom, 0x0042FE) == 0x00155D76  # Inventory icon ROM base
    assert u32(rom, 0x004304) == 0x00FF0D80  # 8-slot inventory RAM base
    assert u16(rom, 0x004322) == 0x0009      # 9 tiles per 3x3 icon
    assert u32(rom, 0x004330) == 0x00FF2FA8  # Spirit/key decompressed buffer
    assert u32(rom, 0x004336) == 0x00FF0E50  # Spirit slot byte RAM
    assert u32(rom, 0x004350) == 0x00FF0DB0  # Special item slot byte RAM

    return {
        "milestone": "M15.324",
        "title": "ROM Inventory/Ring 4x4 Grid UI Layout",
        "routines": {
            "grid_slot_to_xy": {"start": "0x0042A6", "end": "0x0042D0"},
            "render_inventory_grid": {"start": "0x0042E8", "end": "0x004360"},
        },
        "constants": {
            "grid_origin_x": 0x70,
            "grid_origin_y": 0x40,
            "cell_pitch_px": 24,
            "grid_cols": 4,
            "inventory_slots": 8,
            "icon_tile_stride": 9,
            "icon_y_bias": 0x18,
            "icon_rom_base": "0x00155D76",
            "inventory_ram": "0x00FF0D80",
            "spirit_slot_ram": "0x00FF0E50",
            "special_slot_ram": "0x00FF0DB0",
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
