"""Tests for M15.299 Ali Command-Input Special Moves extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_special_move_inputs import DEFAULT_ROM_PATH, extract_special_move_inputs


class RomSpecialMoveInputsTest(unittest.TestCase):
    """Verify 360-spin D-pad LUT and F-B-F lunge sequences."""

    def test_spin_and_fbf_tables(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_special_move_inputs(ROOT / DEFAULT_ROM_PATH)
        lut = data["spin_360"]["dpad_lut_16"]
        self.assertEqual(len(lut), 16)
        self.assertEqual(lut[1], 0)
        self.assertEqual(lut[8], 2)
        self.assertEqual(lut[2], 4)
        self.assertEqual(lut[4], 6)
        fbf = data["fbf_lunge"]["facings"]
        self.assertEqual(len(fbf), 4)
        self.assertEqual(fbf[0]["sequence"], [1, 2, 1, 0])
        self.assertEqual(fbf[1]["sequence"], [8, 4, 8, 0])
        self.assertEqual(fbf[2]["sequence"], [2, 1, 2, 0])
        self.assertEqual(fbf[3]["sequence"], [4, 8, 4, 0])

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_special_move_inputs_m15_299.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.299")
        self.assertEqual(parsed["spin_360"]["max_history_frames"], 30)


if __name__ == "__main__":
    unittest.main()
