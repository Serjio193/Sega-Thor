#!/usr/bin/env python3
"""Unit tests for M15.377 ROM Ali Cartwheel Evade Hitbox, Ledge-Catch Probe & Standup Sync."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ali_cartwheel_ledge import ROM_PATH, extract_evidence


def cartwheel_damage_code(keyframe: int) -> int:
    """Reference simulation of 0x006742..0x00674E."""
    d0 = 0x0E
    if keyframe >= 0x18:
        d0 += 2
    return d0


def cartwheel_contact_check(keyframe: int, contact_flag: bool) -> tuple[int, bool]:
    """Reference simulation of 0x00677A..0x006796."""
    if keyframe < 0x1C and contact_flag:
        return (0x18, True)  # resets to keyframe 24, clears timer
    return (keyframe, False)


def pit_edge_probe(vz: int, gravity: int, is_edge_detected: bool) -> tuple[int, int | None]:
    """Reference simulation of 0x006810..0x006874."""
    next_vz = vz - gravity
    if is_edge_detected:
        return (next_vz, 0x0032)  # state 0x32: Ledge Catch
    return (next_vz, None)


class TestRomAliCartwheelLedge(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["cartwheel_base_damage"], 14)
        self.assertEqual(ev["constants"]["cartwheel_spin_sfx"], "0x1B400344")
        self.assertEqual(ev["constants"]["standup_recovery_sfx"], "0x0080037A")
        self.assertEqual(ev["constants"]["ledge_catch_state"], "0x0032")
        self.assertEqual(ev["constants"]["standup_state"], "0x0034")
        saved = json.loads(
            Path("data/thor-evidence/rom_ali_cartwheel_ledge_m15_377.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_cartwheel_damage_and_contact(self) -> None:
        self.assertEqual(cartwheel_damage_code(10), 14)
        self.assertEqual(cartwheel_damage_code(23), 14)
        self.assertEqual(cartwheel_damage_code(24), 16)
        self.assertEqual(cartwheel_damage_code(30), 16)

        # Contact check before frame 28 resets keyframe to 24
        self.assertEqual(cartwheel_contact_check(15, contact_flag=True), (24, True))
        self.assertEqual(cartwheel_contact_check(15, contact_flag=False), (15, False))
        # Contact check at/after frame 28 does not reset
        self.assertEqual(cartwheel_contact_check(28, contact_flag=True), (28, False))

    def test_pit_edge_probe(self) -> None:
        vz, state = pit_edge_probe(0x20000, 0xC000, is_edge_detected=True)
        self.assertEqual(vz, 0x14000)
        self.assertEqual(state, 0x0032)

        vz2, state2 = pit_edge_probe(0x20000, 0xC000, is_edge_detected=False)
        self.assertEqual(vz2, 0x14000)
        self.assertIsNone(state2)


if __name__ == "__main__":
    unittest.main()
