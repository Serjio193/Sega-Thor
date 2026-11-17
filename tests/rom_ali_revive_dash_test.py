#!/usr/bin/env python3
"""Unit tests for M15.374 ROM Ali auto-revive refill, item 0x2A scanner & dash speed."""

import unittest
from pathlib import Path

from tools.extract_rom_ali_revive_dash import extract_evidence


def step_auto_revive(
    hp: int,
    max_hp: int,
    sp: int,
    max_sp: int,
    frame: int,
) -> tuple[int, int, bool]:
    """Mirror 0x006126..0x006162 -> (hp, sp, done)."""
    if (frame & 1) == 0:
        hp = min(hp + 3, max_hp)
    else:
        sp = min(sp + 3, max_sp)
    done = (hp == max_hp) and (sp == max_sp)
    return hp, sp, done


def compute_dash_velocity(vx: int, vy: int) -> tuple[int, int]:
    """Mirror 0x00626E..0x006278: v += (v >> 1)."""
    return vx + (vx >> 1), vy + (vy >> 1)


class AliReviveDashTest(unittest.TestCase):
    def test_rom_constants(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["auto_revive"]["hp_sp_step"], 3)
        self.assertEqual(ev["auto_revive"]["refill_chime_sfx"], "0x00C0034B")
        self.assertEqual(ev["auto_revive"]["revive_jingle_sfx"], "0x00D00355")
        self.assertEqual(ev["revive_item_scanner"]["revive_item_id"], "0x2A")
        self.assertEqual(ev["dash_dispatcher"]["dash_state"], 4)

    def test_revive_step_and_dash_multiplier(self) -> None:
        hp, sp, done = step_auto_revive(10, 15, 10, 15, 0)
        self.assertEqual((hp, sp, done), (13, 10, False))
        hp, sp, done = step_auto_revive(hp, 15, sp, 15, 1)
        self.assertEqual((hp, sp, done), (13, 13, False))
        hp, sp, done = step_auto_revive(hp, 15, sp, 15, 2)
        self.assertEqual((hp, sp, done), (15, 13, False))
        hp, sp, done = step_auto_revive(hp, 15, sp, 15, 3)
        self.assertEqual((hp, sp, done), (15, 15, True))

        self.assertEqual(compute_dash_velocity(0x20000, 0x10000), (0x30000, 0x18000))


if __name__ == "__main__":
    unittest.main()
