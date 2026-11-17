#!/usr/bin/env python3
"""Unit tests for M15.370 ROM Ali 4-dir ledge vault launch & 12-frame climb gate."""

import unittest
from pathlib import Path

from tools.extract_rom_ali_ledge_vault_gate import LAUNCH_SPECS, extract_evidence


def step_climb_hold_gate(
    dpad_held: bool,
    cancel_pressed: bool,
    counter: int,
) -> tuple[str, int]:
    """Mirror 0x005C82 climb hold gate -> (action, next_counter)."""
    if not dpad_held:
        return "drop_to_idle", 0
    if cancel_pressed:
        return "holding", counter
    nxt = counter + 1
    if nxt < 12:
        return "holding", nxt
    return "launch_vault", nxt


class AliLedgeVaultGateTest(unittest.TestCase):
    def test_rom_profiles_and_velocities(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(len(ev["launch_profiles"]), 4)
        self.assertEqual(ev["launch_profiles"][0]["vy"], -0x00026666)
        self.assertEqual(ev["launch_profiles"][1]["vx"], 0x0002B333)
        self.assertEqual(ev["vault_common"]["sfx_id"], "0x00800340")

    def test_climb_hold_gate_12_frame_transition(self) -> None:
        self.assertEqual(step_climb_hold_gate(False, False, 5), ("drop_to_idle", 0))
        self.assertEqual(step_climb_hold_gate(True, False, 0), ("holding", 1))
        self.assertEqual(step_climb_hold_gate(True, False, 10), ("holding", 11))
        self.assertEqual(step_climb_hold_gate(True, False, 11), ("launch_vault", 12))
        self.assertEqual(step_climb_hold_gate(True, True, 5), ("holding", 5))


if __name__ == "__main__":
    unittest.main()
