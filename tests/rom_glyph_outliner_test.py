#!/usr/bin/env python3
"""Unit tests for M15.325 ROM 1-bpp Dialogue Glyph Outliner & 4-Tile VDP Rasterizer."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_glyph_outliner import ROM_PATH, extract_evidence


def rasterize_outlined_glyph(rows_14w: list[int]) -> bytes:
    """Reference simulation of 0x00C222..0x00C2EA returning 128 bytes (4x32B 4-bpp tiles)."""
    canvas = [0] * (18 * 17)
    pos = 18  # skip top 18-byte margin
    for w in rows_14w:
        canvas[pos] = 0
        pos += 1
        for bit in range(15, -1, -1):
            canvas[pos] = 0xFF if ((w >> bit) & 1) else 0x00
            pos += 1
        canvas[pos] = 0
        pos += 1

    # Outline pass starting at offset 0x12 (18) for 0x120 (288) cells
    for i in range(18, 18 + 0x120):
        if canvas[i] == 0:
            if (
                (i - 18 >= 0 and (canvas[i - 18] & 0x80))
                or (i + 18 < len(canvas) and (canvas[i + 18] & 0x80))
                or (i - 1 >= 0 and (canvas[i - 1] & 0x80))
                or (i + 1 < len(canvas) and (canvas[i + 1] & 0x80))
            ):
                canvas[i] = 0x0E

    def pack_tile(start: int) -> bytes:
        out = bytearray()
        cur = start
        for _ in range(8):
            for _ in range(4):
                hi = (canvas[cur] & 0x0F) << 4
                lo = canvas[cur + 1] & 0x0F
                out.append(hi | lo)
                cur += 2
            cur += 10  # 18 - 8 = 10
        return bytes(out)

    # Top-left (0x13 = 19), bottom-left (0xA3 = 163), top-right (0x1B = 27), bottom-right (0xAB = 171)
    return pack_tile(0x13) + pack_tile(0xA3) + pack_tile(0x1B) + pack_tile(0xAB)


class TestRomGlyphOutliner(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["glyph_stride_bytes"], 28)
        self.assertEqual(ev["constants"]["outline_color_index"], 14)
        saved = json.loads(
            Path("data/thor-evidence/rom_glyph_outliner_m15_325.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_single_pixel_outline_and_4bpp_packing(self) -> None:
        # Single foreground pixel at row 1, col 1 (bit 14 of row 1 -> top-left tile row 2, col 1)
        rows = [0] * 14
        rows[1] = 0x4000
        tiles = rasterize_outlined_glyph(rows)
        self.assertEqual(len(tiles), 128)
        # Top-left tile row 0 (canvas row 1, col 0..1) has outline above fg at col 1 -> 0x0E
        self.assertEqual(tiles[0], 0x0E)
        # Top-left tile row 1 (canvas row 2, col 0..1) has outline 0xE and fg 0xF -> 0xEF
        self.assertEqual(tiles[1 * 4 + 0], 0xEF)


if __name__ == "__main__":
    unittest.main()
