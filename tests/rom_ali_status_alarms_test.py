#!/usr/bin/env python3
"""Unit tests for M15.366 ROM Ali status tick, low-HP/SP alarms & respawn anchor."""

import unittest
from pathlib import Path

from tools.extract_rom_ali_status_alarms import extract_evidence


def step_low_hp_alarm(timer: int, hp: int) -> tuple[int, bool]:
    if timer != 0:
        return timer - 1, False
    if hp == 0 or hp > 0x40:
        return 0, False
    return (hp >> 1) + 10, True


def step_low_sp_alarm(timer: int, spirit_type: int, sp: int) -> tuple[int, bool]:
    if timer != 0:
        return timer - 1, False
    if spirit_type == 0 or spirit_type == 0x16 or sp > 0x18:
        return 0, False
    return sp + 12, True


class AliStatusAlarmsTest(unittest.TestCase):
    def test_rom_constants(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["low_hp_alarm"]["sfx_id"], "0x00A0038C")
        self.assertEqual(ev["low_sp_alarm"]["sfx_id"], "0x00A004DD")
        self.assertEqual(ev["safe_respawn_anchor"]["allowed_states"], [0, 2, 4])

    def test_hp_sp_heartbeat_cadence(self) -> None:
        self.assertEqual(step_low_hp_alarm(3, 10), (2, False))
        self.assertEqual(step_low_hp_alarm(0, 0x40), (42, True))
        self.assertEqual(step_low_hp_alarm(0, 0x41), (0, False))
        self.assertEqual(step_low_sp_alarm(0, 0x16, 5), (0, False))
        self.assertEqual(step_low_sp_alarm(0, 0x18, 0x18), (36, True))


if __name__ == "__main__":
    unittest.main()
