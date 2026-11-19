#!/usr/bin/env python3
"""
Unit tests for ROM Boss AI Projectile States $0C/$14/$12, Hit Callbacks & State $0E
Hit-Reaction Deceleration (0x014D1A..0x014F60).
Milestone M15.505.
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_ai_projectiles_flinch_m15_505.json"
)


def step_feather_projectile(proj):
    """Emulate 0x014D1A..0x014D9E State $0C Feather/Spore Projectile step."""
    # Timer countdown
    if proj.get("timer", 0) > 0:
        proj["timer"] -= 1

    # Kinematics
    proj["x"] = proj.get("x", 0) + proj.get("x_vel", 0)
    proj["y"] = proj.get("y", 0) + proj.get("y_vel", 0)
    proj["z_vel"] = proj.get("z_vel", 0) + 0x6000
    proj["z"] = proj.get("z", 0) + proj["z_vel"]

    # Life / size scaling based on life_timer (+0x30)
    life = proj.get("life_timer", 0)
    if life <= 4:
        proj["size_3d"] = 8
    elif life <= 16:
        proj["size_3d"] = 16
    elif life <= 28:
        proj["size_3d"] = 24
    else:
        proj["size_3d"] = 24
        proj["flags_36"] = proj.get("flags_36", 0) | 0x08  # bit 3 (blink)

    # Despawn on animation loop complete (+0x37:1)
    if (proj.get("flags_37", 0) & 0x02) != 0:
        proj["status"] = 0  # despawned


def hit_callback_feather(ali, proj):
    """Emulate 0x014DA0..0x014E0A Hit Callback for Feather/Spore against Ali."""
    if (ali.get("flags_38", 0) & 0x08) != 0:
        return False  # Invulnerable

    ali["attacker"] = proj
    is_airborne = ali.get("z", 0) != 0

    if is_airborne:
        ali["damage_86"] = 0xA00C
        ali["z_vel"] = 0x28000
    else:
        ali["damage_86"] = 0x200C
        ali["z_vel"] = 0

    ali["hitstun_83"] = 6
    ali["field_9d"] = 0
    ali["knockback_mode_84"] = 2
    ali["sfx"] = 0x700345
    ali["x_vel"] = proj.get("x_vel", 0)
    ali["y_vel"] = proj.get("y_vel", 0)
    ali["player_state_198e"] = 0x0E
    ali["flags_38"] = ali.get("flags_38", 0) | 0x08  # bit 3 invulnerable
    proj["timer"] = 10
    return True


def step_fireball_projectile(proj, hit_ali=False):
    """Emulate 0x014E1A..0x014E84 State $12 Fireball Projectile step."""
    # Check 1: Cancel on incoming damage
    if proj.get("incoming_damage", 0) != 0:
        proj["incoming_damage"] = 0
        proj["anim_id"] = 0x16
        proj["state"] = 0x14
        return "FADEOUT_DAMAGE"

    # Check 2: Cancel on wall / barrier collision (+0x40|+0x41 bit 6)
    wall_flags = proj.get("wall_40", 0) | proj.get("wall_41", 0)
    if (wall_flags & 0x40) != 0:
        proj["anim_id"] = 0x16
        proj["state"] = 0x14
        return "FADEOUT_WALL"

    # Check 3: AABB hit against Ali
    if hit_ali:
        proj["anim_id"] = 0x16
        proj["state"] = 0x14
        return "FADEOUT_HIT"

    # Kinematics
    proj["x"] = proj.get("x", 0) + proj.get("x_vel", 0)
    proj["y"] = proj.get("y", 0) + proj.get("y_vel", 0)
    proj["z"] = proj.get("z", 0) + proj.get("z_vel", 0)

    # Floor contact: z < 0
    if proj["z"] < 0:
        proj["z"] = 0
        proj["anim_id"] = 0x16
        proj["state"] = 0x14
        return "FADEOUT_FLOOR"

    return "ACTIVE"


def hit_callback_fireball(ali, proj, computed_facing=0):
    """Emulate 0x014E86..0x014EE4 Hit Callback for Fireball against Ali."""
    if (ali.get("flags_38", 0) & 0x08) != 0:
        return False

    ali["attacker"] = proj
    is_airborne = ali.get("z", 0) != 0

    if is_airborne:
        ali["damage_86"] = 0xA008
        ali["z_vel"] = 0x28000
    else:
        ali["damage_86"] = 0x2008
        ali["z_vel"] = 0

    ali["hitstun_83"] = 8
    ali["field_9d"] = 0
    ali["knockback_mode_84"] = computed_facing
    ali["sfx"] = 0x700345
    ali["x_vel"] = proj.get("x_vel", 0)
    ali["y_vel"] = proj.get("y_vel", 0)
    return True


def apply_friction_step(vel, step=0x8000):
    """M68K signed friction: moves toward zero by step, clamps at 0."""
    if vel > 0:
        vel -= step
        if vel < 0:
            vel = 0
    elif vel < 0:
        vel += step
        if vel > 0:
            vel = 0
    return vel


def step_hit_reaction_decel(boss):
    """Emulate 0x014EE6..0x014F60 State $0E Hit-Reaction Deceleration."""
    boss["flags_36"] = boss.get("flags_36", 0) | 0x40  # bit 6: flinch

    # XY Friction step $8000
    boss["x_vel"] = apply_friction_step(boss.get("x_vel", 0), 0x8000)
    boss["y_vel"] = apply_friction_step(boss.get("y_vel", 0), 0x8000)
    boss["x"] = boss.get("x", 0) + boss["x_vel"]
    boss["y"] = boss.get("y", 0) + boss["y_vel"]

    # Z deceleration by $A000
    boss["z_vel"] = boss.get("z_vel", 0) - 0xA000
    if boss["z_vel"] >= 0:
        boss["flags_38"] = boss.get("flags_38", 0) | 0x08  # bit 3 invulnerable
        return "DECELERATING_AIR"

    boss["z_vel"] = 0
    residual = (boss["x_vel"] | boss["y_vel"]) & 0xFFFFF000
    if residual != 0:
        boss["flags_38"] = boss.get("flags_38", 0) | 0x08
        return "DECELERATING_GROUND"

    # Motion completely stopped: clear flinch flag
    boss["flags_36"] &= ~0x40

    # Counter-attack branch based on hit counter +$B6
    hit_count = boss.get("hit_counter", 0)
    if (hit_count & 3) == 0:
        boss["state"] = 0x08  # State 8 (every 4th hit)
        boss["anim_id"] = 0x00
        boss["timer"] = 10
        boss["sfx"] = 0x800251
        return "COUNTER_ATTACK_STATE8"
    else:
        boss["state"] = 0x10  # State $10
        boss["anim_id"] = 0x12
        return "COUNTER_ATTACK_STATE10"


class TestROMBossAIProjectilesFlinch(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.505")
        self.assertEqual(self.evidence["range"]["start"], "0x014D1A")
        self.assertEqual(self.evidence["range"]["end"], "0x014F60")
        self.assertEqual(self.evidence["range"]["size_bytes"], 582)
        self.assertEqual(len(self.evidence["routines"]), 6)
        self.assertEqual(self.evidence["instruction_count"], 152)

    def test_feather_projectile_step_and_size_scaling(self):
        proj = {
            "status": 1,
            "timer": 3,
            "x": 100,
            "y": 100,
            "z": 0,
            "x_vel": 0x10000,
            "y_vel": 0,
            "z_vel": 0,
            "life_timer": 2,
            "flags_37": 0
        }

        # Step 1: life_timer <= 4 -> size 8
        step_feather_projectile(proj)
        self.assertEqual(proj["timer"], 2)
        self.assertEqual(proj["size_3d"], 8)
        self.assertEqual(proj["z_vel"], 0x6000)

        # Step 2: life_timer 10 -> size 16
        proj["life_timer"] = 10
        step_feather_projectile(proj)
        self.assertEqual(proj["size_3d"], 16)

        # Step 3: life_timer 20 -> size 24
        proj["life_timer"] = 20
        step_feather_projectile(proj)
        self.assertEqual(proj["size_3d"], 24)

        # Step 4: life_timer 30 -> size 24 and bit 3 of flags_36 set (blink)
        proj["life_timer"] = 30
        step_feather_projectile(proj)
        self.assertEqual(proj["size_3d"], 24)
        self.assertTrue((proj["flags_36"] & 0x08) != 0)

        # Step 5: Animation loop finish (+0x37:1 set) -> despawns slot
        proj["flags_37"] = 0x02
        step_feather_projectile(proj)
        self.assertEqual(proj["status"], 0)

    def test_hit_callback_feather(self):
        proj = {"x_vel": 0x20000, "y_vel": 0x10000}
        ali_ground = {"z": 0, "flags_38": 0}

        # Ground hit: 0x200C damage
        hit = hit_callback_feather(ali_ground, proj)
        self.assertTrue(hit)
        self.assertEqual(ali_ground["damage_86"], 0x200C)
        self.assertEqual(ali_ground["hitstun_83"], 6)
        self.assertEqual(ali_ground["knockback_mode_84"], 2)
        self.assertEqual(ali_ground["sfx"], 0x700345)
        self.assertEqual(ali_ground["player_state_198e"], 0x0E)
        self.assertTrue((ali_ground["flags_38"] & 0x08) != 0)
        self.assertEqual(proj["timer"], 10)

        # Airborne hit: 0xA00C damage and vertical impulse $28000
        ali_air = {"z": 15, "flags_38": 0}
        hit_air = hit_callback_feather(ali_air, proj)
        self.assertTrue(hit_air)
        self.assertEqual(ali_air["damage_86"], 0xA00C)
        self.assertEqual(ali_air["z_vel"], 0x28000)

        # Invulnerable Ali ignores hit
        ali_invuln = {"z": 0, "flags_38": 0x08}
        self.assertFalse(hit_callback_feather(ali_invuln, proj))

    def test_fireball_projectile_transitions(self):
        # Case A: Incoming damage cancels to State 0x14 fadeout
        proj1 = {"incoming_damage": 5, "x_vel": 0, "y_vel": 0, "z_vel": 0, "z": 10}
        res1 = step_fireball_projectile(proj1)
        self.assertEqual(res1, "FADEOUT_DAMAGE")
        self.assertEqual(proj1["state"], 0x14)
        self.assertEqual(proj1["anim_id"], 0x16)

        # Case B: Wall collision cancels to fadeout
        proj2 = {"wall_40": 0x40, "z": 10}
        res2 = step_fireball_projectile(proj2)
        self.assertEqual(res2, "FADEOUT_WALL")
        self.assertEqual(proj2["state"], 0x14)

        # Case C: Floor contact (z < 0) clamps to 0 and enters fadeout
        proj3 = {"z": 5, "z_vel": -10}
        res3 = step_fireball_projectile(proj3)
        self.assertEqual(res3, "FADEOUT_FLOOR")
        self.assertEqual(proj3["z"], 0)
        self.assertEqual(proj3["state"], 0x14)

    def test_hit_callback_fireball(self):
        proj = {"x_vel": 0x18000, "y_vel": 0x18000}
        ali_ground = {"z": 0, "flags_38": 0}

        # Ground hit: 0x2008 damage, stun 8
        hit = hit_callback_fireball(ali_ground, proj, computed_facing=4)
        self.assertTrue(hit)
        self.assertEqual(ali_ground["damage_86"], 0x2008)
        self.assertEqual(ali_ground["hitstun_83"], 8)
        self.assertEqual(ali_ground["knockback_mode_84"], 4)
        self.assertEqual(ali_ground["sfx"], 0x700345)

        # Airborne hit: 0xA008 damage and vertical impulse $28000
        ali_air = {"z": 20, "flags_38": 0}
        hit_air = hit_callback_fireball(ali_air, proj, computed_facing=2)
        self.assertTrue(hit_air)
        self.assertEqual(ali_air["damage_86"], 0xA008)
        self.assertEqual(ali_air["z_vel"], 0x28000)

    def test_state0e_hit_reaction_deceleration(self):
        # Boss in air with knockback
        boss = {
            "x": 100,
            "y": 100,
            "x_vel": 0x18000,
            "y_vel": -0x10000,
            "z_vel": 0x14000,
            "hit_counter": 4  # 4 is divisible by 4
        }

        # Step 1: z_vel decreases by $A000 -> $A000 >= 0, still airborne
        res1 = step_hit_reaction_decel(boss)
        self.assertEqual(res1, "DECELERATING_AIR")
        self.assertEqual(boss["x_vel"], 0x10000)  # friction 0x8000
        self.assertEqual(boss["y_vel"], -0x8000)   # friction 0x8000
        self.assertEqual(boss["z_vel"], 0x0A000)
        self.assertTrue((boss["flags_36"] & 0x40) != 0)  # flinch set

        # Step 2: z_vel decreases by $A000 -> 0 >= 0, still airborne
        res2 = step_hit_reaction_decel(boss)
        self.assertEqual(res2, "DECELERATING_AIR")
        self.assertEqual(boss["x_vel"], 0x08000)
        self.assertEqual(boss["y_vel"], 0)
        self.assertEqual(boss["z_vel"], 0)

        # Step 3: z_vel decreases below 0 -> clamped to 0; x_vel friction reduces 0x8000 to 0.
        # Since motion is now completely halted, it immediately triggers counter-attack State 8!
        res3 = step_hit_reaction_decel(boss)
        self.assertEqual(res3, "COUNTER_ATTACK_STATE8")
        self.assertEqual(boss["state"], 0x08)
        self.assertEqual(boss["timer"], 10)
        self.assertFalse((boss["flags_36"] & 0x40) != 0)  # flinch flag cleared

        # Step 4: If hit_counter is not divisible by 4 (e.g. 5) -> State $10
        boss["hit_counter"] = 5
        boss["z_vel"] = 0
        boss["x_vel"] = 0
        boss["y_vel"] = 0
        res4 = step_hit_reaction_decel(boss)
        self.assertEqual(res4, "COUNTER_ATTACK_STATE10")
        self.assertEqual(boss["state"], 0x10)
        self.assertEqual(boss["anim_id"], 0x12)


if __name__ == "__main__":
    unittest.main()
