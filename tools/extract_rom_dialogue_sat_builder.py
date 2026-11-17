#!/usr/bin/env python3
"""Extract and verify ROM dialogue cursor & border SAT builder (0x00C756..0x00C84C)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_dialogue_sat_builder_m15_317.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00C75C) == 0x00FF13CC  # SAT buffer base
    assert u32(rom, 0x00C762) == 0x00FF188C  # SAT byte offset
    assert u32(rom, 0x00C768) == 0x00FF188A  # SAT sprite count
    assert u32(rom, 0x00C76E) == 0x00FF1886  # Cursor enabled flag
    assert u32(rom, 0x00C778) == 0x00FF1657  # Frame blink counter
    assert u16(rom, 0x00C780) == 0x86DC      # Cursor base tile attr
    assert u16(rom, 0x00C792) == 0x011C      # Cursor X
    assert u16(rom, 0x00C796) == 0x014C      # Cursor base Y
    assert u32(rom, 0x00C79A) == 0x00FF1880  # Choice row index (0..3)
    assert u16(rom, 0x00C7C2) == 0x0C00      # Border palette/priority base
    assert u16(rom, 0x00C81E) == 0x00A0      # Border start X
    assert u16(rom, 0x00C822) == 0x0110      # Border Y base offset
    assert u16(rom, 0x00C826) == 0x0007      # 8 sprites per bar (dbra 7)
    assert u16(rom, 0x00C82E) == 0x0020      # Border X step (32 px)
    assert u16(rom, 0x00C83A) == 0x0050      # Max 80 SAT sprites

    return {
        "milestone": "M15.317",
        "title": "ROM Dialogue Cursor & Border SAT Builder",
        "routines": {
            "build_dialogue_sat": {"start": "0x00C756", "end": "0x00C81A"},
            "emit_8_sprite_bar": {"start": "0x00C81C", "end": "0x00C836"},
            "append_sat_entry": {"start": "0x00C838", "end": "0x00C84C"},
        },
        "constants": {
            "sat_buffer_ram": "0x00FF13CC",
            "sat_byte_offset_ram": "0x00FF188C",
            "sat_count_ram": "0x00FF188A",
            "sat_max_sprites": 80,
            "cursor_tile_base": "0x86DC",
            "cursor_alt_palette_bit": 12,
            "cursor_x": "0x011C",
            "cursor_y_base": "0x014C",
            "cursor_y_step": 12,
            "blink_bit": 4,
            "border_palette_base": "0x0C00",
            "border_start_x": "0x00A0",
            "border_x_step": "0x0020",
            "border_y_bias": "0x0110",
            "border_sprites_per_bar": 8,
            "border_tile_step": 4,
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
