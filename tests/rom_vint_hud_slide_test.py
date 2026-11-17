"""Tests for M15.312 V-Int Sound Tick & HUD Slide Offset extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_vint_hud_slide import (
    DEFAULT_ROM_PATH,
    extract_vint_hud_slide,
    step_hud_slide_offset,
)


class RomVintHudSlideTest(unittest.TestCase):
    """Verify V-Int HUD slide limits and wrap behavior."""

    def test_hud_slide_constants_and_step(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_vint_hud_slide(ROOT / DEFAULT_ROM_PATH)
        vt = data["vint_tail"]
        self.assertEqual(vt["sound_tick_target"], "0x060000")
        self.assertEqual(vt["fwd_stop_offset"], 0x20)
        self.assertEqual(vt["rev_stop_offset"], 0x138)
        self.assertEqual(vt["wrap_limit"], 0x158)
        self.assertEqual(step_hud_slide_offset(0x20, False), 0x20)
        self.assertEqual(step_hud_slide_offset(0x156, False), 0)
        self.assertEqual(step_hud_slide_offset(0x138, True), 0x138)
        self.assertEqual(step_hud_slide_offset(0, True), 0x158)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_vint_hud_slide_m15_312.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.312")
        self.assertEqual(parsed["vint_tail"]["entry_addr"], "0x002380")


if __name__ == "__main__":
    unittest.main()
