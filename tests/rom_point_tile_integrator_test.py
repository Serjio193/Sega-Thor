#!/usr/bin/env python3
"""Unit tests for M15.404 ROM Point Tile Solidity, Air/Elevation Integrator & Tile Attribute Sampler."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_point_tile_integrator import ROM_PATH, extract_evidence


def simulate_point_tile_integrator(
    cur_ground_h: int,
    target_tile_h: int,
    air_h: int,
    start_y: int,
    tile_attr: int,
) -> tuple[bool, int, int, int]:
    """Reference simulation of 0x0098EC..0x0099B6.

    Returns (is_solid, final_y, final_air_h, final_ground_h).
    """
    if target_tile_h < 0:
        return True, start_y, air_h, cur_ground_h

    if cur_ground_h < 0:
        return False, start_y, air_h, target_tile_h

    delta = cur_ground_h - target_tile_h
    if delta == 0:
        return False, start_y, air_h, target_tile_h

    # Check if entity has enough clearance/air height to step up
    if air_h + delta < 0:
        return True, start_y, air_h, cur_ground_h

    cur_y = start_y
    cur_air = air_h
    rem = delta

    if delta > 0:
        # Descending slope: increment y and air
        while rem > 0:
            cur_y += 1
            cur_air += 1
            rem -= 1
    else:
        # Ascending slope: decrement y and air
        while rem < 0:
            cur_y -= 1
            cur_air -= 1
            rem += 1

    return False, cur_y, cur_air, target_tile_h


class TestRomPointTileIntegrator(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.404")
        self.assertEqual(ev["routines"]["point_tile_solidity_probe"]["ground_height_offset"], "0x14")
        self.assertEqual(ev["routines"]["point_tile_solidity_probe"]["air_height_offset"], "0x10")
        self.assertEqual(ev["routines"]["sample_point_tile_attr"]["attribute_mask"], "0x0F")
        saved = json.loads(
            Path("data/thor-evidence/rom_point_tile_integrator_m15_404.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_slope_descending(self) -> None:
        # Ground drops from 20 to 16 (delta = 4). Entity has air_h = 2.
        is_solid, final_y, final_air, final_ground = simulate_point_tile_integrator(
            cur_ground_h=20,
            target_tile_h=16,
            air_h=2,
            start_y=100,
            tile_attr=1,
        )
        self.assertFalse(is_solid)
        self.assertEqual(final_y, 104)
        self.assertEqual(final_air, 6)
        self.assertEqual(final_ground, 16)

    def test_slope_ascending(self) -> None:
        # Ground rises from 16 to 20 (delta = -4). Entity has air_h = 5 (clearance ok).
        is_solid, final_y, final_air, final_ground = simulate_point_tile_integrator(
            cur_ground_h=16,
            target_tile_h=20,
            air_h=5,
            start_y=100,
            tile_attr=1,
        )
        self.assertFalse(is_solid)
        self.assertEqual(final_y, 96)
        self.assertEqual(final_air, 1)
        self.assertEqual(final_ground, 20)

    def test_steep_wall_blocks(self) -> None:
        # Ground rises from 16 to 24 (delta = -8). Entity only has air_h = 2 -> blocked!
        is_solid, final_y, final_air, final_ground = simulate_point_tile_integrator(
            cur_ground_h=16,
            target_tile_h=24,
            air_h=2,
            start_y=100,
            tile_attr=1,
        )
        self.assertTrue(is_solid)
        self.assertEqual(final_y, 100)
        self.assertEqual(final_air, 2)


if __name__ == "__main__":
    unittest.main()
