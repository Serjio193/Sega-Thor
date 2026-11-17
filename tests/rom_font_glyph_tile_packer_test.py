#!/usr/bin/env python3
"""
rom_font_glyph_tile_packer_test.py — Unit tests for M15.427:
ROM Dynamic 1-Bit Font Glyph Expansion, 4-Neighbor Outline Filter & 4bpp Tile Packer (0x00C222..0x00C2EC)
"""

import json
import os
import struct
import unittest
from tools.extract_rom_font_glyph_tile_packer import (
    expand_glyph_to_canvas,
    apply_4neighbor_outline,
    pack_quadrant_8x8,
    render_font_glyph,
    START_ADDR,
    END_ADDR,
    SIZE,
    FONT_TABLE_ADDR,
    CANVAS_RAM_ADDR,
    TILES_OUT_RAM,
    ROM_PATH,
)

class TestRomFontGlyphTilePacker(unittest.TestCase):
    def setUp(self):
        self.evidence_path = "data/thor-evidence/rom_font_glyph_tile_packer_m15_427.json"

    def test_evidence_json_validity(self):
        self.assertTrue(os.path.exists(self.evidence_path))
        with open(self.evidence_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.assertEqual(data["start_address"], f"0x{START_ADDR:06X}")
        self.assertEqual(data["end_address"], f"0x{END_ADDR:06X}")
        self.assertEqual(data["size_bytes"], SIZE)
        self.assertEqual(data["font_table_rom"], f"0x{FONT_TABLE_ADDR:08X}")
        self.assertEqual(data["canvas_ram"], f"0x{CANVAS_RAM_ADDR:08X}")
        self.assertEqual(data["tiles_out_ram"], f"0x{TILES_OUT_RAM:08X}")

    def test_glyph_expansion_to_canvas(self):
        # 14 words: row 0 has a single bit set in the middle (bit 8: 0x0080)
        words = [0x0080] + [0] * 13
        canvas = expand_glyph_to_canvas(words)
        self.assertEqual(len(canvas), 324)

        # Row 0: all 18 bytes are zero
        self.assertEqual(canvas[0:18], [0] * 18)

        # Row 1 (words[0]):
        # col 0: 0
        self.assertEqual(canvas[18], 0)
        # col 8 (bit 8): 0xFF
        self.assertEqual(canvas[18 + 1 + 8], 0xFF)
        # col 17: 0
        self.assertEqual(canvas[18 + 17], 0)

        # Row 15: all 18 bytes are zero
        self.assertEqual(canvas[18 * 15:18 * 16], [0] * 18)

    def test_4neighbor_outline_filter(self):
        # Empty canvas with single 0xFF at row 5, col 5
        canvas = [0] * 324
        idx = 5 * 18 + 5
        canvas[idx] = 0xFF

        outlined = apply_4neighbor_outline(canvas)
        # Pixel itself stays 0xFF
        self.assertEqual(outlined[idx], 0xFF)

        # 4 cardinal neighbors get outline color 0x0E
        self.assertEqual(outlined[idx - 18], 0x0E)  # Top
        self.assertEqual(outlined[idx + 18], 0x0E)  # Bottom
        self.assertEqual(outlined[idx - 1], 0x0E)   # Left
        self.assertEqual(outlined[idx + 1], 0x0E)   # Right

        # Diagonals remain 0
        self.assertEqual(outlined[idx - 18 - 1], 0)
        self.assertEqual(outlined[idx - 18 + 1], 0)
        self.assertEqual(outlined[idx + 18 - 1], 0)
        self.assertEqual(outlined[idx + 18 + 1], 0)

    def test_4bpp_quadrant_packing(self):
        canvas = [0] * 288
        # Fill row 1, cols 1..8 with 0x0E
        for c in range(1, 9):
            canvas[1 * 18 + c] = 0x0E
        tile0 = pack_quadrant_8x8(canvas, start_row=1, start_col=1)
        self.assertEqual(len(tile0), 32)
        # First row: 8 pixels of 0x0E -> 4 bytes of 0xEE
        self.assertEqual(tile0[0:4], [0xEE, 0xEE, 0xEE, 0xEE])
        # Remaining 7 rows are zero
        self.assertEqual(tile0[4:32], [0x00] * 28)

    def test_actual_rom_glyph_rendering(self):
        with open(ROM_PATH, "rb") as f:
            rom = f.read()

        # Read first glyph from 0x16C1CA (14 words)
        words = list(struct.unpack_from(">14H", rom, FONT_TABLE_ADDR))
        rendered_tiles = render_font_glyph(words)
        self.assertEqual(len(rendered_tiles), 128)

        # Verify non-trivial output (contains outline and foreground nibbles)
        nibbles = []
        for b in rendered_tiles:
            nibbles.append((b >> 4) & 0xF)
            nibbles.append(b & 0xF)
        self.assertTrue(any(n == 0x0E for n in nibbles))
        self.assertTrue(any(n == 0x0F for n in nibbles))

if __name__ == "__main__":
    unittest.main()
