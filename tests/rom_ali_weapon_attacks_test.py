#!/usr/bin/env python3
"""
rom_ali_weapon_attacks_test.py — M15.389: Unit tests for ROM Ali Weapon Combat Attack Initializers
Tests:
  1. test_sword_attack_variations
  2. test_bow_attack_draw
  3. test_bomb_attack_placement
  4. test_sprint_dash_tackle
  5. test_attack_commit_finalizer
"""

import unittest


def evaluate_sword_attack(dash_leap_motion, dpad_moving, is_running):
    # 0x00803C..0x00808E
    if dash_leap_motion:
        return {"anim": 0x66, "state": 0x5C, "type": "leap_slash"}
    if dpad_moving and is_running:
        return {"anim": 0x60, "state": 0x54, "type": "running_slash"}
    return {"anim": 0x4E, "state": 0x46, "type": "stationary_slash"}


def evaluate_bow_attack(target_entity_exists, weapon_id):
    # 0x008090..0x0080C0
    if target_entity_exists:
        return {"action": "abort"}
    anim = 0x5C if weapon_id == 8 else 0x54
    return {
        "action": "bow_draw",
        "anim": anim,
        "state": 0x4C,
        "sfx": 0x600377,
    }


def evaluate_bomb_attack(target_entity_exists):
    # 0x0080C2..0x0080D6
    if target_entity_exists:
        return {"action": "abort"}
    return {
        "action": "bomb_place",
        "anim": 0x58,
        "state": 0x50,
    }


def evaluate_sprint_dash_attack(equipped_weapon, special_motion):
    # 0x0080D8..0x00814C
    if equipped_weapon == 0x0C:  # Sword redirects
        return {"route": "sword_running"}
    if special_motion is not None:
        return {"route": special_motion}
    # Sprint tackle
    return {
        "route": "sprint_tackle",
        "anim": 0x40,
        "state": 0x38,
        "flag_39_bit2": True,
        "timer_9a": 0x15,
    }


def commit_attack(anim, state):
    # 0x00820E..0x00822C
    return {
        "anim": anim,
        "state": state,
        "recovery_timer_198b": 4,
        "clear_ring_count": True,
        "unroll_stack": True,
    }


class TestRomAliWeaponAttacks(unittest.TestCase):
    def test_sword_attack_variations(self):
        # 1. Leap slash via dash-leap motion
        res_leap = evaluate_sword_attack(dash_leap_motion=True, dpad_moving=True, is_running=True)
        self.assertEqual(res_leap["type"], "leap_slash")
        self.assertEqual(res_leap["anim"], 0x66)
        self.assertEqual(res_leap["state"], 0x5C)

        # 2. Running slash
        res_run = evaluate_sword_attack(dash_leap_motion=False, dpad_moving=True, is_running=True)
        self.assertEqual(res_run["type"], "running_slash")
        self.assertEqual(res_run["anim"], 0x60)
        self.assertEqual(res_run["state"], 0x54)

        # 3. Stationary slash
        res_stat = evaluate_sword_attack(dash_leap_motion=False, dpad_moving=False, is_running=False)
        self.assertEqual(res_stat["type"], "stationary_slash")
        self.assertEqual(res_stat["anim"], 0x4E)
        self.assertEqual(res_stat["state"], 0x46)

    def test_bow_attack_draw(self):
        # Normal Bow (ID 6 or 7)
        res_norm = evaluate_bow_attack(target_entity_exists=False, weapon_id=6)
        self.assertEqual(res_norm["action"], "bow_draw")
        self.assertEqual(res_norm["anim"], 0x54)
        self.assertEqual(res_norm["state"], 0x4C)
        self.assertEqual(res_norm["sfx"], 0x600377)

        # Fire Bow (ID 8)
        res_fire = evaluate_bow_attack(target_entity_exists=False, weapon_id=8)
        self.assertEqual(res_fire["anim"], 0x5C)
        self.assertEqual(res_fire["state"], 0x4C)

        # Blocked when target exists
        res_block = evaluate_bow_attack(target_entity_exists=True, weapon_id=6)
        self.assertEqual(res_block["action"], "abort")

    def test_bomb_attack_placement(self):
        res = evaluate_bomb_attack(target_entity_exists=False)
        self.assertEqual(res["action"], "bomb_place")
        self.assertEqual(res["anim"], 0x58)
        self.assertEqual(res["state"], 0x50)

    def test_sprint_dash_tackle(self):
        # Dagger sprint tackle fallback
        res = evaluate_sprint_dash_attack(equipped_weapon=0x0A, special_motion=None)
        self.assertEqual(res["route"], "sprint_tackle")
        self.assertEqual(res["anim"], 0x40)
        self.assertEqual(res["state"], 0x38)
        self.assertTrue(res["flag_39_bit2"])
        self.assertEqual(res["timer_9a"], 0x15)

        # Sword redirect
        res_sword = evaluate_sprint_dash_attack(equipped_weapon=0x0C, special_motion=None)
        self.assertEqual(res_sword["route"], "sword_running")

    def test_attack_commit_finalizer(self):
        commit = commit_attack(anim=0x4E, state=0x46)
        self.assertEqual(commit["anim"], 0x4E)
        self.assertEqual(commit["state"], 0x46)
        self.assertEqual(commit["recovery_timer_198b"], 4)
        self.assertTrue(commit["clear_ring_count"])
        self.assertTrue(commit["unroll_stack"])


if __name__ == "__main__":
    unittest.main()
