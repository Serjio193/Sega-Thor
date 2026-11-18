#!/usr/bin/env python3
"""
rom_spatial_audio_pan_test.py — Unit tests for ROM Positional Spatial Audio Dispatcher,
Distance Attenuation & Stereo Pan Subsystem
(0x00E78E..0x00E90E, 384 bytes).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_spatial_audio_pan_m15_452.json"
)


class TestRomSpatialAudioPan(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(EVIDENCE_PATH):
            raise FileNotFoundError(f"Evidence file missing at {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_rom_constants(self):
        self.assertEqual(self.evidence["start_address"], "0x00E78E")
        self.assertEqual(self.evidence["end_address"], "0x00E90E")
        self.assertEqual(self.evidence["byte_count"], 384)
        self.assertEqual(self.evidence["instruction_count"], 114)

        insns = self.evidence["instructions"]
        # Sound driver address $60004
        driver_found = any("60004" in i.get("op_str", "") for i in insns)
        self.assertTrue(driver_found, "Sound driver jump $60004 should be present")

        # Threshold #$200 and #$300
        th200_found = any("200" in i.get("op_str", "") for i in insns)
        th300_found = any("300" in i.get("op_str", "") for i in insns)
        self.assertTrue(th200_found, "Threshold #$200 should be present")
        self.assertTrue(th300_found, "Threshold #$300 should be present")

    def test_non_spatial_bypass(self):
        def evaluate_spatial_filter(sound_id):
            if sound_id < 0x200:
                return "direct_driver_call"
            return "spatial_processing"

        self.assertEqual(evaluate_spatial_filter(0x0010), "direct_driver_call")
        self.assertEqual(evaluate_spatial_filter(0x01FF), "direct_driver_call")
        self.assertEqual(evaluate_spatial_filter(0x0200), "spatial_processing")
        self.assertEqual(evaluate_spatial_filter(0x0350), "spatial_processing")

    def test_isometric_distance_calculation(self):
        def calculate_distance(tgt_x, tgt_y, player_x, player_y):
            dx = abs(tgt_x - player_x)
            dy = abs(tgt_y - player_y)
            dy_isometric = dy + (dy >> 2)  # 1.25x scaling
            return max(dx, dy_isometric)

        # Pure horizontal
        self.assertEqual(calculate_distance(200, 100, 100, 100), 100)

        # Pure vertical (dy=80 -> 80 + 20 = 100)
        self.assertEqual(calculate_distance(100, 180, 100, 100), 100)

        # Diagonal with Y dominance
        # dx=50, dy=60 -> dy_iso = 60 + 15 = 75 -> dist = 75
        self.assertEqual(calculate_distance(150, 160, 100, 100), 75)

    def test_mode_a_sound_attenuation_and_culling(self):
        def process_mode_a(sound_id, tgt_x, tgt_y, player_x, player_y):
            raw_dx = tgt_x - player_x
            dx = abs(raw_dx)
            dy = abs(tgt_y - player_y)
            dy_iso = dy + (dy >> 2)
            dist = max(dx, dy_iso)

            # Near radius: <= 0x58 (88px) no attenuation
            if dist > 0x58:
                dist -= 0x58
            else:
                dist = 0

            step = dist >> 5  # 32px buckets
            if step > 7:
                return None  # culled!

            pan = "center"
            if dx >= 0x40:  # 64px horizontal offset
                pan = "left" if raw_dx < 0 else "right"

            return {"step": step, "pan": pan}

        # Near player: step 0, center
        res = process_mode_a(0x0350, 120, 110, 100, 100)
        self.assertIsNotNone(res)
        self.assertEqual(res["step"], 0)
        self.assertEqual(res["pan"], "center")

        # Far right: dx=100 (>= 64px), dist=100 -> dist-0x58 = 100-88 = 12 -> step 0, pan right
        res = process_mode_a(0x0350, 200, 100, 100, 100)
        self.assertIsNotNone(res)
        self.assertEqual(res["pan"], "right")

        # Distant: dx=400 -> step > 7 -> culled
        res = process_mode_a(0x0350, 500, 100, 100, 100)
        self.assertIsNone(res)

    def test_mode_b_sound_attenuation_and_culling(self):
        def process_mode_b(sound_id, tgt_x, tgt_y, player_x, player_y):
            raw_dx = tgt_x - player_x
            dx = abs(raw_dx)
            dy = abs(tgt_y - player_y)
            dy_iso = dy + (dy >> 2)
            dist = max(dx, dy_iso)

            # Cutoff at 0x110 (272px)
            if dist >= 0x110:
                return None  # culled!

            volume_reduction = 0
            if dist >= 0xB0:
                volume_reduction += 1
            if dist >= 0xE0:
                volume_reduction += 1

            pan = "center"
            if dx >= 0x60:  # 96px horizontal offset
                pan = "left" if raw_dx < 0 else "right"

            return {"reduction": volume_reduction, "pan": pan}

        # Close: reduction 0, center
        res = process_mode_b(0x0250, 110, 110, 100, 100)
        self.assertIsNotNone(res)
        self.assertEqual(res["reduction"], 0)
        self.assertEqual(res["pan"], "center")

        # Mid distance: dist=190 (>= 0xB0, < 0xE0)
        res = process_mode_b(0x0250, 100, 252, 100, 100)  # dy=152 -> 152 + 38 = 190
        self.assertIsNotNone(res)
        self.assertEqual(res["reduction"], 1)

        # Distant cutoff: dist=300 >= 272 -> culled
        res = process_mode_b(0x0250, 400, 100, 100, 100)
        self.assertIsNone(res)


if __name__ == "__main__":
    unittest.main()
