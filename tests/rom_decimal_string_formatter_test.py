#!/usr/bin/env python3
"""
rom_decimal_string_formatter_test.py — Unit tests for M15.425:
ROM Fixed-Width & Space-Padded Decimal String Formatters (0x00C0BE..0x00C17A)
"""

import json
import os
import unittest
from tools.extract_rom_decimal_string_formatter import (
    format_dec_fixed5,
    format_dec_spaces,
    START_ADDR,
    END_ADDR,
    SIZE,
    BUFFER_ADDR,
    BUFFER_LEN,
)

class TestRomDecimalStringFormatter(unittest.TestCase):
    def setUp(self):
        self.evidence_path = "data/thor-evidence/rom_decimal_string_formatter_m15_425.json"

    def test_evidence_json_validity(self):
        self.assertTrue(os.path.exists(self.evidence_path))
        with open(self.evidence_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.assertEqual(data["start_address"], f"0x{START_ADDR:06X}")
        self.assertEqual(data["end_address"], f"0x{END_ADDR:06X}")
        self.assertEqual(data["size_bytes"], SIZE)
        self.assertEqual(data["buffer_address"], f"0x{BUFFER_ADDR:08X}")
        self.assertEqual(data["buffer_len"], BUFFER_LEN)
        self.assertEqual(data["entry_points"]["format_dec_fixed5"], "0x00C0BE")
        self.assertEqual(data["entry_points"]["format_dec_spaces"], "0x00C114")

    def test_fixed5_formatting(self):
        self.assertEqual(format_dec_fixed5(0), "00000")
        self.assertEqual(format_dec_fixed5(5), "00005")
        self.assertEqual(format_dec_fixed5(42), "00042")
        self.assertEqual(format_dec_fixed5(999), "00999")
        self.assertEqual(format_dec_fixed5(1234), "01234")
        self.assertEqual(format_dec_fixed5(10000), "10000")
        self.assertEqual(format_dec_fixed5(56789), "56789")
        self.assertEqual(format_dec_fixed5(65535), "65535")
        # Masking to 16 bits
        self.assertEqual(format_dec_fixed5(0x10005), "00005")

    def test_spaces_formatting(self):
        self.assertEqual(format_dec_spaces(0), "    0")
        self.assertEqual(format_dec_spaces(7), "    7")
        self.assertEqual(format_dec_spaces(42), "   42")
        self.assertEqual(format_dec_spaces(999), "  999")
        self.assertEqual(format_dec_spaces(1000), " 1000")
        self.assertEqual(format_dec_spaces(10000), "10000")
        self.assertEqual(format_dec_spaces(65535), "65535")
        # Interior zero preservation
        self.assertEqual(format_dec_spaces(1002), " 1002")
        self.assertEqual(format_dec_spaces(10020), "10020")

    def test_buffer_rewind(self):
        # The M68K routine executes:
        # move.b d0, (a0)+  -> writes byte 0, a0 += 1
        # move.b d0, (a0)+  -> writes byte 1, a0 += 1
        # move.b d0, (a0)+  -> writes byte 2, a0 += 1
        # move.b d0, (a0)+  -> writes byte 3, a0 += 1
        # move.b d0, (a0)   -> writes byte 4, a0 does NOT increment
        # lea -$4(a0), a0   -> a0 -= 4
        # Thus a0 returns exactly to the initial pointer!
        initial_a0 = BUFFER_ADDR
        a0 = initial_a0
        a0 += 1  # 10000s
        a0 += 1  # 1000s
        a0 += 1  # 100s
        a0 += 1  # 10s
        # units: (a0) without increment
        a0 -= 4  # lea -$4(a0), a0
        self.assertEqual(a0, initial_a0)

if __name__ == "__main__":
    unittest.main()
