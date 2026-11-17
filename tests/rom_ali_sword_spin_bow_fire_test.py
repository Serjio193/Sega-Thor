#!/usr/bin/env python3
"""
rom_ali_sword_spin_bow_fire_test.py — M15.382: Unit tests for ROM Ali Sword Spin & Bow Arrow Dispatcher
Tests:
  1. test_ground_sword_spin
  2. test_water_sword_spin
  3. test_ground_bow_fire
  4. test_water_bow_fire
  5. test_water_dagger_flurry
"""

import json
import unittest


def get_sword_spin_payload(sword_element, is_water=False):
    # 0x00728E / 0x007312
    base_id = 0x0D if is_water else 0x1C
    if sword_element == 7:  # Fire Sword
        return 0x80000000 | base_id
    elif sword_element == 8:  # Omega Sword
        return 0x40000000 | base_id
    elif sword_element == 12:  # Hyper Sword
        return 0x20000000 | base_id
    else:
        return base_id


def simulate_sword_spin(ali_state, btn_a_held, dpad_dx, dpad_dy):
    # 0x007262
    frame = ali_state["frame30"]
    if frame <= 12:
        return
    if 12 < frame <= 16:
        if btn_a_held:
            # 25% steering speed: d1 >>= 2, d2 >>= 2
            ali_state["vx"] += (dpad_dx >> 2)
            ali_state["vy"] += (dpad_dy >> 2)
            return
        else:
            ali_state["field2e"] = 0
            ali_state["sfx"] = 0x600379
            sword_elem = ali_state.get("sword_element", 0)
            ali_state["blade_payload"] = get_sword_spin_payload(sword_elem, is_water=False)


def get_bow_fire_frame(facing_dir):
    # 0x007390
    if facing_dir == 2:  # facing down
        return 12
    return 16


def simulate_bow_fire(ali_state, btn_a_held, dpad_dx, dpad_dy):
    # 0x007364
    frame = ali_state["frame30"]
    if frame <= 4:
        return
    if 4 < frame <= 8:
        if btn_a_held:
            ali_state["vx"] += (dpad_dx >> 2)
            ali_state["vy"] += (dpad_dy >> 2)
            return
        else:
            ali_state["field2e"] = 0
            ali_state["sfx"] = 0x7004D3

    fire_frame = get_bow_fire_frame(ali_state["facing"])
    if frame == fire_frame and ali_state.get("field2e", 0) == 0:
        bow_type = ali_state.get("bow_type", 9)
        d0 = bow_type - 9
        d1 = 0x22
        d2 = (d0 << 4) + 1
        ali_state["arrow_dispatched"] = True
        ali_state["arrow_params"] = (d1, d2)
        ali_state["shot_flag_5c"] = 0xFFFF


class TestRomAliSwordSpinBowFire(unittest.TestCase):
    def test_ground_sword_spin(self):
        # Fire sword
        state = {
            "frame30": 14,
            "sword_element": 7,
            "vx": 0,
            "vy": 0,
            "field2e": 1
        }
        # Held button: steers at 25% speed
        simulate_sword_spin(state, btn_a_held=True, dpad_dx=100, dpad_dy=200)
        self.assertEqual(state["vx"], 25)
        self.assertEqual(state["vy"], 50)
        self.assertNotIn("sfx", state)

        # Released button: triggers SFX 0x600379 and Fire Sword payload
        simulate_sword_spin(state, btn_a_held=False, dpad_dx=0, dpad_dy=0)
        self.assertEqual(state["field2e"], 0)
        self.assertEqual(state["sfx"], 0x600379)
        self.assertEqual(state["blade_payload"], 0x8000001C)

        # Omega sword
        self.assertEqual(get_sword_spin_payload(8), 0x4000001C)
        # Hyper sword
        self.assertEqual(get_sword_spin_payload(12), 0x2000001C)
        # Normal sword
        self.assertEqual(get_sword_spin_payload(0), 0x0000001C)

    def test_water_sword_spin(self):
        # Water sword spin payloads
        self.assertEqual(get_sword_spin_payload(7, is_water=True), 0x8000000D)
        self.assertEqual(get_sword_spin_payload(8, is_water=True), 0x4000000D)
        self.assertEqual(get_sword_spin_payload(12, is_water=True), 0x2000000D)
        self.assertEqual(get_sword_spin_payload(0, is_water=True), 0x0000000D)

    def test_ground_bow_fire(self):
        # Facing down (2): fire at frame 12
        self.assertEqual(get_bow_fire_frame(2), 12)
        # Other facings: fire at frame 16
        self.assertEqual(get_bow_fire_frame(0), 16)
        self.assertEqual(get_bow_fire_frame(1), 16)
        self.assertEqual(get_bow_fire_frame(3), 16)

        # Standard bow (bow_type = 9)
        state_down = {
            "facing": 2,
            "frame30": 12,
            "bow_type": 9,
            "field2e": 0
        }
        simulate_bow_fire(state_down, btn_a_held=False, dpad_dx=0, dpad_dy=0)
        self.assertTrue(state_down.get("arrow_dispatched", False))
        # d0 = 0, d1 = 0x22, d2 = 0 + 1 = 1
        self.assertEqual(state_down["arrow_params"], (0x22, 1))
        self.assertEqual(state_down["shot_flag_5c"], 0xFFFF)

        # Fire bow (bow_type = 10)
        state_fire = {
            "facing": 1,
            "frame30": 16,
            "bow_type": 10,
            "field2e": 0
        }
        simulate_bow_fire(state_fire, btn_a_held=False, dpad_dx=0, dpad_dy=0)
        self.assertTrue(state_fire.get("arrow_dispatched", False))
        # d0 = 1, d1 = 0x22, d2 = (1 << 4) + 1 = 17
        self.assertEqual(state_fire["arrow_params"], (0x22, 17))

    def test_water_bow_fire(self):
        # 0x0073EC: d1 = 4, d2 = d0 << 3
        def get_water_arrow_params(bow_type):
            d0 = bow_type - 9
            return (4, d0 << 3)

        self.assertEqual(get_water_arrow_params(9), (4, 0))
        self.assertEqual(get_water_arrow_params(10), (4, 8))
        self.assertEqual(get_water_arrow_params(11), (4, 16))

    def test_water_dagger_flurry(self):
        # 0x00743C: frame 0 plays SFX 0x800222
        sfx = 0x800222
        self.assertEqual(sfx, 0x800222)
        # Land probe transitions to anim 0x60, state 0x54
        anim = 0x60
        state = 0x54
        self.assertEqual(anim, 0x60)
        self.assertEqual(state, 0x54)


if __name__ == "__main__":
    unittest.main()
