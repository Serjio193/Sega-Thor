#!/usr/bin/env python3
"""
rom_math_sqrt_atan_test.py — Unit tests for ROM Newton-Raphson Integer Square Root
& 256-Step Circular Arctan Engine
(0x00E6BA..0x00E78E, 212 bytes).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_math_sqrt_atan_m15_451.json"
)


class TestRomMathSqrtAtan(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(EVIDENCE_PATH):
            raise FileNotFoundError(f"Evidence file missing at {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_rom_constants(self):
        self.assertEqual(self.evidence["start_address"], "0x00E6BA")
        self.assertEqual(self.evidence["end_address"], "0x00E78E")
        self.assertEqual(self.evidence["byte_count"], 212)
        self.assertEqual(self.evidence["instruction_count"], 68)
        self.assertEqual(self.evidence["tangent_table_address"], "0x05D906")

        expected_tangents = [25, 78, 137, 210, 312, 479, 844, 2599, 65535]
        self.assertEqual(self.evidence["tangent_table"], expected_tangents)

    def test_integer_sqrt_divu_newton_raphson(self):
        def rom_sqrt(val):
            if val == 0:
                return 0
            if val < 0x400:
                d2 = val
            else:
                d2 = 0x400

            while True:
                d1 = d2
                d2 = (val // d1 + d1) // 2
                if d2 >= d1:
                    break
            return d1

        # Perfect squares
        self.assertEqual(rom_sqrt(0), 0)
        self.assertEqual(rom_sqrt(1), 1)
        self.assertEqual(rom_sqrt(4), 2)
        self.assertEqual(rom_sqrt(9), 3)
        self.assertEqual(rom_sqrt(16), 4)
        self.assertEqual(rom_sqrt(25), 5)
        self.assertEqual(rom_sqrt(100), 10)
        self.assertEqual(rom_sqrt(10000), 100)
        self.assertEqual(rom_sqrt(65536), 256)
        self.assertEqual(rom_sqrt(1000000), 1000)

        # Non-perfect squares (floor)
        self.assertEqual(rom_sqrt(2), 1)
        self.assertEqual(rom_sqrt(3), 1)
        self.assertEqual(rom_sqrt(15), 3)
        self.assertEqual(rom_sqrt(99), 9)
        self.assertEqual(rom_sqrt(1000), 31)

    def test_trig_atan_first_octant(self):
        tangents = self.evidence["tangent_table"]

        def first_octant_atan(dx, dy):
            if dx == 0:
                return 0x40  # 64 = 90 degrees
            slope = (dy << 8) // dx
            angle = 0
            for t in tangents:
                if slope <= t:
                    break
                angle += 8
            return angle

        # Horizontal: dy == 0 -> angle 0
        self.assertEqual(first_octant_atan(100, 0), 0)

        # 45 degrees: dx == dy -> slope = 256 <= 312 (entry 4) -> angle = 32
        # (32 / 256 * 360 = 45.0 degrees!)
        self.assertEqual(first_octant_atan(100, 100), 32)

        # Vertical: dx == 0 -> angle 64 (90 degrees)
        self.assertEqual(first_octant_atan(0, 100), 64)

    def test_trig_atan2_all_four_quadrants(self):
        tangents = self.evidence["tangent_table"]

        def first_octant(dx, dy):
            if dx == 0:
                return 0x40
            slope = (dy << 8) // dx
            angle = 0
            for t in tangents:
                if slope <= t:
                    break
                angle += 8
            return angle

        def rom_atan2(dx, dy):
            # 0x00E722: dx in d0, dy in d2
            if dx >= 0:
                if dy >= 0:
                    # Q1
                    return first_octant(dx, dy)
                else:
                    # Q4: neg.w d2; bsr; neg.b d0
                    val = first_octant(dx, -dy)
                    return (-val) & 0xFF
            else:
                neg_dx = -dx
                if dy >= 0:
                    # Q2: neg.b d0; addi.b #$80, d0
                    val = first_octant(neg_dx, dy)
                    return ((-val) + 0x80) & 0xFF
                else:
                    # Q3: addi.b #$80, d0
                    val = first_octant(neg_dx, -dy)
                    return (val + 0x80) & 0xFF

        # Cardinal angles
        self.assertEqual(rom_atan2(100, 0), 0)       # East / Right: 0
        self.assertEqual(rom_atan2(0, 100), 64)      # South / Down: 64 (90 deg)
        self.assertEqual(rom_atan2(-100, 0), 128)    # West / Left: 128 (180 deg)
        self.assertEqual(rom_atan2(0, -100), 192)    # North / Up: 192 (270 deg)

        # Diagonals (32 steps = 45 deg)
        self.assertEqual(rom_atan2(100, 100), 32)    # SE: 45 deg
        self.assertEqual(rom_atan2(-100, 100), 96)   # SW: 135 deg
        self.assertEqual(rom_atan2(-100, -100), 160) # NW: 225 deg
        self.assertEqual(rom_atan2(100, -100), 224)  # NE: 315 deg


if __name__ == "__main__":
    unittest.main()
