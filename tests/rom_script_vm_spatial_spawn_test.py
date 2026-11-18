#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Spatial Positioning, Nearest Offscreen Target
& Edge Spawner Subsystem (0x0103B0..0x010594, Milestone M15.472).
"""

import unittest

class ScriptSpatialSpawnEngine:
    def __init__(self):
        self.plane_x = 0x0200
        self.plane_y = 0x0200
        self.ali_x = 0x0280
        self.ali_y = 0x0280
        self.target_points = [(0xFFFF, 0xFFFF)] * 16

    def copy_fixed_coords(self, coords):
        # 0x0103B0:
        return True, coords[0], coords[1]

    def select_nearest_offscreen_target_point(self):
        # 0x0103BE:
        cam_x = self.plane_x - 0x40
        cam_y = self.plane_y - 0x50
        view_w = 0x1C0
        view_h = 0x180

        min_dist = 0xFFFF
        best_pt = None

        for pt in self.target_points:
            x, y = pt
            if x == 0xFFFF and y == 0xFFFF:
                continue

            # Check if point is inside viewport
            if cam_x <= x <= (cam_x + view_w) and cam_y <= y <= (cam_y + view_h):
                continue  # Inside view -> reject

            # Manhattan distance to Ali
            dist = abs(x - self.ali_x) + abs(y - self.ali_y)
            if dist < min_dist:
                min_dist = dist
                best_pt = (x, y)

        if best_pt is not None:
            return True, best_pt[0], best_pt[1]
        return False, None, None

    def select_edge_spawn_point(self, edge, prng_offset):
        # 0x01046A:
        if edge == 0:  # Top
            x = self.plane_x + (prng_offset << 3)
            y = self.plane_y - 0x50
            facing = 2  # Down
        elif edge == 1:  # Right
            x = self.plane_x + 0x190
            y = self.plane_y + (prng_offset << 3)
            facing = 3  # Left
        elif edge == 2:  # Bottom
            x = self.plane_x + (prng_offset << 3)
            y = self.plane_y + 0x180
            facing = 0  # Up
        else:  # Left (edge == 3)
            x = self.plane_x - 0x50
            y = self.plane_y + (prng_offset << 3)
            facing = 1  # Right

        return True, x, y, facing


class TestScriptSpatialSpawnEngine(unittest.TestCase):
    def test_copy_fixed_coords(self):
        engine = ScriptSpatialSpawnEngine()
        success, x, y = engine.copy_fixed_coords((150, 220))
        self.assertTrue(success)
        self.assertEqual(x, 150)
        self.assertEqual(y, 220)

    def test_select_nearest_offscreen_point(self):
        engine = ScriptSpatialSpawnEngine()
        # Viewport: [0x200 - 0x40 = 0x1C0, 0x1C0 + 0x1C0 = 0x380] x [0x200 - 0x50 = 0x1B0, 0x1B0 + 0x180 = 0x330]
        # Ali at (0x280, 0x280) - well within view.
        # Pt 0: (0x280, 0x280) -> on-screen, rejected.
        # Pt 1: (0x390, 0x280) -> off-screen right! Manhattan dist = (0x390 - 0x280) = 0x110 = 272.
        # Pt 2: (0x450, 0x280) -> off-screen right! Manhattan dist = (0x450 - 0x280) = 0x1D0 = 464.
        engine.target_points[0] = (0x0280, 0x0280)
        engine.target_points[1] = (0x0390, 0x0280)
        engine.target_points[2] = (0x0450, 0x0280)

        success, x, y = engine.select_nearest_offscreen_target_point()
        self.assertTrue(success)
        self.assertEqual((x, y), (0x0390, 0x0280))

    def test_all_points_rejected(self):
        engine = ScriptSpatialSpawnEngine()
        # All points on-screen or 0xFFFF
        engine.target_points[0] = (0x0280, 0x0280)  # inside view
        engine.target_points[1] = (0xFFFF, 0xFFFF)  # unused
        success, x, y = engine.select_nearest_offscreen_target_point()
        self.assertFalse(success)
        self.assertIsNone(x)

    def test_select_edge_spawn_point(self):
        engine = ScriptSpatialSpawnEngine()
        # Top edge (0), offset 5 -> x = 0x200 + 40 = 0x228, y = 0x200 - 0x50 = 0x1B0, facing = 2 (Down)
        success, x, y, facing = engine.select_edge_spawn_point(edge=0, prng_offset=5)
        self.assertTrue(success)
        self.assertEqual((x, y, facing), (0x0228, 0x01B0, 2))

        # Right edge (1), offset 3 -> x = 0x200 + 0x190 = 0x390, y = 0x200 + 24 = 0x218, facing = 3 (Left)
        success, x, y, facing = engine.select_edge_spawn_point(edge=1, prng_offset=3)
        self.assertTrue(success)
        self.assertEqual((x, y, facing), (0x0390, 0x0218, 3))

        # Bottom edge (2), offset 4 -> x = 0x200 + 32 = 0x220, y = 0x200 + 0x180 = 0x380, facing = 0 (Up)
        success, x, y, facing = engine.select_edge_spawn_point(edge=2, prng_offset=4)
        self.assertTrue(success)
        self.assertEqual((x, y, facing), (0x0220, 0x0380, 0))

        # Left edge (3), offset 2 -> x = 0x200 - 0x50 = 0x1B0, y = 0x200 + 16 = 0x210, facing = 1 (Right)
        success, x, y, facing = engine.select_edge_spawn_point(edge=3, prng_offset=2)
        self.assertTrue(success)
        self.assertEqual((x, y, facing), (0x01B0, 0x0210, 1))

if __name__ == '__main__':
    unittest.main()
