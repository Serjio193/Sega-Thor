"""Tests for M15.292 Continue Game SRAM Load & Entity Restore extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_continue_game_load import DEFAULT_ROM_PATH, extract_continue_game_load


class RomContinueGameLoadTest(unittest.TestCase):
    """Verify Continue Game SRAM primary/backup fallback and restore constants."""

    def test_continue_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_continue_game_load(ROOT / DEFAULT_ROM_PATH)
        cl = data["continue_load"]
        self.assertEqual(cl["sram_payload_words"], 0x12A)
        self.assertEqual(cl["sram_payload_bytes"], 596)
        self.assertEqual(cl["entity_copy_words"], 0xBC)
        self.assertEqual(cl["entity_copy_bytes"], 376)
        self.assertEqual(cl["post_load_invuln_frames"], 30)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_continue_game_load_m15_292.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.292")
        self.assertEqual(parsed["continue_load"]["entry_addr"], "0x00161C")


if __name__ == "__main__":
    unittest.main()
