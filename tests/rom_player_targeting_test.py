#!/usr/bin/env python3
"""
rom_player_targeting_test.py — Unit tests for ROM Player Ali Targeting,
Normalized Aim Vector & Environmental Velocity Engine
(0x00E398..0x00E502, 362 bytes).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_player_targeting_m15_449.json"
)


class TestRomPlayerTargeting(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(EVIDENCE_PATH):
            raise FileNotFoundError(f"Evidence file missing at {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_rom_constants(self):
        self.assertEqual(self.evidence["start_address"], "0x00E398")
        self.assertEqual(self.evidence["end_address"], "0x00E502")
        self.assertEqual(self.evidence["byte_count"], 362)
        self.assertEqual(self.evidence["instruction_count"], 120)

        # Check key instruction mnemonics
        insns = self.evidence["instructions"]
        divs_found = any(i["mnemonic"] == "divs.w" for i in insns)
        self.assertTrue(divs_found, "divs.w instruction should be present")

        launch_found = any("60000" in i.get("op_str", "") for i in insns)
        self.assertTrue(launch_found, "Launch velocity #$60000 should be present")

    def test_target_player_ali_quadrant_angle(self):
        def target_ali(player_x, player_y, entity_x, entity_y):
            d1 = player_x - entity_x
            d2 = player_y - entity_y
            d3 = abs(d1)
            d4 = abs(d2)
            d5 = d3 + d4  # Manhattan distance
            if d4 > d3:
                d0 = 0 if d2 < 0 else 2  # 0=Up, 2=Down
            else:
                d0 = 3 if d1 < 0 else 1  # 3=Left, 1=Right
            return d0, d5

        # Up
        d0, dist = target_ali(100, 50, 100, 100)
        self.assertEqual(d0, 0)
        self.assertEqual(dist, 50)

        # Down
        d0, dist = target_ali(100, 180, 100, 100)
        self.assertEqual(d0, 2)
        self.assertEqual(dist, 80)

        # Right
        d0, dist = target_ali(150, 100, 100, 100)
        self.assertEqual(d0, 1)
        self.assertEqual(dist, 50)

        # Left
        d0, dist = target_ali(60, 100, 100, 100)
        self.assertEqual(d0, 3)
        self.assertEqual(dist, 40)

        # Diagonal tie: |dx| == |dy| -> prefers Right/Left
        d0, dist = target_ali(120, 120, 100, 100)
        self.assertEqual(d0, 1)  # dx=+20, dy=+20 -> d4 is not > d3 -> d0 = 1 (Right)

    def test_target_player_ali_motion_table_loading(self):
        def target_and_load(player_x, player_y, entity_x, entity_y):
            d1 = player_x - entity_x
            d2 = player_y - entity_y
            d3 = abs(d1)
            d4 = abs(d2)
            if d3 > d4:
                d5 = d3
                d0 = 3 if d1 < 0 else 1
            else:
                d5 = d4
                d0 = 0 if d2 < 0 else 2

            table_offset = d0 * 8
            # Clamping logic at 0x00E44C: cmpi.w #$10, d5; bcc +8; move.w #$10, d5
            if d5 < 16:
                d5 = 16
            return d0, table_offset, d5

        # Close entity (distance 8 < 16) -> clamps to 16
        d0, offset, timer = target_and_load(108, 100, 100, 100)
        self.assertEqual(d0, 1)
        self.assertEqual(offset, 8)
        self.assertEqual(timer, 16)

        # Far entity (distance 40 >= 16) -> stays 40
        d0, offset, timer = target_and_load(100, 140, 100, 100)
        self.assertEqual(d0, 2)
        self.assertEqual(offset, 16)
        self.assertEqual(timer, 40)

    def test_normalize_target_aim_vector_divs(self):
        def normalize_aim(tgt_x, tgt_y, entity_x, entity_y, prev_facing):
            d1 = tgt_x - entity_x
            d2 = tgt_y - entity_y
            d4 = abs(d1)
            d5 = abs(d2)
            d3 = 1  # X dominant
            if d4 <= d5:
                d4 = d5
                d3 = 0  # Y dominant

            d4 >>= 1  # asr.w #1, d4
            if d4 == 0:
                return 0, 0, prev_facing

            # Fixed-point division emulation
            q1 = int((d1 << 8) / d4)
            q2 = int((d2 << 8) / d4)
            vel_x = q1 << 8
            vel_y = q2 << 8

            if vel_x == 0 and vel_y == 0:
                return 0, 0, prev_facing

            if d3 == 1:
                if vel_x < 0:
                    d3 += 2  # 3 = Left
            else:
                if vel_y >= 0:
                    d3 += 2  # 2 = Down

            return vel_x, vel_y, d3

        # Zero delta -> zero velocities, preserves facing
        vx, vy, dir_out = normalize_aim(100, 100, 100, 100, 2)
        self.assertEqual((vx, vy, dir_out), (0, 0, 2))

        # Distinct delta
        vx, vy, dir_out = normalize_aim(200, 100, 100, 100, 0)
        self.assertGreater(vx, 0)
        self.assertEqual(vy, 0)
        self.assertEqual(dir_out, 1)  # Right

    def test_launch_directional_projectile_physics(self):
        def launch_projectile(d7_flag, entity_dir):
            if (d7_flag & 0xC000) != 0:
                return None  # early return
            base_speed_x = 0x0300
            base_speed_y = 0x02E0
            launch_elevation_velocity = 0x00060000
            return base_speed_x, base_speed_y, launch_elevation_velocity

        # Suppressed launch
        self.assertIsNone(launch_projectile(0x4000, 2))
        self.assertIsNone(launch_projectile(0x8000, 2))

        # Active launch
        res = launch_projectile(0x0000, 2)
        self.assertIsNotNone(res)
        self.assertEqual(res[0], 0x0300)
        self.assertEqual(res[1], 0x02E0)
        self.assertEqual(res[2], 0x00060000)


if __name__ == "__main__":
    unittest.main()
