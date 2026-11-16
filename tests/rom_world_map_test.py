"""Tests for M15.287 World Map Screen & Overworld/Dungeon Pin Tables."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_world_map import DEFAULT_ROM_PATH, extract_world_map


class RomWorldMapTest(unittest.TestCase):
    """Verify World Map graphics chunks and coordinate tables."""

    def test_map_tables_and_math(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_world_map(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(len(data["overworld_areas"]), 29)
        self.assertEqual(len(data["dungeon_pins"]), 16)
        self.assertEqual(data["overworld_areas"][0]["dx"], 52)
        self.assertEqual(data["overworld_areas"][0]["dy"], 24)
        self.assertEqual(data["dungeon_pins"][1]["screen_x"], 201)
        self.assertEqual(data["dungeon_pins"][1]["screen_y"], 160)
        self.assertEqual(len(data["world_map"]["gfx_chunks"]), 3)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_world_map_m15_287.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.287")
        self.assertEqual(parsed["world_map"]["coord_calc_addr"], "0x004B4E")


if __name__ == "__main__":
    unittest.main()
