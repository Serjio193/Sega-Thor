#!/usr/bin/env python3
"""Unit tests for M15.321 ROM Room Height-Band Table Builder & Tileset Chunk Loader."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_room_height_tileset import ROM_PATH, extract_evidence


def build_height_band_tables(bands_s8: tuple[int, int, int, int]) -> tuple[list[int], list[int]]:
    """Reference simulation of 0x00D43A..0x00D46A.

    Returns (band_steps_4w, height_lut_8w_u16).
    """
    steps: list[int] = []
    heights: list[int] = []
    accum = 0
    for b in bands_s8:
        step = b << 3
        steps.append(step)
        heights.append(accum & 0xFFFF)
        heights.append((accum + (step >> 1)) & 0xFFFF)
        accum += step
    heights[-1] = 0xF000
    return (steps, heights)


def quantize_camera_pos(x: int, y: int) -> tuple[int, int]:
    """Reference simulation of 0x00D48A..0x00D4A4."""
    qx = max(0, x & ~7)
    qy = max(0, y & ~7)
    return (qx, qy)


class TestRomRoomHeightTileset(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["height_sentinel"], "0xF000")
        self.assertEqual(ev["constants"]["chunk_table_rom"], "0x0005CE96")
        saved = json.loads(
            Path("data/thor-evidence/rom_room_height_tileset_m15_321.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_height_band_builder_and_camera_quantize(self) -> None:
        steps, heights = build_height_band_tables((2, 4, 2, 2))
        self.assertEqual(steps, [16, 32, 16, 16])
        self.assertEqual(heights, [0, 8, 16, 32, 48, 56, 64, 0xF000])
        self.assertEqual(quantize_camera_pos(133, 219), (128, 216))
        self.assertEqual(quantize_camera_pos(-16, 15), (0, 8))


if __name__ == "__main__":
    unittest.main()
