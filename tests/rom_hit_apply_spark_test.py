#!/usr/bin/env python3
"""Unit tests for M15.355 ROM hit-apply, guard deflect, knockback & hit-spark."""

import unittest
from pathlib import Path

from tools.extract_rom_hit_apply_spark import KNOCKBACK_PROFILES, extract_evidence


def clamp_midpoint(a_min: int, a_max: int, b_min: int, b_max: int) -> int:
    lo = max(a_min, b_min)
    hi = min(a_max, b_max)
    return lo + ((hi - lo) >> 1)


def select_knockback_profile(damage_word: int, target_z_nonzero: bool) -> tuple[int, int, int, bool]:
    tier = (damage_word >> 14) & 3
    set_air_flag = False
    if tier == 0 and target_z_nonzero:
        tier = 2
        set_air_flag = True
    p = KNOCKBACK_PROFILES[tier]
    return p["vx_mag"], p["vy_mag"], p["vz"], set_air_flag


class HitApplySparkTest(unittest.TestCase):
    def test_rom_jump_table_and_constants(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["enemy_gauge_latch"]["timer_frames"], 0x5A)
        self.assertEqual(ev["guard_deflect"]["block_sfx_id"], "0x00900202")
        self.assertEqual(ev["hit_spark_pool"]["slot_count"], 4)

    def test_knockback_tiers_and_spark_midpoint(self) -> None:
        self.assertEqual(select_knockback_profile(0x0010, False), (0x300, 0x2C0, 0x00000, False))
        self.assertEqual(select_knockback_profile(0x0010, True), (0x400, 0x3C0, 0x28000, True))
        self.assertEqual(select_knockback_profile(0x4010, False), (0x000, 0x000, 0x70000, False))
        self.assertEqual(select_knockback_profile(0xC010, False), (0x700, 0x680, 0x30000, False))

        mid_z = clamp_midpoint(0, 16, 4, 20)
        mid_x = clamp_midpoint(100, 130, 110, 140)
        mid_y = clamp_midpoint(200, 220, 204, 240)
        self.assertEqual((mid_x, mid_y - mid_z), (120, 202))


if __name__ == "__main__":
    unittest.main()
