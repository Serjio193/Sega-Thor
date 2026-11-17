"""Tests for M15.298 Hit-Reaction Knockback Physics & Shield Parry extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_hit_reactions import DEFAULT_ROM_PATH, extract_hit_reactions


class RomHitReactionsTest(unittest.TestCase):
    """Verify all 4 hit-reaction physics profiles, shield parry, and spark pool."""

    def test_hit_reactions_and_sparks(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_hit_reactions(ROOT / DEFAULT_ROM_PATH)
        r = {x["selector"]: x for x in data["reactions"]}
        self.assertEqual(r[0]["target_addr"], "0x00BF2E")
        self.assertEqual((r[0]["horiz_x"], r[0]["horiz_y"], r[0]["vz_fixed"]), (0x300, 0x2C0, 0))
        self.assertEqual(r[2]["target_addr"], "0x00BF98")
        self.assertEqual(r[2]["vz_fixed"], 0x70000)
        self.assertEqual(r[4]["target_addr"], "0x00BF58")
        self.assertEqual((r[4]["horiz_x"], r[4]["horiz_y"], r[4]["vz_fixed"]), (0x400, 0x3C0, 0x28000))
        self.assertEqual(r[6]["target_addr"], "0x00BF78")
        self.assertEqual((r[6]["horiz_x"], r[6]["horiz_y"], r[6]["vz_fixed"]), (0x700, 0x680, 0x30000))
        self.assertEqual(data["shield_parry"]["sfx_id"], "0x00900202")
        self.assertEqual(data["hit_spark_pool"]["spark_timer_frames"], 5)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_hit_reactions_m15_298.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.298")
        self.assertEqual(len(parsed["reactions"]), 4)


if __name__ == "__main__":
    unittest.main()
