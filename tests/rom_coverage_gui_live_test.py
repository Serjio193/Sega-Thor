"""Check progress reporting for the read-only LIVE checkpoint watcher."""

import struct
import sys
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/bizhawk-native-ring"))
from rom_coverage_gui_live import LiveProgress


class LiveProgressTests(unittest.TestCase):
    def test_initial_checkpoint_is_reported_as_baseline(self):
        progress = LiveProgress()
        progress.record(None, struct.pack("<HH", 0, 1), 10.0)
        label = progress.label("LIVE", False, 12.0)
        self.assertIn("checkpoint checked 2s ago", label)
        self.assertIn("no coverage increase seen yet", label)

    def test_growth_and_unchanged_refresh_remain_visible(self):
        progress = LiveProgress()
        before = struct.pack("<HHHH", 0, 0, 0, 0)
        after = struct.pack("<HHHH", 1, 2, 4, 0)
        progress.record(None, before, 10.0)
        progress.record(before, after, 12.0)
        label = progress.label("LIVE", False, 13.0)
        self.assertIn("+3 covered / +2 classified bytes", label)
        self.assertIn("last increase 1s ago", label)
        progress.record(after, after, 20.0)
        label = progress.label("LIVE", False, 21.0)
        self.assertIn("checkpoint checked 1s ago", label)
        self.assertIn("+3 covered / +2 classified bytes", label)
        self.assertIn("last increase 9s ago", label)

    def test_pending_and_failed_refresh_are_explicit(self):
        progress = LiveProgress()
        self.assertIn("waiting for a readable checkpoint", progress.label("LIVE", False, 0))
        self.assertIn("reading changed checkpoint", progress.label("LIVE", True, 0))
        progress.fail(ValueError("checksum mismatch"))
        self.assertIn("checksum mismatch", progress.label("LIVE", False, 0))

    def test_non_live_mode_does_not_claim_to_watch(self):
        self.assertIn("MONITOR OFF", LiveProgress().label("CHECKPOINT", False, 0))


if __name__ == "__main__":
    unittest.main()
