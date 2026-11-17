#!/usr/bin/env python3
"""Unit tests for M15.324 ROM Inventory/Ring 4x4 Grid UI Layout (0x0042A6..0x004360)."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_inventory_grid_ui import ROM_PATH, extract_evidence


def grid_slot_to_xy(slot: int) -> tuple[int, int]:
    """Reference simulation of 0x0042A6..0x0042D0."""
    col = slot & 0x03
    row = (slot & 0x0C) >> 2
    return (0x70 + col * 24, 0x40 + row * 24)


class TestRomInventoryGridUi(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["grid_origin_x"], 0x70)
        self.assertEqual(ev["constants"]["grid_origin_y"], 0x40)
        self.assertEqual(ev["constants"]["cell_pitch_px"], 24)
        saved = json.loads(
            Path("data/thor-evidence/rom_inventory_grid_ui_m15_324.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_grid_slot_coordinates(self) -> None:
        self.assertEqual(grid_slot_to_xy(0), (112, 64))
        self.assertEqual(grid_slot_to_xy(3), (184, 64))
        self.assertEqual(grid_slot_to_xy(4), (112, 88))
        self.assertEqual(grid_slot_to_xy(7), (184, 88))
        self.assertEqual(grid_slot_to_xy(15), (184, 136))


if __name__ == "__main__":
    unittest.main()
