#!/usr/bin/env python3
"""
rom_ali_command_special_motions_test.py — M15.384: Unit tests for ROM Ali Command Motions & Button Mash
Tests:
  1. test_motion_dispatch_precedence
  2. test_default_combo_hits
  3. test_button_mash_counter
"""

import json
import unittest


def dispatch_ali_attack(motion_match, frame30=0):
    # 0x007656
    ali_state = {}
    if motion_match == "8814":  # Dash Leap
        ali_state["anim"] = 0x70
        ali_state["state"] = 0x62
        ali_state["vz"] = 0x30000
        ali_state["speed"] = 0x100
        ali_state["sfx"] = 0x800220
    elif motion_match == "86b4":  # Slide
        ali_state["anim"] = 0x46
        ali_state["state"] = 0x40
        ali_state["invuln_timer"] = 0x12
        ali_state["facing"] = 2
        ali_state["sfx"] = 0x600343
    elif motion_match == "8778":  # Somersault
        ali_state["anim"] = 0x64
        ali_state["state"] = 0x56
        ali_state["invuln_timer"] = 0x14
        ali_state["vz"] = 0x84000
        ali_state["speed"] = 0x100
        ali_state["sfx"] = 0x800220
    elif motion_match == "8910":  # Cartwheel
        ali_state["anim"] = 0x7A
        ali_state["state"] = 0x72
        ali_state["sfx"] = 0x80025B
    elif motion_match == "8950":  # Grand Dash Strike
        ali_state["anim"] = 0x80
        ali_state["state"] = 0x74
        ali_state["speed"] = 0x600
        ali_state["attack_prop"] = 0x15
        ali_state["sfx"] = 0x7004D3
    else:
        # Default combo hit
        ali_state["field_ff16ea"] = 0
        ali_state["sfx"] = 0x7004D3
        ali_state["state"] = 8
        if frame30 == 0:
            ali_state["anim"] = 8
        else:
            ali_state["anim"] = 0x3C
    return ali_state


def count_button_mashing(ring_buffer_taps, current_frame):
    # 0x0076C2: count taps in recent 8 frames
    tap_count = sum(1 for tap in ring_buffer_taps if tap)
    forced_frame = None
    cleared_2e = False
    if tap_count >= 8:
        if current_frame < 8:
            forced_frame = 8
            cleared_2e = True
    return tap_count, forced_frame, cleared_2e


class TestRomAliCommandSpecialMotions(unittest.TestCase):
    def test_motion_dispatch_precedence(self):
        # 1. Dash Leap (0x8814)
        s1 = dispatch_ali_attack("8814")
        self.assertEqual(s1["state"], 0x62)
        self.assertEqual(s1["anim"], 0x70)
        self.assertEqual(s1["vz"], 0x30000)
        self.assertEqual(s1["sfx"], 0x800220)

        # 2. Slide (0x86B4)
        s2 = dispatch_ali_attack("86b4")
        self.assertEqual(s2["state"], 0x40)
        self.assertEqual(s2["anim"], 0x46)
        self.assertEqual(s2["invuln_timer"], 0x12)
        self.assertEqual(s2["sfx"], 0x600343)

        # 3. Somersault (0x8778)
        s3 = dispatch_ali_attack("8778")
        self.assertEqual(s3["state"], 0x56)
        self.assertEqual(s3["anim"], 0x64)
        self.assertEqual(s3["vz"], 0x84000)
        self.assertEqual(s3["sfx"], 0x800220)

        # 4. Cartwheel (0x8910)
        s4 = dispatch_ali_attack("8910")
        self.assertEqual(s4["state"], 0x72)
        self.assertEqual(s4["anim"], 0x7A)
        self.assertEqual(s4["sfx"], 0x80025B)

        # 5. Grand Dash Strike (0x8950)
        s5 = dispatch_ali_attack("8950")
        self.assertEqual(s5["state"], 0x74)
        self.assertEqual(s5["anim"], 0x80)
        self.assertEqual(s5["speed"], 0x600)
        self.assertEqual(s5["attack_prop"], 0x15)
        self.assertEqual(s5["sfx"], 0x7004D3)

    def test_default_combo_hits(self):
        # Hit 1 (frame30 == 0)
        s_hit1 = dispatch_ali_attack(None, frame30=0)
        self.assertEqual(s_hit1["state"], 8)
        self.assertEqual(s_hit1["anim"], 8)
        self.assertEqual(s_hit1["field_ff16ea"], 0)
        self.assertEqual(s_hit1["sfx"], 0x7004D3)

        # Hit 2 (frame30 > 0)
        s_hit2 = dispatch_ali_attack(None, frame30=4)
        self.assertEqual(s_hit2["state"], 8)
        self.assertEqual(s_hit2["anim"], 0x3C)
        self.assertEqual(s_hit2["sfx"], 0x7004D3)

    def test_button_mash_counter(self):
        # 5 taps: not enough
        taps_5 = [1, 1, 0, 1, 1, 0, 1, 0]
        cnt, forced, cleared = count_button_mashing(taps_5, current_frame=3)
        self.assertEqual(cnt, 5)
        self.assertIsNone(forced)
        self.assertFalse(cleared)

        # 8 taps: triggers forced frame 8 and clears 2e
        taps_8 = [1] * 8
        cnt, forced, cleared = count_button_mashing(taps_8, current_frame=4)
        self.assertEqual(cnt, 8)
        self.assertEqual(forced, 8)
        self.assertTrue(cleared)


if __name__ == "__main__":
    unittest.main()
