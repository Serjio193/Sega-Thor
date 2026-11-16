"""Tests for M15.289 Attract/Demo Stream Table extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_attract_demos import DEFAULT_ROM_PATH, extract_attract_demos


class RomAttractDemosTest(unittest.TestCase):
    """Verify all 9 attract demo stream offsets and playback constants."""

    def test_demo_table_offsets(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_attract_demos(ROOT / DEFAULT_ROM_PATH)
        demos = data["demos"]
        self.assertEqual(len(demos), 9)
        self.assertEqual(demos[0]["rom_addr"], "0x1698FE")
        self.assertEqual(demos[1]["rom_addr"], "0x169D2A")
        self.assertEqual(demos[8]["rom_addr"], "0x16B91C")
        self.assertEqual(data["attract_setup"]["delta_decode_bytes"], 0x1FFF)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_attract_demos_m15_289.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.289")
        self.assertEqual(parsed["attract_setup"]["table_addr"], "0x008A94")


if __name__ == "__main__":
    unittest.main()
