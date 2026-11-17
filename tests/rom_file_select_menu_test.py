"""Tests for M15.290 Save/Load File Select Menu extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_file_select_menu import DEFAULT_ROM_PATH, extract_file_select_menu


class RomFileSelectMenuTest(unittest.TestCase):
    """Verify File Select 2x3 grid cursor, SFX IDs, and SRAM last-slot addr."""

    def test_file_select_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_file_select_menu(ROOT / DEFAULT_ROM_PATH)
        fs = data["file_select"]
        self.assertEqual(len(data["grid_options"]), 6)
        self.assertEqual(fs["sram_last_slot_addr"], "0x2025C9")
        self.assertEqual(fs["confirm_sfx"], "0x00A00256")
        self.assertEqual(fs["cursor_move_sfx"], "0x006003A6")
        self.assertEqual(fs["invalid_erase_sfx"], "0x00A00253")

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_file_select_menu_m15_290.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.290")
        self.assertEqual(parsed["file_select"]["entry_addr"], "0x0004C6")


if __name__ == "__main__":
    unittest.main()
