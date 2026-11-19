#!/usr/bin/env python3
"""
Unit tests for ROM Composite Serpent/Multi-Segment Boss States 0x00..0x16
(Grab, Slam, Spit, Death Gravity Fall & 4-Segment Despawn Stagger)
(0x015E0E..0x0160E2, Milestone M15.513).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_serpent_boss_states_m15_513.json"
)


def step_state00_anchor_sync(boss, root):
    """Emulate State $00 Anchor Offset Sync (0x015E0E..0x015E3C)."""
    d3 = root.get("x", 0) - 0x20
    d4 = root.get("y", 0) + 0x18
    d3 -= boss.get("x", 0)
    d4 -= boss.get("y", 0)
    boss["delta_x_72"] = d3
    boss["delta_y_76"] = d4
    boss["z_10"] = root.get("z", 0)


def step_state08_ascend(boss):
    """Emulate State $08 Ascend (0x015E3C..0x015E56)."""
    boss["timer"] -= 1
    if boss["timer"] != 0:
        boss["z_10"] += 3
        return False
    boss["state"] = 2
    boss["timer"] = 17
    return True


def step_state02_hover_grab(boss, ali, at_ground_boundary=False):
    """Emulate State $02 Hover & Ali Grab Probe (0x015E56..0x015EEA)."""
    if not at_ground_boundary:
        boss["z_10"] -= 4
        return "DESCENDING"

    # Evaluates Ali
    is_airborne = ali.get("z_10", 0) != 0
    is_immune = (ali.get("flags_38", 0) & 0x08) != 0

    if is_airborne or is_immune:
        # Miss
        boss["state"] = 4
        boss["anim_id"] = 0x34
        boss["timer"] = 17
        boss["multiplier_a6"] = -boss.get("multiplier_a6", 1)
        boss["multiplier_aa"] = -boss.get("multiplier_aa", 1)
        return "GRAB_MISS"

    # Proximity check
    d1_x = abs((boss.get("x", 0) + 4) - ali.get("x", 0))
    if d1_x > 16:
        boss["state"] = 4
        boss["anim_id"] = 0x34
        boss["timer"] = 17
        boss["multiplier_a6"] = -boss.get("multiplier_a6", 1)
        boss["multiplier_aa"] = -boss.get("multiplier_aa", 1)
        return "GRAB_MISS"

    d1_y = abs((boss.get("y", 0) + 16) - ali.get("y", 0))
    if d1_y > 16:
        boss["state"] = 4
        boss["anim_id"] = 0x34
        boss["timer"] = 17
        boss["multiplier_a6"] = -boss.get("multiplier_a6", 1)
        boss["multiplier_aa"] = -boss.get("multiplier_aa", 1)
        return "GRAB_MISS"

    # Successful Grab!
    boss["state"] = 6
    boss["anim_id"] = 0x36
    boss["timer"] = 17
    boss["multiplier_a6"] = -boss.get("multiplier_a6", 1)
    boss["multiplier_aa"] = -boss.get("multiplier_aa", 1)

    ali["lock_1982"] = True
    ali["flags_36"] = ali.get("flags_36", 0) | 0x02  # bit 1 set
    ali["flags_38"] = ali.get("flags_38", 0) | 0x0C  # bits 2 & 3 set
    return "GRAB_SUCCESS"


def step_state04_return_ascend(boss, at_top_boundary=False):
    """Emulate State $04 Return Ascend (0x015EEA..0x015F04)."""
    if not at_top_boundary:
        boss["z_10"] += 4
        return False
    boss["state"] = 0
    boss["anim_id"] = 0x12
    return True


def step_state06_slam_setup(boss, arena_x, at_top_boundary=False):
    """Emulate State $06 Slam Setup (0x015F04..0x015F34)."""
    if not at_top_boundary:
        boss["z_10"] += 4
        return "ASCENDING"
    boss["timer"] = 7
    if arena_x <= 0x100:
        boss["state"] = 0x0A
        return "BRANCH_RIGHT"
    else:
        boss["state"] = 0x14
        return "BRANCH_LEFT"


def step_rightward_slam(boss, ali):
    """Emulate States $0A, $0C, $0E (0x015F34..0x015FE0)."""
    if boss["state"] == 0x0A:
        boss["timer"] -= 1
        if boss["timer"] != 0:
            boss["delta_x_72"] -= 3
            return "WINDUP_LEFT"
        boss["state"] = 0x0C
        boss["timer"] = 5
        return "ENTER_LUNGE"

    elif boss["state"] == 0x0C:
        boss["timer"] -= 1
        if boss["timer"] != 0:
            boss["delta_x_72"] += 16
            return "LUNGE_RIGHT"
        # Release and slam
        boss["state"] = 0x0E
        boss["timer"] = 13
        boss["anim_id"] = 0x12
        boss["flags_38"] = boss.get("flags_38", 0) | 0x08  # bit 3 invulnerable

        ali["lock_1982"] = False
        ali["flags_36"] = ali.get("flags_36", 0) & ~0x02
        ali["flags_38"] = ali.get("flags_38", 0) & 0xF3
        ali["damage_86"] = 0x8014
        ali["x_vel_4e"] = 0x00200000
        ali["y_vel_52"] = 0
        ali["z_vel_56"] = -1
        ali["z_10"] -= 32
        ali["delta_x_72"] = ali.get("delta_x_72", 0) + 4
        ali["facing_16"] = 1
        ali["knockback_mode_84"] = 1
        return "RELEASE_RIGHT"

    elif boss["state"] == 0x0E:
        boss["timer"] -= 1
        if boss["timer"] != 0:
            boss["delta_x_72"] -= 4
            return "RECOVER_LEFT"
        boss["state"] = 0
        boss["flags_38"] = boss.get("flags_38", 0) & ~0x08
        return "RESET_STATE0"


def step_leftward_slam(boss, ali):
    """Emulate States $14, $16, recovery (0x015FE0..0x01608C)."""
    if boss["state"] == 0x14:
        boss["timer"] -= 1
        if boss["timer"] != 0:
            boss["delta_x_72"] += 3
            return "WINDUP_RIGHT"
        boss["state"] = 0x16
        boss["timer"] = 5
        return "ENTER_LUNGE"

    elif boss["state"] == 0x16:
        boss["timer"] -= 1
        if boss["timer"] != 0:
            boss["delta_x_72"] -= 16
            return "LUNGE_LEFT"
        # Release and slam
        boss["state"] = 0x18
        boss["timer"] = 13
        boss["anim_id"] = 0x12
        boss["flags_38"] = boss.get("flags_38", 0) | 0x08

        ali["lock_1982"] = False
        ali["flags_36"] = ali.get("flags_36", 0) & ~0x02
        ali["flags_38"] = ali.get("flags_38", 0) & 0xF3
        ali["damage_86"] = 0x8014
        ali["x_vel_4e"] = -0x00200000
        ali["y_vel_52"] = 0
        ali["z_vel_56"] = -1
        ali["z_10"] -= 32
        ali["delta_x_72"] = ali.get("delta_x_72", 0) - 4
        ali["facing_16"] = 1
        ali["knockback_mode_84"] = 3
        return "RELEASE_LEFT"

    elif boss["state"] == 0x18:
        boss["timer"] -= 1
        if boss["timer"] != 0:
            boss["delta_x_72"] += 4
            return "RECOVER_RIGHT"
        boss["state"] = 0
        boss["flags_38"] = boss.get("flags_38", 0) & ~0x08
        return "RESET_STATE0"


def step_state10_death_fall(boss, segments):
    """Emulate State $10 Death Fall & Staggered Despawn (0x01608C..0x0160E2)."""
    if boss["z_10"] <= 0:
        return "ALREADY_GROUNDED"

    boss["z_vel_56"] = boss.get("z_vel_56", 0) - 0xC000
    boss["z_10"] += boss["z_vel_56"]

    if boss["z_10"] > 0:
        return "FALLING"

    # Ground impact
    boss["z_10"] = 0
    # Configure 4 child segments
    timers = [4, 8, 12, 16]
    for i, seg in enumerate(segments[:4]):
        seg["mode_66"] = 3
        seg["timer_2"] = timers[i]
    return "IMPACT_TRIGGERED"


class TestROMSerpentBossStates(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(__file__)
        ev_file = os.path.join(self_dir, "..", "data", "thor-evidence", "rom_serpent_boss_states_m15_513.json")
        with open(ev_file, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        self.assertEqual(self.evidence["milestone"], "M15.513")
        self.assertEqual(self.evidence["range"]["start"], "0x015E0E")
        self.assertEqual(self.evidence["range"]["end"], "0x0160E2")
        self.assertEqual(self.evidence["range"]["size_bytes"], 724)
        self.assertEqual(self.evidence["instruction_count"], 168)
        self.assertEqual(len(self.evidence["routines"]), 8)
        self.assertEqual(len(self.evidence["death_stagger_config"]), 4)

    def test_state00_anchor_sync(self):
        root = {"x": 200, "y": 150, "z": 45}
        boss = {"x": 180, "y": 160}
        step_state00_anchor_sync(boss, root)
        # target_x = 200 - 32 = 168; delta_x = 168 - 180 = -12
        # target_y = 150 + 24 = 174; delta_y = 174 - 160 = 14
        self.assertEqual(boss["delta_x_72"], -12)
        self.assertEqual(boss["delta_y_76"], 14)
        self.assertEqual(boss["z_10"], 45)

    def test_state08_ascend(self):
        boss = {"timer": 2, "z_10": 10, "state": 8}
        # Step 1
        transitioned = step_state08_ascend(boss)
        self.assertFalse(transitioned)
        self.assertEqual(boss["z_10"], 13)
        self.assertEqual(boss["timer"], 1)

        # Step 2: timer expires
        transitioned = step_state08_ascend(boss)
        self.assertTrue(transitioned)
        self.assertEqual(boss["state"], 2)
        self.assertEqual(boss["timer"], 17)

    def test_state02_hover_grab_probe(self):
        boss = {"x": 100, "y": 100, "z_10": 20, "multiplier_a6": 10, "multiplier_aa": 20}
        ali_ground = {"x": 104, "y": 116, "z_10": 0, "flags_38": 0}

        # In flight descent
        res = step_state02_hover_grab(boss, ali_ground, at_ground_boundary=False)
        self.assertEqual(res, "DESCENDING")
        self.assertEqual(boss["z_10"], 16)

        # Airborne Ali -> miss
        ali_air = {"x": 104, "y": 116, "z_10": 5, "flags_38": 0}
        res = step_state02_hover_grab(boss, ali_air, at_ground_boundary=True)
        self.assertEqual(res, "GRAB_MISS")
        self.assertEqual(boss["state"], 4)
        self.assertEqual(boss["anim_id"], 0x34)
        self.assertEqual(boss["multiplier_a6"], -10)
        self.assertEqual(boss["multiplier_aa"], -20)

        # Invulnerable Ali -> miss
        ali_invuln = {"x": 104, "y": 116, "z_10": 0, "flags_38": 0x08}
        res = step_state02_hover_grab(boss, ali_invuln, at_ground_boundary=True)
        self.assertEqual(res, "GRAB_MISS")

        # Grounded vulnerable in range -> Success!
        boss["multiplier_a6"] = 5
        boss["multiplier_aa"] = 5
        res = step_state02_hover_grab(boss, ali_ground, at_ground_boundary=True)
        self.assertEqual(res, "GRAB_SUCCESS")
        self.assertEqual(boss["state"], 6)
        self.assertEqual(boss["anim_id"], 0x36)
        self.assertEqual(boss["multiplier_a6"], -5)
        self.assertEqual(boss["multiplier_aa"], -5)
        self.assertTrue(ali_ground["lock_1982"])
        self.assertTrue(ali_ground["flags_36"] & 0x02)
        self.assertEqual(ali_ground["flags_38"] & 0x0C, 0x0C)

    def test_state04_return_ascend(self):
        boss = {"z_10": 0, "state": 4}
        res = step_state04_return_ascend(boss, at_top_boundary=False)
        self.assertFalse(res)
        self.assertEqual(boss["z_10"], 4)

        res = step_state04_return_ascend(boss, at_top_boundary=True)
        self.assertTrue(res)
        self.assertEqual(boss["state"], 0)
        self.assertEqual(boss["anim_id"], 0x12)

    def test_state06_slam_setup_branching(self):
        boss = {"z_10": 0}
        res = step_state06_slam_setup(boss, arena_x=0x80, at_top_boundary=False)
        self.assertEqual(res, "ASCENDING")
        self.assertEqual(boss["z_10"], 4)

        # arena_x <= 0x100 -> branch right (State 0x0A)
        res = step_state06_slam_setup(boss, arena_x=0x80, at_top_boundary=True)
        self.assertEqual(res, "BRANCH_RIGHT")
        self.assertEqual(boss["state"], 0x0A)
        self.assertEqual(boss["timer"], 7)

        # arena_x > 0x100 -> branch left (State 0x14)
        res = step_state06_slam_setup(boss, arena_x=0x150, at_top_boundary=True)
        self.assertEqual(res, "BRANCH_LEFT")
        self.assertEqual(boss["state"], 0x14)
        self.assertEqual(boss["timer"], 7)

    def test_rightward_slam_spit_sequence(self):
        boss = {"state": 0x0A, "timer": 1, "delta_x_72": 0}
        ali = {"lock_1982": True, "flags_36": 2, "flags_38": 0x0C, "z_10": 50, "delta_x_72": 10}

        # Windup left
        res = step_rightward_slam(boss, ali)
        self.assertEqual(res, "ENTER_LUNGE")
        self.assertEqual(boss["state"], 0x0C)
        self.assertEqual(boss["timer"], 5)

        # Lunge right & release
        boss["timer"] = 1
        res = step_rightward_slam(boss, ali)
        self.assertEqual(res, "RELEASE_RIGHT")
        self.assertEqual(boss["state"], 0x0E)
        self.assertEqual(boss["timer"], 13)
        self.assertTrue(boss["flags_38"] & 0x08)
        self.assertFalse(ali["lock_1982"])
        self.assertEqual(ali["damage_86"], 0x8014)
        self.assertEqual(ali["x_vel_4e"], 0x00200000)
        self.assertEqual(ali["z_10"], 18)
        self.assertEqual(ali["delta_x_72"], 14)
        self.assertEqual(ali["facing_16"], 1)
        self.assertEqual(ali["knockback_mode_84"], 1)

        # Recovery left
        boss["timer"] = 1
        res = step_rightward_slam(boss, ali)
        self.assertEqual(res, "RESET_STATE0")
        self.assertEqual(boss["state"], 0)
        self.assertFalse(boss["flags_38"] & 0x08)

    def test_leftward_slam_spit_sequence(self):
        boss = {"state": 0x14, "timer": 1, "delta_x_72": 0}
        ali = {"lock_1982": True, "flags_36": 2, "flags_38": 0x0C, "z_10": 50, "delta_x_72": 10}

        # Windup right
        res = step_leftward_slam(boss, ali)
        self.assertEqual(res, "ENTER_LUNGE")
        self.assertEqual(boss["state"], 0x16)
        self.assertEqual(boss["timer"], 5)

        # Lunge left & release
        boss["timer"] = 1
        res = step_leftward_slam(boss, ali)
        self.assertEqual(res, "RELEASE_LEFT")
        self.assertEqual(boss["state"], 0x18)
        self.assertEqual(boss["timer"], 13)
        self.assertEqual(ali["damage_86"], 0x8014)
        self.assertEqual(ali["x_vel_4e"], -0x00200000)
        self.assertEqual(ali["z_10"], 18)
        self.assertEqual(ali["delta_x_72"], 6)
        self.assertEqual(ali["knockback_mode_84"], 3)

        # Recovery right
        boss["timer"] = 1
        res = step_leftward_slam(boss, ali)
        self.assertEqual(res, "RESET_STATE0")
        self.assertEqual(boss["state"], 0)

    def test_state10_death_gravity_fall_and_stagger(self):
        boss = {"z_10": 0x20000, "z_vel_56": 0}
        segments = [{} for _ in range(4)]

        # Still falling
        res = step_state10_death_fall(boss, segments)
        self.assertEqual(res, "FALLING")
        self.assertEqual(boss["z_vel_56"], -0xC000)
        self.assertEqual(boss["z_10"], 0x14000)

        # Hits ground
        boss["z_10"] = 0x4000
        res = step_state10_death_fall(boss, segments)
        self.assertEqual(res, "IMPACT_TRIGGERED")
        self.assertEqual(boss["z_10"], 0)
        # Check 4 segment staggered timers and mode
        expected_timers = [4, 8, 12, 16]
        for i, seg in enumerate(segments):
            self.assertEqual(seg["mode_66"], 3)
            self.assertEqual(seg["timer_2"], expected_timers[i])


if __name__ == "__main__":
    unittest.main()
