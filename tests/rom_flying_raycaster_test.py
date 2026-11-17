#!/usr/bin/env python3
"""Unit tests for M15.403 ROM Flying/Projectile Dual-Axis (±X, ±Y) Tilemap Raycaster & Point Collision Dispatcher."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_flying_raycaster import ROM_PATH, extract_evidence


def simulate_point_raycast_x(
    start_x: int,
    step_dx: int,
    solid_points: set[int],
) -> tuple[int, bool]:
    """Simulate 0x009708..0x0097E2 horizontal point raycast without radius offset."""
    if step_dx == 0:
        return start_x, False

    cur_x = start_x
    if step_dx > 0:
        rem = step_dx
        while True:
            # 0x009740: d4 = -( (cur_x & 7) - 7 )
            d4 = 7 - (cur_x & 7)
            if rem <= d4:
                if rem > 0:
                    cur_x += rem
                break
            rem -= d4
            cur_x += d4
            # Point probe at cur_x + 1
            if (cur_x + 1) in solid_points:
                return cur_x, True  # Hit wall, bit 6 set
            rem -= 1
            cur_x += 1
    else:
        rem = -step_dx
        while True:
            d4 = cur_x & 7
            if rem <= d4:
                if rem > 0:
                    cur_x -= rem
                break
            rem -= d4
            cur_x -= d4
            if (cur_x - 1) in solid_points:
                return cur_x, True
            rem -= 1
            cur_x -= 1

    return cur_x, False


def simulate_point_raycast_y(
    start_y: int,
    step_dy: int,
    solid_points: set[int],
) -> tuple[int, bool]:
    """Simulate 0x0097E6..0x0098BE vertical point raycast."""
    if step_dy == 0:
        return start_y, False

    cur_y = start_y
    if step_dy > 0:
        rem = step_dy
        while True:
            d4 = 7 - (cur_y & 7)
            if rem <= d4:
                if rem > 0:
                    cur_y += rem
                break
            rem -= d4
            cur_y += d4
            if (cur_y + 1) in solid_points:
                return cur_y, True
            rem -= 1
            cur_y += 1
    else:
        rem = -step_dy
        while True:
            d4 = cur_y & 7
            if rem <= d4:
                if rem > 0:
                    cur_y -= rem
                break
            rem -= d4
            cur_y -= d4
            if (cur_y - 1) in solid_points:
                return cur_y, True
            rem -= 1
            cur_y -= 1

    return cur_y, False


class TestRomFlyingRaycaster(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.403")
        self.assertEqual(ev["routines"]["flying_horizontal_raycast"]["pos_x_dir"], "0x11")
        self.assertEqual(ev["routines"]["flying_horizontal_raycast"]["neg_x_dir"], "0x13")
        self.assertEqual(ev["routines"]["flying_vertical_raycast"]["pos_y_dir"], "0x12")
        self.assertEqual(ev["routines"]["flying_vertical_raycast"]["neg_y_dir"], "0x10")
        self.assertEqual(ev["routines"]["flying_surface_sample_and_exit"]["exit_target"], "0x009382")
        saved = json.loads(
            Path("data/thor-evidence/rom_flying_raycaster_m15_403.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_flying_horizontal_move_clear(self) -> None:
        # Move +10 from x=100 without collisions
        final_x, hit = simulate_point_raycast_x(100, 10, set())
        self.assertEqual(final_x, 110)
        self.assertFalse(hit)

        # Move -10 from x=100 without collisions
        final_x, hit = simulate_point_raycast_x(100, -10, set())
        self.assertEqual(final_x, 90)
        self.assertFalse(hit)

    def test_flying_horizontal_wall_hit(self) -> None:
        # Moving +10 from 100 hits point 104
        final_x, hit = simulate_point_raycast_x(100, 10, {104})
        self.assertTrue(hit)
        self.assertEqual(final_x, 103)

        # Moving -10 from 100 hits point 95
        final_x, hit = simulate_point_raycast_x(100, -10, {95})
        self.assertTrue(hit)
        self.assertEqual(final_x, 96)

    def test_flying_vertical_move(self) -> None:
        # Move +15 from y=200 without collisions
        final_y, hit = simulate_point_raycast_y(200, 15, set())
        self.assertEqual(final_y, 215)
        self.assertFalse(hit)

        # Moving +15 hits barrier at 208
        final_y, hit = simulate_point_raycast_y(200, 15, {208})
        self.assertTrue(hit)
        self.assertEqual(final_y, 207)


if __name__ == "__main__":
    unittest.main()
