#!/usr/bin/env python3
"""
Unit tests for ROM Composite Serpent Boss State $12 Homing Orb Projectile, Ali Grab Pinning,
Homing Hit Callback & Smooth Turn-Rate Steering (0x0160E2..0x016294).
Milestone M15.514.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct
import unittest

ROOT_DIR = Path(__file__).resolve().parent.parent
EVIDENCE_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_serpent_boss_homing_orb_m15_514.json"
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"


def quantize_4way_facing(angle: int) -> int:
    """
    Emulates 0x016238..0x01625A 4-way facing quantization.
    Angle in 0..255 -> facing in {0: Down, 1: Right, 2: Up, 3: Left}.
    """
    a = angle & 0xFF
    d0 = 1
    if a < 0x20:
        return d0
    d0 -= 1  # 0 (Down)
    if a < 0x60:
        return d0
    d0 += 3  # 3 (Left)
    if a < 0xA0:
        return d0
    d0 -= 1  # 2 (Up)
    if a < 0xE0:
        return d0
    d0 -= 1  # 1 (Right)
    return d0


def step_homing_turn_rate(current_angle: int, target_angle: int, speed: int) -> int:
    """
    Emulates 0x0161FA..0x016234 Smooth Homing Turn-Rate Limiter.
    """
    d3 = current_angle & 0xFF
    if speed == 0x00030000:
        return d3  # skips turn rate adjustment

    d0 = target_angle & 0xFF
    d4 = (d3 - d0) & 0xFF  # clockwise diff
    d0_diff = (d0 - d3) & 0xFF  # counter-clockwise diff
    d1 = 8

    if d4 < d0_diff:
        # Clockwise is shorter
        turn = d4 if d4 <= d1 else d1
        delta = (-turn) & 0xFF
    else:
        # Counter-clockwise is shorter (or equal)
        turn = d0_diff if d0_diff <= d1 else d1
        delta = turn & 0xFF

    return (d3 + delta) & 0xFF


def step_substep_anchor(entity: dict) -> bool:
    """
    Emulates 0x016168..0x016182 Sub-Step Anchor Integrator.
    Returns: bool (True if step performed and timer > 0; False if timer expired).
    """
    t = entity.get("timer", 0) - 1
    if t == 0:
        entity["timer"] = 0
        return False  # Zero flag set, no position step
    entity["timer"] = t
    entity["pos_72"] = entity.get("pos_72", 0) + entity.get("delta_a6", 0)
    entity["pos_76"] = entity.get("pos_76", 0) + entity.get("delta_aa", 0)
    return True  # Zero flag cleared


def pin_ali_to_grab(boss: dict, ali: dict):
    """
    Emulates 0x016184..0x0161A0 Ali Grab Pinning.
    """
    ali["x"] = boss.get("x", 0)
    ali["y"] = boss.get("y", 0) - 8
    ali["z"] = boss.get("z", 0)


def step_homing_hit_callback(orb: dict, ali: dict, sfx_callback=None) -> str:
    """
    Emulates 0x0161A2..0x0161FA Homing Orb Hit Callback.
    """
    if (ali.get("flag_38", 0) & (1 << 3)) != 0:
        # Guarded / invulnerable
        orb["vx"] = 0
        orb["vy"] = 0
        orb["anim_id"] = 0x38
        orb["flag_38"] = orb.get("flag_38", 0) | (1 << 3)
        return "guarded"

    # Vulnerable Ali hit
    ali["attacker_ptr"] = orb.get("ptr", 0)
    ali["hitstun_timer"] = 8
    ali["flag_9d"] = 0
    ali["reaction_mode"] = orb.get("facing_7c", 0)

    if sfx_callback:
        sfx_callback(0x800216)

    ali["vx"] = orb.get("vx", 0)
    ali["vy"] = orb.get("vy", 0)

    if ali.get("z", 0) == 0:
        # Grounded
        ali["vz"] = 0
        ali["damage_in"] = 2
    else:
        # Airborne
        ali["vz"] = 0x00010000
        ali["damage_in"] = 0x8002

    # Zero orb velocities and trigger fadeout
    orb["vx"] = 0
    orb["vy"] = 0
    orb["anim_id"] = 0x38
    orb["flag_38"] = orb.get("flag_38", 0) | (1 << 3)
    return "hit_applied"


class TestROMSerpentBossHomingOrb(unittest.TestCase):
    def setUp(self):
        self.evidence = json.loads(EVIDENCE_PATH.read_text())
        self.rom = ROM_PATH.read_bytes()

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.514")
        self.assertEqual(meta["range_start"], "0x0160E2")
        self.assertEqual(meta["range_end"], "0x016294")
        self.assertEqual(meta["range_size_bytes"], 434)
        self.assertEqual(meta["instruction_count"], 124)
        self.assertEqual(meta["routine_count"], 5)

        routines = self.evidence["routines"]
        self.assertEqual(routines[0]["instruction_count"], 38)
        self.assertEqual(routines[1]["instruction_count"], 8)
        self.assertEqual(routines[2]["instruction_count"], 6)
        self.assertEqual(routines[3]["instruction_count"], 17)
        self.assertEqual(routines[4]["instruction_count"], 55)

    def test_substep_anchor_integrator(self):
        entity = {
            "timer": 3,
            "delta_a6": 0x00010000,
            "delta_aa": -0x00020000,
            "pos_72": 0x00500000,
            "pos_76": 0x00300000,
        }
        # Step 1: timer 3 -> 2, accumulates
        active = step_substep_anchor(entity)
        self.assertTrue(active)
        self.assertEqual(entity["timer"], 2)
        self.assertEqual(entity["pos_72"], 0x00510000)
        self.assertEqual(entity["pos_76"], 0x002E0000)

        # Step 2: timer 2 -> 1, accumulates
        active2 = step_substep_anchor(entity)
        self.assertTrue(active2)
        self.assertEqual(entity["timer"], 1)

        # Step 3: timer 1 -> 0, expires, no step
        active3 = step_substep_anchor(entity)
        self.assertFalse(active3)
        self.assertEqual(entity["timer"], 0)

    def test_ali_grab_pinning(self):
        boss = {"x": 320, "y": 240, "z": 15}
        ali = {"x": 0, "y": 0, "z": 0}
        pin_ali_to_grab(boss, ali)
        self.assertEqual(ali["x"], 320)
        self.assertEqual(ali["y"], 232)  # 240 - 8
        self.assertEqual(ali["z"], 15)

    def test_homing_hit_callback_grounded(self):
        sfx_played = []
        orb = {"ptr": 0xFF2A00, "vx": 0x00020000, "vy": 0x00010000, "facing_7c": 1}
        ali = {"flag_38": 0, "z": 0}

        res = step_homing_hit_callback(orb, ali, sfx_callback=lambda sfx: sfx_played.append(sfx))
        self.assertEqual(res, "hit_applied")
        self.assertEqual(ali["attacker_ptr"], 0xFF2A00)
        self.assertEqual(ali["hitstun_timer"], 8)
        self.assertEqual(ali["reaction_mode"], 1)
        self.assertEqual(ali["damage_in"], 2)
        self.assertEqual(ali["vz"], 0)
        self.assertEqual(ali["vx"], 0x00020000)
        self.assertEqual(ali["vy"], 0x00010000)
        self.assertEqual(sfx_played, [0x800216])
        self.assertEqual(orb["vx"], 0)
        self.assertEqual(orb["vy"], 0)
        self.assertEqual(orb["anim_id"], 0x38)
        self.assertEqual(orb["flag_38"] & (1 << 3), 1 << 3)

    def test_homing_hit_callback_airborne(self):
        orb = {"ptr": 0xFF2A00, "vx": 0x00010000, "vy": -0x00010000, "facing_7c": 2}
        ali = {"flag_38": 0, "z": 12}

        res = step_homing_hit_callback(orb, ali)
        self.assertEqual(res, "hit_applied")
        self.assertEqual(ali["damage_in"], 0x8002)
        self.assertEqual(ali["vz"], 0x00010000)

    def test_homing_hit_callback_guarded(self):
        orb = {"ptr": 0xFF2A00, "vx": 0x00010000, "vy": 0}
        ali = {"flag_38": (1 << 3), "z": 0}

        res = step_homing_hit_callback(orb, ali)
        self.assertEqual(res, "guarded")
        self.assertNotIn("damage_in", ali)
        self.assertEqual(orb["anim_id"], 0x38)
        self.assertEqual(orb["vx"], 0)

    def test_smooth_homing_turn_rate_limits(self):
        # 1. Speed == $30000 skips turn
        res_skip = step_homing_turn_rate(current_angle=0x40, target_angle=0x80, speed=0x00030000)
        self.assertEqual(res_skip, 0x40)

        # 2. Angle difference <= 8 units: turns exactly to target
        res_close = step_homing_turn_rate(current_angle=0x40, target_angle=0x45, speed=0x00020000)
        self.assertEqual(res_close, 0x45)

        # 3. Angle difference > 8 units counter-clockwise: clamps to +8
        res_ccw = step_homing_turn_rate(current_angle=0x40, target_angle=0x60, speed=0x00020000)
        self.assertEqual(res_ccw, 0x48)  # 0x40 + 8

        # 4. Angle difference > 8 units clockwise: clamps to -8
        res_cw = step_homing_turn_rate(current_angle=0x40, target_angle=0x20, speed=0x00020000)
        self.assertEqual(res_cw, 0x38)  # 0x40 - 8

        # 5. Wrap around 0/256 circle: from 0x04 to 0xFC (clockwise is 8 units: 0x04 -> 0xFC)
        res_wrap = step_homing_turn_rate(current_angle=0x04, target_angle=0xFA, speed=0x00020000)
        self.assertEqual(res_wrap, 0xFC)  # 0x04 - 8 = 0xFC

    def test_angle_4way_facing_quantization(self):
        # Right: [0x00..0x1F] and [0xE0..0xFF] -> 1
        self.assertEqual(quantize_4way_facing(0x00), 1)
        self.assertEqual(quantize_4way_facing(0x1F), 1)
        self.assertEqual(quantize_4way_facing(0xE0), 1)
        self.assertEqual(quantize_4way_facing(0xFF), 1)

        # Down: [0x20..0x5F] -> 0
        self.assertEqual(quantize_4way_facing(0x20), 0)
        self.assertEqual(quantize_4way_facing(0x5F), 0)

        # Left: [0x60..0x9F] -> 3
        self.assertEqual(quantize_4way_facing(0x60), 3)
        self.assertEqual(quantize_4way_facing(0x9F), 3)

        # Up: [0xA0..0xDF] -> 2
        self.assertEqual(quantize_4way_facing(0xA0), 2)
        self.assertEqual(quantize_4way_facing(0xDF), 2)

    def test_sin_cos_vector_stepping_from_rom(self):
        # Read from actual ROM table at 0x5D706
        table_addr = 0x05D706

        def calc_step(angle, speed):
            scale = speed >> 7
            d3 = angle & 0xFF
            d7 = (d3 + 0x40) & 0xFF
            sin_b = struct.unpack_from('>b', self.rom, table_addr + d3 * 2)[0]
            cos_b = struct.unpack_from('>b', self.rom, table_addr + d7 * 2)[0]
            vx = cos_b * scale
            vy = -sin_b * scale
            return vx, vy

        # Angle 0 (Right): sin=0, cos=+127
        vx0, vy0 = calc_step(0x00, 0x00020000)
        self.assertGreater(vx0, 0)
        self.assertEqual(vy0, 0)

        # Angle 0x40 (Down/90 deg in Thor math): sin=+127, cos=0 -> vy < 0
        vx40, vy40 = calc_step(0x40, 0x00020000)
        self.assertEqual(vx40, 0)
        self.assertLess(vy40, 0)


if __name__ == "__main__":
    unittest.main()
