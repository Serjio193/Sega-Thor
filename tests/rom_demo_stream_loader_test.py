#!/usr/bin/env python3
"""Unit tests for M15.359 ROM attract-mode 9-slot demo-stream loader & delta decoder."""

import unittest
from pathlib import Path

from tools.extract_rom_demo_stream_loader import extract_evidence


def next_demo_index(prev_idx: int) -> int:
    nxt = (prev_idx + 1) & 0xFF
    return 0 if nxt > 8 else nxt


def delta_decode_bytes(buf: list[int]) -> list[int]:
    """Mirror 0x008A3A..0x008A48 cumulative 8-bit delta decode."""
    out = list(buf)
    acc = out[0] & 0xFF
    for i in range(1, len(out)):
        acc = (acc + out[i]) & 0xFF
        out[i] = acc
    return out


class DemoStreamLoaderTest(unittest.TestCase):
    def test_rom_stream_table(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(len(ev["streams"]), 9)
        self.assertEqual(ev["streams"][0]["rom_addr"], "0x1698FE")
        self.assertEqual(ev["streams"][8]["rom_addr"], "0x16B91C")

    def test_demo_index_wrap_and_delta_decode(self) -> None:
        self.assertEqual(next_demo_index(0), 1)
        self.assertEqual(next_demo_index(8), 0)
        self.assertEqual(delta_decode_bytes([10, 5, 250, 1]), [10, 15, 9, 10])


if __name__ == "__main__":
    unittest.main()
