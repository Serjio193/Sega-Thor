import hashlib
import json
import sqlite3
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from live_forward_scaling_audit import RECORD
from master_v2_forensic_reconciliation import (AMBIGUOUS, PARTIAL, Snapshot,
                                                exact_transition, scan_flow,
                                                session_probe)


class ForensicReconciliationTests(unittest.TestCase):
    def test_exact_transition_requires_single_added_run(self):
        parent = Snapshot("parent", Path("parent.sqlite"), frozenset({1, 2}),
                          {"000001:1111": 4}, {"000001:EXECUTED_NEXT:000002": 3})
        child = Snapshot("child", Path("child.sqlite"), frozenset({1, 2, 9}),
                         {"000001:1111": 7}, {"000001:EXECUTED_NEXT:000002": 5})
        self.assertEqual(exact_transition([parent, child], 9), (parent, child))
        self.assertIsNone(exact_transition([parent, child], 8))

    def test_lineage_only_has_no_transition(self):
        snapshot = Snapshot("only", Path("only.sqlite"), frozenset({1, 2, 9}), {}, {})
        self.assertIsNone(exact_transition([snapshot], 9))
        self.assertEqual(AMBIGUOUS, "STILL_AMBIGUOUS")

    def test_missing_flow_is_partial_source(self):
        self.assertEqual(PARTIAL, "PARTIALLY_COMMITTED")
        item = {"source_evidence": {"raw_path": "missing", "index_path": "missing"}}
        with self.assertRaisesRegex(ValueError, "FLOW_EVIDENCE_UNAVAILABLE"):
            scan_flow(item)

    def test_scan_flow_is_deterministic(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw = b"".join((RECORD.pack(1, 2, 0, 0x100, 0x104, 0x200, 1, 0, 0, 0, 0, 0),
                            RECORD.pack(3, 4, 0, 0x104, 0x108, 0x202, 1, 0, 0, 0, 0, 0)))
            raw_path = root / "raw.bin"
            raw_path.write_bytes(raw)
            index_path = root / "index.jsonl"
            index_path.write_text(json.dumps({
                "raw_offset": 0, "raw_length": len(raw),
                "raw_sha256": hashlib.sha256(raw).hexdigest(),
                "segment": {"run_id": 9, "record_count": 2}
            }) + "\n", encoding="utf-8")
            entry = {"run_id": 9, "source_evidence": {
                "raw_path": str(raw_path), "index_path": str(index_path),
                "raw_sha256": hashlib.sha256(raw).hexdigest()},
                "segments": 1, "records": 2}
            first = scan_flow(entry)
            second = scan_flow(entry)
            self.assertEqual(first, second)
            self.assertEqual(first["instruction_occurrence_contribution"], 2)
            self.assertEqual(first["relation_contribution"], 2)

    def test_session_probe_is_read_only(self):
        with tempfile.TemporaryDirectory() as directory:
            db_path = Path(directory) / "session.sqlite"
            db = sqlite3.connect(db_path)
            db.executescript("""
                CREATE TABLE map_meta(key TEXT PRIMARY KEY, value TEXT);
                CREATE TABLE map_import(id INTEGER);
                INSERT INTO map_meta VALUES ('live_forward_run_id','9');
                INSERT INTO map_meta VALUES ('live_forward_session_state','CLOSED');
                INSERT INTO map_import VALUES (1);
            """)
            db.commit()
            before = db_path.stat().st_size
            db.close()
            result = session_probe(db_path, 9)
            self.assertTrue(result["present"])
            self.assertTrue(result["run_id_match"])
            self.assertEqual(result["map_import_rows"], 1)
            self.assertEqual(before, db_path.stat().st_size)


if __name__ == "__main__":
    unittest.main()
