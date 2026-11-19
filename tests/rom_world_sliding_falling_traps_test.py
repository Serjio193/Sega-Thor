#!/usr/bin/env python3
"""
Unit tests for ROM World Interactive Objects: Sliding/Retracting Wall Trap, Falling Rock Boulder
& Impact AABB (0x0171C4..0x0173D6).
Milestone M15.521.
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
    / "rom_world_sliding_falling_traps_m15_521.json"
)


# ==============================================================================
# Emulation Routines
# ==============================================================================

def step_sliding_wall_bridge(
    wall: dict,
    persistent_flags: dict[int, bool],
    sfx_log: list[int],
    anim_log: list[int],
) -> None:
    """
    Emulates 0x0171C4..0x0172C0 Sliding/Retracting Wall or Bridge Trap.
    """
    flag_id = wall.get("persistent_id", 0)
    is_flag_set = persistent_flags.get(flag_id, False)
    state = wall.get("submode_66", 0)

    # State 0: Initialization
    if state == 0:
        wall["submode_66"] = 1
        wall["timer_2"] = wall.get("param_a0", 0)
        wall["z_10"] = wall.get("param_8c", 0)
        wall["anchor_x_a6"] = wall.get("x", 0)
        wall["offset_aa"] = 0
        return

    # State 1: Retracting / Retracted
    if state == 1:
        wall["x"] = wall.get("anchor_x_a6", 0)
        offset = wall.get("offset_aa", 0)
        if offset > 0:
            offset = max(0, offset - 8)
            wall["offset_aa"] = offset

        # Directional offset based on +$16 == 1
        if wall.get("facing_16", 0) == 1:
            wall["x"] += offset
        else:
            wall["x"] -= offset

        timer = wall.get("timer_2", 0)
        if timer > 0:
            timer -= 1
            wall["timer_2"] = timer
            if timer != 0:
                return

        # Check persistent bit via 0x018030
        if is_flag_set:
            return  # stays retracted

        # Trigger extend
        wall["submode_66"] = 2
        wall["timer_2"] = wall.get("param_9e", 0)
        sfx_log.append(0x700391)
        return

    # State 2: Extending / Extended
    if state == 2:
        wall["x"] = wall.get("anchor_x_a6", 0)
        wall["flag_37"] = wall.get("flag_37", 0) | (1 << 3)
        offset = wall.get("offset_aa", 0)

        if offset < 0x28:  # 40 px max
            offset = min(0x28, offset + 8)
            wall["offset_aa"] = offset
            wall["flag_37"] = wall.get("flag_37", 0) & ~(1 << 3)

        if wall.get("facing_16", 0) == 1:
            wall["x"] += offset
        else:
            wall["x"] -= offset

        anim_log.append(0)  # jsr $BC8E.l with d0 = 0

        # Check retract conditions
        retract = False
        if is_flag_set:
            retract = True
        else:
            timer = wall.get("timer_2", 0)
            if timer == 0:
                retract = True
            else:
                timer -= 1
                wall["timer_2"] = timer
                if timer == 0:
                    retract = True

        if retract:
            wall["submode_66"] = 1
            wall["timer_2"] = wall.get("param_a0", 0)


def step_falling_rock_boulder(
    rock: dict,
    ali: dict,
    in_aabb: bool,
    sfx_log: list[int],
    system_vars: dict,
) -> None:
    """
    Emulates 0x0172C2..0x017342 Falling Rock / Boulder Trap.
    """
    state = rock.get("submode_66", 0)

    # State 0: Init
    if state == 0:
        rock["submode_66"] = 0xFF
        rock["z_10"] = rock.get("param_8c", 0)
        rock["flag_38"] = rock.get("flag_38", 0) | (1 << 6)  # aerial collision
        return

    # Active tick: falling vs landed
    z = rock.get("z_10", 0)
    if z != 0:
        # Check impact AABB vs Ali
        step_falling_rock_impact_aabb(rock, ali, in_aabb, sfx_log)

        # Apply gravity -$C000 (-0.75 px in 16.16)
        vz = rock.get("vz_56", 0) - 0xC000
        rock["vz_56"] = vz
        z += vz
        rock["z_10"] = z

        if z < 0:
            rock["z_10"] = 0
            if rock.get("bounced_a6", 0) == 0:
                rock["bounced_a6"] = 0xFF
                rock["z_10"] = 1
                rock["vz_56"] = 0x00030000  # +3.0 px upward bounce
                system_vars["screen_shake_ff17c4"] = 3
                sfx_log.append(0x800386)
            else:
                rock["vz_56"] = 0
    else:
        # Landed: fade increment unless anchor deltas != 0
        fade = rock.get("fade_2e", 0) + 1
        dx = rock.get("delta_x_72", 0)
        dy = rock.get("delta_y_76", 0)
        if (dx | dy) != 0:
            fade -= 1
        rock["fade_2e"] = fade


def step_falling_rock_impact_aabb(
    rock: dict,
    ali: dict,
    in_aabb: bool,
    sfx_log: list[int],
) -> bool:
    """
    Emulates 0x017344..0x0173D4 Falling Rock / Hazard Impact AABB.
    """
    if not in_aabb:
        return False

    # Guard and invulnerability filters
    if (ali.get("flag_38", 0) & (1 << 3)) != 0:
        return False
    if ali.get("cooldown_82", 0) != 0:
        return False

    ali["attacker_ptr"] = rock.get("ptr", 0)
    ali["damage_86"] = 0x8007  # bit 15 set | 7 HP
    ali["cooldown_82"] = 0
    ali["hitstun_83"] = 0
    ali["flag_9d"] = 0
    ali["vz_56"] = 0x00020000
    ali["z_10"] = ali.get("z_10", 0) + 1

    # Direction bit 1 flip
    facing = ali.get("facing_16", 0) ^ 2
    ali["knockback_angle_84"] = facing

    # Directional knockback magnitudes $310 / $300
    ali["vx_4e"] = 0x310 if facing in (0, 1) else -0x310
    ali["vy_52"] = 0x300 if facing in (0, 2) else -0x300

    if ali.get("type_0", 0) == 2:
        sfx_log.append(0x900237)
    else:
        sfx_log.append(0x800346)
    return True


# ==============================================================================
# Unit Tests
# ==============================================================================

class TestROMWorldSlidingFallingTraps(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.521")
        self.assertEqual(meta["range_start"], "0x0171C4")
        self.assertEqual(meta["range_end"], "0x0173D6")
        self.assertEqual(meta["range_size_bytes"], 530)
        self.assertEqual(meta["instruction_count"], 137)
        self.assertEqual(meta["routine_count"], 3)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertIn("AI_World_SlidingRetractingWallBridgeTrap", routines)
        self.assertIn("AI_World_FallingRockBoulderTrap", routines)
        self.assertIn("AI_World_FallingRockImpactAABB", routines)

        self.assertEqual(routines["AI_World_SlidingRetractingWallBridgeTrap"]["instruction_count"], 67)
        self.assertEqual(routines["AI_World_SlidingRetractingWallBridgeTrap"]["size_bytes"], 254)
        self.assertEqual(routines["AI_World_FallingRockBoulderTrap"]["instruction_count"], 32)
        self.assertEqual(routines["AI_World_FallingRockBoulderTrap"]["size_bytes"], 130)
        self.assertEqual(routines["AI_World_FallingRockImpactAABB"]["instruction_count"], 38)
        self.assertEqual(routines["AI_World_FallingRockImpactAABB"]["size_bytes"], 146)

    # --------------------------------------------------------------------------
    # Sliding / Retracting Wall Trap Tests
    # --------------------------------------------------------------------------

    def test_sliding_wall_init_submode_0(self):
        wall = {"submode_66": 0, "param_a0": 30, "param_8c": 16, "x": 120}
        sfx, anim = [], []
        step_sliding_wall_bridge(wall, persistent_flags={}, sfx_log=sfx, anim_log=anim)
        self.assertEqual(wall["submode_66"], 1)
        self.assertEqual(wall["timer_2"], 30)
        self.assertEqual(wall["z_10"], 16)
        self.assertEqual(wall["anchor_x_a6"], 120)
        self.assertEqual(wall["offset_aa"], 0)

    def test_sliding_wall_retracting_motion(self):
        # Facing 1: x += offset
        wall = {
            "submode_66": 1,
            "anchor_x_a6": 100,
            "offset_aa": 24,
            "facing_16": 1,
            "timer_2": 10,
        }
        sfx, anim = [], []
        step_sliding_wall_bridge(wall, persistent_flags={}, sfx_log=sfx, anim_log=anim)
        self.assertEqual(wall["offset_aa"], 16)
        self.assertEqual(wall["x"], 116)
        self.assertEqual(wall["timer_2"], 9)

        # Facing != 1: x -= offset
        wall2 = {
            "submode_66": 1,
            "anchor_x_a6": 100,
            "offset_aa": 16,
            "facing_16": 0,
            "timer_2": 5,
        }
        step_sliding_wall_bridge(wall2, persistent_flags={}, sfx_log=sfx, anim_log=anim)
        self.assertEqual(wall2["offset_aa"], 8)
        self.assertEqual(wall2["x"], 92)

    def test_sliding_wall_retracting_trigger_on_persistent_flag_and_timer(self):
        # Timer hits 0 and persistent flag clear -> extends!
        wall = {
            "submode_66": 1,
            "anchor_x_a6": 100,
            "offset_aa": 0,
            "timer_2": 1,
            "param_9e": 45,
            "persistent_id": 9,
        }
        p_flags = {9: False}
        sfx, anim = [], []
        step_sliding_wall_bridge(wall, p_flags, sfx, anim)
        self.assertEqual(wall["submode_66"], 2)
        self.assertEqual(wall["timer_2"], 45)
        self.assertEqual(sfx, [0x700391])

        # If persistent flag is set -> remains in State 1
        wall_latched = {
            "submode_66": 1,
            "anchor_x_a6": 100,
            "offset_aa": 0,
            "timer_2": 0,
            "param_9e": 45,
            "persistent_id": 9,
        }
        p_flags[9] = True
        sfx.clear()
        step_sliding_wall_bridge(wall_latched, p_flags, sfx, anim)
        self.assertEqual(wall_latched["submode_66"], 1)
        self.assertEqual(sfx, [])

    def test_sliding_wall_extending_motion(self):
        wall = {
            "submode_66": 2,
            "anchor_x_a6": 200,
            "offset_aa": 16,
            "facing_16": 1,
            "timer_2": 20,
            "persistent_id": 2,
        }
        sfx, anim = [], []
        step_sliding_wall_bridge(wall, persistent_flags={2: False}, sfx_log=sfx, anim_log=anim)
        self.assertEqual(wall["offset_aa"], 24)
        self.assertEqual(wall["x"], 224)
        self.assertEqual(anim, [0])
        self.assertEqual(wall["flag_37"] & (1 << 3), 0)  # cleared while extending

    def test_sliding_wall_extending_retract(self):
        # Retracts when timer expires
        wall = {
            "submode_66": 2,
            "anchor_x_a6": 200,
            "offset_aa": 40,
            "facing_16": 1,
            "timer_2": 1,
            "param_a0": 60,
            "persistent_id": 5,
        }
        sfx, anim = [], []
        step_sliding_wall_bridge(wall, persistent_flags={5: False}, sfx_log=sfx, anim_log=anim)
        self.assertEqual(wall["submode_66"], 1)
        self.assertEqual(wall["timer_2"], 60)

        # Retracts immediately if persistent bit becomes set
        wall_interrupt = {
            "submode_66": 2,
            "anchor_x_a6": 200,
            "offset_aa": 40,
            "timer_2": 50,
            "param_a0": 70,
            "persistent_id": 7,
        }
        step_sliding_wall_bridge(wall_interrupt, persistent_flags={7: True}, sfx_log=sfx, anim_log=anim)
        self.assertEqual(wall_interrupt["submode_66"], 1)
        self.assertEqual(wall_interrupt["timer_2"], 70)

    # --------------------------------------------------------------------------
    # Falling Rock Boulder Trap Tests
    # --------------------------------------------------------------------------

    def test_falling_rock_init(self):
        rock = {"submode_66": 0, "param_8c": 120, "flag_38": 0}
        sfx = []
        sys_vars = {}
        step_falling_rock_boulder(rock, ali={}, in_aabb=False, sfx_log=sfx, system_vars=sys_vars)
        self.assertEqual(rock["submode_66"], 0xFF)
        self.assertEqual(rock["z_10"], 120)
        self.assertTrue(rock["flag_38"] & (1 << 6))

    def test_falling_rock_ballistic_descent(self):
        rock = {"submode_66": 0xFF, "z_10": 0x00050000, "vz_56": 0}
        sfx = []
        sys_vars = {}
        step_falling_rock_boulder(rock, ali={}, in_aabb=False, sfx_log=sfx, system_vars=sys_vars)
        self.assertEqual(rock["vz_56"], -0xC000)
        self.assertEqual(rock["z_10"], 0x00050000 - 0xC000)

    def test_falling_rock_first_bounce_and_screenshake(self):
        rock = {"submode_66": 0xFF, "z_10": 0x2000, "vz_56": -0x5000, "bounced_a6": 0}
        sfx = []
        sys_vars = {}
        step_falling_rock_boulder(rock, ali={}, in_aabb=False, sfx_log=sfx, system_vars=sys_vars)
        self.assertEqual(rock["bounced_a6"], 0xFF)
        self.assertEqual(rock["z_10"], 1)
        self.assertEqual(rock["vz_56"], 0x00030000)
        self.assertEqual(sys_vars["screen_shake_ff17c4"], 3)
        self.assertEqual(sfx, [0x800386])

    def test_falling_rock_second_landing_clears_vz(self):
        rock = {"submode_66": 0xFF, "z_10": 0x1000, "vz_56": -0x4000, "bounced_a6": 0xFF}
        sfx = []
        sys_vars = {}
        step_falling_rock_boulder(rock, ali={}, in_aabb=False, sfx_log=sfx, system_vars=sys_vars)
        self.assertEqual(rock["z_10"], 0)
        self.assertEqual(rock["vz_56"], 0)
        self.assertEqual(sfx, [])

    def test_falling_rock_landed_fade_timer(self):
        rock = {"submode_66": 0xFF, "z_10": 0, "fade_2e": 5, "delta_x_72": 0, "delta_y_76": 0}
        step_falling_rock_boulder(rock, ali={}, in_aabb=False, sfx_log=[], system_vars={})
        self.assertEqual(rock["fade_2e"], 6)

        # Frozen when scrolling
        rock_scrolling = {"submode_66": 0xFF, "z_10": 0, "fade_2e": 5, "delta_x_72": 1, "delta_y_76": 0}
        step_falling_rock_boulder(rock_scrolling, ali={}, in_aabb=False, sfx_log=[], system_vars={})
        self.assertEqual(rock_scrolling["fade_2e"], 5)

    # --------------------------------------------------------------------------
    # Falling Rock Impact AABB Tests
    # --------------------------------------------------------------------------

    def test_falling_rock_impact_aabb_miss(self):
        rock = {"ptr": 0xFF3000}
        ali = {"flag_38": 0}
        hit = step_falling_rock_impact_aabb(rock, ali, in_aabb=False, sfx_log=[])
        self.assertFalse(hit)
        self.assertNotIn("damage_86", ali)

    def test_falling_rock_impact_guarded_or_invulnerable(self):
        rock = {"ptr": 0xFF3000}
        # Guarded
        ali_guard = {"flag_38": (1 << 3), "cooldown_82": 0}
        hit = step_falling_rock_impact_aabb(rock, ali_guard, in_aabb=True, sfx_log=[])
        self.assertFalse(hit)

        # Invulnerable
        ali_invuln = {"flag_38": 0, "cooldown_82": 12}
        hit = step_falling_rock_impact_aabb(rock, ali_invuln, in_aabb=True, sfx_log=[])
        self.assertFalse(hit)

    def test_falling_rock_impact_vulnerable_hit_player1(self):
        rock = {"ptr": 0xFF3000}
        ali = {
            "type_0": 1,
            "flag_38": 0,
            "cooldown_82": 0,
            "facing_16": 0,
            "z_10": 0,
        }
        sfx = []
        hit = step_falling_rock_impact_aabb(rock, ali, in_aabb=True, sfx_log=sfx)
        self.assertTrue(hit)
        self.assertEqual(ali["attacker_ptr"], 0xFF3000)
        self.assertEqual(ali["damage_86"], 0x8007)
        self.assertEqual(ali["vz_56"], 0x00020000)
        self.assertEqual(ali["z_10"], 1)
        self.assertEqual(ali["knockback_angle_84"], 2)  # 0 ^ 2
        self.assertEqual(sfx, [0x800346])

    def test_falling_rock_impact_vulnerable_hit_type2(self):
        rock = {"ptr": 0xFF3000}
        ali = {
            "type_0": 2,
            "flag_38": 0,
            "cooldown_82": 0,
            "facing_16": 1,
            "z_10": 5,
        }
        sfx = []
        hit = step_falling_rock_impact_aabb(rock, ali, in_aabb=True, sfx_log=sfx)
        self.assertTrue(hit)
        self.assertEqual(ali["damage_86"], 0x8007)
        self.assertEqual(ali["knockback_angle_84"], 3)  # 1 ^ 2
        self.assertEqual(sfx, [0x900237])


if __name__ == "__main__":
    unittest.main()
