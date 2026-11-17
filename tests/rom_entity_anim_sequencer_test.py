"""Tests for M15.310 Entity Animation Frame Sequencer extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_entity_anim_sequencer import (
    DEFAULT_ROM_PATH,
    extract_entity_anim_sequencer,
    step_anim_keyframe,
)


class RomEntityAnimSequencerTest(unittest.TestCase):
    """Verify 4-byte anim keyframe sequencer and wrap/completion flags."""

    def test_anim_sequencer_and_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_entity_anim_sequencer(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(data["anim_sequencer"]["keyframe_stride_bytes"], 4)
        self.assertEqual(data["physics_stepper"]["tile_step_pixels"], 8)
        kf = [(0x10, 6), (0x12, 0x8002)]
        s1 = step_anim_keyframe(kf, 0, 1 << 3)
        self.assertEqual(s1["frame_id"], 0x12)
        self.assertEqual(s1["timer_2e"], 0)
        self.assertTrue((s1["combat_flags_37"] & (1 << 2)) != 0)
        self.assertTrue((s1["combat_flags_37"] & (1 << 1)) != 0)
        self.assertTrue((s1["combat_flags_37"] & (1 << 3)) == 0)
        s2 = step_anim_keyframe(kf, s1["cursor_bytes"], s1["combat_flags_37"])
        self.assertEqual(s2["cursor_bytes"], 0)
        self.assertEqual(s2["frame_id"], 0x10)
        self.assertEqual(s2["timer_2e"], 2)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_entity_anim_sequencer_m15_310.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.310")
        self.assertEqual(parsed["anim_sequencer"]["entry_addr"], "0x008FF0")


if __name__ == "__main__":
    unittest.main()
