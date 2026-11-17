#!/usr/bin/env python3
"""Unit tests for M15.399 ROM Horizontal (±X) Tilemap Raycaster, Surface Height Probe & 0x0096E8 Terrain LUT."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_horizontal_raycaster import ROM_PATH, extract_evidence


def terrain_lut_lookup(lut: list[int], raw_val: int) -> int:
    """Simulate 0x0090FC..0x009100 / 0x00919E..0x0091A2 LUT lookup: lut[raw_val & 0xF]."""
    return lut[raw_val & 0x0F]


def compute_tile_distance_right(x_with_radius: int) -> int:
    """Simulate 0x0090AC..0x0090B4: dist to next right 8-px tile boundary."""
    rem = x_with_radius & 0x7
    # subq.w #7, rem; neg.w rem -> -(rem - 7) = 7 - rem
    return 7 - rem


def compute_tile_distance_left(x_minus_radius: int) -> int:
    """Simulate 0x009152..0x009154: dist to left 8-px tile boundary."""
    return x_minus_radius & 0x7


def simulate_horizontal_raycast(
    start_x: int,
    radius_x: int,
    step_dx: int,
    solid_tiles: set[int],
    lut: list[int],
) -> tuple[int, int, bool, int]:
    """Reference simulation of 0x009084..0x0091F6 raycast loop.

    Returns (final_x, steps_taken, hit_wall, surface_type).
    """
    if step_dx == 0:
        return start_x, 0, False, terrain_lut_lookup(lut, 1)

    steps_taken = 0
    cur_x = start_x

    if step_dx > 0:
        rem_dist = step_dx
        while True:
            probe_x = cur_x + radius_x
            d4 = compute_tile_distance_right(probe_x)
            if rem_dist <= d4:
                if rem_dist > 0:
                    cur_x += rem_dist
                break
            # Step to next tile boundary
            rem_dist -= d4
            cur_x += d4
            tile_col = (cur_x + radius_x) >> 3
            if (tile_col + 1) in solid_tiles:
                # Solid hit (bcs.w $9122)
                return cur_x, steps_taken, True, terrain_lut_lookup(lut, 1)
            steps_taken += 1
            # Next iteration step is 8
            rem_dist -= 1
            cur_x += 1
    else:
        rem_dist = -step_dx
        while True:
            probe_x = cur_x - radius_x
            d4 = compute_tile_distance_left(probe_x)
            if rem_dist <= d4:
                if rem_dist > 0:
                    cur_x -= rem_dist
                break
            rem_dist -= d4
            cur_x -= d4
            tile_col = (cur_x - radius_x) >> 3
            if (tile_col - 1) in solid_tiles:
                # Solid hit
                return cur_x, steps_taken, True, terrain_lut_lookup(lut, 1)
            steps_taken += 1
            rem_dist -= 1
            cur_x -= 1

    surface_type = terrain_lut_lookup(lut, 1)
    return cur_x, steps_taken, False, surface_type


class TestRomHorizontalRaycaster(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.399")
        self.assertEqual(ev["routines"]["pos_x_raycaster"]["direction_code"], "0x11")
        self.assertEqual(ev["routines"]["neg_x_raycaster"]["direction_code"], "0x13")
        saved = json.loads(
            Path("data/thor-evidence/rom_horizontal_raycaster_m15_399.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_terrain_surface_lut(self) -> None:
        lut = [0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]
        self.assertEqual(terrain_lut_lookup(lut, 0x01), 0x00)
        self.assertEqual(terrain_lut_lookup(lut, 0x04), 0x04)
        self.assertEqual(terrain_lut_lookup(lut, 0x08), 0x06)
        self.assertEqual(terrain_lut_lookup(lut, 0x0C), 0x05)
        self.assertEqual(terrain_lut_lookup(lut, 0x11), 0x00)  # masked with 0x0F

    def test_tile_distances(self) -> None:
        # Right distance: 7 - (x & 7)
        self.assertEqual(compute_tile_distance_right(0), 7)
        self.assertEqual(compute_tile_distance_right(6), 1)
        self.assertEqual(compute_tile_distance_right(7), 0)

        # Left distance: x & 7
        self.assertEqual(compute_tile_distance_left(0), 0)
        self.assertEqual(compute_tile_distance_left(6), 6)
        self.assertEqual(compute_tile_distance_left(7), 7)

    def test_raycast_movement_positive_x(self) -> None:
        lut = [0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]
        # Move +10 with no solid walls: start at 100
        final_x, steps, hit, _ = simulate_horizontal_raycast(100, 8, 10, set(), lut)
        self.assertEqual(final_x, 110)
        self.assertFalse(hit)

        # Wall at tile column 14 (pixels 112..119):
        # probe_x = 100 + 8 = 108. Next tile col = 14 (108 + 4 = 112).
        final_x, steps, hit, _ = simulate_horizontal_raycast(100, 8, 10, {14}, lut)
        self.assertTrue(hit)
        self.assertEqual(final_x, 103)  # stopped before entering solid tile

    def test_raycast_movement_negative_x(self) -> None:
        lut = [0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]
        # Move -10 with no solid walls: start at 100
        final_x, steps, hit, _ = simulate_horizontal_raycast(100, 8, -10, set(), lut)
        self.assertEqual(final_x, 90)
        self.assertFalse(hit)

        # Wall at tile column 10 (pixels 80..87):
        final_x, steps, hit, _ = simulate_horizontal_raycast(100, 8, -10, {10}, lut)
        self.assertTrue(hit)
        self.assertEqual(final_x, 96)


if __name__ == "__main__":
    unittest.main()
