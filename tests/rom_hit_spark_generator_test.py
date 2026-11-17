#!/usr/bin/env python3
"""
rom_hit_spark_generator_test.py — Unit tests for M15.424:
ROM Combat Hit Spark Particle Generator & 3D Overlap Centerpoint Calculator (0x00C012..0x00C0BE)
"""

import json
import os
import unittest
from tools.extract_rom_hit_spark_generator import (
    compute_spark_coords,
    START_ADDR,
    END_ADDR,
    SIZE,
    BUFFER_ADDR,
    SLOT_COUNT,
    SLOT_SIZE,
    PARTICLE_TYPE,
    LIFETIME_FRAMES,
)

class TestRomHitSparkGenerator(unittest.TestCase):
    def setUp(self):
        self.evidence_path = "data/thor-evidence/rom_hit_spark_generator_m15_424.json"

    def test_evidence_json_validity(self):
        self.assertTrue(os.path.exists(self.evidence_path))
        with open(self.evidence_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.assertEqual(data["start_address"], f"0x{START_ADDR:06X}")
        self.assertEqual(data["end_address"], f"0x{END_ADDR:06X}")
        self.assertEqual(data["size_bytes"], SIZE)
        self.assertEqual(data["buffer_address"], f"0x{BUFFER_ADDR:08X}")
        self.assertEqual(data["slot_count"], SLOT_COUNT)
        self.assertEqual(data["slot_size_bytes"], SLOT_SIZE)
        self.assertEqual(data["particle_type"], PARTICLE_TYPE)
        self.assertEqual(data["lifetime_frames"], LIFETIME_FRAMES)

    def test_clamped_midpoint_calculation(self):
        target = {
            "x": 100,
            "y": 200,
            "z": 0,
            "elev": 10,
            "rad_x": 20,
            "rad_y": 15,
            "height": 40,
        }
        hitbox = {
            "x_min": 80,
            "x_max": 110,
            "y_min": 190,
            "y_max": 210,
            "z_min": 5,
            "z_max": 25,
        }
        spark_x, spark_y, mid_z = compute_spark_coords(hitbox, target)

        # Target Z: [10, 50]. Overlap with [5, 25] -> [10, 25].
        # Mid Z: 10 + (15 // 2) = 17
        self.assertEqual(mid_z, 17)

        # Target X: [80, 120]. Overlap with [80, 110] -> [80, 110].
        # Mid X: 80 + (30 // 2) = 95
        self.assertEqual(spark_x, 95)

        # Target Y: [200+10-15, 200+10+15] = [195, 225].
        # Overlap with [190, 210] -> [195, 210].
        # Mid Y: 195 + (15 // 2) = 202
        # Projected Y: mid_y - mid_z = 202 - 17 = 185
        self.assertEqual(spark_y, 185)

    def test_slot_allocation_pool(self):
        # 4 slots of 8 bytes: [+0=active_type, +2=x, +4=y, +6=lifetime]
        slots = [{"active": 0} for _ in range(4)]

        def allocate_spark(pool):
            for i, s in enumerate(pool):
                if s["active"] == 0:
                    s["active"] = PARTICLE_TYPE
                    s["lifetime"] = LIFETIME_FRAMES
                    return i
            return -1

        self.assertEqual(allocate_spark(slots), 0)
        self.assertEqual(allocate_spark(slots), 1)
        self.assertEqual(allocate_spark(slots), 2)
        self.assertEqual(allocate_spark(slots), 3)
        # All full -> returns -1 (rts without allocating)
        self.assertEqual(allocate_spark(slots), -1)

    def test_ground_level_spark_projection(self):
        # When target is at elevation 0 and Z is 0:
        target = {
            "x": 50,
            "y": 80,
            "z": 0,
            "elev": 0,
            "rad_x": 10,
            "rad_y": 10,
            "height": 20,
        }
        hitbox = {
            "x_min": 45,
            "x_max": 55,
            "y_min": 75,
            "y_max": 85,
            "z_min": 0,
            "z_max": 10,
        }
        spark_x, spark_y, mid_z = compute_spark_coords(hitbox, target)
        self.assertEqual(mid_z, 5)
        self.assertEqual(spark_x, 50)
        self.assertEqual(spark_y, 80 - 5)  # 75

if __name__ == "__main__":
    unittest.main()
