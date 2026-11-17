#!/usr/bin/env python3
"""Unit tests for M15.345 ROM Projectile Hit Knockback, Enemy Gauge Timer & Gravity."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_projectile_hit_knockback import ROM_PATH, extract_evidence


def step_projectile_z_gravity(z_16_16: int, vz_16_16: int) -> tuple[int, int, bool]:
    """Reference simulation of 0x00EBD8..0x00EBF2."""
    vz_16_16 -= 0x00006000
    z_16_16 += vz_16_16
    if z_16_16 < 0:
        return (0, 0, True)
    return (z_16_16, vz_16_16, False)


class TestRomProjectileHitKnockback(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["enemy_gauge_timer_frames"], 90)
        self.assertEqual(ev["constants"]["projectile_gravity_16_16"], "0x00006000")
        saved = json.loads(
            Path("data/thor-evidence/rom_projectile_hit_knockback_m15_345.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_projectile_gravity_and_ground_clamp(self) -> None:
        z, vz, hit = step_projectile_z_gravity(0x00020000, 0x00004000)
        self.assertFalse(hit)
        self.assertEqual((z, vz), (0x0001E000, -0x00002000))
        z2, vz2, hit2 = step_projectile_z_gravity(0x00001000, 0x00000000)
        self.assertTrue(hit2)
        self.assertEqual((z2, vz2), (0, 0))


if __name__ == "__main__":
    unittest.main()
