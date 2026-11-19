#!/usr/bin/env python3
"""
Unit tests for ROM Multi-Part Boss/Golem (0x9A) Phase Damage Handler, Plane A Tilemap
Damage Patches, Tentacle/Arm Sub-State Machine (0..4) & Spore Homing (0x015758..0x0159E2).
Milestone M15.510.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

EVIDENCE_PATH = (
    Path(__file__).resolve().parent.parent
    / "data"
    / "thor-evidence"
    / "rom_golem_boss_damage_arm_m15_510.json"
)


def step_golem_hit_damage(golem: dict, death_callback=None, debris_callback=None, patch_callback=None):
    """
    Emulates 0x015758..0x015850 Golem Body Hit Damage and 2-Phase Fracture Check.
    Returns: bool indicating whether routine jumped to tentacle arm (True) or exited early on death (False).
    """
    pending_damage = golem.get("damage_in", 0)
    if pending_damage == 0:
        return True  # beq.w 0x01585C

    golem["damage_in"] = 0
    golem["hit_flag_82"] = 0

    if golem.get("state", 0) == 4:
        return True  # cmpi.w #4, $4(a6); beq.w 0x01585C

    masked_damage = pending_damage & 0x0FFF
    new_hp = golem.get("hp", 0) - masked_damage
    if new_hp < 0:
        new_hp = 0
    golem["hp"] = new_hp

    if new_hp == 0:
        if death_callback:
            death_callback(mode=7)
        return False  # rts on death

    golem["state"] = 4
    golem["anim_id"] = 6
    golem["hitstun_timer"] = 8

    max_hp = golem.get("max_hp", 0)
    cur_hp = golem.get("hp", 0)
    phase = golem.get("fracture_phase", 0)

    if phase == 0:
        # Phase 0 check at 0x015808
        threshold = max_hp >> 1
        if cur_hp <= threshold:
            golem["fracture_phase"] = 1
            golem["flag_ff2a3e_bit1"] = 1
            if debris_callback:
                body_x = golem.get("body_x", 0)
                body_y = golem.get("body_y", 0)
                body_z = golem.get("body_z", 0)
                debris_callback(particle_type=3, x=body_x - 128, y=body_y + 8, z=body_z)
            if patch_callback:
                patch_callback(phase=0, rect=[0, 32, 5, 37], dest=[32, 17])
        return True

    elif phase == 1:
        # Phase 1 check at 0x0157B2
        threshold = max_hp >> 2
        if cur_hp <= threshold:
            golem["fracture_phase"] = 2
            if debris_callback:
                body_x = golem.get("body_x", 0)
                body_y = golem.get("body_y", 0)
                body_z = golem.get("body_z", 0)
                debris_callback(particle_type=3, x=body_x - 136, y=body_y - 64, z=body_z)
            if patch_callback:
                patch_callback(phase=1, rect=[6, 32, 11, 37], dest=[33, 12])
        return True

    return True


def step_tentacle_arm(arm: dict, ali: dict, steer_fn, sfx_callback=None):
    """
    Emulates 0x01585C..0x0159B2 Tentacle Arm Sub-State Machine (States 0..4).
    """
    state = arm.get("state", 0)
    if state == 0:
        arm["state"] = 0
        return "idle"

    elif state == 1:
        # State 1: Extend / Attack
        arm["timer"] -= 1
        if arm["timer"] == 0:
            arm["state"] = 2
            arm["anim_id"] = 10
            return "timeout_to_retract"

        ali_tx = ali.get("target_x", 0)
        ali_ty = ali.get("target_y", 0)
        d4, d1, d2 = steer_fn(arm, ali_tx, ali_ty)

        if d4 <= 4:
            # Check Ali guard
            if (ali.get("flag_38", 0) & (1 << 3)) != 0 or ali.get("hit_flag_82", 0) != 0:
                arm["state"] = 2
                arm["anim_id"] = 10
                return "blocked_to_retract"

            # Apply hit to Ali
            ali["attacker_ptr"] = arm.get("ptr", 0)
            ali["hit_flag_82"] = 0
            ali_damage = 10
            if ali.get("z", 0) != 0:
                ali_damage |= 0x8000
            ali["damage_in"] = ali_damage
            ali["flag_9d"] = 0
            ali["hit_reaction_mode"] = 3
            ali["vx"] = -0x00050000
            ali["vy"] = 0
            ali["vz"] = 0x00020000
            ali["hitstun_timer"] = 14

            if sfx_callback:
                sfx_callback(0x800381)

            arm["state"] = 3
            arm["timer"] = 8
            arm["anim_id"] = 10
            return "hit_ali_to_cooldown"

        else:
            # Move at 2x speed
            d1_int = (d1 >> 16) if abs(d1) > 0xFFFF else d1
            vx = d1_int * 2
            vy = d2 * 2
            arm["offset_1e"] = arm.get("offset_1e", 0) + vx
            arm["pos_76"] = arm.get("pos_76", 0) + vy
            return "extending"

    elif state == 2:
        # State 2: Retract toward Home
        home_x = arm.get("home_x", 0)
        home_y = arm.get("home_y", 0)
        d4, d1, d2 = steer_fn(arm, home_x, home_y)

        if d4 <= 4:
            arm["state"] = 0
            return "retract_complete_to_idle"
        else:
            d1_int = (d1 >> 16) if abs(d1) > 0xFFFF else d1
            vx = d1_int * 2
            vy = d2 * 2
            arm["offset_1e"] = arm.get("offset_1e", 0) + vx
            arm["pos_76"] = arm.get("pos_76", 0) + vy
            return "retracting"

    elif state == 3:
        # State 3: Cooldown
        arm["timer"] -= 1
        if arm["timer"] == 0:
            arm["state"] = 2
            return "cooldown_to_retract"
        return "cooldown"

    elif state == 4:
        # State 4: Hitstun Retract
        arm["hitstun_timer"] -= 1
        if arm["hitstun_timer"] == 0:
            arm["flag_36"] = arm.get("flag_36", 0) & ~(1 << 6)
            arm["flag_39"] = arm.get("flag_39", 0) & ~(1 << 3)
            arm["state"] = 2
            arm["anim_id"] = 10
            return "hitstun_expired_to_retract"
        else:
            arm["flag_36"] = arm.get("flag_36", 0) | (1 << 6)
            home_x = arm.get("home_x", 0)
            home_y = arm.get("home_y", 0)
            d4, d1, d2 = steer_fn(arm, home_x, home_y)
            if d4 > 2:
                d1_int = (d1 >> 16) if abs(d1) > 0xFFFF else d1
                arm["offset_1e"] = arm.get("offset_1e", 0) + d1_int
                arm["pos_76"] = arm.get("pos_76", 0) + d2
            return "hitstun_retracting"

    return "unknown"


def step_spore_homing(spore: dict, parent: dict, steer_fn):
    """
    Emulates 0x0159B4..0x0159E0 Spore Homing Return (+A2 == 3).
    """
    parent_x = parent.get("x", 0)
    parent_y = parent.get("y", 0)
    d4, d1, d2 = steer_fn(spore, parent_x, parent_y)

    if d4 <= 10:
        spore["type"] = 0
        return "despawned"
    else:
        # 4x speed
        vx = d1 * 4
        vy = d2 * 4
        spore["pos_72"] = spore.get("pos_72", 0) + vx
        spore["pos_76"] = spore.get("pos_76", 0) + vy
        return "homing"


class TestROMGolemBossDamageArm(unittest.TestCase):
    def setUp(self):
        self.evidence = json.loads(EVIDENCE_PATH.read_text())

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.510")
        self.assertEqual(meta["range_start"], "0x015758")
        self.assertEqual(meta["range_end"], "0x0159E2")
        self.assertEqual(meta["range_size_bytes"], 650)
        self.assertEqual(meta["total_instructions"], 160)
        self.assertEqual(meta["code_segment_count"], 3)
        self.assertEqual(meta["tilemap_patch_record_count"], 2)

        segs = self.evidence["code_segments"]
        self.assertEqual(len(segs), 3)
        self.assertEqual(segs[0]["name"], "AI_GolemBoss_BodyHitDamage")
        self.assertEqual(segs[0]["instruction_count"], 42)
        self.assertEqual(segs[0]["size_bytes"], 164)
        self.assertEqual(segs[1]["name"], "AI_GolemBoss_Phase0Check")
        self.assertEqual(segs[1]["instruction_count"], 18)
        self.assertEqual(segs[1]["size_bytes"], 72)
        self.assertEqual(segs[2]["name"], "AI_GolemBoss_TentacleArmAndSporeHoming")
        self.assertEqual(segs[2]["instruction_count"], 100)
        self.assertEqual(segs[2]["size_bytes"], 390)

    def test_tilemap_patch_records(self):
        records = self.evidence["tilemap_patch_records"]
        self.assertEqual(len(records), 2)

        p1 = records[0]
        self.assertEqual(p1["phase"], 1)
        self.assertEqual(p1["address"], "0x0157FC")
        self.assertEqual(p1["source_rect"], [6, 32, 11, 37])
        self.assertEqual(p1["dest_coord"], [33, 12])
        self.assertEqual(p1["width_tiles"], 5)
        self.assertEqual(p1["height_tiles"], 5)
        self.assertEqual(p1["raw_hex"], "00060020000b00250021000c")

        p0 = records[1]
        self.assertEqual(p0["phase"], 0)
        self.assertEqual(p0["address"], "0x015850")
        self.assertEqual(p0["source_rect"], [0, 32, 5, 37])
        self.assertEqual(p0["dest_coord"], [32, 17])
        self.assertEqual(p0["width_tiles"], 5)
        self.assertEqual(p0["height_tiles"], 5)
        self.assertEqual(p0["raw_hex"], "000000200005002500200011")

    def test_golem_damage_and_death(self):
        death_called = []
        golem = {"hp": 10, "max_hp": 200, "damage_in": 0x100A, "state": 0}
        cont = step_golem_hit_damage(golem, death_callback=lambda mode: death_called.append(mode))
        self.assertFalse(cont)
        self.assertEqual(golem["hp"], 0)
        self.assertEqual(death_called, [7])

    def test_golem_damage_state4_bypass(self):
        golem = {"hp": 150, "max_hp": 200, "damage_in": 25, "state": 4}
        cont = step_golem_hit_damage(golem)
        self.assertTrue(cont)
        self.assertEqual(golem["hp"], 150)
        self.assertEqual(golem["damage_in"], 0)

    def test_phase0_fracture_threshold(self):
        debris_events = []
        patch_events = []
        golem = {
            "hp": 200,
            "max_hp": 200,
            "damage_in": 105,  # 200 - 105 = 95 <= 100 (50%)
            "fracture_phase": 0,
            "body_x": 300,
            "body_y": 200,
            "body_z": 0,
        }
        cont = step_golem_hit_damage(
            golem,
            debris_callback=lambda **kwargs: debris_events.append(kwargs),
            patch_callback=lambda **kwargs: patch_events.append(kwargs),
        )
        self.assertTrue(cont)
        self.assertEqual(golem["hp"], 95)
        self.assertEqual(golem["fracture_phase"], 1)
        self.assertEqual(golem["flag_ff2a3e_bit1"], 1)
        self.assertEqual(debris_events, [{"particle_type": 3, "x": 172, "y": 208, "z": 0}])
        self.assertEqual(patch_events, [{"phase": 0, "rect": [0, 32, 5, 37], "dest": [32, 17]}])

    def test_phase1_fracture_threshold(self):
        debris_events = []
        patch_events = []
        golem = {
            "hp": 90,
            "max_hp": 200,
            "damage_in": 50,  # 90 - 50 = 40 <= 50 (25%)
            "fracture_phase": 1,
            "body_x": 300,
            "body_y": 200,
            "body_z": 0,
        }
        cont = step_golem_hit_damage(
            golem,
            debris_callback=lambda **kwargs: debris_events.append(kwargs),
            patch_callback=lambda **kwargs: patch_events.append(kwargs),
        )
        self.assertTrue(cont)
        self.assertEqual(golem["hp"], 40)
        self.assertEqual(golem["fracture_phase"], 2)
        self.assertEqual(debris_events, [{"particle_type": 3, "x": 164, "y": 136, "z": 0}])
        self.assertEqual(patch_events, [{"phase": 1, "rect": [6, 32, 11, 37], "dest": [33, 12]}])

    def test_tentacle_arm_extend_and_ali_hit(self):
        sfx_list = []
        arm = {"state": 1, "timer": 10, "offset_1e": 0, "pos_76": 0, "ptr": 0xFF1E50}
        ali_vuln = {"target_x": 100, "target_y": 100, "z": 0, "flag_38": 0, "hit_flag_82": 0}

        # 1. Close distance hit
        res = step_tentacle_arm(
            arm, ali_vuln,
            steer_fn=lambda a, tx, ty: (3, 2, 2),  # d4 = 3 <= 4
            sfx_callback=lambda sfx: sfx_list.append(sfx),
        )
        self.assertEqual(res, "hit_ali_to_cooldown")
        self.assertEqual(arm["state"], 3)
        self.assertEqual(arm["timer"], 8)
        self.assertEqual(arm["anim_id"], 10)
        self.assertEqual(ali_vuln["damage_in"], 10)
        self.assertEqual(ali_vuln["vx"], -0x00050000)
        self.assertEqual(ali_vuln["vz"], 0x00020000)
        self.assertEqual(ali_vuln["hitstun_timer"], 14)
        self.assertEqual(sfx_list, [0x800381])

        # 2. Airborne hit gives bit 15
        arm2 = {"state": 1, "timer": 10, "offset_1e": 0, "pos_76": 0}
        ali_air = {"target_x": 100, "target_y": 100, "z": 15, "flag_38": 0, "hit_flag_82": 0}
        step_tentacle_arm(arm2, ali_air, steer_fn=lambda a, tx, ty: (2, 1, 1))
        self.assertEqual(ali_air["damage_in"], 0x800A)

        # 3. Guarded Ali transitions to State 2 without hit
        arm3 = {"state": 1, "timer": 10, "offset_1e": 0, "pos_76": 0}
        ali_guard = {"target_x": 100, "target_y": 100, "z": 0, "flag_38": (1 << 3), "hit_flag_82": 0}
        res3 = step_tentacle_arm(arm3, ali_guard, steer_fn=lambda a, tx, ty: (2, 1, 1))
        self.assertEqual(res3, "blocked_to_retract")
        self.assertEqual(arm3["state"], 2)

    def test_tentacle_arm_cooldown_and_retract(self):
        arm = {"state": 3, "timer": 1, "home_x": 50, "home_y": 50}
        ali = {}
        res = step_tentacle_arm(arm, ali, steer_fn=lambda a, tx, ty: (10, 2, 2))
        self.assertEqual(res, "cooldown_to_retract")
        self.assertEqual(arm["state"], 2)

        # Retracting at distance > 4
        res_retract = step_tentacle_arm(arm, ali, steer_fn=lambda a, tx, ty: (10, 2, 3))
        self.assertEqual(res_retract, "retracting")
        self.assertEqual(arm["offset_1e"], 4)
        self.assertEqual(arm["pos_76"], 6)

        # Retract reaches home when distance <= 4
        res_home = step_tentacle_arm(arm, ali, steer_fn=lambda a, tx, ty: (3, 0, 0))
        self.assertEqual(res_home, "retract_complete_to_idle")
        self.assertEqual(arm["state"], 0)

    def test_tentacle_arm_hitstun_state4(self):
        arm = {"state": 4, "hitstun_timer": 3, "home_x": 50, "home_y": 50}
        ali = {}
        # Timer > 0: moves toward home at 1x speed
        res = step_tentacle_arm(arm, ali, steer_fn=lambda a, tx, ty: (5, 2, 3))
        self.assertEqual(res, "hitstun_retracting")
        self.assertEqual(arm["hitstun_timer"], 2)
        self.assertEqual(arm["flag_36"], 1 << 6)
        self.assertEqual(arm["offset_1e"], 2)  # 1x speed
        self.assertEqual(arm["pos_76"], 3)  # 1x speed

        # Timer reaches 0: clears bits and switches to State 2
        arm["hitstun_timer"] = 1
        res_exp = step_tentacle_arm(arm, ali, steer_fn=lambda a, tx, ty: (5, 2, 3))
        self.assertEqual(res_exp, "hitstun_expired_to_retract")
        self.assertEqual(arm["state"], 2)
        self.assertEqual(arm["flag_36"] & (1 << 6), 0)

    def test_spore_homing_return(self):
        parent = {"x": 200, "y": 150}
        spore = {"type": 0x9A, "pos_72": 100, "pos_76": 50}

        # Distance > 10: homing at 4x speed
        res = step_spore_homing(spore, parent, steer_fn=lambda s, px, py: (20, 2, 3))
        self.assertEqual(res, "homing")
        self.assertEqual(spore["pos_72"], 108)  # 100 + 2*4
        self.assertEqual(spore["pos_76"], 62)   # 50 + 3*4

        # Distance <= 10: despawn
        res_despawn = step_spore_homing(spore, parent, steer_fn=lambda s, px, py: (8, 1, 1))
        self.assertEqual(res_despawn, "despawned")
        self.assertEqual(spore["type"], 0)


if __name__ == "__main__":
    unittest.main()
