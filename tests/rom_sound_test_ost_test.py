"""Tests for M15.294 Secret Sound Test Menu & 34-Track Named OST Table."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_sound_test_ost import DEFAULT_ROM_PATH, extract_sound_test_ost


class RomSoundTestOstTest(unittest.TestCase):
    """Verify Sound Test labels and all 34 BGM track names/IDs."""

    def test_sound_test_tracks(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_sound_test_ost(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(data["labels"]["title"], "SOUND TEST")
        self.assertEqual(len(data["tracks"]), 34)
        self.assertEqual(data["tracks"][0]["bgm_cmd"], "0x0115")
        self.assertEqual(data["tracks"][0]["title"], "Introduction")
        self.assertEqual(data["tracks"][5]["bgm_cmd"], "0x0104")
        self.assertEqual(data["tracks"][5]["title"], "The Story of Thor")
        self.assertEqual(data["tracks"][33]["bgm_cmd"], "0x0121")
        self.assertEqual(data["tracks"][33]["title"], "Master mind")

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_sound_test_ost_m15_294.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.294")
        self.assertEqual(parsed["sound_test"]["track_count"], 34)


if __name__ == "__main__":
    unittest.main()
