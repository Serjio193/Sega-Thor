#!/usr/bin/env python3
"""
Unit tests for ROM Serpent Boss Phase 1 States 0x00..0x18 (Hover, Bounds-Clamped Glide,
Dive Leap, Ground Slam Tremor & 4-Shot Fan Barrage)
(0x0164E2..0x016764, Milestone M15.516).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_serpent_boss_phase1_states_m15_516.json"
)

ANGLE_FAN_TABLE = [0xB5, 0xCB, 0x9F, 0xE1]


def clamp_x_velocity(boss_x, dx_vel):
    """Emulate 0x01656A..0x016588 clamp X helper."""
    if dx_vel >= 0:
        if boss_x > 0x1A0:
            return 0
    else:
        if boss_x < 0x60:
            return 0
    return dx_vel


def step_state00_hover(boss):
    """Emulate State $00 Idle/Hover Wait (0x0164E2..0x0164F2)."""
    boss["timer"] -= 1
    if boss["timer"] <= 0:
        return "TRIGGER_ACTION_16862"
    return "HOVERING"


def step_state16_death_gravity(boss):
    """Emulate State $16 Death Gravity Fall (0x0164F2..0x01651C)."""
    if boss["z"] <= 0:
        return "ALREADY_GROUNDED"

    boss["z_vel"] -= 0xC000
    boss["z"] += boss["z_vel"]

    if boss["z"] > 0:
        return "FALLING"

    boss["z"] = 0
    return "IMPACT_CLEANUP"


def step_state02_glide(boss):
    """Emulate State $02 Arena Glide & Footstep SFX (0x01652E..0x01656A)."""
    # X velocity is a2 << 16
    dx = (boss.get("vel_a2", 0) & 0xFFFF) << 16
    if dx >= 0x80000000:
        dx -= 0x100000000

    effective_dx = clamp_x_velocity(boss["x"], dx)
    boss["x_accum"] = boss.get("x_accum", 0) + effective_dx
    boss["x"] = boss.get("x", 0) + (effective_dx >> 16)

    boss["timer"] = boss.get("timer", 0) + 1
    played_sfx = False
    if (boss["timer"] & 7) == 0:
        played_sfx = True

    if (boss.get("flags_f3", 0) & 0x02) != 0:
        return "EXIT_SEQUENCER_1685A", played_sfx

    return "GLIDING", played_sfx


def step_windup_states(boss, arm_slot_active=True):
    """Emulate States $04, $06, $08, $0A (0x01658A..0x0165E2)."""
    state = boss.get("state", 4)
    if state == 4:
        if (boss.get("flags_f3", 0) & 0x02) != 0:
            boss["state"] = 6
            boss["timer"] = 20
            boss["anim_call"] = 0x2A
            return "ENTER_STATE_6"
        return "WAIT_STATE_4"

    elif state == 6:
        boss["timer"] -= 1
        if boss["timer"] == 0:
            boss["state"] = 8
            boss["anim_call"] = 0x0E
            return "ENTER_STATE_8"
        return "WAIT_STATE_6"

    elif state == 8:
        if (boss.get("flags_f3", 0) & 0x02) != 0:
            return "EXIT_SEQUENCER_1685A"
        return "WAIT_STATE_8"

    elif state == 0x0A:
        if not arm_slot_active:
            return "TRIGGER_ACTION_16862"
        return "WAIT_ARM_STATE_A"


def step_dive_slam(boss, ali):
    """Emulate States $0C, $0E, $10 Dive Leap & Ground Slam (0x0165E2..0x0166D2)."""
    state = boss.get("state", 0x0C)
    if state == 0x0C:
        if (boss.get("flags_f3", 0) & 0x02) != 0:
            boss["state"] = 0x0E
            boss["anim_call"] = 0x26
            # Trajectory calculation: (ali - boss) / 42
            d1 = (ali["x"] - boss["x"]) >> 6
            d2 = (ali["y"] - boss["y"]) >> 6
            vx = int(d1 / 42) << 6
            vy = int(d2 / 42) << 6
            boss["vx"] = vx
            boss["vy"] = vy
            boss["vz"] = 0x100000
            return "LEAP_INITIATED"
        return "WAIT_LEAP"

    elif state == 0x0E:
        if boss.get("arm_active_ec", 1) <= 0:
            return "ARM_INACTIVE"

        boss["vz"] -= 0xC000
        boss["z"] += boss["vz"]

        if boss["z"] > 0x30:
            # In flight
            boss["y_accum"] = boss.get("y_accum", 0) + boss["vy"]
            eff_vx = clamp_x_velocity(boss["x"], boss["vx"])
            boss["x_accum"] = boss.get("x_accum", 0) + eff_vx
            return "IN_FLIGHT"
        else:
            # Landing
            boss["z"] = 0x30
            boss["vz"] = 0
            boss["state"] = 0x10
            boss["anim_call"] = 0x22
            return "LANDED_GROUND"

    elif state == 0x10:
        ec = boss.get("arm_active_ec", 0)
        shake_triggered = False
        if ec == 4:
            shake_triggered = True

        if (boss.get("flags_f3", 0) & 0x02) != 0:
            return "EXIT_SEQUENCER_1685A", shake_triggered
        return "SLAM_WAIT", shake_triggered


def step_fan_barrage(boss):
    """Emulate States $12, $14 Fan Barrage (0x0166D2..0x016764)."""
    state = boss.get("state", 0x12)
    if state == 0x12:
        if (boss.get("flags_f3", 0) & 0x02) != 0:
            boss["state"] = 0x14
            boss["substate_2a"] = 2
            boss["anim_call"] = 0x2A
            return "ENTER_BARRAGE", None
        return "WAIT_STATE_12", None

    elif state == 0x14:
        spawned = None
        if boss.get("anim_frame_30", 0) == 8:
            d3 = boss.get("burst_counter_a0", 0)
            if d3 > 0:
                boss["shot_counter_2e"] = boss.get("shot_counter_2e", 0) + 1
                d3 -= 1
                if d3 == 0:
                    boss["shot_counter_2e"] = 0
                boss["burst_counter_a0"] = d3

                if (d3 & 1) == 0:
                    idx = (d3 >> 1) & 3
                    angle = ANGLE_FAN_TABLE[idx]
                    spawned = {"type": "FAN_SHOT", "angle": angle, "speed": 0x30000}
            else:
                # Aimed/center shot
                spawned = {"type": "CENTER_AIMED", "angle": 0xC0, "speed": 0x40000}

        if (boss.get("flags_37", 0) & 0x02) != 0:
            boss["state"] = 8
            boss["substate_2a"] = 0
            boss["anim_call"] = 0x0E
            return "BARRAGE_FINISHED", spawned

        return "BARRAGE_ACTIVE", spawned


class TestROMSerpentBossPhase1States(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(__file__)
        ev_file = os.path.join(self_dir, "..", "data", "thor-evidence", "rom_serpent_boss_phase1_states_m15_516.json")
        with open(ev_file, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        self.assertEqual(self.evidence["milestone"], "M15.516")
        self.assertEqual(self.evidence["range"]["start"], "0x0164E2")
        self.assertEqual(self.evidence["range"]["end"], "0x016764")
        self.assertEqual(self.evidence["range"]["size_bytes"], 642)
        self.assertEqual(self.evidence["instruction_count"], 171)
        self.assertEqual(len(self.evidence["routines"]), 8)
        self.assertEqual(self.evidence["angle_fan_table"]["values"], [0xB5, 0xCB, 0x9F, 0xE1])

    def test_state00_hover_wait(self):
        boss = {"timer": 2}
        res = step_state00_hover(boss)
        self.assertEqual(res, "HOVERING")
        self.assertEqual(boss["timer"], 1)

        res = step_state00_hover(boss)
        self.assertEqual(res, "TRIGGER_ACTION_16862")
        self.assertEqual(boss["timer"], 0)

    def test_state16_death_gravity_fall(self):
        boss = {"z": 0x20000, "z_vel": 0}
        res = step_state16_death_gravity(boss)
        self.assertEqual(res, "FALLING")
        self.assertEqual(boss["z_vel"], -0xC000)
        self.assertEqual(boss["z"], 0x14000)

        # Ground impact
        boss["z"] = 0x4000
        res = step_state16_death_gravity(boss)
        self.assertEqual(res, "IMPACT_CLEANUP")
        self.assertEqual(boss["z"], 0)

    def test_helper_clamp_x_position(self):
        # Moving right (dx >= 0)
        self.assertEqual(clamp_x_velocity(100, 0x10000), 0x10000)
        self.assertEqual(clamp_x_velocity(0x1A0, 0x10000), 0x10000)
        self.assertEqual(clamp_x_velocity(0x1A1, 0x10000), 0)

        # Moving left (dx < 0)
        self.assertEqual(clamp_x_velocity(200, -0x10000), -0x10000)
        self.assertEqual(clamp_x_velocity(0x60, -0x10000), -0x10000)
        self.assertEqual(clamp_x_velocity(0x5F, -0x10000), 0)

    def test_state02_arena_glide_and_sfx(self):
        boss = {"x": 200, "vel_a2": 1, "timer": 6, "flags_f3": 0}
        # Frame 7 (timer 7) -> no sfx
        res, sfx = step_state02_glide(boss)
        self.assertEqual(res, "GLIDING")
        self.assertFalse(sfx)
        self.assertEqual(boss["timer"], 7)

        # Frame 8 (timer 8) -> sfx triggered!
        res, sfx = step_state02_glide(boss)
        self.assertEqual(res, "GLIDING")
        self.assertTrue(sfx)
        self.assertEqual(boss["timer"], 8)

        # Exit on flags_f3 & 2
        boss["flags_f3"] = 2
        res, sfx = step_state02_glide(boss)
        self.assertEqual(res, "EXIT_SEQUENCER_1685A")

    def test_states_04_06_08_0a_windup_arm_wait(self):
        boss = {"state": 4, "flags_f3": 0}
        self.assertEqual(step_windup_states(boss), "WAIT_STATE_4")

        boss["flags_f3"] = 2
        self.assertEqual(step_windup_states(boss), "ENTER_STATE_6")
        self.assertEqual(boss["state"], 6)
        self.assertEqual(boss["timer"], 20)
        self.assertEqual(boss["anim_call"], 0x2A)

        boss["timer"] = 1
        self.assertEqual(step_windup_states(boss), "ENTER_STATE_8")
        self.assertEqual(boss["state"], 8)
        self.assertEqual(boss["anim_call"], 0x0E)

        boss["flags_f3"] = 2
        self.assertEqual(step_windup_states(boss), "EXIT_SEQUENCER_1685A")

        # State 0x0A check
        boss["state"] = 0x0A
        self.assertEqual(step_windup_states(boss, arm_slot_active=True), "WAIT_ARM_STATE_A")
        self.assertEqual(step_windup_states(boss, arm_slot_active=False), "TRIGGER_ACTION_16862")

    def test_states_0c_0e_10_dive_slam(self):
        boss = {"state": 0x0C, "x": 100 << 6, "y": 100 << 6, "flags_f3": 2}
        ali = {"x": (100 + 42) << 6, "y": (100 + 84) << 6}

        # State 0x0C Leap init: dx = 42 -> vx = (42/42)<<6 = 64; dy = 84 -> vy = (84/42)<<6 = 128
        res = step_dive_slam(boss, ali)
        self.assertEqual(res, "LEAP_INITIATED")
        self.assertEqual(boss["state"], 0x0E)
        self.assertEqual(boss["anim_call"], 0x26)
        self.assertEqual(boss["vx"], 64)
        self.assertEqual(boss["vy"], 128)
        self.assertEqual(boss["vz"], 0x100000)

        # State 0x0E in flight
        boss["z"] = 0x100000
        boss["x"] = 200
        res = step_dive_slam(boss, ali)
        self.assertEqual(res, "IN_FLIGHT")

        # State 0x0E landing: downward velocity brings height <= 0x30
        boss["z"] = 0x30
        boss["vz"] = -0x10000
        res = step_dive_slam(boss, ali)
        self.assertEqual(res, "LANDED_GROUND")
        self.assertEqual(boss["z"], 0x30)
        self.assertEqual(boss["vz"], 0)
        self.assertEqual(boss["state"], 0x10)
        self.assertEqual(boss["anim_call"], 0x22)

        # State 0x10 screen shake when arm_active_ec == 4
        boss["arm_active_ec"] = 4
        boss["flags_f3"] = 0
        res, shake = step_dive_slam(boss, ali)
        self.assertEqual(res, "SLAM_WAIT")
        self.assertTrue(shake)

    def test_states_12_14_fan_barrage(self):
        boss = {"state": 0x12, "flags_f3": 2}
        res, shot = step_fan_barrage(boss)
        self.assertEqual(res, "ENTER_BARRAGE")
        self.assertEqual(boss["state"], 0x14)
        self.assertEqual(boss["substate_2a"], 2)

        # State 0x14 with counter = 5 (odd) -> decrements to 4 (even) -> spawns fan shot!
        # d3=4: (4 >> 1) & 3 = 2 -> ANGLE_FAN_TABLE[2] = 0x9F
        boss["anim_frame_30"] = 8
        boss["burst_counter_a0"] = 5
        boss["flags_37"] = 0
        res, shot = step_fan_barrage(boss)
        self.assertEqual(res, "BARRAGE_ACTIVE")
        self.assertIsNotNone(shot)
        self.assertEqual(shot["type"], "FAN_SHOT")
        self.assertEqual(shot["angle"], 0x9F)
        self.assertEqual(shot["speed"], 0x30000)
        self.assertEqual(boss["burst_counter_a0"], 4)

        # When burst counter reaches 0 -> spawns aimed center shot!
        boss["burst_counter_a0"] = 0
        res, shot = step_fan_barrage(boss)
        self.assertEqual(res, "BARRAGE_ACTIVE")
        self.assertIsNotNone(shot)
        self.assertEqual(shot["type"], "CENTER_AIMED")
        self.assertEqual(shot["angle"], 0xC0)
        self.assertEqual(shot["speed"], 0x40000)

        # On anim finished bit 1 in flags_37 -> transitions to State 8
        boss["flags_37"] = 2
        res, shot = step_fan_barrage(boss)
        self.assertEqual(res, "BARRAGE_FINISHED")
        self.assertEqual(boss["state"], 8)
        self.assertEqual(boss["substate_2a"], 0)
        self.assertEqual(boss["anim_call"], 0x0E)


if __name__ == "__main__":
    unittest.main()
