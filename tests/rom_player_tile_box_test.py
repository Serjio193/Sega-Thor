#!/usr/bin/env python3
"""Unit tests for M15.323 ROM Player Tile-Box Containment & Overlap Checks."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_player_tile_box import ROM_PATH, extract_evidence


def player_inside_tile_box(px: int, py: int, r: int, tx1: int, ty1: int, tx2: int, ty2: int) -> bool:
    """Reference simulation of 0x00C17A..0x00C1CC."""
    x_min, x_max = tx1 * 8, tx2 * 8 + 7
    y_min, y_max = ty1 * 8, ty2 * 8 + 7
    return (px - r >= x_min) and (px + r <= x_max) and (py - r >= y_min) and (py + r <= y_max)


def player_overlaps_tile_box(px: int, py: int, r: int, tx1: int, ty1: int, tx2: int, ty2: int) -> bool:
    """Reference simulation of 0x00C1CE..0x00C220."""
    x_min, x_max = tx1 * 8, tx2 * 8 + 7
    y_min, y_max = ty1 * 8, ty2 * 8 + 7
    return (px - r <= x_max) and (px + r >= x_min) and (py - r <= y_max) and (py + r >= y_min)


class TestRomPlayerTileBox(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["tile_max_bias_packed"], "0x00070007")
        self.assertEqual(ev["constants"]["player_entity_ram"], "0x00FF19E8")
        saved = json.loads(
            Path("data/thor-evidence/rom_player_tile_box_m15_323.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_containment_and_overlap(self) -> None:
        # Tile box [2..4] x [2..4] -> pixels [16..39] x [16..39]
        self.assertTrue(player_inside_tile_box(28, 28, 8, 2, 2, 4, 4))
        self.assertFalse(player_inside_tile_box(18, 28, 8, 2, 2, 4, 4))
        self.assertTrue(player_overlaps_tile_box(18, 28, 8, 2, 2, 4, 4))
        self.assertFalse(player_overlaps_tile_box(7, 28, 8, 2, 2, 4, 4))


if __name__ == "__main__":
    unittest.main()
