"""Tests for M15.282 Top-Level Game Mode State Machine extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_game_modes import DEFAULT_ROM_PATH, extract_game_modes


class RomGameModesTest(unittest.TestCase):
    """Verify mode pointer table 0x00045E and $FF10AC transition sites."""

    def test_mode_table_handlers(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_game_modes(ROOT / DEFAULT_ROM_PATH)
        by_id = {m["mode_id"]: m["handler_addr"] for m in data["modes"]}
        self.assertEqual(
            by_id,
            {
                0x00: "0x00307A",
                0x04: "0x0032F8",
                0x08: "0x0089B2",
                0x0C: "0x0089B2",
                0x10: "0x03A748",
            },
        )
        self.assertGreaterEqual(len(data["next_mode_refs"]), 9)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_game_modes_m15_282.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.282")
        self.assertEqual(parsed["boot_and_loop"]["mode_table_addr"], "0x00045E")
        self.assertEqual(parsed["boot_and_loop"]["ram_next_mode"], "0xFF10AC")


if __name__ == "__main__":
    unittest.main()
