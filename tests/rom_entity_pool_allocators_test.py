"""Tests for M15.307 WRAM Entity, Effect & Projectile Object Pool Allocators."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_entity_pool_allocators import (
    DEFAULT_ROM_PATH,
    extract_entity_pool_allocators,
)


class RomEntityPoolAllocatorsTest(unittest.TestCase):
    """Verify all 4 WRAM object pools, strides, and capacities."""

    def test_pool_specs(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_entity_pool_allocators(ROOT / DEFAULT_ROM_PATH)
        by_name = {p["name"]: p for p in data["pools"]}
        self.assertEqual(by_name["effect_pool_8"]["ram_base"], "0xFF2ABC")
        self.assertEqual(by_name["effect_pool_8"]["slot_count"], 8)
        self.assertEqual(by_name["effect_pool_8"]["vram_tile_base"], "0x0694")
        self.assertEqual(by_name["special_pool_6"]["ram_base"], "0xFF2D8C")
        self.assertEqual(by_name["special_pool_6"]["slot_count"], 6)
        self.assertEqual(by_name["full_entity_pool_18"]["ram_base"], "0xFF1C1C")
        self.assertEqual(by_name["full_entity_pool_18"]["slot_count"], 18)
        self.assertEqual(by_name["enemy_entity_pool_17"]["ram_base"], "0xFF1CD8")
        self.assertEqual(by_name["enemy_entity_pool_17"]["slot_count"], 17)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_entity_pool_allocators_m15_307.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.307")
        self.assertEqual(len(parsed["pools"]), 4)


if __name__ == "__main__":
    unittest.main()
