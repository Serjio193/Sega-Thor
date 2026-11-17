#!/usr/bin/env python3
"""Unit tests for M15.348 ROM Enemy Loot / 2-Tier Drop-Table Roller."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_enemy_loot_drop import ROM_PATH, extract_evidence


def roll_enemy_loot(
    fixed_drop: int,
    no_drop_flag: bool,
    prob1: int,
    item1: int,
    prob2: int,
    item2: int,
    roll1: int,
    roll2: int,
) -> int | None:
    """Reference simulation of 0x00F47A..0x00F4C6."""
    if fixed_drop != 0:
        return None if fixed_drop == 0xFFFF else fixed_drop
    if no_drop_flag:
        return None
    if prob1 != 0 and (roll1 & 0xFFFF) <= prob1:
        return item1
    if prob2 != 0 and (roll2 & 0xFFFF) <= prob2:
        return item2
    return None


class TestRomEnemyLootDrop(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["pickup_entity_type"], "0x0008")
        self.assertEqual(ev["constants"]["pickup_pop_z_vel_16_16"], "0x00080000")
        saved = json.loads(
            Path("data/thor-evidence/rom_enemy_loot_drop_m15_348.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_two_tier_drop_and_fixed_override(self) -> None:
        self.assertIsNone(roll_enemy_loot(0xFFFF, False, 0xFFFF, 0x1E, 0, 0, 0, 0))
        self.assertEqual(roll_enemy_loot(0x011E, False, 0, 0, 0, 0, 0, 0), 0x011E)
        self.assertEqual(roll_enemy_loot(0, False, 1000, 0x1E, 5000, 0x22, 500, 0), 0x1E)
        self.assertEqual(roll_enemy_loot(0, False, 1000, 0x1E, 5000, 0x22, 2000, 3000), 0x22)
        self.assertIsNone(roll_enemy_loot(0, False, 1000, 0x1E, 5000, 0x22, 2000, 6000))


if __name__ == "__main__":
    unittest.main()
