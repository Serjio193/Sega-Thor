#!/usr/bin/env python3
"""
rom_ali_water_dive_shoreline_test.py — M15.380: Unit tests for ROM Ali Water Dive, Air Combat & Shoreline
Tests:
  1. test_water_paddle_coast
  2. test_water_dive_gravity_arc
  3. test_air_attack_dispatch
  4. test_water_splash_landing
  5. test_shoreline_exit_triggers
"""

import json
import unittest

GRAVITY_ACCEL = 0x0000C000


def simulate_dive_gravity_step(ali_state):
    # 0x006DD2
    vz = ali_state["vz"]
    if vz > 0:
        vz -= GRAVITY_ACCEL
        if vz <= 0:
            vz -= 1
            ali_state["timer30"] = 4
            ali_state["field2e"] = 0
    else:
        vz -= GRAVITY_ACCEL

    ali_state["vz"] = vz
    ali_state["height"] += vz
    if ali_state["height"] <= 0:
        ali_state["landed"] = True
        ali_state["timer30"] = 0x0C
        ali_state["lock_ff17bb"] = False
        return True
    return False


def simulate_in_air_attack(ali_state, btn_a_pressed):
    # 0x006E7A
    if not btn_a_pressed:
        return False

    ali_state["field_ff16ea"] = 0
    weapon = ali_state.get("weapon_id", 0)
    if weapon == 0x0C:  # Dagger
        if ali_state.get("state") == 0x16:
            ali_state["anim"] = 0x62
            ali_state["sfx"] = 0x800222
        else:
            ali_state["anim"] = 0x50
            ali_state["sfx"] = 0x800223
        ali_state["state"] = 0x48
    else:
        ali_state["anim"] = 0x32
        ali_state["state"] = 0x26
        ali_state["sfx"] = 0x700209
    return True


def simulate_splash_landing(ali_state):
    # 0x006F8C
    ali_state["duration4a"] = 0
    ali_state["height"] = 0
    ali_state["vz"] = 0
    ali_state["sfx"] = 0x600343
    ali_state["state"] = 0x00  # returns to idle when anim done


def simulate_shoreline_exit(ali_state, exit_type):
    # 0x006FE4..0x007074
    if exit_type == "primary":
        ali_state["anim"] = 0x72
        ali_state["state"] = 0x64
        ali_state["vz"] = 0x40000
        ali_state["sfx"] = 0x500341
    elif exit_type == "high":
        ali_state["anim"] = 0x74
        ali_state["state"] = 0x66
        ali_state["vz"] = 0x50000
        ali_state["sfx"] = 0x500341
    elif exit_type == "shallow":
        ali_state["anim"] = 0x76
        ali_state["state"] = 0x68
        ali_state["vz"] = 0
        ali_state["sfx"] = None


class TestRomAliWaterDiveShoreline(unittest.TestCase):
    def test_water_paddle_coast(self):
        # 0x006D7A: anim finished (bit 1 of flag37)
        def check_paddle(flag37):
            anim = 0
            state = 0
            if flag37 & 2:
                anim = 0x24
                state = 0x10
            return anim, state

        anim, state = check_paddle(0)
        self.assertEqual(state, 0)
        self.assertEqual(anim, 0)

        anim, state = check_paddle(2)
        self.assertEqual(state, 0x10)
        self.assertEqual(anim, 0x24)

    def test_water_dive_gravity_arc(self):
        state = {
            "vz": 0x24000,
            "height": 0x30000,
            "landed": False
        }
        # Step 1: vz = 0x24000 - 0xC000 = 0x18000; height = 0x30000 + 0x18000 = 0x48000
        landed = simulate_dive_gravity_step(state)
        self.assertFalse(landed)
        self.assertEqual(state["vz"], 0x18000)
        self.assertEqual(state["height"], 0x48000)

        # Step 2: vz = 0x18000 - 0xC000 = 0xC000; height = 0x54000
        simulate_dive_gravity_step(state)
        # Step 3: vz = 0xC000 - 0xC000 = 0 -> transitions negative (-1)
        simulate_dive_gravity_step(state)
        self.assertEqual(state["vz"], -1)

        # Force descending to ground impact
        state["vz"] = -0x50000
        landed = simulate_dive_gravity_step(state)
        self.assertTrue(landed)
        self.assertTrue(state["landed"])
        self.assertEqual(state["timer30"], 0x0C)
        self.assertFalse(state["lock_ff17bb"])

    def test_air_attack_dispatch(self):
        # Dagger normal air attack
        dagger_state = {"weapon_id": 0x0C, "state": 0x0A}
        dispatched = simulate_in_air_attack(dagger_state, btn_a_pressed=True)
        self.assertTrue(dispatched)
        self.assertEqual(dagger_state["anim"], 0x50)
        self.assertEqual(dagger_state["state"], 0x48)
        self.assertEqual(dagger_state["sfx"], 0x800223)

        # Dagger dive air attack (state 0x16)
        dagger_dive = {"weapon_id": 0x0C, "state": 0x16}
        dispatched = simulate_in_air_attack(dagger_dive, btn_a_pressed=True)
        self.assertTrue(dispatched)
        self.assertEqual(dagger_dive["anim"], 0x62)
        self.assertEqual(dagger_dive["state"], 0x48)
        self.assertEqual(dagger_dive["sfx"], 0x800222)

        # Sword air slash
        sword_state = {"weapon_id": 0x0E, "state": 0x0A}
        dispatched = simulate_in_air_attack(sword_state, btn_a_pressed=True)
        self.assertTrue(dispatched)
        self.assertEqual(sword_state["anim"], 0x32)
        self.assertEqual(sword_state["state"], 0x26)
        self.assertEqual(sword_state["sfx"], 0x700209)

    def test_water_splash_landing(self):
        state = {"height": 0x1000, "vz": -0x8000, "duration4a": 20}
        simulate_splash_landing(state)
        self.assertEqual(state["height"], 0)
        self.assertEqual(state["vz"], 0)
        self.assertEqual(state["duration4a"], 0)
        self.assertEqual(state["sfx"], 0x600343)

    def test_shoreline_exit_triggers(self):
        # Primary exit: 0x40000 impulse, anim 0x72, state 0x64
        s1 = {}
        simulate_shoreline_exit(s1, "primary")
        self.assertEqual(s1["anim"], 0x72)
        self.assertEqual(s1["state"], 0x64)
        self.assertEqual(s1["vz"], 0x40000)
        self.assertEqual(s1["sfx"], 0x500341)

        # High exit: 0x50000 impulse, anim 0x74, state 0x66
        s2 = {}
        simulate_shoreline_exit(s2, "high")
        self.assertEqual(s2["anim"], 0x74)
        self.assertEqual(s2["state"], 0x66)
        self.assertEqual(s2["vz"], 0x50000)
        self.assertEqual(s2["sfx"], 0x500341)

        # Shallow exit: anim 0x76, state 0x68
        s3 = {}
        simulate_shoreline_exit(s3, "shallow")
        self.assertEqual(s3["anim"], 0x76)
        self.assertEqual(s3["state"], 0x68)
        self.assertEqual(s3["vz"], 0)
        self.assertIsNone(s3["sfx"])


if __name__ == "__main__":
    unittest.main()
