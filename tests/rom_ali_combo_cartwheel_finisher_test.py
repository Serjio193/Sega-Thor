#!/usr/bin/env python3
"""
rom_ali_combo_cartwheel_finisher_test.py — M15.385: Unit tests for ROM Ali Combat Combo Chains, 6-Cartwheel Super Finisher & Stun Callback
Tests:
  1. test_rapid_spin_facing_rotation
  2. test_combo_chain_transitions_and_motion_precedence
  3. test_cartwheel_chain_counter_and_anim_alternation
  4. test_6th_cartwheel_super_finisher
  5. test_landing_ground_stop_callback
"""

import json
import unittest


def calculate_rapid_spin_facing(frame_counter):
    # 0x00780E..0x00782C
    d1 = 3
    if frame_counter == 4:
        return 3
    d1 -= 1
    if frame_counter == 8:
        return 2
    d1 -= 1
    if frame_counter == 12:
        return 1
    d1 -= 1
    return 0


def evaluate_combo_chain(attack_pressed, buffer_attack_bit, d_pad_dir, special_motion=None):
    # 0x007852..0x0078F6
    if not attack_pressed:
        if not buffer_attack_bit:
            return {"transition": "idle_walk"}
        # Fall through to second hit
        return {
            "transition": "combo_hit_2",
            "facing": d_pad_dir,
            "anim": 0x3A,
            "state": 0x0A,
        }

    # Attack pressed
    if special_motion is not None:
        return {"transition": "special_motion", "motion": special_motion}

    if buffer_attack_bit:
        return {
            "transition": "combo_hit_2",
            "facing": d_pad_dir,
            "anim": 0x3A,
            "state": 0x0A,
        }
    else:
        return {
            "transition": "combo_hit_1",
            "facing": d_pad_dir,
            "anim": 0x0A,
            "state": 0x08,
            "sfx": 0x7004D3,
            "clear_ff16ea": True,
        }


def step_cartwheel_chain(chain_counter, current_anim, facing_dir, cartwheel_motion_detected):
    # 0x00792A..0x0079FA
    if not cartwheel_motion_detected:
        return {"transition": "idle_walk", "counter": chain_counter}

    new_counter = chain_counter + 1
    if new_counter == 6:
        # Super Finisher! 0x0079AC
        # 8-way directional velocity table 0xE2D4 with base 0x800
        # facing: 0: Up (0, -0x800), 2: Right (0x800, 0), 4: Down (0, 0x800), 6: Left (-0x800, 0)
        table_8way = {
            0: (0, -0x800),
            1: (0x5A8, -0x5A8),
            2: (0x800, 0),
            3: (0x5A8, 0x5A8),
            4: (0, 0x800),
            5: (-0x5A8, 0x5A8),
            6: (-0x800, 0),
            7: (-0x5A8, -0x5A8),
        }
        vx, vy = table_8way.get(facing_dir % 8, (0, 0))
        return {
            "transition": "super_finisher",
            "counter": new_counter,
            "vz": 0x40000,
            "elevation": 15,
            "vx": vx,
            "vy": vy,
            "anim": 0x32,
            "state": 0x26,
            "sfx": 0x80025B,
        }
    else:
        # Alternate anim between 0x7C and 0x7E
        next_anim = 0x7C if current_anim == 0x7A else 0x7E
        return {
            "transition": "cartwheel_continue",
            "counter": new_counter,
            "anim": next_anim,
            "state": 0x72,
            "sfx": 0x80025B,
        }


def ground_stop_callback(vz, vx, vy):
    # 0x0079FC..0x007A0E
    if vz == 0:
        return 0, 0
    return vx, vy


class TestRomAliComboCartwheelFinisher(unittest.TestCase):
    def test_rapid_spin_facing_rotation(self):
        self.assertEqual(calculate_rapid_spin_facing(4), 3)
        self.assertEqual(calculate_rapid_spin_facing(8), 2)
        self.assertEqual(calculate_rapid_spin_facing(12), 1)
        self.assertEqual(calculate_rapid_spin_facing(16), 0)
        self.assertEqual(calculate_rapid_spin_facing(20), 0)

    def test_combo_chain_transitions_and_motion_precedence(self):
        # 1. No attack pressed, no buffer -> idle walk
        res1 = evaluate_combo_chain(False, False, 2)
        self.assertEqual(res1["transition"], "idle_walk")

        # 2. No attack pressed, but buffer bit set -> combo hit 2
        res2 = evaluate_combo_chain(False, True, 2)
        self.assertEqual(res2["transition"], "combo_hit_2")
        self.assertEqual(res2["anim"], 0x3A)
        self.assertEqual(res2["state"], 0x0A)

        # 3. Attack pressed, special motion triggers -> motion precedence
        res3 = evaluate_combo_chain(True, False, 2, special_motion="slide")
        self.assertEqual(res3["transition"], "special_motion")
        self.assertEqual(res3["motion"], "slide")

        # 4. Attack pressed, no special motion, no buffer -> combo hit 1
        res4 = evaluate_combo_chain(True, False, 3)
        self.assertEqual(res4["transition"], "combo_hit_1")
        self.assertEqual(res4["anim"], 0x0A)
        self.assertEqual(res4["state"], 0x08)
        self.assertEqual(res4["sfx"], 0x7004D3)
        self.assertTrue(res4["clear_ff16ea"])

        # 5. Attack pressed, no motion, buffer set -> combo hit 2
        res5 = evaluate_combo_chain(True, True, 3)
        self.assertEqual(res5["transition"], "combo_hit_2")
        self.assertEqual(res5["anim"], 0x3A)
        self.assertEqual(res5["state"], 0x0A)

    def test_cartwheel_chain_counter_and_anim_alternation(self):
        # Cartwheel 1 (starts at 0x7A, becomes 0x7C)
        res1 = step_cartwheel_chain(0, 0x7A, 2, True)
        self.assertEqual(res1["counter"], 1)
        self.assertEqual(res1["anim"], 0x7C)
        self.assertEqual(res1["state"], 0x72)
        self.assertEqual(res1["sfx"], 0x80025B)

        # Cartwheel 2 (starts at 0x7C, becomes 0x7E)
        res2 = step_cartwheel_chain(1, 0x7C, 2, True)
        self.assertEqual(res2["counter"], 2)
        self.assertEqual(res2["anim"], 0x7E)
        self.assertEqual(res2["state"], 0x72)

        # Cartwheel chain broken
        res_break = step_cartwheel_chain(2, 0x7E, 2, False)
        self.assertEqual(res_break["transition"], "idle_walk")

    def test_6th_cartwheel_super_finisher(self):
        # On 5th -> 6th cartwheel: Super Finisher triggers!
        res6 = step_cartwheel_chain(5, 0x7E, 2, True)  # facing 2 (Right)
        self.assertEqual(res6["transition"], "super_finisher")
        self.assertEqual(res6["counter"], 6)
        self.assertEqual(res6["vz"], 0x40000)
        self.assertEqual(res6["elevation"], 15)
        self.assertEqual(res6["vx"], 0x800)
        self.assertEqual(res6["vy"], 0)
        self.assertEqual(res6["anim"], 0x32)
        self.assertEqual(res6["state"], 0x26)
        self.assertEqual(res6["sfx"], 0x80025B)

    def test_landing_ground_stop_callback(self):
        # In air (vz > 0): speed maintained
        vx, vy = ground_stop_callback(0x20000, 0x800, 0x400)
        self.assertEqual((vx, vy), (0x800, 0x400))

        # Grounded (vz == 0): speed stopped
        vx, vy = ground_stop_callback(0, 0x800, 0x400)
        self.assertEqual((vx, vy), (0, 0))


if __name__ == "__main__":
    unittest.main()
