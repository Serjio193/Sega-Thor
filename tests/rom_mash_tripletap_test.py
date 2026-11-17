#!/usr/bin/env python3
"""Unit tests for M15.347 ROM Rapid-Mash & Triple-Tap Input Ring Detectors."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_mash_tripletap import ROM_PATH, extract_evidence


def detect_rapid_mash(recent_13_words: list[int]) -> bool:
    """Reference simulation of 0x008910..0x00894E."""
    hits = sum(1 for w in recent_13_words[:13] if (w & 0x10) != 0)
    return hits >= 3


def detect_triple_tap(recent_dpad_bytes: list[int], target_dir_nibble: int) -> bool:
    """Reference simulation of 0x008950..0x0089A8."""
    rem = 3
    for d in recent_dpad_bytes[:25]:
        n = d & 0x0F
        if n == 0:
            continue
        if n != target_dir_nibble:
            return False
        rem -= 1
        if rem == 0:
            return True
    return False


class TestRomMashTripleTap(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["mash_window_frames"], 13)
        self.assertEqual(ev["constants"]["tap_max_window_frames"], 25)
        saved = json.loads(
            Path("data/thor-evidence/rom_mash_tripletap_m15_347.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_mash_and_triple_tap_detection(self) -> None:
        self.assertFalse(detect_rapid_mash([0x10, 0, 0x10] + [0] * 10))
        self.assertTrue(detect_rapid_mash([0x10, 0, 0x10, 0, 0x10] + [0] * 8))
        self.assertTrue(detect_triple_tap([0x08, 0x00, 0x08, 0x08], 0x08))
        self.assertFalse(detect_triple_tap([0x08, 0x04, 0x08, 0x08], 0x08))


if __name__ == "__main__":
    unittest.main()
