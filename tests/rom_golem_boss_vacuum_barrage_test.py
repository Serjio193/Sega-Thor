#!/usr/bin/env python3
"""
Unit tests for ROM Multi-Part Boss/Golem (0x9A) Vacuum Pull, Boulder Barrage Spawner,
Body Knockback & Camera-Relative SFX Helpers (0x0154E8..0x015758).
Milestone M15.509.
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_golem_boss_vacuum_barrage_m15_509.json"
)


def step_vacuum_pull(boss, ali, pull_delta_fn):
    """
    Emulate 0x0154E8..0x01552C State $04 Vacuum Pull.
    pull_delta_fn(boss, ali): returns (dx, dy) direction vector from 0x00E45E.
    Returns: (spore_spawned, reset_state)
    """
    boss["timer"] -= 1
    if boss["timer"] == 0:
        boss["state"] = 0
        boss["timer"] = 0x28
        return False, True

    spore_spawned = (boss["timer"] & 3) == 0

    # 0x00E45E directional vector
    d1, d2 = pull_delta_fn(boss, ali)

    # Scale 2x
    d1 = d1 * 2
    d2 = d2 * 2

    # If airborne, scale another 2x (4x total)
    if ali.get("z", 0) != 0:
        d1 = d1 * 2
        d2 = d2 * 2

    # Invert and pull towards boss
    d1 = -d1
    d2 = -d2
    ali["x"] = ali.get("x", 0) + d1
    ali["y"] = ali.get("y", 0) + d2

    return spore_spawned, False


def step_boulder_barrage(boss, ali, prng_seq, pool_alloc_ok=True):
    """
    Emulate 0x01552E..0x01563A State $02 Boulder Barrage.
    Returns: (spawned_boulder_or_None, reset_triggered)
    """
    boss["timer"] -= 1
    if boss["timer"] >= 0:
        return None, False

    boss["burst_counter"] -= 1
    if boss["burst_counter"] == 0:
        boss["state"] = 0
        boss["timer"] = 0x28
        return None, True

    # Next interval: (prng & 0xF) + 2
    prng_val = prng_seq.pop(0) if prng_seq else 0
    boss["timer"] = (prng_val & 0x0F) + 2

    if not pool_alloc_ok:
        return None, False

    is_aimed = (boss["burst_counter"] & 7) == 0

    boulder = {
        "bounds": 6,
        "height": 16,
        "anim_id": 0x0C,
        "state": 0,
        "sub_mode": 2,
        "sfx": 0x70035C,
        "x": boss.get("x", 0) - 0x30,
        "y": boss.get("y", 0),
        "is_aimed": is_aimed
    }

    if is_aimed:
        # Aimed at Ali with speed 0xC0000, vz = 0x80000, initial z = 6
        boulder["speed"] = 0xC0000
        boulder["z"] = 6
        boulder["z_vel"] = 0x80000
    else:
        # Scatter velocities from PRNG, initial z = 10
        p1 = prng_seq.pop(0) if prng_seq else 0
        p2 = prng_seq.pop(0) if prng_seq else 0
        p3 = prng_seq.pop(0) if prng_seq else 0
        boulder["x_vel"] = (p1 << 1) - 0xA8000
        boulder["y_vel"] = (p2 << 2) - 0x10000
        boulder["z_vel"] = p3 + 0x80000
        boulder["z"] = 10

    return boulder, False


def body_contact_knockback(ali, boss):
    """Emulate 0x015648..0x01569C Body Contact Knockback."""
    is_immune = (ali.get("flags_38", 0) & 0x08) != 0 or ali.get("guard_82", False)

    if is_immune:
        # Nudge X left by -0x40000 (-4.0 fixed)
        ali["x"] = ali.get("x", 0) - 0x40000
        return "NUDGED"

    ali["attacker"] = boss
    ali["guard_82"] = False
    ali["damage_86"] = 0x800A
    ali["field_9d"] = False
    ali["knockback_mode_84"] = 3
    ali["x_vel"] = -0x100000  # -16.0 fixed
    ali["y_vel"] = 0
    ali["z_vel"] = 0x30000    # +3.0 fixed
    ali["sfx"] = 0x800208
    return "KNOCKED_BACK"


def spawn_spore_rock(boss, prng1, prng2, pool_alloc_ok=True):
    """Emulate 0x01569E..0x015718 Spore/Rock Debris Spawner."""
    if not pool_alloc_ok:
        return None

    # Calculate offsets
    dx = -176 + (prng1 >> 4)
    # (prng2 >> 2) + (prng2 >> 3) == (prng2 * 3) // 8
    dy = (prng2 >> 2) + (prng2 >> 3)

    return {
        "x": boss.get("x", 0) + dx,
        "y": boss.get("y", 0) + dy,
        "z": 30,
        "flags_38": 0x04,  # bit 2 flying
        "anim_id": 0x10,
        "state": 0,
        "sub_mode": 3,
        "parent": boss,
        "sfx": 0x80034F
    }


def camera_relative_sfx_coords(camera_anchor_x, camera_anchor_y):
    """Emulate 0x01571A..0x01573A Camera-Relative SFX Coordinates."""
    return camera_anchor_x - 0x30, camera_anchor_y


def check_sub_mode_4(prev_entity):
    """Emulate 0x01573C..0x015756 Sub-Mode 4 Check on slot -0xBC."""
    if prev_entity.get("type", 0) == 0x34 and prev_entity.get("anim_timer_30", 0) == 8:
        return True, 3  # Triggers 0x00E96C with mode 3
    return False, None


class TestROMGolemBossVacuumBarrage(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.509")
        self.assertEqual(self.evidence["range"]["start"], "0x0154E8")
        self.assertEqual(self.evidence["range"]["end"], "0x015758")
        self.assertEqual(self.evidence["range"]["size_bytes"], 624)
        self.assertEqual(len(self.evidence["routines"]), 7)
        self.assertEqual(self.evidence["instruction_count"], 151)

    def test_vacuum_pull_scaling_and_timer(self):
        boss = {"timer": 5, "state": 4}
        ali_ground = {"x": 300, "y": 200, "z": 0}
        dummy_dir = lambda b, a: (10, 5)

        # Frame 1: timer 5 -> 4. Every 4th frame -> spore spawned! Ground scaling = 2x
        # pull = -(10*2, 5*2) = (-20, -10)
        spore, reset = step_vacuum_pull(boss, ali_ground, dummy_dir)
        self.assertTrue(spore)
        self.assertFalse(reset)
        self.assertEqual(ali_ground["x"], 280)
        self.assertEqual(ali_ground["y"], 190)
        self.assertEqual(boss["timer"], 4)

        # Frame 2: timer 4 -> 3. Not 4th frame.
        spore, reset = step_vacuum_pull(boss, ali_ground, dummy_dir)
        self.assertFalse(spore)
        self.assertEqual(boss["timer"], 3)

        # Airborne scaling: 4x pull
        ali_air = {"x": 300, "y": 200, "z": 10}
        boss["timer"] = 3
        # pull = -(10*4, 5*4) = (-40, -20)
        spore, reset = step_vacuum_pull(boss, ali_air, dummy_dir)
        self.assertEqual(ali_air["x"], 260)
        self.assertEqual(ali_air["y"], 180)

        # Expiration: timer reaches 0 -> reset state to 0, timer 0x28
        boss["timer"] = 1
        spore, reset = step_vacuum_pull(boss, ali_air, dummy_dir)
        self.assertTrue(reset)
        self.assertEqual(boss["state"], 0)
        self.assertEqual(boss["timer"], 0x28)

    def test_boulder_barrage_spawner(self):
        # burst_counter = 17 decrements to 16, which is divisible by 8 -> Aimed boulder!
        boss = {"x": 200, "y": 150, "timer": 0, "burst_counter": 17}
        ali = {"x": 100, "y": 150}
        prng = [0x05, 0x10, 0x20, 0x30]  # interval = (5 & 0xF) + 2 = 7

        # 17 decremented to 16 -> (16 & 7) == 0 -> Aimed boulder!
        boulder, reset = step_boulder_barrage(boss, ali, prng)
        self.assertFalse(reset)
        self.assertIsNotNone(boulder)
        self.assertTrue(boulder["is_aimed"])
        self.assertEqual(boulder["speed"], 0xC0000)
        self.assertEqual(boulder["z"], 6)
        self.assertEqual(boulder["z_vel"], 0x80000)
        self.assertEqual(boulder["x"], 200 - 0x30)
        self.assertEqual(boulder["y"], 150)
        self.assertEqual(boss["timer"], 7)
        self.assertEqual(boss["burst_counter"], 16)

        # Next boulder: 16 decremented to 15 -> (15 & 7) != 0 -> Scatter boulder!
        boss["timer"] = 0
        prng_scatter = [0x03, 0x1000, 0x2000, 0x3000]
        boulder_scatter, reset = step_boulder_barrage(boss, ali, prng_scatter)
        self.assertFalse(boulder_scatter["is_aimed"])
        self.assertEqual(boulder_scatter["z"], 10)
        self.assertEqual(boss["burst_counter"], 15)

        # Burst exhaustion: burst_counter reaches 0 -> resets to State 0, timer 0x28
        boss["burst_counter"] = 1
        boss["timer"] = 0
        b_none, reset = step_boulder_barrage(boss, ali, [0])
        self.assertTrue(reset)
        self.assertEqual(boss["state"], 0)
        self.assertEqual(boss["timer"], 0x28)

    def test_body_contact_knockback(self):
        boss = {"name": "golem"}
        ali_vuln = {"x": 100, "flags_38": 0, "guard_82": False}

        # Vulnerable hit: heavy knockback
        res = body_contact_knockback(ali_vuln, boss)
        self.assertEqual(res, "KNOCKED_BACK")
        self.assertEqual(ali_vuln["damage_86"], 0x800A)
        self.assertEqual(ali_vuln["knockback_mode_84"], 3)
        self.assertEqual(ali_vuln["x_vel"], -0x100000)
        self.assertEqual(ali_vuln["z_vel"], 0x30000)
        self.assertEqual(ali_vuln["sfx"], 0x800208)

        # Guarding or invulnerable Ali: nudged left by 0x40000 without damage
        ali_guard = {"x": 0x100000, "flags_38": 0x08, "guard_82": True}
        res_guard = body_contact_knockback(ali_guard, boss)
        self.assertEqual(res_guard, "NUDGED")
        self.assertEqual(ali_guard["x"], 0x100000 - 0x40000)
        self.assertNotIn("damage_86", ali_guard)

    def test_spore_rock_spawner(self):
        boss = {"x": 300, "y": 200}
        # prng1 = 0x80 -> prng1 >> 4 = 8 -> x = 300 - 176 + 8 = 132
        # prng2 = 0x40 (64) -> (64>>2) + (64>>3) = 16 + 8 = 24 -> y = 200 + 24 = 224
        spore = spawn_spore_rock(boss, prng1=0x80, prng2=0x40)
        self.assertIsNotNone(spore)
        self.assertEqual(spore["x"], 132)
        self.assertEqual(spore["y"], 224)
        self.assertEqual(spore["z"], 30)
        self.assertEqual(spore["flags_38"], 0x04)
        self.assertEqual(spore["anim_id"], 0x10)
        self.assertEqual(spore["sub_mode"], 3)
        self.assertEqual(spore["sfx"], 0x80034F)

    def test_camera_relative_sfx_and_sub_mode_4(self):
        # Camera coords: x - 0x30, y
        cx, cy = camera_relative_sfx_coords(0x150, 0x100)
        self.assertEqual(cx, 0x120)
        self.assertEqual(cy, 0x100)

        # Sub-mode 4 check on previous slot
        matching_slot = {"type": 0x34, "anim_timer_30": 8}
        triggered, mode = check_sub_mode_4(matching_slot)
        self.assertTrue(triggered)
        self.assertEqual(mode, 3)

        non_matching_slot = {"type": 0x34, "anim_timer_30": 5}
        triggered, mode = check_sub_mode_4(non_matching_slot)
        self.assertFalse(triggered)
        self.assertIsNone(mode)


if __name__ == "__main__":
    unittest.main()
