#!/usr/bin/env python3
"""Unit tests for ROM 4-direction world boundary clamp subsystem (0x009D4E..0x009DD6)."""

import unittest
from dataclasses import dataclass


@dataclass
class BoundaryClampState:
    x: int
    y: int
    radius: int
    flags_x: int = 0
    flags_y: int = 0


def clamp_pos_x(dx: int, state: BoundaryClampState, width_in_tiles: int) -> int:
    d7 = (state.x + state.radius) & 0xFFFF
    d3 = ((width_in_tiles << 3) - 1) & 0xFFFF
    d3 = (d3 - d7) & 0xFFFF
    # cmp.w d3, d0; bls (unsigned <=)
    if dx <= d3:
        return dx
    dx = d3
    if dx == 0:
        state.flags_x |= 0x48
    return dx


def clamp_neg_x(dx: int, state: BoundaryClampState) -> int:
    d3 = state.x - state.radius
    if d3 < 0:
        state.flags_x |= 0x48
        return 0
    d3 &= 0xFFFF
    if dx <= d3:
        return dx
    dx = d3
    if dx == 0:
        state.flags_x |= 0x48
    return dx


def clamp_pos_y(dy: int, state: BoundaryClampState, height_in_tiles: int) -> int:
    d7 = (state.y + state.radius) & 0xFFFF
    d3 = ((height_in_tiles << 3) - 1) & 0xFFFF
    d3 = (d3 - d7) & 0xFFFF
    if dy <= d3:
        return dy
    dy = d3
    if dy == 0:
        state.flags_y |= 0x48
    return dy


def clamp_neg_y(dy: int, state: BoundaryClampState) -> int:
    d3 = state.y - state.radius
    if d3 < 0:
        state.flags_y |= 0x48
        return 0
    d3 &= 0xFFFF
    if dy <= d3:
        return dy
    dy = d3
    if dy == 0:
        state.flags_y |= 0x48
    return dy


class RomBoundaryClampsTest(unittest.TestCase):
    def test_clamp_pos_x_within_bounds(self):
        # 100 tiles * 8 = 800 width (max_x = 799)
        # x = 100, radius = 8 -> x + rad = 108, remaining = 799 - 108 = 691
        state = BoundaryClampState(x=100, y=100, radius=8)
        step = clamp_pos_x(10, state, 100)
        self.assertEqual(step, 10)
        self.assertEqual(state.flags_x, 0)

    def test_clamp_pos_x_exceeds_bounds(self):
        # x = 790, radius = 5 -> x + rad = 795, remaining = 799 - 795 = 4
        state = BoundaryClampState(x=790, y=100, radius=5)
        step = clamp_pos_x(10, state, 100)
        self.assertEqual(step, 4)
        self.assertEqual(state.flags_x, 0)

    def test_clamp_pos_x_at_boundary_sets_flag(self):
        # x = 794, radius = 5 -> x + rad = 799, remaining = 0
        state = BoundaryClampState(x=794, y=100, radius=5)
        step = clamp_pos_x(10, state, 100)
        self.assertEqual(step, 0)
        self.assertEqual(state.flags_x, 0x48)

    def test_clamp_neg_x_within_bounds(self):
        state = BoundaryClampState(x=50, y=100, radius=6)
        # x - rad = 44. step = 10 <= 44
        step = clamp_neg_x(10, state)
        self.assertEqual(step, 10)
        self.assertEqual(state.flags_x, 0)

    def test_clamp_neg_x_exceeds_bounds(self):
        state = BoundaryClampState(x=12, y=100, radius=6)
        # x - rad = 6. step = 10 > 6 -> clamped to 6
        step = clamp_neg_x(10, state)
        self.assertEqual(step, 6)
        self.assertEqual(state.flags_x, 0)

    def test_clamp_neg_x_at_boundary(self):
        state = BoundaryClampState(x=6, y=100, radius=6)
        # x - rad = 0 -> clamped to 0 -> flag 0x48 set
        step = clamp_neg_x(10, state)
        self.assertEqual(step, 0)
        self.assertEqual(state.flags_x, 0x48)

    def test_clamp_neg_x_out_of_bounds(self):
        state = BoundaryClampState(x=4, y=100, radius=6)
        # x - rad = -2 < 0 -> clr.w d0, flag 0x48 set
        step = clamp_neg_x(10, state)
        self.assertEqual(step, 0)
        self.assertEqual(state.flags_x, 0x48)

    def test_clamp_pos_y_at_boundary_sets_flags_y(self):
        # 60 tiles * 8 = 480 height (max_y = 479)
        # y = 475, radius = 4 -> y + rad = 479, remaining = 0
        state = BoundaryClampState(x=100, y=475, radius=4)
        step = clamp_pos_y(5, state, 60)
        self.assertEqual(step, 0)
        self.assertEqual(state.flags_y, 0x48)

    def test_clamp_neg_y_out_of_bounds_sets_flags_y(self):
        state = BoundaryClampState(x=100, y=2, radius=6)
        # y - rad = -4 < 0 -> clr.w d0, flag 0x48 set on flags_y
        step = clamp_neg_y(5, state)
        self.assertEqual(step, 0)
        self.assertEqual(state.flags_y, 0x48)


if __name__ == "__main__":
    unittest.main()
