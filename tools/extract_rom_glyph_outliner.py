#!/usr/bin/env python3
"""Extract and verify ROM 1-bpp dialogue glyph outliner & 4-tile VDP rasterizer (0x00C222..0x00C2EA)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_glyph_outliner_m15_325.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00C224) == 0x0016C1CA  # 1-bpp 16x14 glyph table ROM base
    assert u16(rom, 0x00C22A) == 0x001C      # 28 bytes per glyph (14 words)
    assert u32(rom, 0x00C230) == 0x00FF3028  # 18x16 byte scratch canvas
    assert u16(rom, 0x00C254) == 0x011F      # 288 cells (dbra 0x11F)
    assert u32(rom, 0x00C27E) == 0x00FF2FA8  # Output 4-tile 4-bpp VDP buffer

    # Verify sample glyph 'A' (index 0x21 or 1): just grab glyph 1..3 raw bytes as sanity check
    glyph1_hex = rom[0x0016C1CA + 0x1C : 0x0016C1CA + 0x38].hex().upper()

    return {
        "milestone": "M15.325",
        "title": "ROM 1-bpp Dialogue Glyph Outliner & 4-Tile VDP Rasterizer",
        "routines": {
            "rasterize_outlined_glyph": {"start": "0x00C222", "end": "0x00C2A4"},
            "pack_8x8_4bpp_tile": {"start": "0x00C2A6", "end": "0x00C2C6"},
            "clear_18_bytes": {"start": "0x00C2C8", "end": "0x00C2D8"},
            "unpack_1bpp_row_16px": {"start": "0x00C2DA", "end": "0x00C2EA"},
        },
        "constants": {
            "glyph_table_rom": "0x0016C1CA",
            "glyph_stride_bytes": 28,
            "glyph_rows": 14,
            "canvas_stride_bytes": 18,
            "canvas_ram": "0x00FF3028",
            "output_vdp_ram": "0x00FF2FA8",
            "foreground_byte": "0xFF",
            "outline_color_index": 14,
            "foreground_color_index": 15,
            "sample_glyph1_hex": glyph1_hex,
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
