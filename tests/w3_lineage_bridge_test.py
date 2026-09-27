"""Exact boundary and lineage checks for legacy and future W3 waves."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))

from w3_lineage_bridge import (
    RECORD, admit_wave, build_wave_index, validate_wave_index, write_wave_index,
    publish_run_indexes, write_event_window_index,
)
from raw_event_envelope import normalize_records, replay_envelope


def records(stream_start: int, pcs: tuple[int, ...]) -> bytes:
    return b"".join(RECORD.pack(stream_start + i, i + 1, i * 100, pc, pc,
        0x4E71, 1, 0, 2, 0, 0, 0) for i, pc in enumerate(pcs))


class W3LineageBridgeTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.wave = self.root / "live-discovery-wave-000001.bin"
        self.parts = [records(100, (0x200, 0x202, 0x200)),
                      records(105, (0x300, 0x302))]
        self.wave.write_bytes(b"".join(self.parts))
        self.rows = []
        offset = 0
        for worker, data in enumerate(self.parts):
            first = RECORD.unpack_from(data)
            last = RECORD.unpack_from(data, len(data) - RECORD.size)
            self.rows.append({"run_id": 91, "cycle": 1, "worker_id": worker,
                "epoch": 2, "capture_id": 9100 + worker, "generation": 1,
                "record_count": len(data) // RECORD.size,
                "records_sha256": hashlib.sha256(data).hexdigest(),
                "segment_sha256": hashlib.sha256(b"seg" + data).hexdigest(),
                "entry_stream_sequence": first[0], "exit_stream_sequence": last[0] + 1,
                "entry_instruction_sequence": first[1],
                "exit_instruction_sequence": last[1] + 1,
                "configured_depth": 16, "valid": True})
            offset += len(data)
        self.index_path = self.root / "wave.index.json"

    def index(self):
        return write_wave_index(self.wave, self.rows, run_id=91, wave_index=1,
            worker_count=2, audit_sha256="a" * 64, output_path=self.index_path)

    def test_exact_windows_are_complete_and_keep_repeated_pc_multiplicity(self):
        metadata = self.index()
        result = validate_wave_index(self.wave, self.index_path,
            expected_index_sha256=metadata["sha256"], expected_audit_sha256="a" * 64)
        self.assertEqual(result["record_count"], 5)
        self.assertEqual(result["segments"], 2)
        index = json.loads(self.index_path.read_text())
        self.assertEqual([s["raw_offset"] for s in index["segments"]], [0, 144])
        self.assertEqual(index["segments"][0]["record_count"], 3)

    def test_partial_overlap_or_changed_window_is_open(self):
        broken = list(self.rows)
        broken[1] = {**broken[1], "records_sha256": "0" * 64}
        with self.assertRaisesRegex(ValueError, "SEGMENT_HASH_MISMATCH"):
            build_wave_index(self.wave, broken, run_id=91, wave_index=1,
                             worker_count=2, audit_sha256="a" * 64)

    def test_missing_worker_or_ambiguous_duplicate_identity_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "WORKER_COVERAGE"):
            build_wave_index(self.wave, self.rows[:1], run_id=91, wave_index=1,
                             worker_count=2, audit_sha256="a" * 64)
        with self.assertRaisesRegex(ValueError, "DUPLICATE_OR_INVALID_WORKER"):
            build_wave_index(self.wave, self.rows + [self.rows[0]], run_id=91,
                             wave_index=1, worker_count=2, audit_sha256="a" * 64)

    def test_two_possible_sources_do_not_get_guessed(self):
        duplicate = {**self.rows[0], "capture_id": 9199}
        with self.assertRaisesRegex(ValueError, "DUPLICATE_OR_INVALID_WORKER"):
            build_wave_index(self.wave, self.rows + [duplicate], run_id=91,
                             wave_index=1, worker_count=2, audit_sha256="a" * 64)

    def test_index_and_raw_hash_tampering_are_rejected(self):
        metadata = self.index()
        value = json.loads(self.index_path.read_text())
        value["segments"][1]["raw_offset"] = 0
        self.index_path.write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError, "INDEX_HASH_MISMATCH"):
            validate_wave_index(self.wave, self.index_path,
                expected_index_sha256=metadata["sha256"], expected_audit_sha256="a" * 64)
        metadata = self.index()
        self.wave.write_bytes(self.wave.read_bytes()[:-1] + b"x")
        with self.assertRaisesRegex(ValueError, "RAW_HASH_MISMATCH"):
            validate_wave_index(self.wave, self.index_path,
                expected_index_sha256=metadata["sha256"], expected_audit_sha256="a" * 64)

    def test_missing_record_fails_full_coverage(self):
        self.wave.write_bytes(self.wave.read_bytes()[:-RECORD.size])
        with self.assertRaisesRegex(ValueError, "WINDOW_TRUNCATED|SEGMENT_HASH_MISMATCH"):
            build_wave_index(self.wave, self.rows, run_id=91, wave_index=1,
                             worker_count=2, audit_sha256="a" * 64)

    def test_wave_windows_are_admitted_to_existing_cartographer(self):
        metadata = self.index()

        class Session:
            def __init__(self):
                self.admissions = []

            def admit(self, segment, rows, data):
                self.admissions.append((segment, rows, data))
                return {"accepted": len(rows)}

        session = Session()
        result = admit_wave(session, self.wave, self.index_path,
            expected_index_sha256=metadata["sha256"], expected_audit_sha256="a" * 64)
        self.assertEqual([x["accepted"] for x in result], [3, 2])
        self.assertEqual([row[3] for row in session.admissions[0][1]], [0x200, 0x202, 0x200])
        self.assertEqual([row[0] for row in session.admissions[1][1]], [105, 106])
        self.assertEqual(session.admissions[0][0]["source_raw_sha256"],
                         hashlib.sha256(self.wave.read_bytes()).hexdigest())

    def test_legacy_w3_records_remain_decodable(self):
        self.assertEqual(self.wave.stat().st_size % RECORD.size, 0)
        self.assertEqual(len(list(RECORD.iter_unpack(self.wave.read_bytes()))), 5)

    def test_future_producer_publishes_hash_bound_companion_index(self):
        audit = self.root / "segment-audits.jsonl"
        audit.write_text("".join(json.dumps(row, sort_keys=True, separators=(",", ":"))
                                  + "\n" for row in self.rows))
        audit_sha = hashlib.sha256(audit.read_bytes()).hexdigest()
        published = publish_run_indexes(self.root, audit, run_id=91,
            worker_count=2, audit_sha256=audit_sha,
            chunk_prefix="live-discovery-wave")
        self.assertEqual(len(published), 1)
        index_path = Path(published[0]["path"])
        self.assertEqual(published[0]["segment_count"], 2)
        checked = validate_wave_index(self.wave, index_path,
            expected_index_sha256=published[0]["sha256"],
            expected_audit_sha256=audit_sha)
        self.assertEqual(checked["status"], "PASS_W3_LINEAGE_INDEX")

    def test_overlap_accounting_closes_duplicates_and_replays_without_raw(self):
        first = self.parts[0]
        second = first[RECORD.size:]
        self.wave.write_bytes(first + second)
        first_row, second_row = self.rows
        overlap_data = RECORD.unpack_from(second)
        overlap_last = RECORD.unpack_from(second, len(second) - RECORD.size)
        second_row = {**second_row, "record_count": 2,
            "records_sha256": hashlib.sha256(second).hexdigest(),
            "segment_sha256": hashlib.sha256(b"overlap" + second).hexdigest(),
            "entry_stream_sequence": overlap_data[0],
            "exit_stream_sequence": overlap_last[0] + 1,
            "entry_instruction_sequence": overlap_data[1],
            "exit_instruction_sequence": overlap_last[1] + 1}
        index = write_wave_index(self.wave, [first_row, second_row], run_id=91,
            wave_index=1, worker_count=2, audit_sha256="a" * 64,
            output_path=self.index_path)
        event_index = self.root / "event-index.jsonl"
        write_event_window_index(self.index_path, event_index,
                                 expected_index_sha256=index["sha256"])
        envelope = self.root / "events.jsonl.gz"
        receipt = normalize_records(self.wave, envelope, "W3_V2", event_index)
        self.assertEqual(receipt["event_counts"]["input_events"], 5)
        self.assertEqual(receipt["event_counts"]["accepted_events"], 3)
        self.assertEqual(receipt["event_counts"]["duplicate_events"], 2)
        self.assertEqual(receipt["event_counts"]["unaccounted_events"], 0)
        self.assertEqual(replay_envelope(envelope, receipt["raw_file"]["sha256"])
                         ["status"], "PASS_RAW_FREE_ENVELOPE_REPLAY")


if __name__ == "__main__":
    unittest.main()
