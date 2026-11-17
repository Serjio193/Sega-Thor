#!/usr/bin/env python3
"""Unit tests for M15.330 ROM Consumable Item Validation & HP/SP/Spirit Restore."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_consumable_effects import ROM_PATH, extract_evidence


def can_use_consumable(
    item_id: int,
    area_id: int,
    hp: int,
    max_hp: int,
    sp: int,
    max_sp: int,
    spirit_active: bool,
    spirit_lock: bool,
    spirits_unlocked_mask: int,
) -> bool:
    """Reference simulation of 0x005236..0x00530A."""
    if area_id == 0x10 or item_id <= 0x19 or item_id > 0x2A:
        return False
    if item_id <= 0x1D:
        if spirit_lock or spirit_active or sp == 0:
            return False
        return bool((spirits_unlocked_mask >> (item_id - 0x1A)) & 1)
    if item_id <= 0x21:
        return hp != max_hp
    if item_id <= 0x25:
        return sp != max_sp
    return hp != max_hp or sp != max_sp


def apply_food_heal(cur: int, max_val: int, tier: int) -> int:
    """Reference simulation of 25%/50%/75%/100% restore tiers (0..3)."""
    if tier == 0:
        gain = max_val >> 2
    elif tier == 1:
        gain = max_val >> 1
    elif tier == 2:
        gain = (max_val >> 1) + (max_val >> 2)
    else:
        gain = max_val
    return min(max_val, cur + gain)


class TestRomConsumableEffects(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(len(ev["jump_table_0x00532C"]), 17)
        self.assertEqual(ev["constants"]["heal_sfx_cmd"], "0x00A00234")
        saved = json.loads(
            Path("data/thor-evidence/rom_consumable_effects_m15_330.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_validation_and_tier_healing(self) -> None:
        # Area 0x10 blocks all consumables
        self.assertFalse(can_use_consumable(0x1E, 0x10, 100, 200, 50, 50, False, False, 0xF))
        # HP food blocked at full HP, allowed when damaged
        self.assertFalse(can_use_consumable(0x1E, 0x00, 200, 200, 50, 50, False, False, 0xF))
        self.assertTrue(can_use_consumable(0x1E, 0x00, 100, 200, 50, 50, False, False, 0xF))
        # Spirit gem requires unlocked bit, SP > 0, no active spirit
        self.assertTrue(can_use_consumable(0x1B, 0x00, 200, 200, 10, 50, False, False, 0x02))
        self.assertFalse(can_use_consumable(0x1B, 0x00, 200, 200, 0, 50, False, False, 0x02))
        # Heal tiers: 25%, 50%, 75%, 100%
        self.assertEqual(apply_food_heal(10, 200, 0), 60)
        self.assertEqual(apply_food_heal(10, 200, 1), 110)
        self.assertEqual(apply_food_heal(10, 200, 2), 160)
        self.assertEqual(apply_food_heal(10, 200, 3), 200)


if __name__ == "__main__":
    unittest.main()
