#!/usr/bin/env python3
"""
Unit tests for ROM Serpent Boss Phase 1 Body Contact AABB, Homing Orb Spawner,
PRNG Action Selector & 4-Limb Animation Broadcaster (0x016764..0x016A64).
Milestone M15.517.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

ROOT_DIR = Path(__file__).resolve().parent.parent
EVIDENCE_PATH = (
    ROOT_DIR
    / "data"
    / "thor-evidence"
    / "rom_serpent_boss_phase1_actions_m15_517.json"
)


def broadcast_4limb_anim(body_slots: list[dict], anim_id: int):
    """
    Emulates 0x016A46..0x016A62 4-Limb Animation Broadcaster.
    """
    for slot in body_slots[:4]:
        slot["anim_id"] = anim_id


def step_body_contact_aabb(
    boss: dict,
    ali: dict,
    contact_detected: bool,
) -> bool:
    """
    Emulates 0x016764..0x0167BA Phase 1 Body Contact AABB & Ali Knockback.
    Returns: bool (True if hit applied, False if no contact or guarded).
    """
    if not contact_detected:
        return False

    if (ali.get("flag_38", 0) & (1 << 3)) != 0:
        return False  # Ali guarded/immune

    ali["attacker_ptr"] = boss.get("ptr", 0)
    ali["damage_in"] = 0x8014
    ali["hitstun_timer"] = 5
    ali["flag_9d"] = 0
    ali["field_16"] = 2
    ali["reaction_mode"] = 2
    ali["vx"] = 0
    ali["vy"] = 0x00040000
    ali["vz"] = -1  # 0xFFFF
    return True


def spawn_homing_orb(
    boss: dict,
    angle: int,
    speed: int,
    prng_val: int = 0,
    pool_alloc_ok: bool = True,
    sfx_callback=None,
) -> dict | None:
    """
    Emulates 0x0167BE..0x016858 Phase 1 Homing Orb Spawner.
    """
    if not pool_alloc_ok:
        return None

    orb = {
        "state": 0x12,
        "tile_palette": 0x0200,
        "field_3d": 8,
        "sub_mode_66": 2,
        "flag_36": 0,
        "flag_37": 0,
        "flag_38": 4,  # flying bit
        "field_16": 1,
        "bounds_48": 8,
        "bounds_49": 8,
        "bounds_4a": 0x10,
        "bounds_46": 8,
        "x": boss.get("x", 0) - 1,
        "y": boss.get("y", 0) + 16,
        "angle_7a": angle,
        "speed_56": speed,
        "anim_id": 0x16,
        "damage_in": 0,
        "lifetime_a1": (prng_val & 7),
        "vx": 0,
        "vy": 0x00020000,
    }

    if sfx_callback:
        sfx_callback(0x70035C)

    return orb


def select_phase1_action(
    boss: dict,
    body_slots: list[dict],
    head_slot: dict,
    ali: dict,
    d4_dist: int,
    d3_height: int,
    d1_dx: int,
    d2_dy: int,
    prng_roll: int = 0,
    dialogue_lock: int = 0,
    sfx_callback=None,
) -> str:
    """
    Emulates 0x01685A..0x016A44 Phase 1 Action Selector.
    """
    broadcast_4limb_anim(body_slots, 0x28)

    if dialogue_lock != 0:
        boss["state"] = 0
        boss["anim_id"] = 0
        boss["timer_2"] = 15
        return "idle_lock"

    # Glide check: d4 < 24
    if d4_dist < 24:
        if d3_height < 32:
            # Dive Leap jump
            boss["state"] = 0x0C
            broadcast_4limb_anim(body_slots, 0x0C)
            if sfx_callback:
                sfx_callback(0x700219)
            return "dive_leap"

        # Glide State 2
        boss["timer_2"] = 0
        if d1_dx >= 0:
            # Right glide
            boss["field_9e"] = (d1_dx + 1) & 0xFF
            if len(body_slots) >= 4:
                body_slots[0]["anim_id"] = 0x20
                body_slots[1]["anim_id"] = 0x1A
                body_slots[2]["anim_id"] = 0x1E
                body_slots[3]["anim_id"] = 0x1C
            boss["state"] = 2
            boss["anim_id"] = 10
            boss["sub_mode_a2"] = 2
            if sfx_callback:
                sfx_callback(0x70021A)
            return "glide_right"
        else:
            # Left glide
            boss["field_9e"] = (d1_dx - 1) & 0xFF
            if len(body_slots) >= 4:
                body_slots[0]["anim_id"] = 0x1A
                body_slots[1]["anim_id"] = 0x20
                body_slots[2]["anim_id"] = 0x1C
                body_slots[3]["anim_id"] = 0x1E
            boss["state"] = 2
            boss["anim_id"] = 10
            boss["sub_mode_a2"] = -2
            if sfx_callback:
                sfx_callback(0x70021A)
            return "glide_left"

    # d4 >= 24
    if d2_dy < 0:
        boss["state"] = 0x0C
        broadcast_4limb_anim(body_slots, 0x0C)
        if sfx_callback:
            sfx_callback(0x700219)
        return "dive_leap"

    # d2 >= 0 -> PRNG roll 0..99
    if prng_roll < 30:
        # Fan Barrage
        if sfx_callback:
            sfx_callback(0x700219)
        boss["state"] = 0x12
        broadcast_4limb_anim(body_slots, 0x0C)
        if head_slot.get("type", 0) == 0x30 and head_slot.get("state", 0) != 0x12:
            boss["field_a0"] = 0
            boss["field_a1"] = 0x28
        else:
            boss["field_a0"] = 0x1E
            boss["field_a1"] = 0x16
        return "fan_barrage"

    elif prng_roll < 60:
        # Arm Grab
        if head_slot.get("type", 0) == 0x30 and head_slot.get("state", 0) != 0x12:
            boss["state"] = 0x0A
            boss["anim_id"] = 8
            head_slot["state"] = 8
            head_slot["anim_id"] = 0x32
            head_slot["timer_2"] = 5
            vx = ((ali.get("x", 0) - 4) - head_slot.get("x", 0)) >> 4
            vy = ((ali.get("y", 0) - 16) - head_slot.get("y", 0)) >> 4
            head_slot["delta_a6"] = vx
            head_slot["delta_aa"] = vy
            if sfx_callback:
                sfx_callback(0x700219)
            return "arm_grab"
        else:
            # Fallback to Fan Barrage
            if sfx_callback:
                sfx_callback(0x700219)
            boss["state"] = 0x12
            broadcast_4limb_anim(body_slots, 0x0C)
            boss["field_a0"] = 0x1E
            boss["field_a1"] = 0x16
            return "fan_barrage_fallback"

    elif prng_roll < 80:
        # State 0x04
        boss["state"] = 0x04
        broadcast_4limb_anim(body_slots, 0x0C)
        return "state_04"

    elif prng_roll < 90:
        # State 0x0C Dive Leap
        boss["state"] = 0x0C
        broadcast_4limb_anim(body_slots, 0x0C)
        if sfx_callback:
            sfx_callback(0x700219)
        return "dive_leap"

    else:
        # State 0 Idle
        boss["state"] = 0
        boss["anim_id"] = 0
        boss["timer_2"] = 15
        return "state_0_idle"


class TestROMSerpentBossPhase1Actions(unittest.TestCase):
    def setUp(self):
        self.evidence = json.loads(EVIDENCE_PATH.read_text())

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.517")
        self.assertEqual(meta["range_start"], "0x016764")
        self.assertEqual(meta["range_end"], "0x016A64")
        self.assertEqual(meta["range_size_bytes"], 768)
        self.assertEqual(meta["instruction_count"], 173)
        self.assertEqual(meta["routine_count"], 4)

        routines = self.evidence["routines"]
        self.assertEqual(routines[0]["instruction_count"], 22)
        self.assertEqual(routines[1]["instruction_count"], 31)
        self.assertEqual(routines[2]["instruction_count"], 110)
        self.assertEqual(routines[3]["instruction_count"], 10)

    def test_phase1_body_contact_aabb_and_knockback(self):
        boss = {"ptr": 0xFF1B00}
        ali_vuln = {"flag_38": 0}

        # 1. Successful hit on vulnerable Ali
        hit = step_body_contact_aabb(boss, ali_vuln, contact_detected=True)
        self.assertTrue(hit)
        self.assertEqual(ali_vuln["attacker_ptr"], 0xFF1B00)
        self.assertEqual(ali_vuln["damage_in"], 0x8014)
        self.assertEqual(ali_vuln["hitstun_timer"], 5)
        self.assertEqual(ali_vuln["field_16"], 2)
        self.assertEqual(ali_vuln["reaction_mode"], 2)
        self.assertEqual(ali_vuln["vx"], 0)
        self.assertEqual(ali_vuln["vy"], 0x00040000)
        self.assertEqual(ali_vuln["vz"], -1)

        # 2. Guarded Ali: hit ignored
        ali_guard = {"flag_38": (1 << 3)}
        hit_guard = step_body_contact_aabb(boss, ali_guard, contact_detected=True)
        self.assertFalse(hit_guard)
        self.assertNotIn("damage_in", ali_guard)

        # 3. No contact: returns False
        self.assertFalse(step_body_contact_aabb(boss, ali_vuln, contact_detected=False))

    def test_phase1_homing_orb_spawner(self):
        sfx_played = []
        boss = {"x": 200, "y": 150}

        # 1. Pool alloc failure
        fail_orb = spawn_homing_orb(boss, 0x40, 0x00020000, pool_alloc_ok=False)
        self.assertIsNone(fail_orb)

        # 2. Successful spawn
        orb = spawn_homing_orb(
            boss, angle=0x40, speed=0x00020000, prng_val=5,
            pool_alloc_ok=True, sfx_callback=lambda sfx: sfx_played.append(sfx),
        )
        self.assertIsNotNone(orb)
        self.assertEqual(orb["state"], 0x12)
        self.assertEqual(orb["tile_palette"], 0x0200)
        self.assertEqual(orb["flag_38"], 4)
        self.assertEqual(orb["x"], 199)  # 200 - 1
        self.assertEqual(orb["y"], 166)  # 150 + 16
        self.assertEqual(orb["angle_7a"], 0x40)
        self.assertEqual(orb["speed_56"], 0x00020000)
        self.assertEqual(orb["anim_id"], 0x16)
        self.assertEqual(orb["lifetime_a1"], 5)
        self.assertEqual(orb["vy"], 0x00020000)
        self.assertEqual(sfx_played, [0x70035C])

    def test_action_selector_dialogue_lock(self):
        boss = {}
        body = [{} for _ in range(4)]
        head = {}
        ali = {}
        res = select_phase1_action(
            boss, body, head, ali,
            d4_dist=10, d3_height=40, d1_dx=5, d2_dy=5,
            dialogue_lock=1,
        )
        self.assertEqual(res, "idle_lock")
        self.assertEqual(boss["state"], 0)
        self.assertEqual(boss["anim_id"], 0)
        self.assertEqual(boss["timer_2"], 15)

    def test_action_selector_glide_right_and_left(self):
        sfx_list = []
        boss = {}
        body = [{} for _ in range(4)]
        head = {}
        ali = {}

        # 1. Right glide: d4 < 24, d3 >= 32, d1 >= 0
        res_r = select_phase1_action(
            boss, body, head, ali,
            d4_dist=15, d3_height=35, d1_dx=8, d2_dy=10,
            sfx_callback=lambda s: sfx_list.append(s),
        )
        self.assertEqual(res_r, "glide_right")
        self.assertEqual(boss["state"], 2)
        self.assertEqual(boss["anim_id"], 10)
        self.assertEqual(boss["sub_mode_a2"], 2)
        self.assertEqual(boss["field_9e"], 9)  # 8 + 1
        self.assertEqual(body[0]["anim_id"], 0x20)
        self.assertEqual(body[1]["anim_id"], 0x1A)
        self.assertEqual(body[2]["anim_id"], 0x1E)
        self.assertEqual(body[3]["anim_id"], 0x1C)
        self.assertEqual(sfx_list, [0x70021A])

        # 2. Left glide: d4 < 24, d3 >= 32, d1 < 0
        sfx_list.clear()
        res_l = select_phase1_action(
            boss, body, head, ali,
            d4_dist=15, d3_height=35, d1_dx=-8, d2_dy=10,
            sfx_callback=lambda s: sfx_list.append(s),
        )
        self.assertEqual(res_l, "glide_left")
        self.assertEqual(boss["state"], 2)
        self.assertEqual(boss["sub_mode_a2"], -2)
        self.assertEqual(body[0]["anim_id"], 0x1A)
        self.assertEqual(body[1]["anim_id"], 0x20)
        self.assertEqual(body[2]["anim_id"], 0x1C)
        self.assertEqual(body[3]["anim_id"], 0x1E)
        self.assertEqual(sfx_list, [0x70021A])

    def test_action_selector_dive_leap_short_height_or_negative_y(self):
        boss = {}
        body = [{} for _ in range(4)]
        head = {}
        ali = {}

        # Short height: d4 < 24 but d3 < 32
        res1 = select_phase1_action(
            boss, body, head, ali,
            d4_dist=15, d3_height=20, d1_dx=5, d2_dy=5,
        )
        self.assertEqual(res1, "dive_leap")
        self.assertEqual(boss["state"], 0x0C)
        for slot in body:
            self.assertEqual(slot["anim_id"], 0x0C)

        # Negative dy: d4 >= 24 but d2 < 0
        res2 = select_phase1_action(
            boss, body, head, ali,
            d4_dist=30, d3_height=40, d1_dx=5, d2_dy=-5,
        )
        self.assertEqual(res2, "dive_leap")
        self.assertEqual(boss["state"], 0x0C)

    def test_action_selector_prng_rolls(self):
        boss = {}
        body = [{} for _ in range(4)]
        head = {"type": 0x30, "state": 0, "x": 100, "y": 80}
        ali = {"x": 164, "y": 144}

        # Roll 15 (< 30) -> Fan Barrage
        res_fb = select_phase1_action(
            boss, body, head, ali,
            d4_dist=30, d3_height=40, d1_dx=10, d2_dy=10, prng_roll=15,
        )
        self.assertEqual(res_fb, "fan_barrage")
        self.assertEqual(boss["state"], 0x12)
        self.assertEqual(boss["field_a0"], 0)
        self.assertEqual(boss["field_a1"], 0x28)

        # Roll 45 (30..59) -> Arm Grab
        res_ag = select_phase1_action(
            boss, body, head, ali,
            d4_dist=30, d3_height=40, d1_dx=10, d2_dy=10, prng_roll=45,
        )
        self.assertEqual(res_ag, "arm_grab")
        self.assertEqual(boss["state"], 0x0A)
        self.assertEqual(boss["anim_id"], 8)
        self.assertEqual(head["state"], 8)
        self.assertEqual(head["anim_id"], 0x32)
        self.assertEqual(head["timer_2"], 5)
        # vx = ((164 - 4) - 100) / 16 = 60 / 16 = 3
        # vy = ((144 - 16) - 80) / 16 = 48 / 16 = 3
        self.assertEqual(head["delta_a6"], 3)
        self.assertEqual(head["delta_aa"], 3)

        # Roll 70 (60..79) -> State 0x04
        res_04 = select_phase1_action(
            boss, body, head, ali,
            d4_dist=30, d3_height=40, d1_dx=10, d2_dy=10, prng_roll=70,
        )
        self.assertEqual(res_04, "state_04")
        self.assertEqual(boss["state"], 0x04)

        # Roll 85 (80..89) -> Dive Leap 0x0C
        res_dl = select_phase1_action(
            boss, body, head, ali,
            d4_dist=30, d3_height=40, d1_dx=10, d2_dy=10, prng_roll=85,
        )
        self.assertEqual(res_dl, "dive_leap")
        self.assertEqual(boss["state"], 0x0C)

        # Roll 95 (90..99) -> State 0 Idle
        res_id = select_phase1_action(
            boss, body, head, ali,
            d4_dist=30, d3_height=40, d1_dx=10, d2_dy=10, prng_roll=95,
        )
        self.assertEqual(res_id, "state_0_idle")
        self.assertEqual(boss["state"], 0)
        self.assertEqual(boss["timer_2"], 15)

    def test_4limb_anim_broadcaster(self):
        body = [{"anim_id": 0} for _ in range(6)]
        broadcast_4limb_anim(body, 0x2A)
        self.assertEqual(body[0]["anim_id"], 0x2A)
        self.assertEqual(body[1]["anim_id"], 0x2A)
        self.assertEqual(body[2]["anim_id"], 0x2A)
        self.assertEqual(body[3]["anim_id"], 0x2A)
        # 5th and 6th slots are untouched
        self.assertEqual(body[4]["anim_id"], 0)
        self.assertEqual(body[5]["anim_id"], 0)


if __name__ == "__main__":
    unittest.main()
