"""Tests for M15.284 Best 5 Players / Clear Records Screen extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_clear_records_screen import DEFAULT_ROM_PATH, extract_clear_records_screen


class RomClearRecordsScreenTest(unittest.TestCase):
    """Verify Best 5 Players screen labels, BGM/SFX IDs, and SRAM addresses."""

    def test_labels_and_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_clear_records_screen(ROOT / DEFAULT_ROM_PATH)
        labels = data["labels"]
        self.assertEqual(labels["title"], "BEST 5 PLAYERS")
        self.assertEqual(labels["col_time"], "TIME")
        self.assertEqual(labels["col_rank"], "RANK")
        self.assertEqual(labels["col_kills"], "KILLS")
        self.assertEqual(labels["col_jewels"], "JEWELS")
        self.assertEqual(labels["rank_1st"], "1st")
        self.assertEqual(labels["rank_5th"], "5th")
        self.assertEqual(data["screen"]["bgm_id"], "0x0121")
        self.assertEqual(data["screen"]["exit_sfx_id"], "0x0007")

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_clear_records_screen_m15_284.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.284")
        self.assertEqual(parsed["screen"]["entry_addr"], "0x03E4FA")


if __name__ == "__main__":
    unittest.main()
