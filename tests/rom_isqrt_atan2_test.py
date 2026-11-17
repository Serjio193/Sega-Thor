#!/usr/bin/env python3
"""Unit tests for M15.341 ROM Integer Square Root & 256-Step atan2 Angle LUT."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_isqrt_atan2 import ROM_PATH, extract_evidence

TAN_LUT = [0x0019, 0x004E, 0x0089, 0x00D2, 0x0138, 0x01DF, 0x034C, 0x0A27, 0xFFFF]


def isqrt_u32(val: int) -> int:
    """Reference simulation of 0x00E6BA..0x00E6EE."""
    if val == 0:
        return 0
    d2 = 0x0400 if val >= 0x0400 else val
    while True:
        d1 = d2
        d2 = ((val // d1) + d1) >> 1
        if d2 >= d1:
            return d1


def atan_q1(dx: int, dy: int) -> int:
    """Reference simulation of 0x00E768..0x00E78C (returns 0..64 in steps of 8)."""
    if dx == 0:
        return 0x40
    ratio = (dy << 8) // dx
    angle = 0
    for thresh in TAN_LUT:
        if ratio <= thresh:
            return angle
        angle += 8
    return angle


def atan2_256(dx: int, dy_up: int) -> int:
    """Reference simulation of 0x00E722..0x00E766 (dy_up = src_y - dst_y)."""
    if dx >= 0:
        if dy_up >= 0:
            return atan_q1(dx, dy_up)
        return (-atan_q1(dx, -dy_up)) & 0xFF
    if dy_up >= 0:
        return ((-atan_q1(-dx, dy_up)) + 0x80) & 0xFF
    return (atan_q1(-dx, -dy_up) + 0x80) & 0xFF


class TestRomIsqrtAtan2(unittest.TestCase):
    def test_rom_evidence_matches_lut(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["tangent_lut_0x05D906"], [f"0x{v:04X}" for v in TAN_LUT])
        saved = json.loads(
            Path("data/thor-evidence/rom_isqrt_atan2_m15_341.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_isqrt_and_atan2_quadrants(self) -> None:
        self.assertEqual(isqrt_u32(0), 0)
        self.assertEqual(isqrt_u32(144), 12)
        self.assertEqual(isqrt_u32(10000), 100)
        self.assertEqual(atan2_256(100, 0), 0)       # Right (0)
        self.assertEqual(atan2_256(100, 100), 32)    # Up-Right (45 deg)
        self.assertEqual(atan2_256(0, 100), 64)      # Up (90 deg)
        self.assertEqual(atan2_256(-100, 0), 128)    # Left (180 deg)
        self.assertEqual(atan2_256(0, -100), 192)    # Down (270 deg)


if __name__ == "__main__":
    unittest.main()
