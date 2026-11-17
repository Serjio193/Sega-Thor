#!/usr/bin/env python3
"""
rom_combat_hit_response_test.py — Unit tests for M15.423:
ROM Entity Combat Hit Response, Knockback & Trajectory Physics Calculator (0x00BE98..0x00C012)
"""

import json
import os
import unittest
from tools.extract_rom_combat_hit_response import (
    is_shield_facing,
    resolve_knockback_mode,
    START_ADDR,
    END_ADDR,
    SIZE,
    KNOCKBACK_TRAJECTORIES,
)

class TestRomCombatHitResponse(unittest.TestCase):
    def setUp(self):
        self.evidence_path = "data/thor-evidence/rom_combat_hit_response_m15_423.json"

    def test_evidence_json_validity(self):
        self.assertTrue(os.path.exists(self.evidence_path))
        with open(self.evidence_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.assertEqual(data["start_address"], f"0x{START_ADDR:06X}")
        self.assertEqual(data["end_address"], f"0x{END_ADDR:06X}")
        self.assertEqual(data["size_bytes"], SIZE)
        self.assertEqual(data["shield_block_sfx"], "0x00900202")

    def test_shield_facing_logic(self):
        # 0=Down, 1=Left, 2=Up, 3=Right
        # Attack facing Down (0) -> Target facing Up (2) blocks
        self.assertTrue(is_shield_facing(attacker_facing=0, target_facing=2))
        # Attack facing Up (2) -> Target facing Down (0) blocks
        self.assertTrue(is_shield_facing(attacker_facing=2, target_facing=0))
        # Attack facing Left (1) -> Target facing Right (3) blocks
        self.assertTrue(is_shield_facing(attacker_facing=1, target_facing=3))
        # Attack facing Right (3) -> Target facing Left (1) blocks
        self.assertTrue(is_shield_facing(attacker_facing=3, target_facing=1))

        # Flank / rear attacks fail to block
        self.assertFalse(is_shield_facing(attacker_facing=0, target_facing=0))
        self.assertFalse(is_shield_facing(attacker_facing=1, target_facing=1))
        self.assertFalse(is_shield_facing(attacker_facing=2, target_facing=1))
        self.assertFalse(is_shield_facing(attacker_facing=3, target_facing=2))

    def test_knockback_mode_resolution(self):
        # packed_a4: (damage_word << 16) | stun_duration
        # mode offset in table is (rol.w #3, damage_word) & 6
        # Bits 14..15 of damage_word:
        # 0x0000 -> offset 0 (mode 0)
        # 0x4000 -> offset 2 (mode 1)
        # 0x8000 -> offset 4 (mode 2)
        # 0xC000 -> offset 6 (mode 3)
        self.assertEqual(resolve_knockback_mode(0x00140003), 0)
        self.assertEqual(resolve_knockback_mode(0x40140003), 1)
        self.assertEqual(resolve_knockback_mode(0x80140003), 2)
        self.assertEqual(resolve_knockback_mode(0xC0140003), 3)

    def test_knockback_parameters(self):
        # Mode 0 Grounded
        traj0_g = KNOCKBACK_TRAJECTORIES[0]["grounded"]
        self.assertEqual(traj0_g["h_speed_x"], 0x0300)
        self.assertEqual(traj0_g["h_speed_y"], 0x02C0)
        self.assertEqual(traj0_g["v_impulse"], 0)

        # Mode 0 Airborne
        traj0_a = KNOCKBACK_TRAJECTORIES[0]["airborne"]
        self.assertEqual(traj0_a["h_speed_x"], 0x0400)
        self.assertEqual(traj0_a["h_speed_y"], 0x03C0)
        self.assertEqual(traj0_a["v_impulse"], 0x00028000)
        self.assertTrue(traj0_a["set_air_flag"])

        # Mode 2 Heavy
        traj2 = KNOCKBACK_TRAJECTORIES[2]["speed"]
        self.assertEqual(traj2["h_speed_x"], 0x0400)
        self.assertEqual(traj2["h_speed_y"], 0x03C0)
        self.assertEqual(traj2["v_impulse"], 0x00028000)

        # Mode 3 Severe
        traj3 = KNOCKBACK_TRAJECTORIES[3]["speed"]
        self.assertEqual(traj3["h_speed_x"], 0x0700)
        self.assertEqual(traj3["h_speed_y"], 0x0680)
        self.assertEqual(traj3["v_impulse"], 0x00030000)

    def test_hud_tracking_trigger(self):
        def should_track_hud(target_addr, status_3a):
            if target_addr == 0x00FF19E8:
                return False
            if (status_3a & 0x01) != 0:
                return False
            return True

        self.assertTrue(should_track_hud(0x00FF1AB4, 0x00))
        self.assertFalse(should_track_hud(0x00FF19E8, 0x00))  # Target is Ali
        self.assertFalse(should_track_hud(0x00FF1AB4, 0x01))  # Status 3A bit 0 set

if __name__ == "__main__":
    unittest.main()
