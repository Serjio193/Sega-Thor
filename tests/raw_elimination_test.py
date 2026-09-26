"""Fail-closed accounting and seal-policy tests for M14.7B."""

import hashlib
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from raw_elimination import SCHEMA, audit_tree, evaluate_seal, event_accounting


class RawEliminationTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.raw = self.root / "raw.bin"
        self.raw.write_bytes(b"raw fixture")
        self.normalized = self.root / "normalized.jsonl"
        self.normalized.write_bytes(b'{"event":"witness"}\n')
        self.session = self.root / "session.sqlite"
        self.session.write_bytes(b"sealed session")
        self.replay = self.root / "replay.json"
        self.replay.write_bytes(b'{"status":"PASS"}')
        self.receipt = self.valid_receipt()
        self.current_versions = {key: value["sha256"]
            for key, value in self.receipt["versions"].items()}

    def artifact(self, path):
        return {"path": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}

    def valid_receipt(self):
        return {"schema": SCHEMA,
            "raw_file": {"path": self.raw.name, "bytes": self.raw.stat().st_size,
                         "sha256": hashlib.sha256(self.raw.read_bytes()).hexdigest()},
            "event_counts": {"input_events": 100, "accepted_events": 95,
                "unresolved_events": 2, "rejected_events": 1, "duplicate_events": 2},
            "versions": {key: {"version": "v1", "sha256": str(index) * 64}
                for index, key in enumerate(
                    ("raw_format", "extractor", "normalizer", "validator",
                     "session_generation", "canonical_generation"), 1)},
            "all_fields_known_to_format_accounted": True, "open_investigations": 0,
            "retained_artifacts": {"normalized_semantic": self.artifact(self.normalized),
                "session_store": self.artifact(self.session),
                "replay_receipt": self.artifact(self.replay)},
            "replay": {"without_raw": True, "graph_hash_match": True,
                "path_hash_match": True, "occurrence_hash_match": True,
                "unresolved_identity_match": True},
            "session_replay": {"without_normalized_evidence": True,
                "graph_hash_match": True, "path_hash_match": True,
                "occurrence_hash_match": True, "unresolved_identity_match": True},
            "unique_branch_witnesses_complete": True,
            "capture_gaps_preserved": True}

    def test_full_event_accounting_includes_all_outcomes(self):
        result = event_accounting(self.receipt["event_counts"])
        self.assertEqual(result["accounted_events"], 100)
        self.assertEqual(result["unaccounted_events"], 0)

    def test_missing_normalized_event_fails_accounting(self):
        counts = dict(self.receipt["event_counts"], accepted_events=94)
        self.assertEqual(event_accounting(counts)["unaccounted_events"], 1)
        self.receipt["event_counts"] = counts
        self.assertFalse(self.seal()["raw_binary_delete_safe"])

    def test_rare_branch_witness_is_required(self):
        self.receipt["unique_branch_witnesses_complete"] = False
        self.assertIn("UNIQUE_BRANCH_WITNESSES_INCOMPLETE", self.seal()["reasons"])

    def test_indirect_target_witnesses_are_part_of_retained_artifact(self):
        self.receipt["retained_artifacts"]["indirect_targets"] = {
            "path": "missing-targets.json", "sha256": "a" * 64}
        self.assertIn("RETAINED_ARTIFACT_MISMATCH:indirect_targets", self.seal()["reasons"])

    def test_return_target_witnesses_are_part_of_retained_artifact(self):
        self.receipt["retained_artifacts"]["return_targets"] = {
            "path": "missing-returns.json", "sha256": "b" * 64}
        self.assertIn("RETAINED_ARTIFACT_MISMATCH:return_targets", self.seal()["reasons"])

    def test_unresolved_event_must_be_accounted_and_replayed(self):
        self.receipt["event_counts"]["unresolved_events"] -= 1
        self.assertIn("UNACCOUNTED_EVENTS", self.seal()["reasons"])

    def test_replay_must_succeed_without_raw(self):
        self.receipt["replay"]["without_raw"] = False
        self.assertIn("REPLAY_WITHOUT_RAW_NOT_PROVEN", self.seal()["reasons"])

    def test_normalized_evidence_has_a_separate_replay_gate(self):
        self.receipt["session_replay"]["without_normalized_evidence"] = False
        result = self.seal()
        self.assertTrue(result["raw_binary_delete_safe"])
        self.assertFalse(result["normalized_evidence_delete_safe"])

    def test_tampered_session_artifact_fails(self):
        self.session.write_bytes(b"tampered")
        self.assertIn("SESSION_STORE_MISSING_OR_MISMATCH",
                      self.seal()["normalized_reasons"])

    def test_stale_extractor_identity_fails(self):
        current = dict(self.current_versions, extractor="0" * 64)
        self.assertIn("CURRENT_VERSION_IDENTITY_UNAVAILABLE_OR_STALE",
                      evaluate_seal(self.receipt, self.root, current)["reasons"])

    def test_stale_canonical_generation_fails(self):
        current = dict(self.current_versions, canonical_generation="0" * 64)
        self.assertIn("CURRENT_VERSION_IDENTITY_UNAVAILABLE_OR_STALE",
                      evaluate_seal(self.receipt, self.root, current)["reasons"])

    def test_duplicate_overlapping_window_is_accounted_as_duplicate(self):
        counts = {"input_events": 10, "accepted_events": 7,
            "unresolved_events": 1, "rejected_events": 1, "duplicate_events": 1}
        self.assertEqual(event_accounting(counts)["unaccounted_events"], 0)

    def test_capture_gap_must_remain_explicit(self):
        self.receipt["capture_gaps_preserved"] = False
        self.assertIn("CAPTURE_GAP_LINEAGE_UNPROVEN", self.seal()["reasons"])

    def test_rejected_events_remain_in_accounting(self):
        self.receipt["event_counts"]["rejected_events"] = 0
        self.assertEqual(self.seal()["accounting"]["unaccounted_events"], 1)

    def test_valid_receipt_marks_both_independent_levels(self):
        result = self.seal()
        self.assertTrue(result["raw_binary_delete_safe"])
        self.assertTrue(result["normalized_evidence_delete_safe"])

    def test_audit_never_deletes_and_classifies_capsules(self):
        capsule = self.root / "capsule-01-lease.bin"
        capsule.write_bytes(b"capsule")
        report = audit_tree(self.root)
        self.assertFalse(report["automatic_delete_performed"])
        self.assertEqual(next(x for x in report["files"] if x["path"].endswith(capsule.name))[
            "schema"], "AUTO67_CAPSULE")

    def seal(self):
        return evaluate_seal(self.receipt, self.root, self.current_versions)


if __name__ == "__main__":
    unittest.main()
