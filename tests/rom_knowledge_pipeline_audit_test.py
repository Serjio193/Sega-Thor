"""Regression tests for monotonic runtime-evidence audit semantics."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))

from rom_knowledge_evidence_preservation import evidence_ref_preserved, rows_preserved


def row(locator: dict, *, source_sha: str = "a" * 64) -> tuple:
    return ("ref-1", "RUNTIME_OCCURRENCE", "occurrence-1", source_sha,
            "RUNTIME_OCCURRENCE:INSTRUCTION", 1, json.dumps(locator, sort_keys=True))


class RuntimeEvidenceAuditTests(unittest.TestCase):
    def test_enriched_capture_windows_preserve_previous_facts(self):
        old = row({"capture_ids": [1], "windows": [{"worker_id": 0}],
                   "session_source_sha256": "b" * 64, "pc": 0x1234})
        new = row({"capture_ids": [1, 2], "windows": [
            {"worker_id": 0}, {"worker_id": 1}],
            "session_source_sha256s": ["b" * 64, "c" * 64], "pc": 0x1234})
        self.assertTrue(evidence_ref_preserved(old, new))
        self.assertTrue(rows_preserved("evidence_ref", {old}, {new}))

    def test_missing_old_window_is_not_monotonic(self):
        old = row({"capture_ids": [1, 2], "windows": [{"worker_id": 0},
                   {"worker_id": 1}]})
        new = row({"capture_ids": [1], "windows": [{"worker_id": 0}]})
        self.assertFalse(evidence_ref_preserved(old, new))

    def test_changed_fact_identity_is_rejected(self):
        old = row({"pc": 0x1234})
        changed = row({"pc": 0x1234}, source_sha="d" * 64)
        self.assertFalse(evidence_ref_preserved(old, changed))

    def test_old_scalar_session_source_migrates_into_union(self):
        old = row({"session_source_sha256": "b" * 64})
        new = row({"session_source_sha256s": ["b" * 64, "c" * 64]})
        self.assertTrue(evidence_ref_preserved(old, new))

    def test_unrelated_rows_still_require_exact_presence(self):
        self.assertFalse(rows_preserved("claim", {("old", 1)}, {("new", 1)}))
        self.assertTrue(rows_preserved("claim", {("old", 1)},
                                       {("old", 1), ("new", 1)}))


if __name__ == "__main__":
    unittest.main()
