#!/usr/bin/env python3
"""Unit tests for M15.351 ROM Ali Jump Trigger, Pickup Probe & 4-Dir Interaction Box."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ali_jump_pickup import ROM_PATH, extract_evidence


def trigger_ali_jump(cur_state: int, dpad_held: bool) -> tuple[int, int, bool]:
    """Reference simulation of 0x008236..0x0082AC returning (new_state, vz_16_16, clear_vel)."""
    if dpad_held and cur_state == 4:
        return (22, 0x00070000, False)
    return (16, 0x00070000, True)


class TestRomAliJumpPickup(unittest.TestCase):
    def test_rom_evidence_matches_constants_and_boxes(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["jump_z_vel_16_16"], "0x00070000")
        self.assertEqual(ev["interact_boxes"]["facing_1_right"], [4, -8, 32, 8])
        self.assertEqual(ev["interact_boxes"]["facing_3_left"], [-32, -8, -4, 8])
        saved = json.loads(
            Path("data/thor-evidence/rom_ali_jump_pickup_m15_351.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_running_vs_standing_jump(self) -> None:
        self.assertEqual(trigger_ali_jump(4, True), (22, 0x00070000, False))
        self.assertEqual(trigger_ali_jump(4, False), (16, 0x00070000, True))
        self.assertEqual(trigger_ali_jump(0, True), (16, 0x00070000, True))


if __name__ == "__main__":
    unittest.main()
