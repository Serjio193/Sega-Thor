#!/usr/bin/env python3
"""Unit tests for M15.369 ROM Ali ledge climb, pit fall & push trigger."""

import unittest
from pathlib import Path

from tools.extract_rom_ali_ledge_pit_push import extract_evidence


def resolve_ledge_state(axis: str, neg_bit: bool) -> int:
    if axis == "x":
        return 0x2E if neg_bit else 0x2A
    return 0x2C if neg_bit else 0x28


def can_start_push(contact_addr: int, pushable: bool, pad_nibble: int, required_bit: int) -> bool:
    if contact_addr < 0xFF19E8 or contact_addr >= 0xFF2954 or not pushable:
        return False
    return pad_nibble == (1 << required_bit)


class AliLedgePitPushTest(unittest.TestCase):
    def test_rom_constants(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["pit_fall_and_platform"]["pit_state"], "0x3E")
        self.assertEqual(ev["pit_fall_and_platform"]["fall_sfx_normal"], "0x00B00213")
        self.assertEqual(ev["push_block_trigger"]["push_state"], "0x30")

    def test_ledge_and_push_guards(self) -> None:
        self.assertEqual(resolve_ledge_state("x", False), 0x2A)
        self.assertEqual(resolve_ledge_state("x", True), 0x2E)
        self.assertEqual(resolve_ledge_state("y", False), 0x28)
        self.assertEqual(resolve_ledge_state("y", True), 0x2C)
        self.assertTrue(can_start_push(0xFF1AA4, True, 0x08, 3))
        self.assertFalse(can_start_push(0xFF2954, True, 0x08, 3))
        self.assertFalse(can_start_push(0xFF1AA4, True, 0x09, 3))


if __name__ == "__main__":
    unittest.main()
