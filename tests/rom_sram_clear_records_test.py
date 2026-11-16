"""Tests for M15.288 SRAM Clear Records & Best Time Persistence extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_sram_clear_records import DEFAULT_ROM_PATH, extract_sram_clear_records


class RomSramClearRecordsTest(unittest.TestCase):
    """Verify Rank00 / Race00 SRAM magic headers and layout."""

    def test_magic_and_layout(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_sram_clear_records(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(data["rank_block"]["magic_ascii"], "Rank00")
        self.assertEqual(data["rank_block"]["payload_bytes"], 130)
        self.assertEqual(data["race_block"]["magic_ascii"], "Race00")
        self.assertEqual(data["sram_control"]["stride_bytes"], 2)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_sram_clear_records_m15_288.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.288")
        self.assertEqual(parsed["rank_block"]["sram_base_addr"], "0x2025CD")
        self.assertEqual(parsed["race_block"]["sram_base_addr"], "0x2026E1")


if __name__ == "__main__":
    unittest.main()
