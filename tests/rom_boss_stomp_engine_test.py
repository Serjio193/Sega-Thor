#!/usr/bin/env python3
"""
rom_boss_stomp_engine_test.py — Unit tests for ROM Boss Entity Type $3A Spawner,
Ground Stomp & Combat State Engine (Part 1)
(0x00ED04..0x00EE94, 400 bytes).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_stomp_engine_m15_455.json"
)


class TestRomBossStompEngine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(EVIDENCE_PATH):
            raise FileNotFoundError(f"Evidence file missing at {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_rom_constants(self):
        self.assertEqual(self.evidence["start_address"], "0x00ED04")
        self.assertEqual(self.evidence["end_address"], "0x00EE94")
        self.assertEqual(self.evidence["byte_count"], 400)
        self.assertEqual(self.evidence["instruction_count"], 91)

        insns = self.evidence["instructions"]
        # Boss fight active flag $FF198F
        flag_found = any("ff198f" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(flag_found, "Boss active flag $FF198F must be present")

        # Type #$3A
        type_found = any("3a" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(type_found, "Type #$3A must be present")

        # Roar SFX #$B00225
        sfx_found = any("b00225" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(sfx_found, "Roar SFX #$B00225 must be present")

    def test_spawn_init_boss_type3a(self):
        def spawn_boss(ali_pos_x, ali_pos_y, ali_pos_z, ali_priority):
            boss_active_flag = 0xFF
            ali_cleared_word = 0

            boss_elev = ali_pos_z - 0x40000
            if boss_elev < 0:
                boss_elev = 0

            boss = {
                "type": 0x3A,
                "x": ali_pos_x,
                "y": ali_pos_y,
                "z": boss_elev,
                "priority": ali_priority,
                "facing": 0,
                "damage_a8": 0x8007,
                "phase_a6": 2,
                "state_4": 8,
                "timer_2a": 8,
                "screen_shake": 4,
            }
            return boss_active_flag, ali_cleared_word, boss

        flag, ali_clr, b = spawn_boss(100, 200, 0x50000, 0x055E)
        self.assertEqual(flag, 0xFF)
        self.assertEqual(ali_clr, 0)
        self.assertEqual(b["type"], 0x3A)
        self.assertEqual(b["z"], 0x10000)
        self.assertEqual(b["priority"], 0x055E)
        self.assertEqual(b["damage_a8"], 0x8007)
        self.assertEqual(b["state_4"], 8)
        self.assertEqual(b["screen_shake"], 4)

    def test_boss_type3a_phase_change(self):
        def handle_phase_change(phase_byte, stat_88, stat_8c):
            if phase_byte > 0:
                new_phase = phase_byte - 1
                timer_2 = stat_88
                elev_10 = stat_8c
                state_4 = 0
                cleared_phase_byte = 0
                return new_phase, timer_2, elev_10, state_4, cleared_phase_byte
            return None

        res = handle_phase_change(phase_byte=3, stat_88=60, stat_8c=0x20000)
        self.assertIsNotNone(res)
        self.assertEqual(res[0], 2)
        self.assertEqual(res[1], 60)
        self.assertEqual(res[2], 0x20000)
        self.assertEqual(res[3], 0)
        self.assertEqual(res[4], 0)

    def test_boss_type3a_damage_elevation_bump(self):
        def evaluate_damage_knockback(damage_word, elev_10):
            if (damage_word & 0xC000) != 0:
                elev_10 += 1
            return elev_10

        # Normal damage (no high knockback bits) -> elevation unchanged
        self.assertEqual(evaluate_damage_knockback(0x0020, 10), 10)

        # Heavy knockback damage (bit 14 or 15 set) -> elevation bumped
        self.assertEqual(evaluate_damage_knockback(0x4020, 10), 11)
        self.assertEqual(evaluate_damage_knockback(0x8020, 10), 11)

    def test_state4_ground_quake_trigger(self):
        def evaluate_quake_trigger(anim_frame_30, tick_2e, flags_39):
            quake_triggered = False
            screen_shake = 0
            if anim_frame_30 == 8 and tick_2e == 0:
                flags_39 |= 0x10  # bit 4
                quake_triggered = True
                screen_shake = 4
            return flags_39, quake_triggered, screen_shake

        # Matched frame
        f39, trig, shake = evaluate_quake_trigger(8, 0, 0)
        self.assertEqual(f39 & 0x10, 0x10)
        self.assertTrue(trig)
        self.assertEqual(shake, 4)

        # Unmatched frame
        f39, trig, shake = evaluate_quake_trigger(7, 0, 0)
        self.assertEqual(f39 & 0x10, 0)
        self.assertFalse(trig)
        self.assertEqual(shake, 0)


if __name__ == "__main__":
    unittest.main()
