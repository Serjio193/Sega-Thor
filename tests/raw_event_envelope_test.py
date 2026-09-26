"""Lossless event-envelope and raw-free replay fixtures."""

import hashlib
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from raw_event_envelope import (FORMAT, normalize_capsule, normalize_records,
                                replay_capsule_envelope, replay_envelope)


def record(sequence, *, pc=0x100, address=0x102, value=0x4E71,
           flags=3, cpu=0, domain=0, auxiliary=0):
    return FORMAT.pack(sequence, sequence, sequence * 2, pc, address, value,
                       flags, cpu, 2, domain, 0, auxiliary)


class RawEventEnvelopeTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.raw = self.root / "capture.bin"
        self.output = self.root / "capture.jsonl.gz"

    def test_all_raw_fields_and_bytes_survive_normalization(self):
        original = record(1, pc=0x2345, address=0x6789, value=0x4E75,
                          flags=0x213, cpu=1, domain=7, auxiliary=0xAABBCCDD)
        self.raw.write_bytes(original)
        receipt = normalize_records(self.raw, self.output, "W3_V2")
        item = json.loads(__import__("gzip").open(self.output, "rt").readline())
        self.assertEqual(item["record_hex"], original.hex())
        self.assertEqual(item["fields"]["master_time"], 2)
        self.assertEqual(item["fields"]["auxiliary"], 0xAABBCCDD)
        self.assertTrue(receipt["all_fields_known_to_format_accounted"])

    def test_replay_succeeds_after_raw_file_is_unavailable(self):
        blob = record(1) + record(2, pc=0x102, address=0x104)
        self.raw.write_bytes(blob)
        receipt = normalize_records(self.raw, self.output, "FLOW_V1")
        self.raw.rename(self.root / "raw-unavailable.bin")
        replay = replay_envelope(self.output, receipt["raw_file"]["sha256"])
        self.assertEqual(replay["status"], "PASS_RAW_FREE_ENVELOPE_REPLAY")
        self.assertEqual(replay["raw_sha256"], hashlib.sha256(blob).hexdigest())
        self.assertEqual(replay["unaccounted_events"], 0)

    def test_unresolved_and_rejected_events_are_retained(self):
        blob = record(1, flags=0) + record(2, flags=7)
        self.raw.write_bytes(blob)
        receipt = normalize_records(self.raw, self.output, "FLOW_V1")
        self.assertEqual(receipt["event_counts"]["unresolved_events"], 1)
        self.assertEqual(receipt["event_counts"]["rejected_events"], 1)
        self.assertEqual(receipt["event_counts"]["accounted_events"], 2)

    def test_bus_event_subtype_bits_are_not_misclassified_as_unknown_flags(self):
        self.raw.write_bytes(record(1, flags=0x8800))
        receipt = normalize_records(self.raw, self.output, "FLOW_V1")
        self.assertEqual(receipt["event_counts"]["accepted_events"], 1)
        self.assertEqual(receipt["event_counts"]["unresolved_events"], 0)

    def test_duplicate_overlapping_windows_account_once_as_duplicate(self):
        blob = record(5) + record(5)
        self.raw.write_bytes(blob)
        index = self.root / "segments.jsonl"
        segment = {"run_id": 77, "epoch": 1, "capture_id": 10,
                   "records_sha256": hashlib.sha256(record(5)).hexdigest()}
        rows = []
        for offset, capture in ((0, 10), (FORMAT.size, 11)):
            current = dict(segment, capture_id=capture)
            rows.append({"raw_offset": offset, "raw_length": FORMAT.size,
                         "segment": current})
        index.write_text("".join(json.dumps(row) + "\n" for row in rows), encoding="utf-8")
        receipt = normalize_records(self.raw, self.output, "FLOW_V1", index)
        self.assertEqual(receipt["event_counts"]["input_events"], 2)
        self.assertEqual(receipt["event_counts"]["duplicate_events"], 1)
        self.assertEqual(receipt["event_counts"]["unaccounted_events"], 0)
        self.assertEqual(replay_envelope(self.output)["event_counts"]["duplicate_events"], 1)

    def test_capture_gap_keeps_distinct_window_boundaries(self):
        blob = record(1) + record(3)
        self.raw.write_bytes(blob)
        index = self.root / "segments.jsonl"
        rows = []
        for offset, sequence, capture in ((0, 1, 1), (FORMAT.size, 3, 2)):
            part = record(sequence)
            rows.append({"raw_offset": offset, "raw_length": len(part), "segment": {
                "run_id": 77, "epoch": 1, "capture_id": capture,
                "records_sha256": hashlib.sha256(part).hexdigest()}})
        index.write_text("".join(json.dumps(row) + "\n" for row in rows), encoding="utf-8")
        normalize_records(self.raw, self.output, "FLOW_V1", index)
        import gzip
        with gzip.open(self.output, "rt", encoding="utf-8") as stream:
            windows = [json.loads(line)["window"]["segment"]["capture_id"] for line in stream]
        self.assertEqual(windows, [1, 2])

    def test_unknown_format_and_truncation_fail_closed(self):
        self.raw.write_bytes(b"")
        with self.assertRaisesRegex(ValueError, "TRUNCATED"):
            normalize_records(self.raw, self.output, "FLOW_V1")
        self.raw.write_bytes(record(1)[:-1])
        with self.assertRaisesRegex(ValueError, "TRUNCATED"):
            normalize_records(self.raw, self.output, "FLOW_V1")
        self.raw.write_bytes(record(1))
        with self.assertRaisesRegex(ValueError, "UNSUPPORTED"):
            normalize_records(self.raw, self.output, "UNKNOWN")

    def test_existing_normalized_artifact_is_never_overwritten(self):
        self.raw.write_bytes(record(1))
        self.output.write_bytes(b"existing")
        with self.assertRaisesRegex(FileExistsError, "overwrite"):
            normalize_records(self.raw, self.output, "FLOW_V1")
        self.assertEqual(self.output.read_bytes(), b"existing")

    def test_tampered_normalized_fields_fail_raw_free_replay(self):
        self.raw.write_bytes(record(1))
        normalize_records(self.raw, self.output, "W3_V2")
        import gzip
        with gzip.open(self.output, "rt", encoding="utf-8") as stream:
            item = json.loads(stream.readline())
        item["fields"]["pc"] = 0x999
        with gzip.open(self.output, "wt", encoding="utf-8") as stream:
            stream.write(json.dumps(item) + "\n")
        with self.assertRaisesRegex(ValueError, "FIELD_MISMATCH"):
            replay_envelope(self.output)

    def test_tampered_disposition_fails_independent_replay(self):
        self.raw.write_bytes(record(1))
        normalize_records(self.raw, self.output, "FLOW_V1")
        import gzip
        with gzip.open(self.output, "rt", encoding="utf-8") as source:
            item = json.loads(source.readline())
        item["disposition"] = "UNRESOLVED"
        with gzip.open(self.output, "wt", encoding="utf-8") as target:
            target.write(json.dumps(item) + "\n")
        with self.assertRaisesRegex(ValueError, "CLASSIFICATION_MISMATCH"):
            replay_envelope(self.output)

    def test_o67v_capsule_replays_without_raw_and_retains_lease_frame(self):
        capsule = self.root / "capsule-01-leaseA.bin"
        header = b"O67V" + struct.pack("<IIIII", 2, 1, 100, 84, 1)
        event = struct.pack("<IIIII", 7, 100, 0xFF0000, 0x1234, 1)
        capsule.write_bytes(header + event)
        output = self.root / "capsule.jsonl.gz"
        receipt = normalize_capsule(capsule, output)
        capsule.rename(self.root / "capsule-unavailable.bin")
        replay = replay_capsule_envelope(output, receipt["raw_file"]["sha256"])
        self.assertEqual(replay["status"], "PASS_RAW_FREE_CAPSULE_REPLAY")
        self.assertEqual(replay["unaccounted_events"], 0)
        import gzip
        with gzip.open(output, "rt", encoding="utf-8") as stream:
            header_row = json.loads(stream.readline())
            event_row = json.loads(stream.readline())
        self.assertEqual(header_row["lease_id"], "leaseA")
        self.assertEqual(event_row["fields"]["frame"], 100)

    def test_o67c_unknown_kind_remains_unresolved_but_replayable(self):
        capsule = self.root / "capsule-01-legacy.bin"
        header = b"O67C" + struct.pack("<IIII", 1, 50, 80, 1)
        event = struct.pack("<IIII", 3, 50, 0xFF1000, 0x2000)
        capsule.write_bytes(header + event)
        output = self.root / "legacy.jsonl.gz"
        receipt = normalize_capsule(capsule, output)
        self.assertEqual(receipt["event_counts"]["unresolved_events"], 1)
        self.assertEqual(replay_capsule_envelope(
            output, receipt["raw_file"]["sha256"])["unaccounted_events"], 0)

    def test_tampered_capsule_semantics_fail_independent_replay(self):
        capsule = self.root / "capsule-01-leaseA.bin"
        capsule.write_bytes(b"O67V" + struct.pack("<IIIII", 2, 1, 100, 84, 1) +
            struct.pack("<IIIII", 7, 100, 0xFF0000, 0x1234, 1))
        output = self.root / "capsule.jsonl.gz"
        normalize_capsule(capsule, output)
        import gzip
        with gzip.open(output, "rt", encoding="utf-8") as source:
            rows = [json.loads(line) for line in source]
        rows[1]["fields"]["pc"] = 0x999
        with gzip.open(output, "wt", encoding="utf-8") as target:
            target.writelines(json.dumps(row) + "\n" for row in rows)
        with self.assertRaisesRegex(ValueError, "SEMANTICS_MISMATCH"):
            replay_capsule_envelope(output)


if __name__ == "__main__":
    unittest.main()
