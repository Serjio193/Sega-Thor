#!/usr/bin/env python3
"""
Unit tests for ROM World Rolling/Sliding Crusher & Ceiling Crusher Traps
(0x0173D6..0x01778C, Milestone M15.522).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_world_rolling_crusher_traps_m15_522.json"
)


def step_rolling_boulder(obj, target=None, wall_collision=False):
    """Emulate AI_WorldObject_RollingSlidingBoulder_Crusher (0x0173D6..0x017528)."""
    mode = obj.get("mode_9e", 0)

    if mode == 0:  # Idle
        obj["counter_2e"] = obj.get("counter_2e", 0) + 1
        d0 = (obj.get("delta_x_72", 0) | obj.get("delta_y_76", 0))
        if d0 != 0:
            obj["counter_2e"] -= 1
        return "IDLE"

    elif mode == 5:  # Spawn Init
        obj["flags_37"] = 0x40
        obj["flags_38"] = (obj.get("flags_38", 0) & ~0x80) | 0x04  # bclr #7, bset #2
        obj["anim_2a"] = 2
        obj["mode_9e"] = 1
        obj["facing_16"] = 0
        obj["vx_4e"] = obj.get("param_vx_a0", 0)
        obj["vy_52"] = obj.get("param_vy_8c", 0)
        return "INITIALIZED"

    elif mode == 1:  # Rolling / Sliding Motion
        obj["delta_x_72"] = obj.get("vx_4e", 0)
        obj["delta_y_76"] = obj.get("vy_52", 0)

        # Check wall collision: (col_40 | col_41) & 0x40
        col = (obj.get("col_40", 0) | obj.get("col_41", 0)) & 0x40
        if col != 0 or wall_collision:
            obj["flags_38"] = obj.get("flags_38", 0) | 0x04
            obj["anim_2a"] = 4
            obj["mode_9e"] = 2
            obj["screen_shake"] = 4
            obj["sfx"] = 0x800346
            return "WALL_IMPACT"

        # AABB check vs target
        if target is not None:
            # Check AABB: x [-16..+16], y [-16..+16], z [0..32]
            dx = abs(target.get("x", 0) - obj.get("x", 0))
            dy = abs(target.get("y", 0) - obj.get("y", 0))
            dz = target.get("z", 0) - obj.get("z", 0)

            if dx <= 16 and dy <= 16 and 0 <= dz <= 32:
                # Target invulnerability or cooldown
                if (target.get("flags_38", 0) & 0x08) != 0 or target.get("invuln_82", 0) != 0:
                    return "IN_FLIGHT_NO_DAMAGE"

                # Hit target
                target["attacker_7e"] = id(obj)
                target["damage_86"] = 0x8014  # 20 HP
                target["invuln_82"] = 0
                target["reaction_83"] = 0
                target["flinch_9d"] = 0
                target["vz_56"] = 0x10000
                target["z"] = target.get("z", 0) + 1
                target["vx_4e"] = obj.get("vx_4e", 0)
                target["vy_52"] = obj.get("vy_52", 0)
                target["knockback_facing_84"] = 2  # mock facing from 0x00E338

                sfx = 0x900237 if target.get("type_0", 0) == 2 else 0x800346
                target["hit_sfx"] = sfx

                # Boulder transitions to impact state
                obj["flags_38"] = obj.get("flags_38", 0) | 0x04
                obj["anim_2a"] = 4
                obj["mode_9e"] = 2
                obj["screen_shake"] = 4
                obj["sfx"] = 0x800346
                return "HIT_TARGET_AND_IMPACT"

        return "IN_FLIGHT"

    elif mode == 2:  # Shatter / Despawn
        if (obj.get("flags_37", 0) & 0x02) != 0:
            obj["active_0"] = 0
            return "DESPAWNED"
        return "SHATTERING"

    return "UNKNOWN"


def heavy_crush_damage_callback(crusher, target):
    """Emulate Object_HeavyCrush_DamageCallback (0x017722..0x01778C)."""
    # Immunity checks
    if (target.get("flags_38", 0) & 0x08) != 0:  # invulnerability bit 3
        return False
    if (target.get("flags_3a", 0) & 0x01) != 0:  # immunity bit 0
        return False
    if target.get("invuln_82", 0) != 0:  # hit cooldown
        return False

    target["attacker_7e"] = id(crusher)
    target["damage_86"] = 0x8028  # 40 HP crush damage
    target["invuln_82"] = 0
    target["reaction_83"] = 0
    target["flinch_9d"] = 0
    target["vz_56"] = 0x8000
    target["z"] = target.get("z", 0) + 1
    target["vx_4e"] = 0
    target["vy_52"] = 0
    target["knockback_facing_84"] = target.get("facing_16", 0)

    sfx = 0x900237 if target.get("type_0", 0) == 2 else 0x800346
    target["hit_sfx"] = sfx
    return True


def step_ceiling_crusher(obj, room_trigger_bit=False, player=None, target=None):
    """Emulate Ceiling Crusher (Respawning mode 3 / One-Shot mode 4)."""
    mode = obj.get("mode_9e", 3)
    is_one_shot = (mode == 4)
    state = obj.get("state_a6", 0)

    if state == 0:  # Raised Ceiling Wait
        obj["anim_2a"] = 6
        ceil_z = obj.get("ceiling_z_8c", 192)
        obj["z_32"] = ceil_z << 16
        obj["z"] = ceil_z
        obj["flags_36"] = obj.get("flags_36", 0) | 0x08  # bset #3

        if room_trigger_bit:
            obj["state_a6"] = 1
            obj["timer_2"] = obj.get("param_timer_a0", 30)
            return "TRIGGERED_WAIT"
        return "RAISED_WAIT"

    elif state == 1:  # Countdown to Drop
        obj["timer_2"] = obj.get("timer_2", 1) - 1
        if obj["timer_2"] == 0:
            obj["state_a6"] = 2
            obj["vz_56"] = 0
            obj["flags_36"] = (obj.get("flags_36", 0) & ~0x08) | 0x04  # bclr #3, bset #2
            obj["shadow_scale_3d"] = 0
            if not is_one_shot and player is not None:
                obj["x"] = player.get("x", 0)
                obj["y"] = player.get("y", 0) + 1
            obj["sfx"] = 0x800350  # drop SFX
            return "DROP_INITIATED"
        return "COUNTING_DROP"

    elif state == 2:  # Falling & Ground Impact
        # Shadow scale: (3 - min(z >> 6, 3)) << 3 from high word of z
        z_curr_32 = obj.get("z_32", obj.get("z", 0) << 16)
        z_px = z_curr_32 >> 16
        d0 = min(z_px >> 6, 3)
        obj["shadow_scale_3d"] = ((3 - d0) & 0xFF) << 3

        # Gravity: vz -= 0xC000
        obj["vz_56"] = obj.get("vz_56", 0) - 0xC000
        z_next_32 = z_curr_32 + obj["vz_56"]
        obj["z_32"] = z_next_32
        obj["z"] = max(0, z_next_32 >> 16)

        landed = False
        if z_next_32 < 0:
            obj["state_a6"] = 3
            obj["z_32"] = 0
            obj["z"] = 0
            obj["flags_38"] = obj.get("flags_38", 0) | 0x04
            obj["anim_2a"] = 4
            obj["screen_shake"] = 4
            obj["sfx"] = 0x800346
            landed = True

        # AABB check vs target
        hit = False
        if target is not None:
            dx = abs(target.get("x", 0) - obj.get("x", 0))
            dy = abs(target.get("y", 0) - obj.get("y", 0))
            dz = target.get("z", 0) - obj.get("z", 0)
            if dx <= 16 and dy <= 16 and 0 <= dz <= 32:
                hit = heavy_crush_damage_callback(obj, target)

        return "LANDED_GROUND" if landed else ("FALLING_HIT" if hit else "FALLING")

    elif state == 3:  # Retract / Despawn
        if (obj.get("flags_37", 0) & 0x02) != 0:
            if is_one_shot:
                obj["active_0"] = 0
                return "DESPAWNED"
            else:
                obj["state_a6"] = 0
                obj["flags_38"] = obj.get("flags_38", 0) & ~0x04
                # Loops directly to State 0
                obj["anim_2a"] = 6
                ceil_z = obj.get("ceiling_z_8c", 192)
                obj["z_32"] = ceil_z << 16
                obj["z"] = ceil_z
                obj["flags_36"] = obj.get("flags_36", 0) | 0x08
                return "RESET_TO_RAISED"
        return "IMPACT_WAIT"

    return "UNKNOWN"


class TestRomWorldRollingCrusherTraps(unittest.TestCase):
    """Test suite for Milestone M15.522 ROM rolling crusher & ceiling crusher traps."""

    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence file: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            self.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        """Verify M15.522 evidence metadata, routine boundaries, and byte/instruction counts."""
        self.assertEqual(self.evidence["milestone"], "M15.522")
        self.assertEqual(self.evidence["range"]["start"], "0x0173D6")
        self.assertEqual(self.evidence["range"]["end"], "0x01778C")
        self.assertEqual(self.evidence["range"]["size_bytes"], 950)
        self.assertEqual(self.evidence["instruction_count"], 242)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 4)

        expected = [
            ("AI_WorldObject_RollingSlidingBoulder_Crusher", "0x0173D6", "0x017528", 338, 84),
            ("AI_WorldObject_CeilingCrusher_Respawning", "0x017528", "0x017634", 268, 70),
            ("AI_WorldObject_CeilingCrusher_OneShot", "0x017634", "0x017722", 238, 64),
            ("Object_HeavyCrush_DamageCallback", "0x017722", "0x01778C", 106, 24),
        ]

        total_bytes = 0
        total_insns = 0
        for r, (name, start, end, sz, insn_cnt) in zip(routines, expected):
            self.assertEqual(r["name"], name)
            self.assertEqual(r["start"], start)
            self.assertEqual(r["end"], end)
            self.assertEqual(r["size_bytes"], sz)
            self.assertEqual(r["instruction_count"], insn_cnt)
            total_bytes += sz
            total_insns += insn_cnt

        self.assertEqual(total_bytes, 950)
        self.assertEqual(total_insns, 242)

    def test_rolling_boulder_lifecycle_and_wall_impact(self):
        """Test rolling boulder initialization, motion, wall collision impact, and despawn."""
        boulder = {
            "mode_9e": 5,
            "param_vx_a0": 0x4000,
            "param_vy_8c": 0x2000,
            "x": 100,
            "y": 200,
            "z": 0,
        }

        # Step Mode 5 (Init)
        res = step_rolling_boulder(boulder)
        self.assertEqual(res, "INITIALIZED")
        self.assertEqual(boulder["flags_37"], 0x40)
        self.assertEqual(boulder["flags_38"] & 0x80, 0)
        self.assertNotEqual(boulder["flags_38"] & 0x04, 0)  # flying bit
        self.assertEqual(boulder["anim_2a"], 2)
        self.assertEqual(boulder["mode_9e"], 1)
        self.assertEqual(boulder["facing_16"], 0)
        self.assertEqual(boulder["vx_4e"], 0x4000)
        self.assertEqual(boulder["vy_52"], 0x2000)

        # Step Mode 1: normal rolling motion without wall collision
        res = step_rolling_boulder(boulder)
        self.assertEqual(res, "IN_FLIGHT")
        self.assertEqual(boulder["delta_x_72"], 0x4000)
        self.assertEqual(boulder["delta_y_76"], 0x2000)

        # Step Mode 1: wall collision encountered
        boulder["col_40"] = 0x40
        res = step_rolling_boulder(boulder)
        self.assertEqual(res, "WALL_IMPACT")
        self.assertEqual(boulder["anim_2a"], 4)
        self.assertEqual(boulder["mode_9e"], 2)
        self.assertEqual(boulder["screen_shake"], 4)
        self.assertEqual(boulder["sfx"], 0x800346)

        # Step Mode 2: animation still playing
        boulder["flags_37"] = 0x00
        res = step_rolling_boulder(boulder)
        self.assertEqual(res, "SHATTERING")

        # Step Mode 2: animation completed (+$37:1 set) -> despawn
        boulder["flags_37"] = 0x02
        res = step_rolling_boulder(boulder)
        self.assertEqual(res, "DESPAWNED")
        self.assertEqual(boulder["active_0"], 0)

    def test_rolling_boulder_player_collision_and_damage(self):
        """Test rolling boulder collision with player, inflicting 20 HP damage and velocity transfer."""
        boulder = {
            "mode_9e": 1,
            "vx_4e": 0x3000,
            "vy_52": 0x1000,
            "x": 100,
            "y": 100,
            "z": 0,
        }
        player = {
            "type_0": 2,  # Ali
            "x": 108,
            "y": 105,
            "z": 0,
            "flags_38": 0,
            "invuln_82": 0,
            "facing_16": 0,
        }

        res = step_rolling_boulder(boulder, target=player)
        self.assertEqual(res, "HIT_TARGET_AND_IMPACT")
        self.assertEqual(player["damage_86"], 0x8014)  # 20 HP
        self.assertEqual(player["vz_56"], 0x10000)
        self.assertEqual(player["vx_4e"], 0x3000)
        self.assertEqual(player["vy_52"], 0x1000)
        self.assertEqual(player["invuln_82"], 0)
        self.assertEqual(player["flinch_9d"], 0)
        self.assertEqual(player["hit_sfx"], 0x900237)  # Player hit sound
        # Boulder transitioned to impact
        self.assertEqual(boulder["mode_9e"], 2)
        self.assertEqual(boulder["anim_2a"], 4)
        self.assertEqual(boulder["screen_shake"], 4)

    def test_respawning_ceiling_crusher_full_cycle(self):
        """Test respawning ceiling crusher trigger, countdown, player snapping, drop, and loop reset."""
        crusher = {
            "mode_9e": 3,
            "state_a6": 0,
            "ceiling_z_8c": 192,
            "param_timer_a0": 2,
            "x": 50,
            "y": 50,
        }
        player = {"x": 200, "y": 300, "z": 0, "type_0": 2}

        # State 0: waiting for room trigger bit
        res = step_ceiling_crusher(crusher, room_trigger_bit=False)
        self.assertEqual(res, "RAISED_WAIT")
        self.assertEqual(crusher["anim_2a"], 6)
        self.assertEqual(crusher["z"], 192)
        self.assertNotEqual(crusher["flags_36"] & 0x08, 0)

        # Trigger bit set -> enters countdown State 1
        res = step_ceiling_crusher(crusher, room_trigger_bit=True)
        self.assertEqual(res, "TRIGGERED_WAIT")
        self.assertEqual(crusher["state_a6"], 1)
        self.assertEqual(crusher["timer_2"], 2)

        # Countdown tick 1
        res = step_ceiling_crusher(crusher)
        self.assertEqual(res, "COUNTING_DROP")
        self.assertEqual(crusher["timer_2"], 1)

        # Countdown tick 2 -> drop initiates and snaps to player (x, y + 1)
        res = step_ceiling_crusher(crusher, player=player)
        self.assertEqual(res, "DROP_INITIATED")
        self.assertEqual(crusher["state_a6"], 2)
        self.assertEqual(crusher["x"], 200)
        self.assertEqual(crusher["y"], 301)
        self.assertEqual(crusher["sfx"], 0x800350)
        self.assertEqual(crusher["shadow_scale_3d"], 0)

        # State 2: falling under gravity (first frame)
        res = step_ceiling_crusher(crusher)
        self.assertEqual(res, "FALLING")
        self.assertEqual(crusher["vz_56"], -0xC000)

        # Let crusher fall until it hits ground (simulate frames)
        while crusher["state_a6"] == 2:
            res = step_ceiling_crusher(crusher)

        self.assertEqual(res, "LANDED_GROUND")
        self.assertEqual(crusher["state_a6"], 3)
        self.assertEqual(crusher["z"], 0)
        self.assertEqual(crusher["anim_2a"], 4)
        self.assertEqual(crusher["screen_shake"], 4)
        self.assertEqual(crusher["sfx"], 0x800346)

        # State 3: animation completes -> resets back to State 0 (respawning!)
        crusher["flags_37"] = 0x02
        res = step_ceiling_crusher(crusher)
        self.assertEqual(res, "RESET_TO_RAISED")
        self.assertEqual(crusher["state_a6"], 0)
        self.assertEqual(crusher["anim_2a"], 6)
        self.assertEqual(crusher["z"], 192)

    def test_one_shot_ceiling_crusher_despawn(self):
        """Test one-shot ceiling crusher does not snap to player and despawns after impact."""
        crusher = {
            "mode_9e": 4,  # One-shot mode
            "state_a6": 1,
            "timer_2": 1,
            "x": 50,
            "y": 60,
            "ceiling_z_8c": 128,
            "z": 128,
            "z_32": 128 << 16,
        }
        player = {"x": 300, "y": 400}

        # Drop initiated: must NOT snap to player
        res = step_ceiling_crusher(crusher, player=player)
        self.assertEqual(res, "DROP_INITIATED")
        self.assertEqual(crusher["x"], 50)
        self.assertEqual(crusher["y"], 60)

        # Land on ground -> State 3
        while crusher["state_a6"] == 2:
            res = step_ceiling_crusher(crusher)

        self.assertEqual(res, "LANDED_GROUND")
        self.assertEqual(crusher["state_a6"], 3)

        # State 3: anim finished -> despawn (+0 = 0)
        crusher["flags_37"] = 0x02
        res = step_ceiling_crusher(crusher)
        self.assertEqual(res, "DESPAWNED")
        self.assertEqual(crusher["active_0"], 0)

    def test_heavy_crush_damage_callback(self):
        """Test heavy crush damage callback 40 HP crush, upward impulse, and defense bypass."""
        crusher = {"x": 100, "y": 100, "z": 0}
        target_player = {
            "type_0": 2,
            "facing_16": 4,
            "flags_38": 0,
            "flags_3a": 0,
            "invuln_82": 0,
            "vx_4e": 0x2000,
            "vy_52": -0x1000,
            "z": 0,
        }

        # Test successful crush hit
        hit = heavy_crush_damage_callback(crusher, target_player)
        self.assertTrue(hit)
        self.assertEqual(target_player["damage_86"], 0x8028)  # 40 HP
        self.assertEqual(target_player["vz_56"], 0x8000)
        self.assertEqual(target_player["z"], 1)
        self.assertEqual(target_player["vx_4e"], 0)
        self.assertEqual(target_player["vy_52"], 0)
        self.assertEqual(target_player["knockback_facing_84"], 4)
        self.assertEqual(target_player["hit_sfx"], 0x900237)

        # Test immunity bit 0 in $3A
        target_immune = {"flags_38": 0, "flags_3a": 0x01, "invuln_82": 0}
        self.assertFalse(heavy_crush_damage_callback(crusher, target_immune))

        # Test invulnerability bit 3 in $38
        target_invuln = {"flags_38": 0x08, "flags_3a": 0, "invuln_82": 0}
        self.assertFalse(heavy_crush_damage_callback(crusher, target_invuln))

        # Test cooldown $82 != 0
        target_cd = {"flags_38": 0, "flags_3a": 0, "invuln_82": 5}
        self.assertFalse(heavy_crush_damage_callback(crusher, target_cd))


if __name__ == "__main__":
    unittest.main()
