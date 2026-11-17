"""Tests for M15.305 3D AABB Solid Entity Sweep & Push-Contact extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_solid_entity_sweep import (
    DEFAULT_ROM_PATH,
    extract_solid_entity_sweep,
    sweep_solid_x_pos,
)


class RomSolidEntitySweepTest(unittest.TestCase):
    """Verify 21-slot 3D AABB solid sweep and push-contact timer."""

    def test_3d_aabb_sweep_and_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_solid_entity_sweep(ROOT / DEFAULT_ROM_PATH)
        ss = data["solid_sweep"]
        self.assertEqual(ss["slot_count"], 21)
        self.assertEqual(ss["push_contact_timer_frames"], 3)
        blockers = [{"x": 120, "y": 100, "z": 0, "floor_z": 0, "half_w": 10, "half_h": 10, "height_z": 24, "solid": True}]
        res = sweep_solid_x_pos(100, 100, 0, 0, 10, 10, 24, 5, blockers)
        self.assertEqual(res["clamped_dx"], 0)
        self.assertEqual(res["blocker_index"], 0)
        self.assertEqual(res["push_timer"], 3)
        self.assertTrue(res["nudge_blocker"])

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_solid_entity_sweep_m15_305.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.305")
        self.assertEqual(parsed["solid_sweep"]["entry_x_pos_addr"], "0x009DD6")


if __name__ == "__main__":
    unittest.main()
