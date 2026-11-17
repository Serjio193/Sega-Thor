#!/usr/bin/env python3
"""Unit tests for M15.400 ROM Vertical (±Y) Tilemap Raycaster, Collision Sweep & Entity Return Dispatcher."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_vertical_raycaster import ROM_PATH, extract_evidence


def terrain_lut_lookup(lut: list[int], raw_val: int) -> int:
    """Simulate 0x009280..0x009288 / 0x009322..0x00932A LUT lookup: lut[raw_val & 0xF]."""
    return lut[raw_val & 0x0F]


def compute_tile_row_distance_down(y_with_radius: int) -> int:
    """Simulate 0x009234..0x00923C: dist to next bottom 8-px tile boundary."""
    rem = y_with_radius & 0x7
    return 7 - rem


def compute_tile_row_distance_up(y_minus_radius: int) -> int:
    """Simulate 0x0092DA..0x0092DC: dist to top 8-px tile boundary."""
    return y_minus_radius & 0x7


def simulate_vertical_raycast(
    start_y: int,
    radius_y: int,
    step_dy: int,
    solid_tiles: set[int],
    lut: list[int],
) -> tuple[int, int, bool, int]:
    """Reference simulation of 0x0091F6..0x00937E vertical raycast loop.

    Returns (final_y, steps_taken, hit_wall, surface_type).
    """
    if step_dy == 0:
        return start_y, 0, False, terrain_lut_lookup(lut, 1)

    steps_taken = 0
    cur_y = start_y

    if step_dy > 0:
        rem_dist = step_dy
        while True:
            probe_y = cur_y + radius_y
            d4 = compute_tile_row_distance_down(probe_y)
            if rem_dist <= d4:
                if rem_dist > 0:
                    cur_y += rem_dist
                break
            # Step to next tile row
            rem_dist -= d4
            cur_y += d4
            tile_row = (cur_y + radius_y) >> 3
            if (tile_row + 1) in solid_tiles:
                # Solid hit (bcs.w $92AA)
                return cur_y, steps_taken, True, terrain_lut_lookup(lut, 1)
            steps_taken += 1
            rem_dist -= 1
            cur_y += 1
    else:
        rem_dist = -step_dy
        while True:
            probe_y = cur_y - radius_y
            d4 = compute_tile_row_distance_up(probe_y)
            if rem_dist <= d4:
                if rem_dist > 0:
                    cur_y -= rem_dist
                break
            rem_dist -= d4
            cur_y -= d4
            tile_row = (cur_y - radius_y) >> 3
            if (tile_row - 1) in solid_tiles:
                # Solid hit (bcs.w $934C)
                return cur_y, steps_taken, True, terrain_lut_lookup(lut, 1)
            steps_taken += 1
            rem_dist -= 1
            cur_y -= 1

    surface_type = terrain_lut_lookup(lut, 1)
    return cur_y, steps_taken, False, surface_type


class TestRomVerticalRaycaster(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.400")
        self.assertEqual(ev["routines"]["pos_y_raycaster"]["direction_code"], "0x12")
        self.assertEqual(ev["routines"]["neg_y_raycaster"]["direction_code"], "0x10")
        self.assertEqual(ev["routines"]["entity_return_dispatcher"]["return_vector_ptr"], "0x00FF193C")
        saved = json.loads(
            Path("data/thor-evidence/rom_vertical_raycaster_m15_400.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_terrain_surface_lut(self) -> None:
        lut = [0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]
        self.assertEqual(terrain_lut_lookup(lut, 0x01), 0x00)
        self.assertEqual(terrain_lut_lookup(lut, 0x04), 0x04)
        self.assertEqual(terrain_lut_lookup(lut, 0x08), 0x06)
        self.assertEqual(terrain_lut_lookup(lut, 0x0C), 0x05)

    def test_tile_row_distances(self) -> None:
        # Downward distance: 7 - (y & 7)
        self.assertEqual(compute_tile_row_distance_down(0), 7)
        self.assertEqual(compute_tile_row_distance_down(6), 1)
        self.assertEqual(compute_tile_row_distance_down(7), 0)

        # Upward distance: y & 7
        self.assertEqual(compute_tile_row_distance_up(0), 0)
        self.assertEqual(compute_tile_row_distance_up(6), 6)
        self.assertEqual(compute_tile_row_distance_up(7), 7)

    def test_vertical_raycast_down(self) -> None:
        lut = [0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]
        # Move +10 with no walls: start at 100
        final_y, steps, hit, _ = simulate_vertical_raycast(100, 8, 10, set(), lut)
        self.assertEqual(final_y, 110)
        self.assertFalse(hit)

        # Solid wall at tile row 14
        final_y, steps, hit, _ = simulate_vertical_raycast(100, 8, 10, {14}, lut)
        self.assertTrue(hit)
        self.assertEqual(final_y, 103)

    def test_vertical_raycast_up(self) -> None:
        lut = [0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]
        # Move -10 with no walls: start at 100
        final_y, steps, hit, _ = simulate_vertical_raycast(100, 8, -10, set(), lut)
        self.assertEqual(final_y, 90)
        self.assertFalse(hit)

        # Solid wall at tile row 10
        final_y, steps, hit, _ = simulate_vertical_raycast(100, 8, -10, {10}, lut)
        self.assertTrue(hit)
        self.assertEqual(final_y, 96)


if __name__ == "__main__":
    unittest.main()
