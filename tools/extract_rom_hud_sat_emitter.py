#!/usr/bin/env python3
"""Extract and verify ROM HUD frame SAT & item-toast DMA emitter (0x00A342..0x00A436)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_hud_sat_emitter_m15_334.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00A344) == 0x00FF13CC  # SAT buffer base
    assert u32(rom, 0x00A35A) == 0x00FF1858  # Alt HUD layout flag
    assert u32(rom, 0x00A366) == 0x00FF185A  # HUD slide offset X
    assert u32(rom, 0x00A382) == 0x00FF184F  # Enemy HP gauge active flag
    assert u16(rom, 0x00A38C) == 0x0158      # Enemy gauge mirror base (0x158 - slide_x)
    assert u32(rom, 0x00A3AC) == 0x00FF1856  # Item-use toast timer
    assert u16(rom, 0x00A3B6) == 0x0050      # Max 80 SAT sprites
    assert u16(rom, 0x00A3C6) == 0x0118      # Toast sprite Y
    assert u16(rom, 0x00A3CC) == 0x85C7      # Toast sprite tile attr
    assert u16(rom, 0x00A3D6) == 0x0068      # Toast X bias (slide_x + 0x68)
    assert u32(rom, 0x00A402) == 0x000AAEBB  # Item icon ROM base (stride 0x90)
    assert u16(rom, 0x00A414) == 0xB8E0      # Toast icon VRAM dst
    assert u16(rom, 0x00A418) == 0x0090      # Toast icon DMA len words (9 tiles)

    return {
        "milestone": "M15.334",
        "title": "ROM HUD Frame SAT & Item-Toast DMA Emitter",
        "routine": {"start": "0x00A342", "end": "0x00A436"},
        "constants": {
            "primary_hud_sprite_count": 6,
            "enemy_gauge_sprite_count": 3,
            "enemy_mirror_base_x": "0x0158",
            "toast_sprite_y": "0x0118",
            "toast_sprite_size_hi": "0x0A",
            "toast_sprite_tile": "0x85C7",
            "toast_sprite_x_bias": "0x0068",
            "toast_icon_rom_base": "0x000AAEBB",
            "toast_icon_stride": "0x0090",
            "toast_icon_vram_dst": "0xB8E0",
            "toast_icon_dma_words": "0x0090",
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
