#!/usr/bin/env python3
"""Unit tests for M15.327 ROM Ring/Spirit Menu Entry Guard & Input Loop."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ring_menu_loop import ROM_PATH, extract_evidence


def can_open_ring_menu(menu_block: int, player_state: int) -> bool:
    """Reference simulation of 0x004416..0x004438."""
    return menu_block == 0 and player_state <= 2


def decode_ring_menu_action(joypad: int, button_swap: bool) -> str:
    """Reference simulation of 0x0044A6..0x004536."""
    if not button_swap:
        if joypad & 0x60:  # B or C
            return "close"
        if joypad & 0x10:  # A
            return "confirm"
    else:
        if joypad & 0x10:  # A
            return "close"
        if joypad & 0x60:  # B or C
            return "confirm"

    if joypad & 0x80:
        return "start"
    for bit, name in ((0x01, "up"), (0x02, "down"), (0x04, "left"), (0x08, "right")):
        if joypad & bit:
            return name
    return "none"


class TestRomRingMenuLoop(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["max_open_player_state"], 2)
        self.assertEqual(ev["constants"]["error_sfx_cmd"], "0x00A00253")
        self.assertEqual(ev["constants"]["close_sfx_cmd"], "0x00A00347")
        saved = json.loads(
            Path("data/thor-evidence/rom_ring_menu_loop_m15_327.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_entry_guard_and_button_swap(self) -> None:
        self.assertTrue(can_open_ring_menu(0, 2))
        self.assertFalse(can_open_ring_menu(1, 0))
        self.assertFalse(can_open_ring_menu(0, 3))
        self.assertEqual(decode_ring_menu_action(0x10, False), "confirm")
        self.assertEqual(decode_ring_menu_action(0x20, False), "close")
        self.assertEqual(decode_ring_menu_action(0x10, True), "close")
        self.assertEqual(decode_ring_menu_action(0x40, True), "confirm")


if __name__ == "__main__":
    unittest.main()
