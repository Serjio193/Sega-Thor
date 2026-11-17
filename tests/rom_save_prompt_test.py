#!/usr/bin/env python3
"""Unit tests for M15.331 ROM In-Game Save Prompt & Save-Allowed Guard."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_save_prompt import ROM_PATH, extract_evidence


def can_open_save_prompt(
    lock_1996: int,
    lock_17c1: int,
    player_state: int,
    room_save_allowed: bool,
    debug_override: bool,
) -> bool:
    """Reference simulation of 0x005488..0x0054C6."""
    if lock_1996 != 0 or lock_17c1 != 0 or player_state > 2:
        return False
    return room_save_allowed or debug_override


def step_save_prompt(choice: int, joypad: int) -> tuple[int, str]:
    """Reference simulation of 0x0054E2..0x00555E returning (next_choice, outcome)."""
    if joypad & 0x60:  # B or C
        return (choice, "cancel")
    if joypad & 0x10:  # A
        return (choice, "save" if choice == 3 else "cancel")
    if (joypad & 0x02) and choice == 3:  # Down
        return (4, "toggle")
    if (joypad & 0x01) and choice == 4:  # Up
        return (3, "toggle")
    return (choice, "idle")


class TestRomSavePrompt(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["choice_yes"], 3)
        self.assertEqual(ev["constants"]["choice_no"], 4)
        self.assertEqual(ev["constants"]["confirm_sfx"], "0x00A003AA")
        saved = json.loads(
            Path("data/thor-evidence/rom_save_prompt_m15_331.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_guard_and_yes_no_prompt(self) -> None:
        self.assertTrue(can_open_save_prompt(0, 0, 1, True, False))
        self.assertFalse(can_open_save_prompt(0, 0, 1, False, False))
        self.assertFalse(can_open_save_prompt(0, 0, 3, True, False))
        self.assertEqual(step_save_prompt(4, 0x01), (3, "toggle"))
        self.assertEqual(step_save_prompt(3, 0x10), (3, "save"))
        self.assertEqual(step_save_prompt(4, 0x10), (4, "cancel"))
        self.assertEqual(step_save_prompt(3, 0x02), (4, "toggle"))


if __name__ == "__main__":
    unittest.main()
