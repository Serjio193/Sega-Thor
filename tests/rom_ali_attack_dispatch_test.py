#!/usr/bin/env python3
"""Unit tests for M15.340 ROM Ali Attack Action & Weapon-Class State Dispatcher."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ali_attack_dispatch import ROM_PATH, extract_evidence


def dispatch_ground_attack(
    weapon_class: int,
    equipped_item_id: int,
    dpad_held: bool,
    spin_cmd: bool,
    active_proj: bool,
) -> tuple[int, int] | None:
    """Reference simulation of 0x008002..0x0080D4 returning (anim, state) or None."""
    if weapon_class == 0x0C:
        if spin_cmd:
            return (0x66, 0x5C)
        return (0x60, 0x54) if dpad_held else (0x4E, 0x46)
    if weapon_class == 0x0E:
        if active_proj:
            return None
        return (0x5C if equipped_item_id == 8 else 0x54, 0x4C)
    if weapon_class == 0x10:
        if active_proj:
            return None
        return (0x58, 0x50)
    return (0x3A, 0x0A)


class TestRomAliAttackDispatch(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["dispatch_states"]["bow_fire"]["sfx"], "0x00600377")
        self.assertEqual(ev["dispatch_states"]["sword_spin"]["state"], "0x5C")
        saved = json.loads(
            Path("data/thor-evidence/rom_ali_attack_dispatch_m15_340.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_weapon_class_attack_dispatch(self) -> None:
        self.assertEqual(dispatch_ground_attack(0x0A, 1, False, False, False), (0x3A, 0x0A))
        self.assertEqual(dispatch_ground_attack(0x0C, 2, False, False, False), (0x4E, 0x46))
        self.assertEqual(dispatch_ground_attack(0x0C, 2, True, False, False), (0x60, 0x54))
        self.assertEqual(dispatch_ground_attack(0x0C, 2, False, True, False), (0x66, 0x5C))
        self.assertEqual(dispatch_ground_attack(0x0E, 6, False, False, False), (0x54, 0x4C))
        self.assertEqual(dispatch_ground_attack(0x0E, 8, False, False, False), (0x5C, 0x4C))
        self.assertIsNone(dispatch_ground_attack(0x0E, 8, False, False, True))
        self.assertEqual(dispatch_ground_attack(0x10, 9, False, False, False), (0x58, 0x50))


if __name__ == "__main__":
    unittest.main()
