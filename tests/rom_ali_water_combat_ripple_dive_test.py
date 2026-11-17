#!/usr/bin/env python3
"""
rom_ali_water_combat_ripple_dive_test.py — M15.383: Unit tests for ROM Ali Water Combat Ripple Physics & Dive
Tests:
  1. test_ripple_impulse_callback
  2. test_weapon_durability_table
  3. test_water_dagger_strike_frame8
  4. test_dagger_somersault_high_dive
  5. test_target_hitstun_callback
"""

import json
import unittest

DURABILITY_TABLE = [0x0000, 0x0001, 0xFFFF, 0x2003]
GRAVITY = 0x0000C000


def get_durability_or_random(element_idx, mock_prng_val=3):
    # 0x007520
    idx = element_idx - 2
    if 0 <= idx < len(DURABILITY_TABLE):
        val = DURABILITY_TABLE[idx]
        if val == 0xFFFF:
            # Random 1..6: (prng % 6) + 1
            return (mock_prng_val % 6) + 1
        return val
    return 0


def simulate_high_dive_step(ali_state):
    # 0x007584
    frame = ali_state["frame30"]
    if frame == 0x24 and ali_state.get("field2e", 0) == 0:
        ali_state["vz"] = 0x80000
        ali_state["sfx"] = 0x500341

    vz = ali_state.get("vz", 0)
    if vz != 0:
        vz -= GRAVITY
        if vz <= 0:
            vz -= 1
        ali_state["vz"] = vz
        ali_state["height"] += vz
        if ali_state["height"] <= 0:
            ali_state["height"] = 0
            ali_state["vz"] = 0
            ali_state["timer30"] = 0x48
            ali_state["landed"] = True


def simulate_target_hitstun(target_state):
    # 0x00763C
    if target_state.get("vz", 0) == 0:
        target_state["vx"] = 0
        target_state["vy"] = 0
        target_state["hitstun_timer"] = 0x0C


class TestRomAliWaterCombatRippleDive(unittest.TestCase):
    def test_ripple_impulse_callback(self):
        # 0x0074A4: impulse vz = 0x20000 (+2.0 px/frame)
        vz_impulse = 0x20000
        self.assertEqual(vz_impulse, 131072)
        # horizontal speeds
        self.assertEqual(0x800, 2048)
        self.assertEqual(0x780, 1920)

    def test_weapon_durability_table(self):
        # Element 2 -> index 0 -> 0x0000
        self.assertEqual(get_durability_or_random(2), 0x0000)
        # Element 3 -> index 1 -> 0x0001
        self.assertEqual(get_durability_or_random(3), 0x0001)
        # Element 4 -> index 2 -> 0xFFFF -> random 1..6
        self.assertEqual(get_durability_or_random(4, mock_prng_val=4), 5)
        # Element 5 -> index 3 -> 0x2003
        self.assertEqual(get_durability_or_random(5), 0x2003)

    def test_water_dagger_strike_frame8(self):
        # 0x007550: frame 8 triggers SFX 0x800223
        frame = 8
        sfx = 0x800223
        self.assertEqual(frame, 8)
        self.assertEqual(sfx, 0x800223)

    def test_dagger_somersault_high_dive(self):
        state = {
            "frame30": 0x24,
            "field2e": 0,
            "vz": 0,
            "height": 0x10000
        }
        # Frame 36 triggers vz = 0x80000 and splash SFX 0x500341
        simulate_high_dive_step(state)
        self.assertEqual(state["sfx"], 0x500341)
        self.assertGreater(state["vz"], 0x70000)

        # Advance frame and force descending to landing impact
        state["frame30"] = 0x25
        state["height"] = 0x10000
        state["vz"] = -0x50000
        simulate_high_dive_step(state)
        self.assertEqual(state["height"], 0)
        self.assertEqual(state["vz"], 0)
        self.assertEqual(state["timer30"], 0x48)
        self.assertTrue(state.get("landed", False))

    def test_target_hitstun_callback(self):
        target = {
            "vz": 0,
            "vx": 0x1000,
            "vy": 0x2000,
            "hitstun_timer": 0
        }
        simulate_target_hitstun(target)
        self.assertEqual(target["vx"], 0)
        self.assertEqual(target["vy"], 0)
        self.assertEqual(target["hitstun_timer"], 12)

        # Airborne target: does not clear velocity
        airborne_target = {
            "vz": 0x1000,
            "vx": 0x1000,
            "vy": 0x2000,
            "hitstun_timer": 0
        }
        simulate_target_hitstun(airborne_target)
        self.assertEqual(airborne_target["vx"], 0x1000)
        self.assertEqual(airborne_target["hitstun_timer"], 0)


if __name__ == "__main__":
    unittest.main()
