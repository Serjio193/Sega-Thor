#!/usr/bin/env python3
"""
rom_ali_water_swim_test.py вЂ” M15.379: Unit tests for ROM Ali Pit Respawn & Water Swimming Combat
Tests:
  1. test_state58_recovery
  2. test_pit_respawn_interpolation_step
  3. test_water_swimming_locomotion
  4. test_water_weapon_attacks
"""

import json
import unittest


def step_coord_interpolator(current_val, target_val):
    d1 = target_val
    d3 = current_val
    d5_bit7 = 0
    d5 = 0
    if d3 == d1:
        return d3, d5, d5_bit7
    if d3 < d1:
        d3 += 1
        d5 = 1
    else:
        d3 -= 1
        d5 = 1
        d5_bit7 = 1
    return d3, d5, d5_bit7


def simulate_pit_respawn_step(ali_state, anchor_x, anchor_y):
    # 0x006B48
    if ali_state["timer10"] < 8:
        ali_state["timer10"] += 1

    d5_flag = 0
    # Step X twice
    for _ in range(2):
        new_x, d5, d5_b7 = step_coord_interpolator(ali_state["x"], anchor_x)
        ali_state["x"] = new_x
        if d5:
            d5_flag |= 1

    # Step Y twice
    for _ in range(2):
        new_y, d5, d5_b7 = step_coord_interpolator(ali_state["y"], anchor_y)
        ali_state["y"] = new_y
        if d5:
            d5_flag |= 1

    if d5_flag != 0:
        ali_state["arrived"] = False
        return False
    else:
        # Target reached! 0x006BB6
        ali_state["arrived"] = True
        ali_state["facing"] = 2
        ali_state["anim"] = 0xFFFF
        ali_state["field34"] = 0xFFFF
        ali_state["flag36"] = 1
        ali_state["flag37"] = 0
        ali_state["flag39"] = 0
        ali_state["timer10"] = 0
        ali_state["invuln_timer"] = 0x3F
        ali_state["companion_notified"] = True
        ali_state["pit_lock_cleared"] = True
        return True


def simulate_water_swimming(ali_state, d7_buttons, dpad_dx, dpad_dy, inhibited=False):
    # 0x006C46
    ali_state["anim_duration"] = 20

    # Button A pressed? (bit 4)
    if not inhibited and (d7_buttons & 0x10):
        # Weapon attack dispatch
        weapon = ali_state.get("weapon_id", 0)
        ali_state["sound_played"] = None
        if weapon == 0x0C:
            ali_state["anim"] = 0x52
            ali_state["state"] = 0x4A
        elif weapon == 0x0E:
            if ali_state.get("sword_locked", False):
                pass
            else:
                if ali_state.get("special_sword_mode", 0) == 8:
                    ali_state["anim"] = 0x5E
                else:
                    ali_state["anim"] = 0x56
                ali_state["state"] = 0x4E
                ali_state["sound_played"] = 0x600377
        elif weapon == 0x10:
            if ali_state.get("sword_locked", False):
                pass
            else:
                ali_state["anim"] = 0x5A
                ali_state["state"] = 0x52
        else:
            ali_state["anim"] = 0x48
            ali_state["state"] = 0x42
        ali_state["field_ff16ea"] = 0
        return

    # Check movement
    moving = (d7_buttons & 0x2000) != 0 or inhibited  # bit 13
    if moving:
        ali_state["stroke_counter"] += 1
        if dpad_dx != 0 or dpad_dy != 0:
            # Water 25% speed scaling: d1 >>= 2, d2 >>= 2
            scaled_vx = dpad_dx >> 2
            scaled_vy = dpad_dy >> 2
            ali_state["vx"] += scaled_vx
            ali_state["vy"] += scaled_vy
            ali_state["anim"] = 0x20  # stroke
        else:
            if ali_state.get("anim") != 0x10:
                ali_state["anim"] = 0x24  # coast
    else:
        # Idle tread
        if ali_state["stroke_counter"] < 16:
            ali_state["anim"] = 6
            ali_state["state"] = 6
            ali_state["timer10"] = 1
            ali_state["sound_played"] = 0x500341
        else:
            ali_state["anim"] = 0x12
            ali_state["state"] = 0x12
            ali_state["field_ff16ea"] = 0


class TestRomAliWaterSwim(unittest.TestCase):
    def test_state58_recovery(self):
        # 0x006B2A
        def recover(flag37):
            vx = 0
            vy = 0
            anim = 0
            state = 0x58
            if flag37 & 2:
                anim = 0x10
                state = 0x5A
            return vx, vy, anim, state

        vx, vy, anim, state = recover(0)
        self.assertEqual(state, 0x58)
        self.assertEqual(anim, 0)

        vx, vy, anim, state = recover(2)
        self.assertEqual(state, 0x5A)
        self.assertEqual(anim, 0x10)

    def test_pit_respawn_interpolation_step(self):
        state = {
            "x": 100,
            "y": 200,
            "timer10": 0,
            "arrived": False
        }
        anchor_x = 104
        anchor_y = 200

        # Step 1: should move 2 pixels towards anchor_x
        arrived = simulate_pit_respawn_step(state, anchor_x, anchor_y)
        self.assertFalse(arrived)
        self.assertEqual(state["x"], 102)
        self.assertEqual(state["timer10"], 1)

        # Step 2: moves another 2 pixels -> reaches anchor (104) during this frame (d5 non-zero)
        arrived = simulate_pit_respawn_step(state, anchor_x, anchor_y)
        self.assertFalse(arrived)
        self.assertEqual(state["x"], 104)

        # Step 3: already at anchor (104), all steps return d5=0 -> arrival triggered!
        arrived = simulate_pit_respawn_step(state, anchor_x, anchor_y)
        self.assertTrue(arrived)
        self.assertEqual(state["x"], 104)
        self.assertEqual(state["facing"], 2)
        self.assertEqual(state["invuln_timer"], 0x3F)
        self.assertTrue(state["companion_notified"])
        self.assertTrue(state["pit_lock_cleared"])

    def test_water_swimming_locomotion(self):
        state = {
            "vx": 0,
            "vy": 0,
            "stroke_counter": 5,
            "anim": 0,
            "state": 0x10
        }
        # Moving with D-pad (bit 13 of d7 set)
        simulate_water_swimming(state, d7_buttons=0x2000, dpad_dx=0x1000, dpad_dy=0x800)
        self.assertEqual(state["stroke_counter"], 6)
        self.assertEqual(state["vx"], 0x1000 >> 2)
        self.assertEqual(state["vy"], 0x800 >> 2)
        self.assertEqual(state["anim"], 0x20)

        # Idle with stroke counter < 16 -> tread water and splash SFX
        state["stroke_counter"] = 10
        simulate_water_swimming(state, d7_buttons=0, dpad_dx=0, dpad_dy=0)
        self.assertEqual(state["state"], 6)
        self.assertEqual(state["anim"], 6)
        self.assertEqual(state["sound_played"], 0x500341)

        # Idle with stroke counter >= 16 -> state 0x12
        state["stroke_counter"] = 16
        simulate_water_swimming(state, d7_buttons=0, dpad_dx=0, dpad_dy=0)
        self.assertEqual(state["state"], 0x12)
        self.assertEqual(state["anim"], 0x12)

    def test_water_weapon_attacks(self):
        # Dagger (0x0C)
        dagger_state = {"weapon_id": 0x0C, "stroke_counter": 0}
        simulate_water_swimming(dagger_state, d7_buttons=0x10, dpad_dx=0, dpad_dy=0)
        self.assertEqual(dagger_state["anim"], 0x52)
        self.assertEqual(dagger_state["state"], 0x4A)

        # Sword standard (0x0E)
        sword_state = {"weapon_id": 0x0E, "stroke_counter": 0}
        simulate_water_swimming(sword_state, d7_buttons=0x10, dpad_dx=0, dpad_dy=0)
        self.assertEqual(sword_state["anim"], 0x56)
        self.assertEqual(sword_state["state"], 0x4E)
        self.assertEqual(sword_state["sound_played"], 0x600377)

        # Sword special mode (0x0E + mode 8)
        sword_spec = {"weapon_id": 0x0E, "special_sword_mode": 8, "stroke_counter": 0}
        simulate_water_swimming(sword_spec, d7_buttons=0x10, dpad_dx=0, dpad_dy=0)
        self.assertEqual(sword_spec["anim"], 0x5E)
        self.assertEqual(sword_spec["state"], 0x4E)

        # Bow (0x10)
        bow_state = {"weapon_id": 0x10, "stroke_counter": 0}
        simulate_water_swimming(bow_state, d7_buttons=0x10, dpad_dx=0, dpad_dy=0)
        self.assertEqual(bow_state["anim"], 0x5A)
        self.assertEqual(bow_state["state"], 0x52)

        # Unarmed
        unarmed_state = {"weapon_id": 0, "stroke_counter": 0}
        simulate_water_swimming(unarmed_state, d7_buttons=0x10, dpad_dx=0, dpad_dy=0)
        self.assertEqual(unarmed_state["anim"], 0x48)
        self.assertEqual(unarmed_state["state"], 0x42)


if __name__ == "__main__":
    unittest.main()
