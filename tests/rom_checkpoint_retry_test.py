"""Tests for M15.293 Checkpoint Snapshot & Game-Over Retry Restore extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_checkpoint_retry import DEFAULT_ROM_PATH, extract_checkpoint_retry


class RomCheckpointRetryTest(unittest.TestCase):
    """Verify checkpoint snapshot and Game-Over retry restore constants."""

    def test_checkpoint_retry_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_checkpoint_retry(ROOT / DEFAULT_ROM_PATH)
        cr = data["checkpoint_retry"]
        self.assertEqual(cr["snapshot_words"], 0x12A)
        self.assertEqual(cr["snapshot_bytes"], 596)
        self.assertEqual(cr["entity_copy_words"], 0xBC)
        self.assertEqual(cr["entity_copy_bytes"], 376)
        self.assertEqual(cr["post_retry_invuln_frames"], 30)
        self.assertTrue(cr["retry_restores_full_hp"])

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_checkpoint_retry_m15_293.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.293")
        self.assertEqual(parsed["checkpoint_retry"]["snapshot_addr"], "0x0017DA")


if __name__ == "__main__":
    unittest.main()
