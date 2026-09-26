"""Closed capture receipt and closed-only cleanup acceptance tests."""

from __future__ import annotations

import hashlib
import json
import sqlite3
import sys
import tempfile
import unittest
from unittest import mock
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from experiment_lifecycle import MAP_CHECKS, build_receipt, cleanup_closed, write_receipt
from runtime_path_view import iter_runtime_paths


def identity(label: str) -> dict[str, str]:
    return {"generation": label, "map_sha256": hashlib.sha256(label.encode()).hexdigest()}


class ExperimentLifecycleTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.raw = self.root / "capture.bin"
        self.raw.write_bytes(b"test capture payload")
        self.receipts = self.root / "receipts"
        self.receipts.mkdir()

    def tearDown(self):
        self.temp.cleanup()

    def receipt(self, result="MERGED", counts=None, before=None, after=None, **kwargs):
        source_owned_after = kwargs.pop("source_owned_after", 0)
        return build_receipt(experiment_id="exp-1", raw_path=self.raw,
            raw_root=self.root, format_name="FLOW_V1", producer="fixture",
            schema_version="test-v1", counts=counts or {"input_events": 2,
                "merged_events": 1, "already_known_events": 1,
                "unresolved_events": 0, "rejected_events": 0},
            map_before=before or identity("g0"),
            map_after=after or identity("g1"), result=result,
            map_self_check={"status": "PASS",
                "map_sha256": (after or identity("g1"))["map_sha256"],
                **{key: True for key in MAP_CHECKS}}, source_owned_before=0,
            source_owned_after=source_owned_after, **kwargs)

    def test_merged_close_cleanup_and_queries_preserve_divergent_rare_paths(self):
        envelope = self.root / "capture.jsonl.gz"
        envelope.write_bytes(b"temporary normalized envelope")
        receipt = self.receipt(new_occurrences=6, new_paths=2,
                               temporary_artifacts=[envelope])
        path = self.receipts / "exp-1.experiment.json"
        write_receipt(path, receipt)
        db_path = self.root / "map.sqlite"
        db = sqlite3.connect(db_path)
        db.execute("CREATE TABLE live_forward_runtime_occurrence(occurrence_id TEXT, event_json TEXT)")
        rows = []
        for run, pcs in ((1, (0x10, 0x20, 0x30)), (2, (0x10, 0x20, 0x40))):
            for seq, pc in enumerate(pcs):
                event = {"run_id": run, "epoch": 1, "cpu_id": "M68K",
                    "event_kind": "INSTRUCTION", "pc": pc, "address": pc + 2,
                    "value": 0x4e71, "flags": 1, "native_sequence": seq,
                    "occurrence_id": f"{run}-{seq}", "windows": [{"worker_id": 0,
                    "capture_id": run, "generation": 1, "segment_sha256": "a" * 64}]}
                rows.append((event["occurrence_id"], json.dumps(event)))
        db.executemany("INSERT INTO live_forward_runtime_occurrence VALUES (?,?)", rows)
        db.commit()
        planned = cleanup_closed([path], self.root)
        self.assertEqual(planned["status"], "PLAN_ONLY")
        self.assertTrue(self.raw.exists())
        deleted = cleanup_closed([path], self.root, execute=True)
        paths = list(iter_runtime_paths(db))
        db.close()
        self.assertEqual(deleted["bytes"], len(b"test capture payload") +
                         len(b"temporary normalized envelope"))
        self.assertFalse(self.raw.exists())
        self.assertFalse(envelope.exists())
        self.assertEqual({tuple(item["pcs"]) for item in paths},
                         {(0x10, 0x20, 0x30), (0x10, 0x20, 0x40)})

    def test_no_new_knowledge_is_closed_without_generation_change(self):
        receipt = self.receipt(result="NO_NEW_KNOWLEDGE", before=identity("g0"),
                               after=identity("g0"))
        self.assertTrue(receipt["raw_disposable"])

    def test_invalid_is_closed_without_map_mutation_and_requires_reason(self):
        receipt = self.receipt(result="INVALID", before=identity("g0"),
                               after=identity("g0"), invalid_reason="truncated")
        self.assertEqual(receipt["final_status"], "INVALID")
        with self.assertRaisesRegex(ValueError, "INVALID_REASON"):
            self.receipt(result="INVALID", before=identity("g0"),
                         after=identity("g0"), invalid_reason=None)

    def test_accounting_or_ownership_failure_cannot_close(self):
        with self.assertRaisesRegex(ValueError, "ACCOUNTING_INCOMPLETE"):
            self.receipt(counts={"input_events": 2, "merged_events": 1,
                "already_known_events": 0, "unresolved_events": 0, "rejected_events": 0})
        with self.assertRaisesRegex(ValueError, "SOURCE_OWNED_MUTATION"):
            self.receipt(source_owned_after=1)
        proof = {"status": "PASS", "map_sha256": identity("g1")["map_sha256"],
                 **{key: True for key in MAP_CHECKS}}
        proof.pop("alternate_tails_preserved")
        with self.assertRaisesRegex(ValueError, "SELF_CHECK_FAILED"):
            build_receipt(experiment_id="exp-1", raw_path=self.raw, raw_root=self.root,
                format_name="FLOW_V1", producer="fixture", schema_version="test-v1",
                counts={"input_events": 2, "merged_events": 1,
                    "already_known_events": 1, "unresolved_events": 0,
                    "rejected_events": 0}, map_before=identity("g0"),
                map_after=identity("g1"), result="MERGED", map_self_check=proof,
                source_owned_before=0, source_owned_after=0)

    def test_open_or_tampered_receipt_refuses_cleanup(self):
        receipt = self.receipt()
        receipt["experiment_closed"] = False
        path = self.receipts / "exp-1.experiment.json"
        path.write_text(json.dumps(receipt), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "NOT_CLOSED"):
            cleanup_closed([path], self.root, execute=True)
        self.assertTrue(self.raw.exists())
        receipt["experiment_closed"] = True
        receipt["raw_file"]["sha256"] = "0" * 64
        path.write_text(json.dumps(receipt), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, "IDENTITY_MISMATCH"):
            cleanup_closed([path], self.root, execute=True)

    def test_transaction_failure_conditions_leave_raw_and_map_identity_untouched(self):
        with self.assertRaisesRegex(ValueError, "MAP_GENERATION_CHANGE"):
            self.receipt(result="MERGED", before=identity("g0"), after=identity("g0"))
        with self.assertRaisesRegex(ValueError, "INVALID_INPUT_MUTATED_MAP"):
            self.receipt(result="INVALID", before=identity("g0"), after=identity("g1"))
        self.assertTrue(self.raw.exists())

    def test_cleanup_rechecks_all_receipts_before_unlink(self):
        first = self.receipt()
        second_raw = self.root / "second.bin"
        second_raw.write_bytes(b"second")
        second = build_receipt(experiment_id="exp-2", raw_path=second_raw,
            raw_root=self.root, format_name="FLOW_V1", producer="fixture",
            schema_version="test-v1", counts={"input_events": 1,
                "merged_events": 1, "already_known_events": 0,
                "unresolved_events": 0, "rejected_events": 0},
            map_before=identity("g1"), map_after=identity("g2"), result="MERGED",
            map_self_check={"status": "PASS", "map_sha256": identity("g2")["map_sha256"],
                **{key: True for key in MAP_CHECKS}}, source_owned_before=0,
            source_owned_after=0)
        first_path = self.receipts / "first.experiment.json"
        second_path = self.receipts / "second.experiment.json"
        write_receipt(first_path, first)
        write_receipt(second_path, second)
        original = __import__("experiment_lifecycle")._eligible
        calls = 0

        def fail_on_second(receipt_path, root):
            nonlocal calls
            calls += 1
            if calls == 4:
                raise ValueError("STOP_CHANGED_SECOND_RECEIPT")
            return original(receipt_path, root)

        with mock.patch("experiment_lifecycle._eligible", side_effect=fail_on_second):
            with self.assertRaisesRegex(ValueError, "CHANGED_SECOND"):
                cleanup_closed([first_path, second_path], self.root, execute=True)
        self.assertTrue(self.raw.exists())
        self.assertTrue(second_raw.exists())


if __name__ == "__main__":
    unittest.main()
