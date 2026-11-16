"""Tests for M15.283 Title Screen, Attract Timeout, and Secret Input Chords."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_title_and_sound_test import DEFAULT_ROM_PATH, extract_title_and_sound_test


class RomTitleAndSoundTestTest(unittest.TestCase):
    """Verify title screen constants, credits, and secret input chords."""

    def test_title_constants_and_chords(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_title_and_sound_test(ROOT / DEFAULT_ROM_PATH)
        ts = data["title_screen"]
        self.assertEqual(ts["attract_timeout_frames"], 0x2D0)
        self.assertEqual(ts["blink_mask"], 0x7F)
        self.assertEqual(ts["blink_on_threshold"], 0x60)
        self.assertIn("1995 SEGA", ts["copyright_text"])
        self.assertIn("Yuzo Koshiro", ts["music_credit_text"])
        chords = {c["pad_byte_ff165e"]: c for c in data["input_chords"]}
        self.assertEqual(chords["0x90"]["target_addr"], "0x03E4FA")
        self.assertEqual(chords["0xD5"]["target_addr"], "0x03E4FA")
        self.assertEqual(chords["0x8A"]["next_mode"], "0x08")

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_title_and_sound_test_m15_283.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.283")
        self.assertEqual(parsed["title_screen"]["entry_addr"], "0x0032F8")


if __name__ == "__main__":
    unittest.main()
