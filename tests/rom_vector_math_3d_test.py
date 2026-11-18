#!/usr/bin/env python3
"""
rom_vector_math_3d_test.py - M15.457 Test Suite
Tests 3D Vector Mathematics Engine: Euclidean Distance (2D/3D),
Direction Normalization, Sphere Interception Clamp & Projectile Launch Velocity.
"""

import json
import math
import os
import unittest

EVIDENCE_JSON = "data/thor-evidence/rom_vector_math_3d_m15_457.json"

def isqrt_m68k(val: int) -> int:
    """Simulates 0x00E6BA integer square root."""
    if val <= 0:
        return 0
    return int(math.isqrt(val))

def dist_2d(x0: int, y0: int, x1: int, y1: int) -> int:
    """Simulates 0x00F0D2."""
    dx = x1 - x0
    dy = y1 - y0
    sum_sq = dx * dx + dy * dy
    return isqrt_m68k(sum_sq)

def dist_3d(x0: int, y0: int, z0: int, x1: int, y1: int, z1: int) -> int:
    """Simulates 0x00F0EC."""
    dx = x1 - x0
    dy = y1 - y0
    dz = z1 - z0
    sum_sq = dx * dx + dy * dy + dz * dz
    return isqrt_m68k(sum_sq)

def normalize_velocity_3d(x0, y0, z0, x1, y1, z1, speed: int):
    """Simulates 0x00F10C."""
    dx = x1 - x0
    dy = y1 - y0
    dz = z1 - z0
    d = isqrt_m68k(dx * dx + dy * dy + dz * dz)
    if d == 0:
        return 0, 0, 0
    # divs.w d4, d1
    vx = int((speed * dx) / d)
    vy = int((speed * dy) / d)
    vz = int((speed * dz) / d)
    # swap; clr.w; asr.l #4
    vx_fix = ((vx << 16) & 0xFFFFFFFF)
    vy_fix = ((vy << 16) & 0xFFFFFFFF)
    vz_fix = ((vz << 16) & 0xFFFFFFFF)
    # As signed 32-bit arithmetic shift right 4
    if vx_fix >= 0x80000000: vx_fix -= 0x100000000
    if vy_fix >= 0x80000000: vy_fix -= 0x100000000
    if vz_fix >= 0x80000000: vz_fix -= 0x100000000
    vx_fix >>= 4
    vy_fix >>= 4
    vz_fix >>= 4
    return vx_fix & 0xFFFFFFFF, vy_fix & 0xFFFFFFFF, vz_fix & 0xFFFFFFFF

def sphere_clamp_3d(x0, y0, z0, x1, y1, z1, radius: int):
    """Simulates 0x00F160."""
    dx = x1 - x0
    dy = y1 - y0
    dz = z1 - z0
    d = isqrt_m68k(dx * dx + dy * dy + dz * dz)
    if d <= radius:
        carry = False
        return x1, y1, z1, carry
    
    # Clamping: P = P0 + (delta * radius) / dist
    cx = x0 + int((dx * radius) / d)
    cy = y0 + int((dy * radius) / d)
    cz = z0 + int((dz * radius) / d)
    carry = True
    return cx, cy, cz, carry

class TestRomVectorMath3D(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.abspath(os.path.join(self_dir, ".."))
        json_path = os.path.join(repo_root, EVIDENCE_JSON)
        with open(json_path, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.457")
        self.assertEqual(self.evidence["rom_range"]["start"], "0x00F054")
        self.assertEqual(self.evidence["rom_range"]["end"], "0x00F1B2")
        self.assertEqual(self.evidence["rom_range"]["size_bytes"], 350)
        self.assertEqual(self.evidence["rom_range"]["total_instructions"], 135)
        self.assertEqual(len(self.evidence["velocity_table_00F0C2"]), 4)
        self.assertEqual(len(self.evidence["subroutines"]), 6)

    def test_velocity_table_values(self):
        tbl = self.evidence["velocity_table_00F0C2"]
        # Entry 0
        self.assertEqual(tbl[0]["speed_xy"], "0x0300")
        self.assertEqual(tbl[0]["launch_z_elev_velocity"], "0x00030000")
        # Entry 1
        self.assertEqual(tbl[1]["speed_xy"], "0x0500")
        self.assertEqual(tbl[1]["launch_z_elev_velocity"], "0x00040000")
        # Entry 2
        self.assertEqual(tbl[2]["speed_xy"], "0x0700")
        self.assertEqual(tbl[2]["launch_z_elev_velocity"], "0x00050000")
        # Entry 3 (Close-distance vertical pop)
        self.assertEqual(tbl[3]["speed_xy"], "0x0000")
        self.assertEqual(tbl[3]["launch_z_elev_velocity"], "0x00080000")

    def test_euclidean_distance_2d(self):
        self.assertEqual(dist_2d(0, 0, 3, 4), 5)
        self.assertEqual(dist_2d(10, 20, 40, 60), 50)
        self.assertEqual(dist_2d(100, 100, 70, 60), 50)

    def test_euclidean_distance_3d(self):
        self.assertEqual(dist_3d(0, 0, 0, 1, 2, 2), 3)
        self.assertEqual(dist_3d(0, 0, 0, 10, 20, 20), 30)
        self.assertEqual(dist_3d(5, 5, 5, 17, 21, 20), 25)

    def test_normalize_velocity_3d(self):
        # Stationary delta
        self.assertEqual(normalize_velocity_3d(0, 0, 0, 0, 0, 0, 10), (0, 0, 0))
        # 3-4-0 triangle with speed 50 -> components 30 and 40
        # fixed-point shift: val << 12
        vx, vy, vz = normalize_velocity_3d(0, 0, 0, 3, 4, 0, 50)
        self.assertEqual(vx, 30 << 12)
        self.assertEqual(vy, 40 << 12)
        self.assertEqual(vz, 0)

    def test_sphere_interception_clamp_3d(self):
        # Target inside sphere: no clamp, carry = False
        cx, cy, cz, carry = sphere_clamp_3d(0, 0, 0, 10, 10, 10, 50)
        self.assertFalse(carry)
        self.assertEqual((cx, cy, cz), (10, 10, 10))

        # Target outside sphere (dist 50, radius 25): clamped to surface, carry = True
        cx, cy, cz, carry = sphere_clamp_3d(0, 0, 0, 30, 40, 0, 25)
        self.assertTrue(carry)
        self.assertEqual((cx, cy, cz), (15, 20, 0))
        # Surface distance should equal radius
        self.assertEqual(dist_3d(0, 0, 0, cx, cy, cz), 25)

if __name__ == "__main__":
    unittest.main()
