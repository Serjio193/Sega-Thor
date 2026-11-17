#!/usr/bin/env python3
"""Unit tests for M15.336 ROM Enemy AI Steering, Fixed-Point Aim & Z-Gravity Physics."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ai_steering_gravity import ROM_PATH, extract_evidence


def facing_and_manhattan(ex: int, ey: int, px: int, py: int) -> tuple[int, int]:
    """Reference simulation of 0x00E398..0x00E3E6."""
    dx = px - ex
    dy = py - ey
    ax, ay = abs(dx), abs(dy)
    manhattan = ax + ay
    if ay <= ax:
        facing = 1 if dx >= 0 else 3
    else:
        facing = 2 if dy >= 0 else 0
    return (facing, manhattan)


def aim_fixed_point_vector(ex: int, ey: int, tx: int, ty: int, cur_facing: int) -> tuple[int, int, int]:
    """Reference simulation of 0x00E45E..0x00E4CE returning (vx_16_16, vy_16_16, facing)."""
    dx = tx - ex
    dy = ty - ey
    ax, ay = abs(dx), abs(dy)
    major_x = ax > ay
    denom = (ax if major_x else ay) >> 1
    if denom == 0:
        return (0, 0, cur_facing)
    vx = int((dx << 8) / denom) << 8
    vy = int((dy << 8) / denom) << 8
    if vx == 0 and vy == 0:
        return (0, 0, cur_facing)
    if major_x:
        facing = 1 if vx >= 0 else 3
    else:
        facing = 0 if vy < 0 else 2
    return (vx, vy, facing)


def step_z_gravity(z_16_16: int, vz_16_16: int) -> tuple[int, int, bool]:
    """Reference simulation of 0x00E502..0x00E520."""
    if vz_16_16 == 0:
        return (z_16_16, vz_16_16, False)
    vz_16_16 -= 0x0000C000
    if vz_16_16 == 0:
        vz_16_16 -= 1
    z_16_16 += vz_16_16
    return (z_16_16, vz_16_16, z_16_16 < 0)


class TestRomAiSteeringGravity(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["launch_z_vel_16_16"], "0x00060000")
        self.assertEqual(ev["constants"]["gravity_z_16_16"], "0x0000C000")
        saved = json.loads(
            Path("data/thor-evidence/rom_ai_steering_gravity_m15_336.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_steering_aim_and_gravity(self) -> None:
        self.assertEqual(facing_and_manhattan(100, 100, 140, 110), (1, 50))
        self.assertEqual(facing_and_manhattan(100, 100, 95, 60), (0, 45))
        vx, vy, f = aim_fixed_point_vector(0, 0, 20, 10, 2)
        self.assertEqual((vx, vy, f), (0x00020000, 0x00010000, 1))
        # Gravity step from +0xC000 skips exact 0 via -1
        z, vz, landed = step_z_gravity(0x00100000, 0x0000C000)
        self.assertEqual(vz, -1)
        self.assertFalse(landed)


if __name__ == "__main__":
    unittest.main()
