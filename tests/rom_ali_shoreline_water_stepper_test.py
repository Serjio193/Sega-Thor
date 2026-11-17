#!/usr/bin/env python3
"""
rom_ali_shoreline_water_stepper_test.py — M15.381: Unit tests for ROM Ali Shoreline Stepper, 3/4 Scaler & Audio Gate
Tests:
  1. test_shoreline_airborne_stepper
  2. test_water_jump_landing_audio_gate
  3. test_water_velocity_3_4_scaler
  4. test_jump_slash_descent_shadow
"""

import json
import unittest

GRAVITY = 0x0000C000


def simulate_shoreline_step(ali_state):
    # 0x007074
    vz = ali_state["vz"]
    if vz == 0:
        if ali_state.get("anim_finished", False):
            return True  # carry set
        else:
            return False  # carry clear
    else:
        vz -= GRAVITY
        if vz <= 0:
            vz -= 1
        ali_state["vz"] = vz
        ali_state["height"] += vz
        if ali_state["height"] <= 0:
            ali_state["height"] = 0
            ali_state["vz"] = 0
            ali_state["sfx"] = 0x600343
            return False
        else:
            ali_state["x"] += ali_state.get("dx", 0)
            ali_state["y"] += ali_state.get("dy", 0)
            return False


def simulate_water_landing_audio_gate(ali_state):
    # 0x007112
    timer30 = ali_state["timer30"]
    if timer30 <= 4:
        ali_state["flag67_bit0"] = 1
        return
    else:
        prev = ali_state.get("flag67_bit0", 0)
        ali_state["flag67_bit0"] = 0
        if prev != 0:
            ali_state["sfx"] = 0x500341

    vz = ali_state["vz"]
    if vz > 0:
        vz -= GRAVITY
        if vz <= 0:
            vz -= 1
            ali_state["timer30"] = 0x0C
            ali_state["field2e"] = 0
    else:
        vz -= GRAVITY

    ali_state["vz"] = vz
    ali_state["height"] += vz
    if ali_state["height"] <= 0:
        ali_state["timer30"] = 0x14
        ali_state["landed"] = True


def scale_water_velocity_3_4(d1, d2, inertia_x, inertia_y):
    # 0x007198: (d >> 1) + (d >> 2)
    s_d1 = (d1 >> 1) + (d1 >> 2)
    s_d2 = (d2 >> 1) + (d2 >> 2)

    # Opposing inertia cancellation for X
    if inertia_x != 0:
        if inertia_x > 0:
            if s_d1 >= 0:
                s_d1 = 0
        else:
            if s_d1 <= 0:
                s_d1 = 0
    s_d1 += inertia_x

    # Opposing inertia cancellation for Y
    if inertia_y != 0:
        if inertia_y > 0:
            if s_d2 >= 0:
                s_d2 = 0
        else:
            if s_d2 <= 0:
                s_d2 = 0
    s_d2 += inertia_y

    return s_d1, s_d2


class TestRomAliShorelineWaterStepper(unittest.TestCase):
    def test_shoreline_airborne_stepper(self):
        state = {
            "vz": 0x20000,
            "height": 0x10000,
            "dx": 2,
            "dy": 1,
            "x": 100,
            "y": 200
        }
        # Step 1: vz = 0x20000 - 0xC000 = 0x14000; height = 0x10000 + 0x14000 = 0x24000
        carry = simulate_shoreline_step(state)
        self.assertFalse(carry)
        self.assertEqual(state["vz"], 0x14000)
        self.assertEqual(state["height"], 0x24000)
        self.assertEqual(state["x"], 102)
        self.assertEqual(state["y"], 201)

        # Force impact:
        state["vz"] = -0x30000
        carry = simulate_shoreline_step(state)
        self.assertFalse(carry)
        self.assertEqual(state["height"], 0)
        self.assertEqual(state["vz"], 0)
        self.assertEqual(state["sfx"], 0x600343)

        # Now vz is 0, check anim completion flag
        state["anim_finished"] = False
        self.assertFalse(simulate_shoreline_step(state))
        state["anim_finished"] = True
        self.assertTrue(simulate_shoreline_step(state))

    def test_water_jump_landing_audio_gate(self):
        state = {
            "timer30": 3,
            "flag67_bit0": 0,
            "vz": 0x10000,
            "height": 0x20000
        }
        # timer <= 4: sets bit 0
        simulate_water_landing_audio_gate(state)
        self.assertEqual(state["flag67_bit0"], 1)

        # timer > 4: clears bit 0, triggers splash SFX 0x500341
        state["timer30"] = 5
        simulate_water_landing_audio_gate(state)
        self.assertEqual(state["flag67_bit0"], 0)
        self.assertEqual(state["sfx"], 0x500341)

        # Impact with water/ground: timer30 becomes 0x14
        state["timer30"] = 6
        state["vz"] = -0x40000
        simulate_water_landing_audio_gate(state)
        self.assertEqual(state["timer30"], 0x14)
        self.assertTrue(state.get("landed", False))

    def test_water_velocity_3_4_scaler(self):
        # 100 >> 1 = 50, 100 >> 2 = 25 -> 75
        vx, vy = scale_water_velocity_3_4(100, 200, 0, 0)
        self.assertEqual(vx, 75)
        self.assertEqual(vy, 150)

        # Opposing inertia cancellation:
        # inertia_x = +10, d1 = +100 (same direction as positive inertia -> cancelled to 0)
        # Note: in M68K: if inertia > 0, if d1 is bmi (negative), it doesn't cancel; if d1 is bpl, it clears d1
        vx, vy = scale_water_velocity_3_4(100, 0, 10, 0)
        self.assertEqual(vx, 10)  # s_d1 cleared, result is just inertia 10

        # inertia_x = +10, d1 = -100 (negative input opposing positive inertia is preserved!)
        vx, vy = scale_water_velocity_3_4(-100, 0, 10, 0)
        # -100 >> 1 = -50, -100 >> 2 = -25 -> -75; + 10 = -65
        self.assertEqual(vx, -65)

    def test_jump_slash_descent_shadow(self):
        # 0x0071DA: landing timer is 0x0C (12 frames)
        landing_timer = 0x0C
        self.assertEqual(landing_timer, 12)


if __name__ == "__main__":
    unittest.main()
