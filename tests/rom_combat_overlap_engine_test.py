#!/usr/bin/env python3
"""
Unit tests for M15.420: ROM Entity 3D AABB Combat Overlap &
Multi-Pool Collision Iterator Engine (0x00B79A..0x00BAAC).
"""

import os
import struct
import unittest

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")

class TestRomCombatOverlapEngine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_directional_hitbox_bounds_and_opcodes(self):
        # Entry 0x00B79A: moveq #-1, d0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B79A)[0], 0x70FF)
        # Offset +0x42: move.w $42(a6), d1
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B79C)[0], 0x322E)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B79E)[0], 0x0042)
        # Exit RTS at 0x00B850
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B850)[0], 0x4E75)

    def test_facing_hitbox_direction_clears(self):
        def build_directional_box(rad_x, rad_y, facing):
            # Base radii bounds
            x_min = -rad_x
            x_max = rad_x
            y_min = -rad_y
            y_max = rad_y
            if facing == 0:    # Down: d2 cleared (y_min = 0)
                y_min = 0
            elif facing == 1:  # Left: d3 cleared (x_max = 0)
                x_max = 0
            elif facing == 2:  # Up: d4 cleared (y_max = 0)
                y_max = 0
            elif facing == 3:  # Right: d1 cleared (x_min = 0)
                x_min = 0
            return (x_min, x_max, y_min, y_max)

        self.assertEqual(build_directional_box(16, 20, 0), (-16, 16, 0, 20))
        self.assertEqual(build_directional_box(16, 20, 1), (-16, 0, -20, 20))
        self.assertEqual(build_directional_box(16, 20, 2), (-16, 16, -20, 0))
        self.assertEqual(build_directional_box(16, 20, 3), (0, 16, -20, 20))

    def test_target_immunity_and_filters(self):
        def is_target_vulnerable(flags_38, flags_3a, cooldown_82, self_owner, target_addr):
            if (flags_38 & 0x0C) != 0:
                return False  # Intangible / invulnerable
            if (flags_3a & 0x01) != 0:
                return False  # Status protected
            if cooldown_82 != 0:
                return False  # Hit cooldown active
            if self_owner == target_addr:
                return False  # Self damage prevented
            return True

        self.assertTrue(is_target_vulnerable(0x00, 0x00, 0, 0x1000, 0x2000))
        self.assertFalse(is_target_vulnerable(0x04, 0x00, 0, 0x1000, 0x2000))
        self.assertFalse(is_target_vulnerable(0x08, 0x00, 0, 0x1000, 0x2000))
        self.assertFalse(is_target_vulnerable(0x00, 0x01, 0, 0x1000, 0x2000))
        self.assertFalse(is_target_vulnerable(0x00, 0x00, 5, 0x1000, 0x2000))
        self.assertFalse(is_target_vulnerable(0x00, 0x00, 0, 0x1000, 0x1000))

    def test_group2_pool_scanner_bounds(self):
        # 0x00B872: lea $ff19e8.l, a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B872)[0], 0x41F9)
        self.assertEqual(struct.unpack_from(">I", self.rom, 0x00B874)[0], 0x00FF19E8)
        # 21 slots: moveq #$14, d7
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B878)[0], 0x7E14)
        # Context buffer at 0x00B900: lea $ff196a.l, a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B900)[0], 0x41F9)
        self.assertEqual(struct.unpack_from(">I", self.rom, 0x00B902)[0], 0x00FF196A)
        # Exit RTS at 0x00B910
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B910)[0], 0x4E75)

    def test_group4_pool_scanner_bounds(self):
        # 0x00BA08: lea $ff2abc.l, a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00BA08)[0], 0x41F9)
        self.assertEqual(struct.unpack_from(">I", self.rom, 0x00BA0A)[0], 0x00FF2ABC)
        # 8 slots: moveq #7, d7
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00BA0E)[0], 0x7E07)
        # Exit RTS at 0x00BA9A
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00BA9A)[0], 0x4E75)

    def test_aabb_overlap_3d(self):
        def test_aabb_overlap(box1, box2):
            x_overlap = (box1["x_max"] >= box2["x_min"]) and (box1["x_min"] <= box2["x_max"])
            y_overlap = (box1["y_max"] >= box2["y_min"]) and (box1["y_min"] <= box2["y_max"])
            z_overlap = (box1["z_max"] >= box2["z_min"]) and (box1["z_min"] <= box2["z_max"])
            return x_overlap and y_overlap and z_overlap

        b1 = {"x_min": 100, "x_max": 130, "y_min": 200, "y_max": 240, "z_min": 0, "z_max": 30}
        b2_hit = {"x_min": 120, "x_max": 150, "y_min": 220, "y_max": 250, "z_min": 10, "z_max": 40}
        b3_miss = {"x_min": 140, "x_max": 170, "y_min": 220, "y_max": 250, "z_min": 10, "z_max": 40}

        self.assertTrue(test_aabb_overlap(b1, b2_hit))
        self.assertFalse(test_aabb_overlap(b1, b3_miss))

if __name__ == "__main__":
    unittest.main()
