#!/usr/bin/env python3
"""Unit tests for M15.349 ROM Pickup Overflow Reclaimer, Kill-Counter & Event VM Wait-Tick."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_kill_counter_vm_tick import ROM_PATH, extract_evidence


def increment_kill_counter_sat(kills: int, killed_by_ali: bool, no_count_bit: bool) -> int:
    """Reference simulation of 0x00F75E..0x00F786."""
    if not killed_by_ali or no_count_bit:
        return kills
    return 0xFFFF if kills >= 0xFFFF else kills + 1


def step_event_vm_wait(paused: bool, wait_timer: int) -> tuple[int, bool]:
    """Reference simulation of 0x00F788..0x00F7A8 returning (next_timer, should_run_vm)."""
    if paused:
        return (wait_timer, False)
    if wait_timer > 0:
        return (wait_timer - 1, False)
    return (0, True)


class TestRomKillCounterVmTick(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["guaranteed_drop_item"], "0x1101")
        self.assertEqual(ev["constants"]["pickup_vram_base_tile"], "0x0694")
        saved = json.loads(
            Path("data/thor-evidence/rom_kill_counter_vm_tick_m15_349.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_kill_counter_saturation_and_vm_wait(self) -> None:
        self.assertEqual(increment_kill_counter_sat(10, True, False), 11)
        self.assertEqual(increment_kill_counter_sat(0xFFFF, True, False), 0xFFFF)
        self.assertEqual(increment_kill_counter_sat(10, False, False), 10)
        self.assertEqual(step_event_vm_wait(True, 5), (5, False))
        self.assertEqual(step_event_vm_wait(False, 5), (4, False))
        self.assertEqual(step_event_vm_wait(False, 0), (0, True))


if __name__ == "__main__":
    unittest.main()
