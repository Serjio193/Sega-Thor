"""Tests for M15.285 Ali Player 20-State Action & Combat Machine."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_player_state_machine import DEFAULT_ROM_PATH, extract_player_state_machine


class RomPlayerStateMachineTest(unittest.TestCase):
    """Verify Ali's 20-state jump table 0x0059BA and tick thresholds."""

    def test_state_table_targets(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_player_state_machine(ROOT / DEFAULT_ROM_PATH)
        states = data["states"]
        self.assertEqual(len(states), 20)
        by_id = {s["state_id"]: s["target_addr"] for s in states}
        self.assertEqual(by_id[0x00], "0x0061F6")
        self.assertEqual(by_id[0x02], "0x0062E4")
        self.assertEqual(by_id[0x06], "0x006DD2")
        self.assertEqual(by_id[0x08], "0x007852")
        self.assertEqual(by_id[0x14], "0x007A10")
        self.assertEqual(by_id[0x1A], "0x005CF8")
        self.assertEqual(by_id[0x20], "0x005CF8")
        self.assertEqual(by_id[0x26], "0x006EDA")

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_player_state_machine_m15_285.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.285")
        self.assertEqual(parsed["player_tick"]["low_hp_threshold"], 0x40)
        self.assertEqual(parsed["player_tick"]["low_sp_threshold"], 0x18)


if __name__ == "__main__":
    unittest.main()
