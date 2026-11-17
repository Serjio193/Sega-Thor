#!/usr/bin/env python3
"""Unit tests for M15.326 ROM 5-Digit Decimal-to-ASCII Formatter (0x00C0BE..0x00C178)."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_decimal_ascii import ROM_PATH, extract_evidence


def format_u16_zero_padded(val: int) -> str:
    return f"{val & 0xFFFF:05d}"


def format_u16_space_padded(val: int) -> str:
    return f"{val & 0xFFFF:5d}"


class TestRomDecimalAscii(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["output_buf_ram"], "0x00FF192A")
        self.assertEqual(ev["constants"]["divisors"], [10000, 1000, 100, 10, 1])
        saved = json.loads(
            Path("data/thor-evidence/rom_decimal_ascii_m15_326.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_zero_and_space_padded_formatting(self) -> None:
        self.assertEqual(format_u16_zero_padded(0), "00000")
        self.assertEqual(format_u16_zero_padded(105), "00105")
        self.assertEqual(format_u16_zero_padded(65535), "65535")
        self.assertEqual(format_u16_space_padded(0), "    0")
        self.assertEqual(format_u16_space_padded(105), "  105")
        self.assertEqual(format_u16_space_padded(65535), "65535")


if __name__ == "__main__":
    unittest.main()
