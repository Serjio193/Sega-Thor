"""Tests for M15.291 New Game Initialization extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_new_game_init import DEFAULT_ROM_PATH, extract_new_game_init


class RomNewGameInitTest(unittest.TestCase):
    """Verify New Game starting stats, room/spawn, and secret chords."""

    def test_starting_values(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_new_game_init(ROOT / DEFAULT_ROM_PATH)
        ng = data["new_game"]
        self.assertEqual(ng["max_hp"], 200)
        self.assertEqual(ng["hp"], 200)
        self.assertEqual(ng["base_attack_ff0d7e"], 10)
        self.assertEqual(ng["start_room_id"], 0x001B)
        self.assertEqual(ng["start_x"], 256)
        self.assertEqual(ng["start_y"], 80)
        self.assertEqual(len(data["secret_chords"]), 4)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_new_game_init_m15_291.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.291")
        self.assertEqual(parsed["new_game"]["entry_addr"], "0x001300")


if __name__ == "__main__":
    unittest.main()
