#!/usr/bin/env python3
"""
Deterministic unit tests for ROM Boss AI Combat States $10/$06/$08/$0A/$16/$18
& Orbit Target Helpers (0x014A72..0x014D1A).
Milestone M15.504.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

ROOT_DIR = Path(__file__).resolve().parent.parent
EVIDENCE_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_boss_ai_combat_orbit_m15_504.json"

SFX_PROJECTILE = 0x0080034F
ALI_KNOCKBACK_VEL = 0x00020000
HIGH_ORBIT_Y_CLAMP_MIN = 0x00C8


def compute_high_orbit_target(
    ali_x: int, ali_y: int, facing: int, boss_x: int, boss_y: int
) -> tuple[int, int, int, int]:
    """Emulates 0x014C1C..0x014CA0 High Orbit Target Helper."""
    d1 = ali_x
    d2 = ali_y

    if facing == 1:
        d1 -= 0x60
        d2 -= 0x30
    elif facing == 2:
        d2 -= 0x68
    else:  # facing 0 or default
        d1 += 0x60
        d2 -= 0x30

    if d2 < HIGH_ORBIT_Y_CLAMP_MIN:
        d2 = HIGH_ORBIT_Y_CLAMP_MIN

    tgt_x = d1
    tgt_y = d2

    # Chebyshev distance L_infinity = max(|dx|, |dy|)
    dx = abs(tgt_x - boss_x)
    dy = abs(tgt_y - boss_y)
    dist = max(dx, dy)

    # Speed tiers
    if dist > 24:
        speed = 0x00030000
    elif dist > 16:
        speed = 0x00020000
    else:
        speed = 0x00018000

    timer_reload = 4
    return (tgt_x, tgt_y, speed, timer_reload)


def compute_low_orbit_target(
    ali_x: int, ali_y: int, facing: int, boss_x: int, boss_y: int
) -> tuple[int, int, int, int]:
    """Emulates 0x014CA0..0x014D1A Low Orbit Target Helper."""
    d1 = ali_x
    d2 = ali_y

    if facing == 1:
        d1 -= 0x48
        d2 -= 0x30
    elif facing == 2:
        d2 -= 0x38
    else:
        d1 += 0x48
        d2 -= 0x30

    tgt_x = d1
    tgt_y = d2

    dx = abs(tgt_x - boss_x)
    dy = abs(tgt_y - boss_y)
    dist = max(dx, dy)

    if dist > 24:
        speed = 0x00028000
    elif dist > 16:
        speed = 0x00020000
    else:
        speed = 0x00018000

    timer_reload = 4
    return (tgt_x, tgt_y, speed, timer_reload)


class MockBossEntity:
    def __init__(self, x: int = 100, y: int = 250):
        self.state_4 = 0
        self.substate_2a = 0
        self.timer_2 = 1
        self.facing_16 = 0
        self.pos_x = x
        self.pos_y = y
        self.target_x_a6 = 0
        self.target_y_aa = 0
        self.speed_b0 = 0
        self.angle_af = 0
        self.anim_timer_30 = 0
        self.flag_2e = 0
        self.counter_32 = 0
        self.anim_done_37_1 = False
        self.sfx_played: int | None = None
        self.projectile_spawned = False
        self.orbit_helper_called: str | None = None
        self.transitioned_to_014958 = False

    def step_state_10(self, ali_x: int, ali_y: int) -> None:
        self.timer_2 -= 1
        if self.timer_2 == 0:
            tx, ty, spd, reload = compute_high_orbit_target(
                ali_x, ali_y, self.facing_16, self.pos_x, self.pos_y
            )
            self.target_x_a6 = tx
            self.target_y_aa = ty
            self.speed_b0 = spd
            self.timer_2 = reload
            self.orbit_helper_called = "high"

        if self.anim_timer_30 == 0x18 and self.flag_2e == 0:
            self.projectile_spawned = True
            self.sfx_played = SFX_PROJECTILE

        if self.anim_done_37_1:
            self.substate_2a = 8
            self.transitioned_to_014958 = True

    def step_state_06(self, ali_x: int, ali_y: int) -> None:
        self.timer_2 -= 1
        if self.timer_2 == 0:
            tx, ty, spd, reload = compute_low_orbit_target(
                ali_x, ali_y, self.facing_16, self.pos_x, self.pos_y
            )
            self.target_x_a6 = tx
            self.target_y_aa = ty
            self.speed_b0 = spd
            self.timer_2 = reload
            self.orbit_helper_called = "low"

        if self.anim_done_37_1:
            self.substate_2a = 0
            self.state_4 = 8  # transitions to State $08

    def step_state_08(self, ali_x: int, ali_y: int, prng_val: int) -> None:
        self.timer_2 -= 1
        if self.timer_2 == 0:
            tx, ty, spd, reload = compute_low_orbit_target(
                ali_x, ali_y, self.facing_16, self.pos_x, self.pos_y
            )
            self.target_x_a6 = tx
            self.target_y_aa = ty
            self.speed_b0 = spd
            self.timer_2 = reload
            self.orbit_helper_called = "low"

        if self.counter_32 == 12:
            rem = prng_val % 3
            quot = prng_val // 3
            if rem == 0:
                self.projectile_spawned = True
                if (quot & 3) == 0:
                    self.sfx_played = SFX_PROJECTILE

        if self.anim_done_37_1:
            self.substate_2a = 4
            self.state_4 = 0x0A  # transitions to State $0A

    def step_state_0a(self, ali_x: int, ali_y: int) -> None:
        self.timer_2 -= 1
        if self.timer_2 == 0:
            tx, ty, spd, reload = compute_low_orbit_target(
                ali_x, ali_y, self.facing_16, self.pos_x, self.pos_y
            )
            self.target_x_a6 = tx
            self.target_y_aa = ty
            self.speed_b0 = spd
            self.timer_2 = reload
            self.orbit_helper_called = "low"

        if self.anim_done_37_1:
            self.substate_2a = 8
            self.transitioned_to_014958 = True

    def step_state_16(self, hit_landed: bool) -> None:
        if hit_landed:
            self.state_4 = 0x18
            self.substate_2a = 0x1A
            self.speed_b0 = 0x00010000
            return

        self.timer_2 -= 1
        if self.timer_2 == 0:
            self.state_4 = 0x18
            self.substate_2a = 0x1A
            self.speed_b0 = 0x00010000

    def step_state_18(self) -> None:
        if self.anim_done_37_1:
            self.transitioned_to_014958 = True
            self.substate_2a = 8
            self.speed_b0 = 0x00030000
            self.timer_2 = 20
            self.angle_af = (self.angle_af + 0x80) & 0xFF

    def hit_callback_014c04(self, ali_entity: dict[str, int]) -> None:
        if self.facing_16 != 2:
            ali_entity["z_vel_52"] = ALI_KNOCKBACK_VEL


class TestROMBossAICombatOrbit(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure_and_boundaries(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.504")
        self.assertEqual(self.evidence["range"]["start"], "0x014A72")
        self.assertEqual(self.evidence["range"]["end"], "0x014D1A")
        self.assertEqual(self.evidence["range"]["size_bytes"], 680)
        self.assertEqual(self.evidence["instruction_count"], 173)
        self.assertEqual(len(self.evidence["routines"]), 8)

        expected_routines = [
            ("BossAI_State10_CombatAttack", "0x014A72", "0x014ABC", 74),
            ("BossAI_State06_OrbitApproach", "0x014ABC", "0x014AEA", 46),
            ("BossAI_State08_OrbitCombatAttack", "0x014AEA", "0x014B50", 102),
            ("BossAI_State0A_Retreat", "0x014B50", "0x014B7C", 44),
            ("BossAI_State16_ChaseAliHitbox", "0x014B7C", "0x014BB8", 60),
            ("BossAI_State18_HitThrustAndCallback", "0x014BB8", "0x014C1C", 100),
            ("BossAI_HighOrbitTargetHelper", "0x014C1C", "0x014CA0", 132),
            ("BossAI_LowOrbitTargetHelper", "0x014CA0", "0x014D1A", 122),
        ]
        actual = [
            (r["name"], r["start"], r["end"], r["size_bytes"])
            for r in self.evidence["routines"]
        ]
        self.assertEqual(actual, expected_routines)

    def test_high_orbit_targets_and_y_clamping(self) -> None:
        ali_x, ali_y = 200, 350

        # Facing 0: (+0x60, -0x30)
        tx, ty, spd, tm = compute_high_orbit_target(ali_x, ali_y, 0, 200, 350)
        self.assertEqual((tx, ty), (200 + 0x60, 350 - 0x30))
        self.assertEqual(tm, 4)

        # Facing 1: (-0x60, -0x30)
        tx, ty, spd, tm = compute_high_orbit_target(ali_x, ali_y, 1, 200, 350)
        self.assertEqual((tx, ty), (200 - 0x60, 350 - 0x30))

        # Facing 2: (0, -0x68) -> 350 - 104 = 246 >= 200
        tx, ty, spd, tm = compute_high_orbit_target(ali_x, ali_y, 2, 200, 350)
        self.assertEqual((tx, ty), (200, 350 - 0x68))

        # Y Clamp test: Y < 0xC8 (200) clamped to 0xC8
        low_ali_y = 150  # 150 - 0x30 = 102 < 200
        tx, ty, spd, tm = compute_high_orbit_target(ali_x, low_ali_y, 0, 200, 150)
        self.assertEqual(ty, 0x00C8)

    def test_high_orbit_chebyshev_speed_tiers(self) -> None:
        ali_x, ali_y = 200, 300

        # Target will be (296, 252).
        # Distance > 24 -> speed 0x00030000
        _, _, spd1, _ = compute_high_orbit_target(ali_x, ali_y, 0, boss_x=100, boss_y=252)
        self.assertEqual(spd1, 0x00030000)

        # Distance 20 (between 17 and 24) -> speed 0x00020000
        _, _, spd2, _ = compute_high_orbit_target(ali_x, ali_y, 0, boss_x=296 - 20, boss_y=252)
        self.assertEqual(spd2, 0x00020000)

        # Distance 12 (<= 16) -> speed 0x00018000
        _, _, spd3, _ = compute_high_orbit_target(ali_x, ali_y, 0, boss_x=296 - 12, boss_y=252)
        self.assertEqual(spd3, 0x00018000)

    def test_low_orbit_targets_and_speed_tiers(self) -> None:
        ali_x, ali_y = 200, 300

        # Facing 0: (+0x48, -0x30)
        tx, ty, spd, tm = compute_low_orbit_target(ali_x, ali_y, 0, 200, 300)
        self.assertEqual((tx, ty), (200 + 0x48, 300 - 0x30))
        self.assertEqual(tm, 4)

        # Facing 1: (-0x48, -0x30)
        tx, ty, spd, tm = compute_low_orbit_target(ali_x, ali_y, 1, 200, 300)
        self.assertEqual((tx, ty), (200 - 0x48, 300 - 0x30))

        # Facing 2: (0, -0x38)
        tx, ty, spd, tm = compute_low_orbit_target(ali_x, ali_y, 2, 200, 300)
        self.assertEqual((tx, ty), (200, 300 - 0x38))

        # Distance > 24 -> speed 0x00028000 (Low orbit default)
        _, _, spd_far, _ = compute_low_orbit_target(ali_x, ali_y, 0, 100, 100)
        self.assertEqual(spd_far, 0x00028000)

        # Distance <= 16 -> speed 0x00018000
        tx0, ty0, _, _ = compute_low_orbit_target(ali_x, ali_y, 0, 200, 300)
        _, _, spd_close, _ = compute_low_orbit_target(ali_x, ali_y, 0, tx0 - 10, ty0)
        self.assertEqual(spd_close, 0x00018000)

    def test_boss_ai_state_10_combat_attack(self) -> None:
        boss = MockBossEntity()
        boss.timer_2 = 1
        boss.anim_timer_30 = 0x18
        boss.flag_2e = 0
        boss.anim_done_37_1 = True

        boss.step_state_10(200, 300)

        self.assertEqual(boss.orbit_helper_called, "high")
        self.assertTrue(boss.projectile_spawned)
        self.assertEqual(boss.sfx_played, SFX_PROJECTILE)
        self.assertEqual(boss.substate_2a, 8)
        self.assertTrue(boss.transitioned_to_014958)

    def test_boss_ai_state_06_orbit_approach(self) -> None:
        boss = MockBossEntity()
        boss.timer_2 = 1
        boss.anim_done_37_1 = True

        boss.step_state_06(200, 300)

        self.assertEqual(boss.orbit_helper_called, "low")
        self.assertEqual(boss.substate_2a, 0)
        self.assertEqual(boss.state_4, 8)  # State $08 transition

    def test_boss_ai_state_08_attack_prng(self) -> None:
        boss = MockBossEntity()
        boss.timer_2 = 1
        boss.counter_32 = 12
        boss.anim_done_37_1 = True

        # prng = 12: 12 % 3 == 0 (rem=0 -> attack!), 12 // 3 = 4 (4 & 3 == 0 -> SFX!)
        boss.step_state_08(200, 300, prng_val=12)
        self.assertTrue(boss.projectile_spawned)
        self.assertEqual(boss.sfx_played, SFX_PROJECTILE)
        self.assertEqual(boss.substate_2a, 4)
        self.assertEqual(boss.state_4, 0x0A)

        # prng = 7: 7 % 3 == 1 (no attack)
        boss2 = MockBossEntity()
        boss2.counter_32 = 12
        boss2.step_state_08(200, 300, prng_val=7)
        self.assertFalse(boss2.projectile_spawned)

    def test_boss_ai_state_0a_retreat(self) -> None:
        boss = MockBossEntity()
        boss.timer_2 = 1
        boss.anim_done_37_1 = True

        boss.step_state_0a(200, 300)

        self.assertEqual(boss.orbit_helper_called, "low")
        self.assertEqual(boss.substate_2a, 8)
        self.assertTrue(boss.transitioned_to_014958)

    def test_boss_ai_state_16_chase_and_hit(self) -> None:
        boss = MockBossEntity()
        boss.step_state_16(hit_landed=True)
        self.assertEqual(boss.state_4, 0x18)
        self.assertEqual(boss.substate_2a, 0x1A)
        self.assertEqual(boss.speed_b0, 0x00010000)

    def test_boss_ai_state_18_thrust_and_callback(self) -> None:
        boss = MockBossEntity()
        boss.angle_af = 0x20
        boss.anim_done_37_1 = True

        boss.step_state_18()

        self.assertEqual(boss.substate_2a, 8)
        self.assertEqual(boss.speed_b0, 0x00030000)
        self.assertEqual(boss.timer_2, 20)
        self.assertEqual(boss.angle_af, (0x20 + 0x80) & 0xFF)  # Flipped by 180 degrees
        self.assertTrue(boss.transitioned_to_014958)

        # Test hit callback knockback
        ali = {"z_vel_52": 0}
        boss.facing_16 = 0
        boss.hit_callback_014c04(ali)
        self.assertEqual(ali["z_vel_52"], ALI_KNOCKBACK_VEL)

        # If facing is 2, knockback is not applied
        ali2 = {"z_vel_52": 0}
        boss.facing_16 = 2
        boss.hit_callback_014c04(ali2)
        self.assertEqual(ali2["z_vel_52"], 0)


if __name__ == "__main__":
    unittest.main()
