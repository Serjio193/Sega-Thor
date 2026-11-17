#!/usr/bin/env python3
"""
rom_ali_jump_weapon_swap_test.py — M15.388: Unit tests for ROM Ali Jump Initiation, Weapon Swap & Attack Dispatch
Tests:
  1. test_jump_button_initiation
  2. test_weapon_hotkey_swap_success
  3. test_weapon_hotkey_swap_empty_slot
  4. test_dagger_attack_dispatch
  5. test_weapon_attack_routing
"""

import unittest


def handle_jump_button(jump_flag, current_state, can_jump):
    # 0x007E46..0x007EC2
    if jump_flag != 0:
        return {"action": "flag_rearm", "flag": 1}
    if current_state == 4:
        return {"action": "abort_airborne"}
    if not can_jump:
        return {"action": "abort_blocked"}
    return {
        "action": "jump_initiated",
        "state": 0x14,
        "anim": 0x14,
        "clear_1990": True,
        "sfx": 0x900359,
        "unroll_stack": True,
    }


def swap_weapon_slot(slot_idx, inventory_slots, current_weapon_type):
    # 0x007EC4..0x007F80
    if slot_idx < 0 or slot_idx >= len(inventory_slots):
        return {"action": "empty_buzzer", "sfx": 0xA00253}
    item_id = inventory_slots[slot_idx]
    if item_id == 0:
        return {"action": "empty_buzzer", "sfx": 0xA00253}

    # Weapon type lookup
    if item_id == 1:
        weapon_type = 0x0A  # Dagger
    elif item_id <= 5:
        weapon_type = 0x0C  # Sword
    elif item_id <= 8:
        weapon_type = 0x0E  # Bow
    elif item_id <= 11:
        weapon_type = 0x10  # Bomb
    else:
        weapon_type = 0x0E

    return {
        "action": "weapon_equipped",
        "active_slot": slot_idx,
        "active_weapon_id": item_id,
        "weapon_type": weapon_type,
        "hud_display_id": item_id,
        "hud_toast_timer": 0x3C,
        "swap_flag": True,
        "sfx": 0xC0025D,
    }


def dispatch_weapon_attack(equipped_weapon_type):
    # 0x007F82..0x00803C
    if equipped_weapon_type == 0x0C:
        return {"route": "sword"}
    elif equipped_weapon_type == 0x0E:
        return {"route": "bow"}
    elif equipped_weapon_type == 0x10:
        return {"route": "bomb"}
    else:
        # Default Dagger strike
        return {
            "route": "dagger",
            "anim": 0x3A,
            "state": 0x0A,
            "combo_timer": 4,
            "unroll_stack": True,
        }


class TestRomAliJumpWeaponSwap(unittest.TestCase):
    def test_jump_button_initiation(self):
        # Normal ground jump
        res = handle_jump_button(jump_flag=0, current_state=0, can_jump=True)
        self.assertEqual(res["action"], "jump_initiated")
        self.assertEqual(res["state"], 0x14)
        self.assertEqual(res["anim"], 0x14)
        self.assertEqual(res["sfx"], 0x900359)
        self.assertTrue(res["unroll_stack"])

        # Already airborne
        res_air = handle_jump_button(jump_flag=0, current_state=4, can_jump=True)
        self.assertEqual(res_air["action"], "abort_airborne")

        # Slope/flag rearm
        res_flag = handle_jump_button(jump_flag=2, current_state=0, can_jump=True)
        self.assertEqual(res_flag["action"], "flag_rearm")
        self.assertEqual(res_flag["flag"], 1)

    def test_weapon_hotkey_swap_success(self):
        inventory = [1, 2, 6, 9, 0, 0, 0, 0]  # Dagger, Sword, Bow, Bomb
        # Swap to slot 0: Dagger
        res0 = swap_weapon_slot(0, inventory, 0)
        self.assertEqual(res0["action"], "weapon_equipped")
        self.assertEqual(res0["weapon_type"], 0x0A)
        self.assertEqual(res0["hud_toast_timer"], 0x3C)
        self.assertEqual(res0["sfx"], 0xC0025D)

        # Swap to slot 1: Sword
        res1 = swap_weapon_slot(1, inventory, 0x0A)
        self.assertEqual(res1["weapon_type"], 0x0C)

        # Swap to slot 2: Bow
        res2 = swap_weapon_slot(2, inventory, 0x0C)
        self.assertEqual(res2["weapon_type"], 0x0E)

        # Swap to slot 3: Bomb
        res3 = swap_weapon_slot(3, inventory, 0x0E)
        self.assertEqual(res3["weapon_type"], 0x10)

    def test_weapon_hotkey_swap_empty_slot(self):
        inventory = [1, 0, 0, 0, 0, 0, 0, 0]
        res = swap_weapon_slot(3, inventory, 0x0A)
        self.assertEqual(res["action"], "empty_buzzer")
        self.assertEqual(res["sfx"], 0xA00253)

    def test_dagger_attack_dispatch(self):
        res = dispatch_weapon_attack(equipped_weapon_type=0x0A)
        self.assertEqual(res["route"], "dagger")
        self.assertEqual(res["anim"], 0x3A)
        self.assertEqual(res["state"], 0x0A)
        self.assertEqual(res["combo_timer"], 4)
        self.assertTrue(res["unroll_stack"])

    def test_weapon_attack_routing(self):
        self.assertEqual(dispatch_weapon_attack(0x0C)["route"], "sword")
        self.assertEqual(dispatch_weapon_attack(0x0E)["route"], "bow")
        self.assertEqual(dispatch_weapon_attack(0x10)["route"], "bomb")


if __name__ == "__main__":
    unittest.main()
