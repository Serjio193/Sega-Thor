"""Tests for M15.295 CRAM Palette Fade Engine extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_palette_fade import DEFAULT_ROM_PATH, extract_palette_fade, scale_bgr333_color


class RomPaletteFadeTest(unittest.TestCase):
    """Verify CRAM palette fade constants and BGR333 scaling."""

    def test_palette_fade_math_and_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_palette_fade(ROOT / DEFAULT_ROM_PATH)
        pf = data["palette_fade"]
        self.assertEqual(pf["color_count"], 64)
        self.assertEqual(pf["fade_in_step_end"], 8)
        self.assertEqual(pf["fade_out_step_start"], 7)
        self.assertEqual(pf["channel_divisor"], 7)
        self.assertEqual(scale_bgr333_color(0x0EEE, 0), 0x000)
        self.assertEqual(scale_bgr333_color(0x0EEE, 7), 0xEEE)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_palette_fade_m15_295.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.295")
        self.assertEqual(parsed["palette_fade"]["scale_kernel_addr"], "0x002FDA")


if __name__ == "__main__":
    unittest.main()
