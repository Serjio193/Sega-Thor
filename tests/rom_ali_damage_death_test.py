#!/usr/bin/env python3
"""Unit tests for M15.368 ROM Ali HP damage, death trigger & hitstun/knockdown states."""

import unittest
from pathlib import Path

from tools.extract_rom_ali_damage_death import extract_evidence


def apply_ali_damage(
    damage_word: int,
    defense: int,
    hp: int,
    ali_facing: int,
    hit_facing: int,
) -> tuple[int, int, int, int, bool]:
    """Mirror 0x00586C..0x005940 -> (new_hp, state, anim, new_facing, dead)."""
    raw = (damage_word & 0x0FFF) - defense
    dmg = 1 if raw <= 0 else raw
    dmg = min(dmg, hp)
    new_hp = hp - dmg
    dead = new_hp == 0
    flags = damage_word | (0xC000 if dead else 0)

    if (flags & 0xC000) == 0:
        d0 = hit_facing
        if ali_facing == 3 and (d0 & 1):
            d0 ^= 2
        return new_hp, 0x18, 0x18 + d0 * 2, ali_facing, dead
    if hit_facing != ali_facing:
        return new_hp, 0x1A, 0x26, hit_facing ^ 2, dead
    return new_hp, 0x20, 0x2C, ali_facing, dead


class AliDamageDeathTest(unittest.TestCase):
    def test_rom_constants(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["mount_redirect_invuln_frames"], 7)
        self.assertEqual(ev["hit_reactions"]["knockdown_z_lift"], 16)

    def test_damage_clamp_hitstun_and_death(self) -> None:
        # Defense >= raw -> min 1 damage, light hitstun
        self.assertEqual(apply_ali_damage(0x0004, 10, 50, 3, 1), (49, 0x18, 0x1E, 3, False))
        # Heavy front knockdown
        self.assertEqual(apply_ali_damage(0x8010, 2, 50, 0, 2), (36, 0x1A, 0x26, 0, False))
        # Lethal hit forces knockdown from behind (hit_facing == ali_facing)
        self.assertEqual(apply_ali_damage(0x0010, 0, 10, 1, 1), (0, 0x20, 0x2C, 1, True))


if __name__ == "__main__":
    unittest.main()
