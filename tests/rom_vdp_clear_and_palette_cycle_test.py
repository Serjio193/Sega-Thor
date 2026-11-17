"""Tests for M15.306 VDP Full Clear, Armlet Glow & Palette-Cycling Animator."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_vdp_clear_and_palette_cycle import (
    DEFAULT_ROM_PATH,
    extract_vdp_clear_and_palette_cycle,
    step_armlet_glow,
)


class RomVdpClearAndPaletteCycleTest(unittest.TestCase):
    """Verify VDP clear regions, armlet glow transitions, and palette cycler."""

    def test_vdp_clear_and_armlet_glow(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_vdp_clear_and_palette_cycle(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(data["vdp_clear"]["dma_autoinc_reg"], "0x8F01")
        self.assertEqual(data["vdp_clear"]["dma_mode_fill"], "0x9780")
        self.assertEqual(len(data["vdp_clear"]["regions"]), 6)
        self.assertEqual(data["armlet_glow"]["pulse_threshold_ff199e"], 0x14)
        self.assertEqual(step_armlet_glow(0, True, 0, 0), 1)
        self.assertEqual(step_armlet_glow(3, True, 0x14, 0), 4)
        self.assertEqual(step_armlet_glow(4, False, 0, 0), 2)
        self.assertEqual(step_armlet_glow(2, False, 0, 0), 1)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_vdp_clear_and_palette_cycle_m15_306.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.306")
        self.assertEqual(parsed["palette_cycler"]["entry_addr"], "0x008E22")


if __name__ == "__main__":
    unittest.main()
