#!/usr/bin/env python3
"""Unit tests for M15.350 ROM Ali 5-Frame Input-Buffer Action & Spirit-Ball Dispatcher."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ali_input_dispatch import ROM_PATH, extract_evidence


def dispatch_ali_buffered(
    input_locked: bool,
    ring_words: list[int],
    held_joypad: int,
    player_state: int,
    active_spirit: int,
    sp: int,
) -> str:
    """Reference simulation of 0x007DC8..0x007E9C."""
    if input_locked:
        return "none"
    buf = 0
    for w in ring_words[:5]:
        buf |= w
    if buf & 0x40:  # C: Summon
        if player_state != 4 and active_spirit == 0 and sp > 0:
            return "summon"
        return "none"
    if (buf & 0x10) or (held_joypad & 0x10):  # A: Attack
        return "attack"
    if buf & 0x20:  # B: Jump
        return "jump"
    return "none"


class TestRomAliInputDispatch(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["action_buffer_frames"], 5)
        self.assertEqual(ev["constants"]["summon_state"], 20)
        saved = json.loads(
            Path("data/thor-evidence/rom_ali_input_dispatch_m15_350.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_priority_order_and_summon_guard(self) -> None:
        self.assertEqual(dispatch_ali_buffered(True, [0x40], 0x10, 0, 0, 50), "none")
        # C > A > B
        self.assertEqual(dispatch_ali_buffered(False, [0x20, 0x10, 0x40], 0, 0, 0, 50), "summon")
        # Summon blocked if SP==0, falls through to none (0x7E44)
        self.assertEqual(dispatch_ali_buffered(False, [0x40], 0x10, 0, 0, 0), "none")
        self.assertEqual(dispatch_ali_buffered(False, [0x20], 0x10, 0, 0, 0), "attack")
        self.assertEqual(dispatch_ali_buffered(False, [0x20], 0x00, 0, 0, 0), "jump")


if __name__ == "__main__":
    unittest.main()
