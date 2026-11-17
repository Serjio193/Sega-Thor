#!/usr/bin/env python3
"""Unit tests for M15.394 ROM Ali Rapid-Mash, Triple-Tap Motion Detectors & Demo Stream Loader."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_ali_rapid_tripletap_demo import ROM_PATH, extract_evidence


def simulate_rapid_mash(ring: list[int], wr_ptr: int, ring_count: int) -> tuple[bool, int]:
    """Reference simulation of 0x008910..0x00894E."""
    count = 0
    ptr = wr_ptr
    for _ in range(13):
        frame_input = ring[(ptr // 2) % len(ring)]
        if frame_input & (1 << 4):
            count += 1
        ptr = (ptr + 2) & 0x7E
    if count >= 3:
        return True, 0  # Carry set, ring_count cleared
    return False, ring_count  # Carry clear


def simulate_triple_tap(ring: list[int], wr_ptr: int, ring_count: int, facing: int) -> tuple[bool, int]:
    """Reference simulation of 0x008950..0x0089B0."""
    if ring_count == 0:
        return False, ring_count
    frames = min(ring_count, 25)
    facing_targets = {0: 0x01, 1: 0x08, 2: 0x02, 3: 0x04}
    target_dir = facing_targets.get(facing, 0)
    matched = 0
    ptr = wr_ptr
    for _ in range(frames):
        frame_input = ring[(ptr // 2) % len(ring)]
        dir_nibble = (frame_input >> 8) & 0x0F  # byte 1 at 1(a0, d5.w)
        if dir_nibble == 0:
            ptr = (ptr + 2) & 0x7E
            continue
        if dir_nibble != target_dir:
            return False, ring_count
        matched += 1
        if matched == 3:
            return True, 0  # Carry set, ring_count cleared
        ptr = (ptr + 2) & 0x7E
    return False, ring_count


class TestRomAliRapidTripletapDemo(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.394")
        self.assertEqual(len(ev["routines"]["demo_asset_offsets_table"]["offsets"]), 9)
        self.assertEqual(ev["routines"]["demo_asset_offsets_table"]["offsets"][1], "0x042C")
        saved = json.loads(
            Path("data/thor-evidence/rom_ali_rapid_tripletap_demo_m15_394.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_rapid_mash_logic(self) -> None:
        ring = [0] * 64
        # 2 presses in 13 frames -> fail
        ring[0] = 0x0010
        ring[2] = 0x0010
        success, cnt = simulate_rapid_mash(ring, 0, 10)
        self.assertFalse(success)
        self.assertEqual(cnt, 10)

        # 3 presses in 13 frames -> success, clears ring count
        ring[4] = 0x0010
        success, cnt = simulate_rapid_mash(ring, 0, 10)
        self.assertTrue(success)
        self.assertEqual(cnt, 0)

    def test_triple_tap_logic(self) -> None:
        ring = [0] * 64
        # Facing 1 (Right -> 0x08)
        # 3 matching Right taps with neutrals in between
        ring[0] = 0x0800
        ring[1] = 0x0000
        ring[2] = 0x0800
        ring[3] = 0x0000
        ring[4] = 0x0800
        success, cnt = simulate_triple_tap(ring, 0, 10, 1)
        self.assertTrue(success)
        self.assertEqual(cnt, 0)

        # Mismatched direction (e.g. Left 0x04 while facing Right) aborts
        ring[2] = 0x0400
        success, cnt = simulate_triple_tap(ring, 0, 10, 1)
        self.assertFalse(success)
        self.assertEqual(cnt, 10)


if __name__ == "__main__":
    unittest.main()
