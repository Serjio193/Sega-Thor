#!/usr/bin/env python3
"""Unit tests for M15.360 ROM master gameplay frame loop & menu dispatcher."""

import unittest
from pathlib import Path

from tools.extract_rom_main_game_loop import extract_evidence


def resolve_menu_mode(start_bit: bool, edge_bits: int, ali_state: int, lock_1a6e: int) -> int | None:
    """Mirror 0x008BF2..0x008C56 menu mode selection."""
    if start_bit:
        return 0
    if edge_bits & 0x04:
        return 3
    if ali_state > 2 or lock_1a6e != 0:
        return None
    if edge_bits & 0x02:
        return 1
    if edge_bits & 0x01:
        return 2
    return None


class MainGameLoopTest(unittest.TestCase):
    def test_rom_pipeline_bsr_targets(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(len(ev["frame_loop"]["pipeline_calls"]), 14)
        self.assertEqual(ev["frame_loop"]["pipeline_calls"][-1]["target"], "0x00A196")

    def test_menu_mode_priority(self) -> None:
        self.assertEqual(resolve_menu_mode(True, 0x07, 5, 1), 0)
        self.assertEqual(resolve_menu_mode(False, 0x04, 5, 1), 3)
        self.assertIsNone(resolve_menu_mode(False, 0x03, 3, 0))
        self.assertIsNone(resolve_menu_mode(False, 0x03, 1, 1))
        self.assertEqual(resolve_menu_mode(False, 0x02, 2, 0), 1)
        self.assertEqual(resolve_menu_mode(False, 0x01, 0, 0), 2)


if __name__ == "__main__":
    unittest.main()
