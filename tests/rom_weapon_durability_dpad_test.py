#!/usr/bin/env python3
"""Unit tests for M15.356 ROM weapon durability revert & 16-way D-pad velocity."""

import unittest
from pathlib import Path

from tools.extract_rom_weapon_durability_dpad import DPAD_TARGETS, extract_evidence


def step_weapon_durability(weapon_id: int, durability: int) -> tuple[bool, int, int]:
    """Mirror 0x008544..0x0085BC: returns (broke, new_id, new_durability)."""
    if weapon_id < 2 or weapon_id > 12:
        return False, weapon_id, durability
    if durability == 0xFF:
        return False, weapon_id, durability
    if durability == 0 or (durability - 1) == 0:
        return True, 0, 0
    return False, weapon_id, durability - 1


class WeaponDurabilityDpadTest(unittest.TestCase):
    def test_rom_dpad_table_and_constants(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(len(ev["dpad_velocity_table"]), 16)
        self.assertEqual(ev["dpad_velocity_table"][1]["vx"], 0)
        self.assertEqual(ev["dpad_velocity_table"][1]["vy"], -0x00030000)
        self.assertEqual(ev["dpad_velocity_table"][5]["vx"], -0x0002A000)
        self.assertEqual(ev["dpad_velocity_table"][5]["vy"], -0x00025800)
        self.assertEqual(ev["dpad_velocity_table"][3]["moving"], False)

    def test_weapon_durability_decrement_and_revert(self) -> None:
        self.assertEqual(step_weapon_durability(1, 5), (False, 1, 5))
        self.assertEqual(step_weapon_durability(5, 0xFF), (False, 5, 0xFF))
        self.assertEqual(step_weapon_durability(5, 3), (False, 5, 2))
        self.assertEqual(step_weapon_durability(5, 1), (True, 0, 0))
        self.assertEqual(step_weapon_durability(12, 0), (True, 0, 0))


if __name__ == "__main__":
    unittest.main()
