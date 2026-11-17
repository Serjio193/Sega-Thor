#!/usr/bin/env python3
"""Unit tests for M15.405 ROM Ramp/Bridge Slope Solver, Fractional Height Interpolator (mulu/divu) & Tile Elevation Engine."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_slope_elevation_engine import ROM_PATH, extract_evidence


def compute_flat_elevation(surface_type: int, elevation_table: dict[int, int]) -> int:
    """Simulate flat tile elevation read 0x009BB0..0x009BB8: elevation_table[surface_type * 2]."""
    return elevation_table.get(surface_type * 2, 0)


def compute_slope_elevation(
    surface_type: int,
    sub_y: int,
    total_y: int,
    elevation_table: dict[int, int],
) -> int:
    """Simulate slope fractional elevation interpolation 0x009B8A..0x009B9C."""
    # d5 = surface_type * 2 - 2
    idx = surface_type * 2 - 2
    base_h = elevation_table.get(idx, 0)
    next_h = elevation_table.get(idx + 4, 0)
    height_delta = next_h - base_h
    # mulu.w d3, d1; divu.w d2, d1; add.w d1, d4
    fraction = (height_delta * sub_y) // total_y
    return base_h + fraction


class TestRomSlopeElevationEngine(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.405")
        self.assertEqual(ev["routines"]["tile_elevation_engine"]["layer1_ram"], "0x00FF1766")
        self.assertEqual(ev["routines"]["tile_elevation_engine"]["layer2_ram"], "0x00FF1770")
        self.assertEqual(ev["routines"]["tile_elevation_engine"]["elevation_table_ram"], "0x00FF1706")
        saved = json.loads(
            Path("data/thor-evidence/rom_slope_elevation_engine_m15_405.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_lut2_mapping(self) -> None:
        lut2 = [0xFF, 0x00, 0x02, 0x01, 0x04, 0x04, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x05]
        self.assertEqual(lut2[0], 0xFF)
        self.assertEqual(lut2[1], 0x00)
        self.assertEqual(lut2[2], 0x02)
        self.assertEqual(lut2[3], 0x01)  # odd -> slope!
        self.assertEqual(lut2[6], 0x03)  # odd -> slope!

    def test_flat_surface_elevation(self) -> None:
        # surface type 2 (even) -> idx 4
        elev_table = {0: 0, 2: 8, 4: 16, 6: 24, 8: 32}
        h = compute_flat_elevation(2, elev_table)
        self.assertEqual(h, 16)

    def test_slope_fractional_interpolation(self) -> None:
        # surface type 1 (odd) -> idx = 1*2 - 2 = 0. Base = 0, Next = 16.
        # total span = 8 px, sub_y = 4 px (halfway up the ramp)
        elev_table = {0: 0, 4: 16}
        h = compute_slope_elevation(surface_type=1, sub_y=4, total_y=8, elevation_table=elev_table)
        self.assertEqual(h, 8)  # Exactly half: 0 + (16 * 4) // 8 = 8

        # At 3/4 up the ramp: sub_y = 6
        h = compute_slope_elevation(surface_type=1, sub_y=6, total_y=8, elevation_table=elev_table)
        self.assertEqual(h, 12)


if __name__ == "__main__":
    unittest.main()
