#!/usr/bin/env python3
"""
Deterministic unit tests for ROM Boss AI State $1A Death Cutscene Sequence,
Explosion Shake & Smooth Angle Steering Helper (0x014F60..0x015136).
Milestone M15.506.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

ROOT_DIR = Path(__file__).resolve().parent.parent
EVIDENCE_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_boss_ai_death_steering_m15_506.json"

MAX_TURN_RATE_STEP = 8


def compute_smooth_turn_angle(current_angle: int, target_angle: int) -> int:
    """Emulates 0x0150A4..0x0150CC smooth angle turn-rate limiter."""
    curr = current_angle & 0xFF
    tgt = target_angle & 0xFF

    d4 = (curr - tgt) & 0xFF
    d0 = (tgt - curr) & 0xFF
    step_limit = MAX_TURN_RATE_STEP

    if d0 < d4:
        # Step forward (d0 direction)
        clamped_step = min(d0, step_limit)
        curr = (curr + clamped_step) & 0xFF
    else:
        # Step backward (-d4 direction)
        clamped_step = min(d4, step_limit)
        curr = (curr - clamped_step) & 0xFF

    return curr


def determine_facing_from_ali_x(ali_x: int) -> int:
    """Emulates 0x015108..0x015134 Ali-relative facing determination."""
    if ali_x < 0x00A0:  # < 160
        return 3  # left
    elif ali_x >= 0x0160:  # >= 352
        return 1  # right
    else:
        return 2  # center / down


def compute_death_explosion_coords(
    plane_a_x: int, plane_a_y: int,
    plane_b_x: int, plane_b_y: int,
    prng1: int, prng2: int,
) -> tuple[int, int]:
    """Emulates 0x01503C..0x015082 particle position calculation."""
    # d1 = 0x100 - plane_b_x + plane_a_x + (prng1 >> 1)
    # d2 = 0x0E8 - plane_b_y + plane_a_y + (prng2 >> 1) + 0x0E0
    d1 = 0x0100 - plane_b_x + plane_a_x + (prng1 >> 1)
    d2 = 0x00E8 - plane_b_y + plane_a_y + (prng2 >> 1) + 0x00E0
    return (d1, d2)


def compute_screen_shake(frame_counter: int) -> int:
    """Emulates 0x01508C..0x015098 screen shake formula."""
    return (frame_counter & 1) + 2


class MockDeathCutsceneState:
    def __init__(self):
        self.timer_2 = 0
        self.screen_flag_164f = 0xFF
        self.palette_param_1896 = 0
        self.palette_effect_called = False
        self.shake_called = False
        self.pool2_cleared = False
        self.nemesis_dma_called = False
        self.item_spawned = False
        self.item_fields: dict[str, int] = {}
        self.camera_lock_1983 = 0xFF
        self.palette_restore_called = False
        self.event_dispatched_param = 0
        self.boss_defeated_flag = False
        self.environmental_flag = False
        self.boss_despawned = False

    def step(self) -> None:
        self.timer_2 += 1
        d0 = self.timer_2

        if d0 == 0x00B4:  # 180
            self.screen_flag_164f = 0
            self.palette_param_1896 = 6
            self.palette_effect_called = True
            self.shake_called = True
            return

        if d0 < 0x00F0:  # < 240
            self.shake_called = True
            return

        if d0 == 0x012C:  # 300
            self.pool2_cleared = True
            self.nemesis_dma_called = True
            self.item_spawned = True
            self.item_fields = {
                "type": 0x0056,
                "tile_attr": 0x0478,
                "x": 0x0100,
                "y": 0x0098,
                "z_vel": 0x00010000,
                "z_acc": 0x000331DC,
            }
            return

        if d0 == 0x0168:  # 360
            self.camera_lock_1983 = 0
            self.palette_param_1896 = 4
            self.palette_restore_called = True
            self.event_dispatched_param = 0x011F
            self.boss_defeated_flag = True
            self.environmental_flag = True
            return

        if d0 == 0x01A4:  # 420
            self.screen_flag_164f = 0xFF
            self.boss_despawned = True
            return


class TestROMBossAIDeathSteering(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Evidence missing: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure_and_boundaries(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.506")
        self.assertEqual(self.evidence["range"]["start"], "0x014F60")
        self.assertEqual(self.evidence["range"]["end"], "0x015136")
        self.assertEqual(self.evidence["range"]["size_bytes"], 470)
        self.assertEqual(self.evidence["instruction_count"], 121)
        self.assertEqual(len(self.evidence["routines"]), 3)

        expected_routines = [
            ("BossAI_State1A_DeathCutsceneSequence", "0x014F60", "0x01503C", 220),
            ("BossAI_DeathExplosionShake", "0x01503C", "0x0150A0", 100),
            ("BossAI_SmoothAngleSteering", "0x0150A0", "0x015136", 150),
        ]
        actual = [
            (r["name"], r["start"], r["end"], r["size_bytes"])
            for r in self.evidence["routines"]
        ]
        self.assertEqual(actual, expected_routines)

    def test_cutscene_frame_180_stage1(self) -> None:
        state = MockDeathCutsceneState()
        state.timer_2 = 179
        state.step()

        self.assertEqual(state.timer_2, 180)
        self.assertEqual(state.screen_flag_164f, 0)
        self.assertEqual(state.palette_param_1896, 6)
        self.assertTrue(state.palette_effect_called)
        self.assertTrue(state.shake_called)

    def test_cutscene_frames_181_to_239_shake_loop(self) -> None:
        state = MockDeathCutsceneState()
        state.timer_2 = 180  # next step is 181
        state.step()
        self.assertTrue(state.shake_called)

        state.timer_2 = 238  # next step is 239 (< 240)
        state.shake_called = False
        state.step()
        self.assertTrue(state.shake_called)

        state.timer_2 = 239  # next step is 240 (not < 240)
        state.shake_called = False
        state.step()
        self.assertFalse(state.shake_called)

    def test_cutscene_frame_300_item_spawn(self) -> None:
        state = MockDeathCutsceneState()
        state.timer_2 = 299
        state.step()

        self.assertEqual(state.timer_2, 300)
        self.assertTrue(state.pool2_cleared)
        self.assertTrue(state.nemesis_dma_called)
        self.assertTrue(state.item_spawned)
        self.assertEqual(state.item_fields["type"], 0x0056)
        self.assertEqual(state.item_fields["tile_attr"], 0x0478)
        self.assertEqual(state.item_fields["x"], 256)
        self.assertEqual(state.item_fields["y"], 152)
        self.assertEqual(state.item_fields["z_vel"], 0x00010000)
        self.assertEqual(state.item_fields["z_acc"], 0x000331DC)

    def test_cutscene_frame_360_camera_event_flags(self) -> None:
        state = MockDeathCutsceneState()
        state.timer_2 = 359
        state.step()

        self.assertEqual(state.timer_2, 360)
        self.assertEqual(state.camera_lock_1983, 0)
        self.assertEqual(state.palette_param_1896, 4)
        self.assertTrue(state.palette_restore_called)
        self.assertEqual(state.event_dispatched_param, 0x011F)
        self.assertTrue(state.boss_defeated_flag)
        self.assertTrue(state.environmental_flag)

    def test_cutscene_frame_420_boss_despawn(self) -> None:
        state = MockDeathCutsceneState()
        state.timer_2 = 419
        state.step()

        self.assertEqual(state.timer_2, 420)
        self.assertEqual(state.screen_flag_164f, 0xFF)
        self.assertTrue(state.boss_despawned)

    def test_death_explosion_shake_formula(self) -> None:
        # Screen shake: even -> 2, odd -> 3
        self.assertEqual(compute_screen_shake(10), 2)
        self.assertEqual(compute_screen_shake(11), 3)

        # Explosion coords:
        # plane_a = (50, 40), plane_b = (10, 20), prng1 = 8, prng2 = 12
        # x = 256 - 10 + 50 + 4 = 300
        # y = 232 - 20 + 40 + 6 + 224 = 482
        x, y = compute_death_explosion_coords(
            plane_a_x=50, plane_a_y=40,
            plane_b_x=10, plane_b_y=20,
            prng1=8, prng2=12,
        )
        self.assertEqual((x, y), (300, 482))

    def test_smooth_angle_steering_turn_rate_limiter(self) -> None:
        # Turn delta <= 8 steps exactly
        self.assertEqual(compute_smooth_turn_angle(current_angle=100, target_angle=105), 105)
        self.assertEqual(compute_smooth_turn_angle(current_angle=100, target_angle=95), 95)

        # Turn delta > 8 is clamped to 8
        self.assertEqual(compute_smooth_turn_angle(current_angle=100, target_angle=130), 108)
        self.assertEqual(compute_smooth_turn_angle(current_angle=100, target_angle=70), 92)

        # Wrap around circle boundary (e.g. current 254, target 6 -> delta +8 across 0)
        self.assertEqual(compute_smooth_turn_angle(current_angle=254, target_angle=6), 6)
        # Wrap backward (e.g. current 2, target 240 -> delta -18 -> clamped -8 -> 250)
        self.assertEqual(compute_smooth_turn_angle(current_angle=2, target_angle=240), 250)

    def test_facing_determination_from_ali_x(self) -> None:
        # Ali X < 160 -> facing 3 (left)
        self.assertEqual(determine_facing_from_ali_x(100), 3)
        self.assertEqual(determine_facing_from_ali_x(159), 3)

        # Ali X in 160..351 -> facing 2 (center / down)
        self.assertEqual(determine_facing_from_ali_x(160), 2)
        self.assertEqual(determine_facing_from_ali_x(250), 2)
        self.assertEqual(determine_facing_from_ali_x(351), 2)

        # Ali X >= 352 -> facing 1 (right)
        self.assertEqual(determine_facing_from_ali_x(352), 1)
        self.assertEqual(determine_facing_from_ali_x(400), 1)


if __name__ == "__main__":
    unittest.main()
