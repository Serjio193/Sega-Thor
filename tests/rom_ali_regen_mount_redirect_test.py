#!/usr/bin/env python3
"""Unit tests for M15.367 ROM Ali HP/SP passive regen & mount damage redirect."""

import unittest
from pathlib import Path

from tools.extract_rom_ali_regen_mount_redirect import extract_evidence


def step_passive_regen(
    hp: int,
    max_hp: int,
    sp: int,
    max_sp: int,
    frame: int,
    idle: bool,
    spirit_active: bool,
    acc_flags: int,
    in_water: bool,
) -> tuple[int, int]:
    """Mirror 0x005710..0x0057B0 HP & SP passive regeneration."""
    if idle and (acc_flags & 0x01) and in_water:
        mask = 0x07 if (acc_flags & 0x08) else 0x0F
        if (frame & mask) == 0 and 0 < hp < max_hp:
            hp += 1
    if not spirit_active and (in_water or (acc_flags & 0x04)):
        mask = 0x03 if (acc_flags & 0x08) else 0x07
        if (frame & mask) == 0 and sp < max_sp:
            sp += 1
    return hp, sp


class AliRegenMountRedirectTest(unittest.TestCase):
    def test_rom_constants(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["ali_default_height"], 40)
        self.assertEqual(ev["mount_damage_redirect"]["mount_entity_type"], "0x1C")

    def test_hp_and_sp_regen_cadence(self) -> None:
        self.assertEqual(step_passive_regen(50, 100, 10, 50, 8, True, False, 0x09, True), (51, 11))
        self.assertEqual(step_passive_regen(50, 100, 10, 50, 8, True, False, 0x01, True), (50, 11))
        self.assertEqual(step_passive_regen(50, 100, 10, 50, 0, False, True, 0x05, False), (50, 10))


if __name__ == "__main__":
    unittest.main()
