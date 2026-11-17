#!/usr/bin/env python3
"""Unit tests for M15.373 ROM Game Over death sequence, banner & palette."""

import unittest
from pathlib import Path

from tools.extract_rom_game_over_banner import extract_evidence


def is_game_over_button_pressed(pad_word: int) -> bool:
    """Mirror 0x0060A8..0x0060C6: checks bits 4, 5, 6, 7 (A, B, C, Start)."""
    return bool(pad_word & 0x00F0)


class GameOverBannerTest(unittest.TestCase):
    def test_rom_banner_and_palette(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["post_death_delay_frames"], 40)
        self.assertEqual(ev["game_over_sfx"], "0x00D00355")
        self.assertEqual(ev["banner_graphic"]["rom_addr"], "0x0541FE")
        self.assertEqual(ev["banner_graphic"]["nametable_addr"], "0xE000")
        self.assertEqual(len(ev["palette_line_3"]["colors"]), 16)
        self.assertEqual(ev["palette_line_3"]["colors"][0], "0x0000")
        self.assertEqual(ev["palette_line_3"]["colors"][1], "0x08CE")

    def test_button_wait_gate(self) -> None:
        self.assertFalse(is_game_over_button_pressed(0x000F))  # D-pad only
        self.assertTrue(is_game_over_button_pressed(0x0080))   # Start
        self.assertTrue(is_game_over_button_pressed(0x0010))   # A
        self.assertTrue(is_game_over_button_pressed(0x0020))   # B
        self.assertTrue(is_game_over_button_pressed(0x0040))   # C


if __name__ == "__main__":
    unittest.main()
