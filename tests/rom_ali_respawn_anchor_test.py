#!/usr/bin/env python3
"""Unit tests for M15.378 ROM Ali Pit-Fall Damage, Safe Respawn Anchor & 3px Coordinate Interpolator."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ali_respawn_anchor import ROM_PATH, extract_evidence


def apply_pit_damage_tick(timer: int, hp: int, has_revive_item: bool) -> tuple[int, int, int | None]:
    """Reference simulation of 0x006944..0x006996."""
    next_timer = timer - 1
    if next_timer == 10:
        next_hp = max(0, hp - 20)
        if next_hp == 0:
            if not has_revive_item:
                return (next_timer, next_hp, 0x0070)  # Fatal death state
            return (next_timer, next_hp, None)       # Auto-revive trigger
        return (next_timer, next_hp, None)
    return (next_timer, hp, None)


def step_1px_interpolator(current: int, target: int, counter: int) -> tuple[int, int]:
    """Reference simulation of 0x006B12..0x006B28."""
    if current < target:
        return (current + 1, counter + 1)
    elif current > target:
        return (current - 1, counter + 1)
    return (current, counter)


def step_3px_coordinate_axis(current: int, target: int, counter: int) -> tuple[int, int]:
    """Simulates 3 consecutive BSR 0x006B12 calls per axis."""
    for _ in range(3):
        current, counter = step_1px_interpolator(current, target, counter)
    return (current, counter)


class TestRomAliRespawnAnchor(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["pit_damage_timer_threshold"], 10)
        self.assertEqual(ev["constants"]["pit_fall_damage_hp"], 20)
        self.assertEqual(ev["constants"]["fatal_death_state"], "0x0070")
        self.assertEqual(ev["constants"]["game_over_fade_frames"], 40)
        self.assertEqual(ev["constants"]["respawn_invuln_timer_frames"], 63)
        self.assertEqual(ev["constants"]["post_respawn_invuln_frames"], 12)
        saved = json.loads(
            Path("data/thor-evidence/rom_ali_respawn_anchor_m15_378.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_pit_damage_application(self) -> None:
        # Timer not 10: no damage
        t, hp, death = apply_pit_damage_tick(timer=15, hp=50, has_revive_item=False)
        self.assertEqual((t, hp, death), (14, 50, None))

        # Timer reaching 10: applies 20 damage
        t, hp, death = apply_pit_damage_tick(timer=11, hp=50, has_revive_item=False)
        self.assertEqual((t, hp, death), (10, 30, None))

        # Timer reaching 10 with HP <= 20 and no revive: fatal death state 0x70
        t, hp, death = apply_pit_damage_tick(timer=11, hp=15, has_revive_item=False)
        self.assertEqual((t, hp, death), (10, 0, 0x0070))

        # Timer reaching 10 with HP <= 20 and revive present: skips death state 0x70
        t, hp, death = apply_pit_damage_tick(timer=11, hp=15, has_revive_item=True)
        self.assertEqual((t, hp, death), (10, 0, None))

    def test_coordinate_interpolator(self) -> None:
        # Move +3 pixels towards target 100 from 90
        coord, d5 = step_3px_coordinate_axis(current=90, target=100, counter=0)
        self.assertEqual((coord, d5), (93, 3))

        # Move -3 pixels towards target 90 from 100
        coord, d5 = step_3px_coordinate_axis(current=100, target=90, counter=0)
        self.assertEqual((coord, d5), (97, 3))

        # At target: does not change
        coord, d5 = step_3px_coordinate_axis(current=100, target=100, counter=0)
        self.assertEqual((coord, d5), (100, 0))


if __name__ == "__main__":
    unittest.main()
