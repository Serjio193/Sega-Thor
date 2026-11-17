#!/usr/bin/env python3
"""Unit tests for M15.361 ROM 5-pool resetter, 32B template init & tint ramp."""

import unittest
from pathlib import Path

from tools.extract_rom_entity_template_pool_init import extract_evidence


def step_tint_intensity(cur: int, active: bool, pulse_timer: int, pulse_parity: int) -> int:
    """Mirror 0x008DD4..0x008E20."""
    if active:
        if cur < 3:
            return cur + 1
        cur = 3
        if pulse_timer >= 0x14 and (pulse_parity & 1) == 0:
            return 4
        return 3
    if cur == 0:
        return 0
    if cur < 4:
        return cur - 1
    return 2


class EntityTemplatePoolInitTest(unittest.TestCase):
    def test_rom_templates_and_pools(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(len(ev["pool_resetter_008c5a"]), 5)
        ali_tpl = ev["template_initializer_008d06"]["sample_templates"][0]
        self.assertEqual(ali_tpl["height_4a"], 48)
        self.assertEqual(ali_tpl["radius_42_44_46"], 10)
        self.assertEqual(ali_tpl["ptr_22_hitbox"], "0x058000")

    def test_tint_ramp_state_machine(self) -> None:
        self.assertEqual(step_tint_intensity(0, True, 0, 0), 1)
        self.assertEqual(step_tint_intensity(3, True, 0x14, 0), 4)
        self.assertEqual(step_tint_intensity(3, True, 0x14, 1), 3)
        self.assertEqual(step_tint_intensity(4, False, 0, 0), 2)
        self.assertEqual(step_tint_intensity(2, False, 0, 0), 1)
        self.assertEqual(step_tint_intensity(0, False, 0, 0), 0)


if __name__ == "__main__":
    unittest.main()
