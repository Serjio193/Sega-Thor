#!/usr/bin/env python3
"""
rom_bow_arrow_engine_test.py - M15.460 Test Suite
Tests Player Ali Bow Arrow Projectile Spawner, Ground/Water Offsets
& Parabolic Launch Physics Subsystem.
"""

import json
import os
import unittest

EVIDENCE_JSON = "data/thor-evidence/rom_bow_arrow_engine_m15_460.json"

def calculate_arrow_trajectory(elevation_px: int):
    """Simulates 0x00F434..0x00F45C elevation-dependent launch speed selection."""
    if elevation_px >= 16:
        speed_h = 0x0300
        vel_z = 0x00020000 # +2.0 px/frame
    else:
        speed_h = 0x0400
        vel_z = 0x00028000 # +2.5 px/frame
    return speed_h, vel_z

def calculate_arrow_anchor(facing: int, d1_val: int, is_water: bool, scale_bit: bool):
    """Simulates 0x00F3DE..0x00F40A launch anchor calculation."""
    if is_water:
        offsets = [(14, -18), (18, 4), (-6, 20), (-18, 4)]
    else:
        offsets = [(6, -56), (32, -34), (-10, -8), (-32, -34)]
        
    dx, dy = offsets[facing & 3]
    dy += d1_val
    
    anchor_x = dx << 16
    anchor_y = dy << 16
    
    if scale_bit:
        anchor_x = (anchor_x * 2) & 0xFFFFFFFF
        anchor_y = (anchor_y * 2) & 0xFFFFFFFF
    else:
        anchor_x &= 0xFFFFFFFF
        anchor_y &= 0xFFFFFFFF
        
    return anchor_x, anchor_y

class TestRomBowArrowEngine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.abspath(os.path.join(self_dir, ".."))
        json_path = os.path.join(repo_root, EVIDENCE_JSON)
        with open(json_path, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.460")
        self.assertEqual(self.evidence["rom_range"]["start"], "0x00F396")
        self.assertEqual(self.evidence["rom_range"]["end"], "0x00F476")
        self.assertEqual(self.evidence["rom_range"]["size_bytes"], 224)
        self.assertEqual(len(self.evidence["ground_launch_offsets"]), 4)
        self.assertEqual(len(self.evidence["water_launch_offsets"]), 4)

    def test_ground_launch_offsets(self):
        g = self.evidence["ground_launch_offsets"]
        self.assertEqual((g[0]["offset_dx"], g[0]["offset_dy"]), (6, -56))
        self.assertEqual((g[1]["offset_dx"], g[1]["offset_dy"]), (32, -34))
        self.assertEqual((g[2]["offset_dx"], g[2]["offset_dy"]), (-10, -8))
        self.assertEqual((g[3]["offset_dx"], g[3]["offset_dy"]), (-32, -34))

    def test_water_launch_offsets(self):
        w = self.evidence["water_launch_offsets"]
        self.assertEqual((w[0]["offset_dx"], w[0]["offset_dy"]), (14, -18))
        self.assertEqual((w[1]["offset_dx"], w[1]["offset_dy"]), (18, 4))
        self.assertEqual((w[2]["offset_dx"], w[2]["offset_dy"]), (-6, 20))
        self.assertEqual((w[3]["offset_dx"], w[3]["offset_dy"]), (-18, 4))

    def test_elevation_dependent_arc(self):
        # High elevation >= 16 (e.g. 16, 20) -> speed 0x300, vz 0x20000
        sp_h, vz = calculate_arrow_trajectory(16)
        self.assertEqual(sp_h, 0x0300)
        self.assertEqual(vz, 0x00020000)

        sp_h, vz = calculate_arrow_trajectory(25)
        self.assertEqual(sp_h, 0x0300)
        self.assertEqual(vz, 0x00020000)

        # Low elevation < 16 (e.g. 0, 15) -> speed 0x400, vz 0x28000
        sp_h, vz = calculate_arrow_trajectory(0)
        self.assertEqual(sp_h, 0x0400)
        self.assertEqual(vz, 0x00028000)

        sp_h, vz = calculate_arrow_trajectory(15)
        self.assertEqual(sp_h, 0x0400)
        self.assertEqual(vz, 0x00028000)

    def test_anchor_calculation(self):
        # Ground facing Right (32, -34), d1=4
        ax, ay = calculate_arrow_anchor(facing=1, d1_val=4, is_water=False, scale_bit=False)
        self.assertEqual(ax, 0x00200000)
        self.assertEqual(ay, (-30 << 16) & 0xFFFFFFFF)

        # Water facing Down (-6, 20), d1=0, scale_bit=True
        ax_w, ay_w = calculate_arrow_anchor(facing=2, d1_val=0, is_water=True, scale_bit=True)
        self.assertEqual(ax_w, (-12 << 16) & 0xFFFFFFFF)
        self.assertEqual(ay_w, (40 << 16) & 0xFFFFFFFF)

if __name__ == "__main__":
    unittest.main()
