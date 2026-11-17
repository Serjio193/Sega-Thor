#!/usr/bin/env python3
"""Unit tests for M15.337 ROM Debug Stage/Room Warp Menu & Spawn Table."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_debug_warp_menu import ROM_PATH, extract_evidence


class TestRomDebugWarpMenu(unittest.TestCase):
    def test_rom_evidence_matches_saved_and_spawn_table(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertGreater(ev["active_area_count"], 10)
        self.assertEqual(ev["constants"]["repeat_initial_frames"], 10)
        self.assertEqual(ev["constants"]["repeat_held_frames"], 3)
        saved = json.loads(
            Path("data/thor-evidence/rom_debug_warp_menu_m15_337.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_area_0_first_spawn_coords_multiple_of_8(self) -> None:
        ev = extract_evidence(ROM_PATH.read_bytes())
        a0 = ev["areas"][0]
        self.assertEqual(a0["area_id"], 0)
        self.assertEqual(a0["first_room"]["spawn_x_px"] % 8, 0)
        self.assertEqual(a0["first_room"]["spawn_y_px"] % 8, 0)


if __name__ == "__main__":
    unittest.main()
