"""Tests for M15.286 Ali Player Complete 59-State Table & Death/Revive extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_player_extended_states import DEFAULT_ROM_PATH, extract_player_extended_states


class RomPlayerExtendedStatesTest(unittest.TestCase):
    """Verify all 59 player states (0x00..0x74) and death/revive constants."""

    def test_all_59_states(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_player_extended_states(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(len(data["states"]), 59)
        by_id = {s["state_id"]: s["target_addr"] for s in data["states"]}
        self.assertEqual(by_id[0x28], "0x005BA2")
        self.assertEqual(by_id[0x3E], "0x006800")
        self.assertEqual(by_id[0x5E], "0x005F92")
        self.assertEqual(by_id[0x60], "0x005FEA")
        self.assertEqual(by_id[0x6C], "0x00611E")
        self.assertEqual(by_id[0x74], "0x006738")

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_player_extended_states_m15_286.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.286")
        self.assertEqual(parsed["table"]["total_states"], 59)
        self.assertEqual(parsed["death_and_revive"]["revive_step_per_frame"], 3)


if __name__ == "__main__":
    unittest.main()
