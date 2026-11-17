#!/usr/bin/env python3
"""Unit tests for M15.396 ROM Master Entity Subsystems Clear, Universal Constructor & Spirit Glow Pulse."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_entity_constructor_lifecycle import ROM_PATH, extract_evidence


def simulate_spirit_glow_pulse(glow_level: int, has_spirit: bool, timer_val: int) -> int:
    """Reference simulation of 0x008DD4..0x008E22."""
    d0 = glow_level & 0xFF
    if has_spirit:
        if d0 < 3:
            return d0 + 1
        d0 = 3
        if timer_val >= 20 and (timer_val & 1) == 0:
            return d0 + 1
        return d0
    else:
        if d0 == 0:
            return 0
        if d0 >= 4:
            return 2
        return d0 - 1


class TestRomEntityConstructorLifecycle(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.396")
        self.assertEqual(len(ev["routines"]["master_entity_subsystems_clear"]["pools"]), 5)
        self.assertEqual(ev["routines"]["master_entity_subsystems_clear"]["pools"][4]["slots"], 19)
        saved = json.loads(
            Path("data/thor-evidence/rom_entity_constructor_lifecycle_m15_396.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_spirit_glow_pulse_ramp_and_pulse(self) -> None:
        # Ramp up towards 3 when spirit summoned
        self.assertEqual(simulate_spirit_glow_pulse(0, True, 0), 1)
        self.assertEqual(simulate_spirit_glow_pulse(1, True, 0), 2)
        self.assertEqual(simulate_spirit_glow_pulse(2, True, 0), 3)

        # Pulse to 4 on even frames >= 20
        self.assertEqual(simulate_spirit_glow_pulse(3, True, 19), 3)
        self.assertEqual(simulate_spirit_glow_pulse(3, True, 20), 4)
        self.assertEqual(simulate_spirit_glow_pulse(3, True, 21), 3)
        self.assertEqual(simulate_spirit_glow_pulse(3, True, 22), 4)

    def test_spirit_glow_fade_down(self) -> None:
        # When spirit dismissed, glow fades down
        self.assertEqual(simulate_spirit_glow_pulse(4, False, 0), 2)
        self.assertEqual(simulate_spirit_glow_pulse(3, False, 0), 2)
        self.assertEqual(simulate_spirit_glow_pulse(2, False, 0), 1)
        self.assertEqual(simulate_spirit_glow_pulse(1, False, 0), 0)
        self.assertEqual(simulate_spirit_glow_pulse(0, False, 0), 0)


if __name__ == "__main__":
    unittest.main()
