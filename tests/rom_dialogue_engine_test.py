"""Tests for M15.281 Dialogue / Text Engine & 116-entry string table."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_dialogue_engine import DEFAULT_ROM_PATH, extract_dialogue_engine


class RomDialogueEngineTest(unittest.TestCase):
    """Verify dialogue table 0x051514, control codes, and font metadata."""

    def test_dialogue_table_and_strings(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_dialogue_engine(ROOT / DEFAULT_ROM_PATH)
        entries = data["entries"]
        self.assertEqual(len(entries), 116)
        self.assertEqual(entries[0]["text_addr"], "0x0515FC")
        self.assertIn("I am Dytto, a water spirit.", entries[0]["text"])
        self.assertIn("Efreet is my name.", entries[9]["text"])
        self.assertEqual(len(data["control_codes"]), 7)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_dialogue_engine_m15_281.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.281")
        self.assertEqual(parsed["font"]["compressed_addr"], "0x16943C")
        self.assertEqual(parsed["engine"]["entry_count"], 116)


if __name__ == "__main__":
    unittest.main()
