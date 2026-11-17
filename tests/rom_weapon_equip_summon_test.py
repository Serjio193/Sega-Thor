#!/usr/bin/env python3
"""Unit tests for M15.338 ROM Spirit Summon Trigger & Weapon Quick-Equip Class Mapper."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_weapon_equip_summon import ROM_PATH, extract_evidence


def map_weapon_class(item_id: int) -> int:
    """Reference simulation of 0x007F10..0x007F80."""
    if item_id == 1:
        return 0x0A
    if item_id <= 5:
        return 0x0C
    if item_id <= 8:
        return 0x0E
    if item_id <= 0x0B:
        return 0x10
    if item_id <= 0x0C:
        return 0x0E
    return 0x0A


class TestRomWeaponEquipSummon(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["summon_player_state"], 20)
        self.assertEqual(ev["constants"]["equip_toast_frames"], 60)
        saved = json.loads(
            Path("data/thor-evidence/rom_weapon_equip_summon_m15_338.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_weapon_class_mapping(self) -> None:
        self.assertEqual(map_weapon_class(1), 0x0A)
        self.assertEqual(map_weapon_class(3), 0x0C)
        self.assertEqual(map_weapon_class(7), 0x0E)
        self.assertEqual(map_weapon_class(10), 0x10)
        self.assertEqual(map_weapon_class(12), 0x0E)
        self.assertEqual(map_weapon_class(13), 0x0A)


if __name__ == "__main__":
    unittest.main()
