"""Regression tests for live session-map progress sidecars."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/bizhawk-native-ring"))
from live_session_progress import LiveSessionProgressPublisher
from live_session_progress import LiveSessionProgressView, read_live_session_progress


ROM_SHA = "a" * 64


class LiveSessionProgressTests(unittest.TestCase):
    def test_atomic_snapshot_and_exact_rom_bound_progress(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "live-session-progress.json"
            publisher = LiveSessionProgressPublisher(path, "session-1", ROM_SHA, 30)
            publisher.publish("RUNNING", 10, 0, 240, 210)
            snapshot = read_live_session_progress(path, ROM_SHA)
            label = LiveSessionProgressView(path).label(ROM_SHA)

            self.assertEqual(snapshot["segments_admitted"], 10)
            self.assertIn("admitted segments 10/30 (33.3%)", label)
            self.assertIn("nodes 240 · edges 210", label)
            self.assertIn("SOURCE_OWNED 0", label)
            self.assertFalse(path.with_name(path.name + ".tmp").exists())

    def test_mismatched_rom_and_invalid_schema_fail_closed(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            publisher = LiveSessionProgressPublisher(path, "session-1", ROM_SHA, 2)
            publisher.publish("RUNNING", 1, 0, 1, 0)
            with self.assertRaisesRegex(ValueError, "ROM identity"):
                read_live_session_progress(path, "b" * 64)
            path.write_text(json.dumps({"schema": "unknown"}), encoding="utf-8")
            label = LiveSessionProgressView(path).label(ROM_SHA)
            self.assertIn("rejected", label)

    def test_stale_running_and_failed_state_are_visible(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            publisher = LiveSessionProgressPublisher(path, "session-1", ROM_SHA, 2)
            publisher.publish("RUNNING", 1, 0, 1, 0)
            payload = json.loads(path.read_text(encoding="utf-8"))
            payload["updated_at"] -= 6
            path.write_text(json.dumps(payload), encoding="utf-8")
            self.assertIn("STALE", LiveSessionProgressView(path).label(ROM_SHA))

            publisher.publish("FAILED", 1, 1, 1, 0, error="segment audit failed")
            label = LiveSessionProgressView(path).label(ROM_SHA)
            self.assertIn("FAILED", label)
            self.assertIn("segment audit failed", label)

    def test_writer_refuses_existing_sidecar_and_bad_counts(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "progress.json"
            publisher = LiveSessionProgressPublisher(path, "session-1", ROM_SHA, 2)
            publisher.publish("STARTING", 0, 0, 0, 0)
            with self.assertRaises(FileExistsError):
                LiveSessionProgressPublisher(path, "session-2", ROM_SHA, 2)
            with self.assertRaisesRegex(ValueError, "exceed"):
                publisher.publish("RUNNING", 3, 0, 0, 0)


if __name__ == "__main__":
    unittest.main()
