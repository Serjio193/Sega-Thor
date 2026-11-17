"""Tests for M15.304 Floor Elevation Probe, Terrain LUT & Room Boundary Clamp."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_terrain_probe_and_clamp import (
    DEFAULT_ROM_PATH,
    clamp_entity_to_room_bounds,
    extract_terrain_probe_and_clamp,
)


class RomTerrainProbeAndClampTest(unittest.TestCase):
    """Verify 16-byte terrain LUT and 4-side room boundary clamping."""

    def test_terrain_lut_and_room_clamp(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_terrain_probe_and_clamp(ROOT / DEFAULT_ROM_PATH)
        lut = data["terrain_probe"]["terrain_lut_16"]
        self.assertEqual(len(lut), 16)
        self.assertEqual(lut[1], 0)
        self.assertEqual(lut[2], 2)
        self.assertEqual(lut[3], 1)
        self.assertEqual(lut[12], 5)
        # Right wall clamp (40 tiles = 320px -> max edge 319)
        res_r = clamp_entity_to_room_bounds(311, 100, 5, 0, 8, 40, 32)
        self.assertEqual(res_r["dx"], 0)
        self.assertEqual(res_r["col_x_flags"], 0x48)
        # Left wall clamp
        res_l = clamp_entity_to_room_bounds(8, 100, -5, 0, 8, 40, 32)
        self.assertEqual(res_l["dx"], 0)
        self.assertEqual(res_l["col_x_flags"], 0x48)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_terrain_probe_and_clamp_m15_304.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.304")
        self.assertEqual(parsed["terrain_probe"]["entry_addr"], "0x009D00")


if __name__ == "__main__":
    unittest.main()
