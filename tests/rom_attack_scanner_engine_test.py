#!/usr/bin/env python3
"""
rom_attack_scanner_engine_test.py — Unit tests for M15.422:
ROM Universal Attack Frame Scanner & Hitbox Intersection Engine (0x00BC8E..0x00BE98)
"""

import json
import os
import unittest
from tools.extract_rom_attack_scanner_engine import (
    calculate_ali_strength_bonus,
    pack_attack_damage,
    START_ADDR,
    END_ADDR,
    SIZE,
)

class TestRomAttackScannerEngine(unittest.TestCase):
    def setUp(self):
        self.evidence_path = "data/thor-evidence/rom_attack_scanner_engine_m15_422.json"

    def test_evidence_json_validity(self):
        self.assertTrue(os.path.exists(self.evidence_path))
        with open(self.evidence_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.assertEqual(data["start_address"], f"0x{START_ADDR:06X}")
        self.assertEqual(data["end_address"], f"0x{END_ADDR:06X}")
        self.assertEqual(data["size_bytes"], SIZE)
        self.assertIn("scan_all_entities", data["entry_points"])
        self.assertIn("scan_group3_a", data["entry_points"])
        self.assertIn("scan_group3_b", data["entry_points"])
        self.assertIn("scan_single", data["entry_points"])

    def test_ali_strength_bonus(self):
        self.assertEqual(calculate_ali_strength_bonus(0), 0)
        self.assertEqual(calculate_ali_strength_bonus(9), 0)
        self.assertEqual(calculate_ali_strength_bonus(10), 1)
        self.assertEqual(calculate_ali_strength_bonus(19), 1)
        self.assertEqual(calculate_ali_strength_bonus(20), 2)
        self.assertEqual(calculate_ali_strength_bonus(35), 3)
        self.assertEqual(calculate_ali_strength_bonus(49), 4)
        self.assertEqual(calculate_ali_strength_bonus(50), 5)
        self.assertEqual(calculate_ali_strength_bonus(99), 5)
        self.assertEqual(calculate_ali_strength_bonus(100), 7) # +2
        self.assertEqual(calculate_ali_strength_bonus(199), 7)
        self.assertEqual(calculate_ali_strength_bonus(200), 11) # +4
        self.assertEqual(calculate_ali_strength_bonus(255), 11)

    def test_pack_attack_damage(self):
        # Base damage 8, raw element 0x32 (element=0x3, stun=2)
        # stat_96 = 25 (bonus = 2), weapon_bonus = 10, is_ali = True
        # Total dmg = ((8 + 2) | 0x3000) + 10 = 10 | 0x3000 + 10 = 0x3014
        # stun duration = (0x32 & 0xF) + 1 = 3
        # packed_a4 = (0x3014 << 16) | 3
        tot, packed = pack_attack_damage(
            base_dmg=8,
            element_raw=0x32,
            stat_96=25,
            weapon_bonus_90=10,
            is_ali=True,
        )
        self.assertEqual(tot, 0x3014)
        self.assertEqual(packed, (0x3014 << 16) | 3)

        # Standard enemy (is_ali = False): stat bonus ignored
        tot_enemy, packed_enemy = pack_attack_damage(
            base_dmg=12,
            element_raw=0x01,
            stat_96=150,
            weapon_bonus_90=0,
            is_ali=False,
        )
        self.assertEqual(tot_enemy, 12)
        self.assertEqual(packed_enemy, (12 << 16) | 2)

    def test_hitbox_directional_mirroring(self):
        # Facing: 0=Down, 1=Left/Raw, 2=Up, 3=Right/Flipped
        def transform_facing(facing: int, raw_box: list[int]):
            # raw_box = [z_min, z_max, x_min, y_min, x_max, y_max]
            z_min, z_max, x_min, y_min, x_max, y_max = raw_box
            d5_flipped = (facing == 3)
            if d5_flipped:
                x_min, x_max = -x_max, -x_min
            return [z_min, z_max, x_min, y_min, x_max, y_max]

        box = [-10, 20, 10, -5, 30, 5]
        # Facing 1 (unflipped)
        self.assertEqual(transform_facing(1, box), box)
        # Facing 3 (flipped)
        res3 = transform_facing(3, box)
        self.assertEqual(res3, [-10, 20, -30, -5, -10, 5])

    def test_3d_intersection_check(self):
        def check_overlap(box, target):
            # Target span:
            # Z: [t_z + t_elev, t_z + t_elev + t_height]
            tz0 = target["z"] + target["elev"]
            tz1 = tz0 + target["height"]
            if box["z_max"] < tz0 or box["z_min"] > tz1:
                return False
            # X: [t_x - t_rad_x, t_x + t_rad_x]
            tx0 = target["x"] - target["rad_x"]
            tx1 = target["x"] + target["rad_x"]
            if box["x_min"] > tx1 or box["x_max"] < tx0:
                return False
            # Y: [t_y + t_elev - t_rad_y, t_y + t_elev + t_rad_y]
            ty0 = target["y"] + target["elev"] - target["rad_y"]
            ty1 = target["y"] + target["elev"] + target["rad_y"]
            if box["y_min"] > ty1 or box["y_max"] < ty0:
                return False
            return True

        target = {"x": 100, "y": 200, "z": 0, "elev": 5, "rad_x": 15, "rad_y": 10, "height": 30}
        # Hitbox that overlaps
        hit_box = {"x_min": 90, "x_max": 120, "y_min": 195, "y_max": 215, "z_min": 0, "z_max": 20}
        self.assertTrue(check_overlap(hit_box, target))

        # Hitbox missing on Z
        miss_z = {"x_min": 90, "x_max": 120, "y_min": 195, "y_max": 215, "z_min": 40, "z_max": 60}
        self.assertFalse(check_overlap(miss_z, target))

    def test_target_filter_logic(self):
        def is_valid_target(slot_type, attacker_id, target_id, intangibility_flags, cooldown):
            if slot_type <= 0:
                return False
            if target_id == attacker_id:
                return False
            if (intangibility_flags & 0x08) != 0:
                return False
            if cooldown != 0:
                return False
            return True

        self.assertTrue(is_valid_target(1, 0xFF19E8, 0xFF1AB4, 0x00, 0))
        # Inactive slot
        self.assertFalse(is_valid_target(0, 0xFF19E8, 0xFF1AB4, 0x00, 0))
        # Self
        self.assertFalse(is_valid_target(1, 0xFF19E8, 0xFF19E8, 0x00, 0))
        # Intangible (bit 3 of 0x38 set)
        self.assertFalse(is_valid_target(1, 0xFF19E8, 0xFF1AB4, 0x08, 0))
        # Cooldown active
        self.assertFalse(is_valid_target(1, 0xFF19E8, 0xFF1AB4, 0x00, 5))

    def test_post_loop_durability_trigger(self):
        def should_trigger_durability(attacker_addr, hit_flag, attack_type):
            if attacker_addr != 0x00FF19E8:
                return False
            if not hit_flag:
                return False
            return attack_type in (6, 10)

        self.assertTrue(should_trigger_durability(0x00FF19E8, True, 6))
        self.assertTrue(should_trigger_durability(0x00FF19E8, True, 10))
        self.assertFalse(should_trigger_durability(0x00FF19E8, True, 4))
        self.assertFalse(should_trigger_durability(0x00FF19E8, False, 6))
        self.assertFalse(should_trigger_durability(0x00FF1CD8, True, 6))

if __name__ == "__main__":
    unittest.main()
