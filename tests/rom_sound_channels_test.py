"""Tests for M15.301 Sound Driver 11-Channel FM/PSG/SFX RAM Map extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_sound_channels import DEFAULT_ROM_PATH, extract_sound_channels


class RomSoundChannelsTest(unittest.TestCase):
    """Verify all 11 sound channel RAM addresses and strides."""

    def test_11_sound_channels(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_sound_channels(ROOT / DEFAULT_ROM_PATH)
        ch = data["channels"]
        self.assertEqual(len(ch), 11)
        self.assertEqual(ch[0]["ram_addr"], "0xFF0022")
        self.assertEqual(ch[5]["ram_addr"], "0xFF0414")
        self.assertEqual(ch[6]["ram_addr"], "0xFF049E")
        self.assertEqual(ch[8]["ram_addr"], "0xFF05B2")
        self.assertEqual(ch[9]["ram_addr"], "0xFF0628")
        self.assertEqual(ch[10]["ram_addr"], "0xFF06F2")
        self.assertEqual(data["sound_channels"]["fade_init_timer"], 0x7E)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_sound_channels_m15_301.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.301")
        self.assertEqual(parsed["sound_channels"]["channel_count"], 11)


if __name__ == "__main__":
    unittest.main()
