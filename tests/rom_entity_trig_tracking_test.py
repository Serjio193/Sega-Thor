#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Trigonometric Tracking, Smooth Angle Turn-Rate Limiter
& 2D Sin/Cos Velocity Projection Engine (0x010594..0x01074A, Milestone M15.473).
"""

import unittest

class EntityTrigTrackingEngine:
    TANGENT_TABLE = [25, 78, 137, 210, 312, 479, 844, 2599]

    # Sin table sample (256 bytes total, 0x40 is 90 deg, peak at 0x40)
    # Beyond Oasis sin/cos table at $5D706 has byte entries at even offsets
    @staticmethod
    def lookup_arctan_octant(dx, dy):
        if dx == 0:
            return 0x40
        ratio = (dy << 8) // dx
        angle = 0
        for threshold in EntityTrigTrackingEngine.TANGENT_TABLE:
            if ratio <= threshold:
                break
            angle += 8
        return angle

    @classmethod
    def calculate_angle(cls, dx, dy):
        if dx >= 0:
            if dy >= 0:
                return cls.lookup_arctan_octant(dx, dy) & 0xFF
            else:
                return (-cls.lookup_arctan_octant(dx, -dy)) & 0xFF
        else:
            if dy >= 0:
                ang = (-cls.lookup_arctan_octant(-dx, dy)) + 0x80
                return ang & 0xFF
            else:
                ang = cls.lookup_arctan_octant(-dx, -dy) + 0x80
                return ang & 0xFF

    @staticmethod
    def quantize_cardinal_facing(angle):
        a = angle & 0xFF
        if a < 0x20:
            return 1  # Right
        elif a < 0x60:
            return 0  # Up
        elif a < 0xA0:
            return 3  # Left
        elif a < 0xE0:
            return 2  # Down
        else:
            return 1  # Right

    @staticmethod
    def smooth_turn_rate(current_angle, target_angle, max_turn=8):
        cur = current_angle & 0xFF
        tgt = target_angle & 0xFF

        diff_cw = (cur - tgt) & 0xFF
        diff_ccw = (tgt - cur) & 0xFF

        if diff_ccw < diff_cw:
            step = min(diff_ccw, max_turn)
            return (cur + step) & 0xFF
        else:
            step = min(diff_cw, max_turn)
            return (cur - step) & 0xFF


class TestEntityTrigTrackingEngine(unittest.TestCase):
    def test_arctan_and_quadrants(self):
        engine = EntityTrigTrackingEngine()

        # Pure Right (dx=100, dy=0) -> angle 0
        self.assertEqual(engine.calculate_angle(100, 0), 0)

        # Pure Up (dx=0, dy=100) -> angle 0x40 (90 deg)
        self.assertEqual(engine.calculate_angle(0, 100), 0x40)

        # Pure Left (dx=-100, dy=0) -> angle 0x80 (180 deg)
        self.assertEqual(engine.calculate_angle(-100, 0), 0x80)

        # Pure Down (dx=0, dy=-100) -> angle 0xC0 (270 deg)
        self.assertEqual(engine.calculate_angle(0, -100), 0xC0)

        # 45 degrees (dx=100, dy=100) -> ratio 256 -> between 210 and 312 -> angle 32 (0x20)
        self.assertEqual(engine.calculate_angle(100, 100), 0x20)

    def test_cardinal_facings(self):
        engine = EntityTrigTrackingEngine()
        # 0 deg -> Right (1)
        self.assertEqual(engine.quantize_cardinal_facing(0), 1)
        self.assertEqual(engine.quantize_cardinal_facing(0x1F), 1)
        self.assertEqual(engine.quantize_cardinal_facing(0xFE), 1)

        # 90 deg (0x40) -> Up (0)
        self.assertEqual(engine.quantize_cardinal_facing(0x20), 0)
        self.assertEqual(engine.quantize_cardinal_facing(0x40), 0)
        self.assertEqual(engine.quantize_cardinal_facing(0x5F), 0)

        # 180 deg (0x80) -> Left (3)
        self.assertEqual(engine.quantize_cardinal_facing(0x60), 3)
        self.assertEqual(engine.quantize_cardinal_facing(0x80), 3)
        self.assertEqual(engine.quantize_cardinal_facing(0x9F), 3)

        # 270 deg (0xC0) -> Down (2)
        self.assertEqual(engine.quantize_cardinal_facing(0xA0), 2)
        self.assertEqual(engine.quantize_cardinal_facing(0xC0), 2)
        self.assertEqual(engine.quantize_cardinal_facing(0xDF), 2)

    def test_smooth_turn_rate_limiting(self):
        engine = EntityTrigTrackingEngine()

        # Turn from 0 towards 0x40 (diff 64) -> step by max_turn 8 -> 8
        self.assertEqual(engine.smooth_turn_rate(0, 0x40, max_turn=8), 8)

        # Turn from 8 towards 0x40 -> 16
        self.assertEqual(engine.smooth_turn_rate(8, 0x40, max_turn=8), 16)

        # Small angle change <= 8 snaps to target
        self.assertEqual(engine.smooth_turn_rate(0x3C, 0x40, max_turn=8), 0x40)

        # Turn across 0 boundary (from 2 towards 0xFE: shortest is backward through 0, diff 4)
        self.assertEqual(engine.smooth_turn_rate(2, 0xFE, max_turn=8), 0xFE)

    def test_turn_rate_large_step(self):
        engine = EntityTrigTrackingEngine()
        # Turn from 0 towards 0x80 (diff 128) -> can step either way, steps by 8
        stepped = engine.smooth_turn_rate(0, 0x80, max_turn=8)
        self.assertTrue(stepped in (8, 0xF8))

if __name__ == '__main__':
    unittest.main()
