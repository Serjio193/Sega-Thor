"""Tests for M15.300 HUD HP/SP/Enemy Gauge Bar VDP Renderer extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_hud_gauge_bars import (
    DEFAULT_ROM_PATH,
    compute_gauge_fill_rows,
    extract_hud_gauge_bars,
)


class RomHudGaugeBarsTest(unittest.TestCase):
    """Verify HUD gauge VDP commands, 4bpp fills, and 48-row scaler."""

    def test_gauge_constants_and_scaler(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_hud_gauge_bars(ROOT / DEFAULT_ROM_PATH)
        by_name = {g["name"]: g for g in data["gauges"]}
        self.assertEqual(by_name["ali_hp"]["vdp_cmd"], "0x70E00003")
        self.assertEqual(by_name["ali_hp"]["fill_pattern_4bpp"], "0x4E2222E4")
        self.assertEqual(by_name["ali_sp"]["vdp_cmd"], "0x72800003")
        self.assertEqual(by_name["ali_sp"]["fill_pattern_4bpp"], "0x4E6666E4")
        self.assertEqual(by_name["target_enemy_hp"]["vdp_cmd"], "0x74200003")
        self.assertEqual(by_name["target_enemy_hp"]["fill_pattern_4bpp"], "0xDE4444ED")
        self.assertEqual(compute_gauge_fill_rows(0, 200), 0)
        self.assertEqual(compute_gauge_fill_rows(1, 200), 1)
        self.assertEqual(compute_gauge_fill_rows(100, 200), 24)
        self.assertEqual(compute_gauge_fill_rows(200, 200), 48)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_hud_gauge_bars_m15_300.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.300")
        self.assertEqual(parsed["gauge_renderer"]["erase_or_mask"], "0x00EEEE00")


if __name__ == "__main__":
    unittest.main()
