"""Focused R7 bounded FLOW handoff tests."""
from __future__ import annotations

import hashlib
from pathlib import Path
import sys
import threading
import unittest

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from flow_stream import BoundedFlowHandoff  # noqa: E402
from live_forward_scaling_audit import RECORD  # noqa: E402


def segment(number: int, blob: bytes) -> tuple[dict, list[tuple[int, ...]], bytes]:
    rows = list(RECORD.iter_unpack(blob))
    return ({"valid": True, "ready_for_cartographer": True, "run_id": 7,
             "epoch": 1, "worker_id": 0, "capture_id": number,
             "generation": number, "entry_stream_sequence": number,
             "exit_stream_sequence": number + len(rows), "record_count": len(rows),
             "records_sha256": hashlib.sha256(blob).hexdigest()}, rows, blob)


class FlowStreamTests(unittest.TestCase):
    def test_empty_stream(self) -> None:
        handoff = BoundedFlowHandoff(4)
        handoff.close()
        self.assertEqual(list(handoff.consume()), [])
        self.assertEqual(handoff.stats().segments, 0)

    def test_small_chunks_preserve_order_and_hashes(self) -> None:
        handoff = BoundedFlowHandoff(4)
        blob = RECORD.pack(0, 0, 0, 0x100, 0x200, 0x4E75, 1, 0, 0, 0, 0, 0)
        for number in range(1, 4):
            handoff.submit(*segment(number, blob))
        handoff.close()
        self.assertEqual([item.segment["capture_id"] for item in handoff.consume()], [1, 2, 3])
        stats = handoff.stats()
        self.assertEqual((stats.segments, stats.records, stats.raw_bytes), (3, 3, len(blob) * 3))
        self.assertEqual((stats.disk_reads, stats.disk_writes, stats.fallback), (0, 0, "DISABLED"))

    def test_malformed_record_count_rejected(self) -> None:
        handoff = BoundedFlowHandoff(2)
        blob = RECORD.pack(0, 0, 0, 0x100, 0x200, 0x4E75, 1, 0, 0, 0, 0, 0)
        bad, rows, _ = segment(1, blob)
        bad["record_count"] = 2
        with self.assertRaises(ValueError):
            handoff.submit(bad, rows, blob)

    def test_truncated_record_rejected(self) -> None:
        handoff = BoundedFlowHandoff(2)
        bad, rows, blob = segment(1, RECORD.pack(0, 0, 0, 0x100, 0x200, 0x4E75, 1, 0, 0, 0, 0, 0))
        with self.assertRaises(ValueError):
            handoff.submit(bad, rows, blob[:-1])

    def test_duplicate_records_are_not_silently_created(self) -> None:
        handoff = BoundedFlowHandoff(2)
        blob = RECORD.pack(0, 0, 0, 0x100, 0x200, 0x4E75, 1, 0, 0, 0, 0, 0)
        handoff.submit(*segment(1, blob))
        handoff.submit(*segment(1, blob))
        self.assertEqual(handoff.stats().segments, 2)

    def test_bounded_buffer_peak_is_capped(self) -> None:
        handoff = BoundedFlowHandoff(1)
        blob = RECORD.pack(0, 0, 0, 0x100, 0x200, 0x4E75, 1, 0, 0, 0, 0, 0)
        consumed = []
        thread = threading.Thread(target=lambda: consumed.extend(handoff.consume()))
        thread.start()
        for number in range(1, 5):
            handoff.submit(*segment(number, blob))
        handoff.close()
        thread.join()
        self.assertLessEqual(handoff.stats().buffer_peak_chunks, 1)
        self.assertGreater(handoff.stats().buffer_peak_bytes, 0)
        self.assertEqual(len(consumed), 4)

    def test_consumer_failure_is_visible_to_producer(self) -> None:
        handoff = BoundedFlowHandoff(1)
        handoff.fail(RuntimeError("consumer boom"))
        with self.assertRaises(RuntimeError):
            list(handoff.consume())

    def test_no_disk_io_accounting(self) -> None:
        stats = BoundedFlowHandoff(2).stats()
        self.assertEqual(stats.disk_reads, 0)
        self.assertEqual(stats.disk_writes, 0)


if __name__ == "__main__":
    unittest.main()
