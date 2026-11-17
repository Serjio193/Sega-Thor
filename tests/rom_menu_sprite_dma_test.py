#!/usr/bin/env python3
"""Unit tests for M15.322 ROM Inventory/Menu Sprite & Font DMA Strip Emitter."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_menu_sprite_dma import ROM_PATH, extract_evidence

TILE_COUNT_LUT = [1, 2, 3, 4, 2, 4, 6, 8, 3, 6, 9, 12, 4, 8, 12, 16]


def remap_menu_char(ch: int) -> int:
    """Reference simulation of 0x004136..0x004148."""
    ch &= 0xFF
    if 0x2B <= ch <= 0x2F:
        return (ch + 0x30) & 0xFF
    return ch


def emit_font_strip_plan(text: bytes, start_vram_tile: int, x: int) -> tuple[list[tuple[int, int]], list[tuple[int, int, int]]]:
    """Reference simulation of 0x0040EA..0x004152.

    Returns (sat_chunks: [(width_tiles, x_pos)], dma_chars: [(char_code, vram_byte_dst, len_words)]).
    """
    sat_chunks: list[tuple[int, int]] = []
    dma_chars: list[tuple[int, int, int]] = []
    rem = len(text)
    cur_tile = start_vram_tile
    cur_x = x
    while rem > 0:
        w = 4 if rem >= 4 else rem
        rem -= w
        sat_chunks.append((w, cur_x))
        cur_tile += w
        cur_x += w * 8

    vram_tile = start_vram_tile
    size_code = (0)  # 1x1 after 0x004128 (&= 0xF3) -> lut[0] = 1
    tile_cnt = TILE_COUNT_LUT[size_code]
    for b in text:
        code = remap_menu_char(b)
        dma_chars.append((code, vram_tile << 5, tile_cnt << 4))
        vram_tile += tile_cnt
    return (sat_chunks, dma_chars)


class TestRomMenuSpriteDma(unittest.TestCase):
    def test_rom_evidence_matches_lut_and_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["sprite_tile_count_lut"]["values"], TILE_COUNT_LUT)
        self.assertEqual(ev["constants"]["digit_base_tile_attr"], "0x85D0")
        saved = json.loads(
            Path("data/thor-evidence/rom_menu_sprite_dma_m15_322.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_font_strip_chunking_and_ascii_remap(self) -> None:
        chunks, dmas = emit_font_strip_plan(b"HP-10", 0x0100, 0x80)
        self.assertEqual(chunks, [(4, 0x80), (1, 0xA0)])
        self.assertEqual(len(dmas), 5)
        # '-' (0x2D) remaps to 0x2D + 0x30 = 0x5D
        self.assertEqual(dmas[2], (0x5D, 0x0102 << 5, 16))


if __name__ == "__main__":
    unittest.main()
