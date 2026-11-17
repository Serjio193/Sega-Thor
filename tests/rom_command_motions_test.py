#!/usr/bin/env python3
"""Unit tests for M15.357 ROM 360-spin & F-B-F command-motion recognizers."""

import unittest
from pathlib import Path

from tools.extract_rom_command_motions import extract_evidence


def detect_spin_360(samples: list[int], lut: list[int]) -> bool:
    """Mirror 0x0086B4..0x008767 over up to 30 ring samples (low nibble pad bits)."""
    if not samples:
        return False
    limit = min(len(samples), 30)
    stage = 0
    expected = 0
    step_dir = 0
    for i in range(limit):
        d = lut[samples[i] & 0x0F]
        if d < 0 or (d & 1) != 0:
            continue
        if stage == 0:
            expected = d
            stage = 1
        elif stage == 1:
            cw = (expected + 2) & 7
            ccw = (expected - 2) & 7
            if d == cw:
                step_dir = 2
                stage = 2
                expected = (cw + 2) & 7
            elif d == ccw:
                step_dir = -2
                stage = 2
                expected = (ccw - 2) & 7
        elif stage == 2:
            if d == expected:
                stage = 3
                expected = (expected + step_dir) & 7
        else:
            if d == expected:
                return True
    return False


class CommandMotionsTest(unittest.TestCase):
    def test_rom_tables(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["spin_360_recognizer"]["max_samples"], 30)
        self.assertEqual(len(ev["fbf_dash_recognizer"]["patterns"]), 4)
        self.assertEqual(ev["fbf_dash_recognizer"]["patterns"][1]["sequence"][:3], [8, 4, 8])

    def test_spin_360_cw_and_ccw(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        lut = extract_evidence(rom_bytes)["spin_360_recognizer"]["pad_to_dir8_lut"]
        self.assertTrue(detect_spin_360([1, 8, 2, 4], lut))  # Up -> Right -> Down -> Left
        self.assertTrue(detect_spin_360([2, 8, 1, 4], lut))  # Down -> Right -> Up -> Left
        self.assertFalse(detect_spin_360([1, 2, 1, 2], lut))


if __name__ == "__main__":
    unittest.main()
