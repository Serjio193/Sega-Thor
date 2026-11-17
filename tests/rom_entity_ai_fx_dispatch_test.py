#!/usr/bin/env python3
"""Unit tests for M15.364 ROM entity AI dispatch table & isometric extent updater."""

import unittest
from pathlib import Path

from tools.extract_rom_entity_ai_fx_dispatch import extract_evidence


def compute_iso_half_extents(w: int, h: int, facing: int) -> tuple[int, int]:
    """Mirror 0x00DF24..0x00DF48: swap on odd facing, y_half = 0.75 * ceil_even(h)."""
    d1, d2 = w & 0xFF, h & 0xFF
    if facing & 1:
        d1, d2 = d2, d1
    d2 = (d2 + 1) >> 1
    d2 = d2 + (d2 >> 1)
    return d1, d2


class EntityAiFxDispatchTest(unittest.TestCase):
    def test_rom_handler_table(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["entity_ai_loop_00deec"]["handler_count"], 89)
        self.assertEqual(ev["entity_ai_loop_00deec"]["handlers"][0]["type_id"], "0x16")
        self.assertEqual(ev["fx_callback_loop_00e0ba"]["slot_count"], 6)
        self.assertEqual(ev["toast_and_pickup_loop_00e0f4"]["slot_count"], 8)

    def test_iso_half_extents(self) -> None:
        self.assertEqual(compute_iso_half_extents(16, 12, 0), (16, 9))
        self.assertEqual(compute_iso_half_extents(16, 12, 1), (12, 12))
        self.assertEqual(compute_iso_half_extents(10, 11, 2), (10, 9))


if __name__ == "__main__":
    unittest.main()
