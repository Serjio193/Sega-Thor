"""Tests for M15.302 Entity Struct Constructor & 0x03F2FA Field Map extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_entity_struct_layout import DEFAULT_ROM_PATH, extract_entity_struct_layout


class RomEntityStructLayoutTest(unittest.TestCase):
    """Verify 0x008D06 constructor constants and 16 descriptor fields."""

    def test_entity_layout_and_ali_sample(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_entity_struct_layout(ROOT / DEFAULT_ROM_PATH)
        c = data["constructor"]
        self.assertEqual(c["master_table_addr"], "0x03F2FA")
        self.assertEqual(c["light_entity_ram_threshold"], "0xFF2954")
        self.assertEqual(len(data["descriptor_fields"]), 16)
        ali = data["sample_ali_id2"]
        self.assertGreater(ali["height_z"], 0)
        self.assertGreater(ali["hitbox_half_w"], 0)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_entity_struct_layout_m15_302.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.302")
        self.assertEqual(parsed["constructor"]["entry_addr"], "0x008D06")


if __name__ == "__main__":
    unittest.main()
