import base64
import sys
import time
import unittest
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from auto67_capsule import CapsulePool  # noqa: E402
from auto67_capsule_codec import CapsuleFormatError  # noqa: E402
from auto67_live import Dispatcher  # noqa: E402
from auto67_native_snapshot import (  # noqa: E402
    NATIVE_RECORD,
    NativeSnapshotAdmission,
    decode_native_snapshot,
    freeze_native_event,
    to_predecessor_capture,
)
from auto67_predecessor import resolve  # noqa: E402
from auto67_snapshot_admission import SnapshotPool  # noqa: E402


def fixture(occurrence_sequence=7, latest=3):
    rom = bytearray(0x200)
    rom[0x100:0x102] = bytes.fromhex("2840")  # MOVEA.L D0,A4
    rom[0x102:0x104] = bytes.fromhex("4e71")
    rom[0x104:0x106] = bytes.fromhex("4e71")
    rows = [(1, 0x100, 0x2840, 0)]
    rows.extend((sequence, 0x104, 0x4E71, 0)
                for sequence in range(2, latest + 1))
    raw = b"".join(NATIVE_RECORD.pack(*row) for row in rows)
    compressor = zlib.compressobj(wbits=-zlib.MAX_WBITS)
    compressed = compressor.compress(raw) + compressor.flush()
    occurrence_id = f"epoch=1:seq={occurrence_sequence}"
    identity = (f"NR-0001-{occurrence_sequence:010d}-"
                f"{latest:016d}")
    source = {
        "schema": "oasis.auto67.native-snapshot.v1",
        "snapshot_identity": identity,
        "occurrence_id": occurrence_id,
        "snapshot_epoch": 1,
        "first_sequence": 1,
        "latest_sequence": latest,
        "count": latest,
        "consumer_sequence": latest,
        "record_bytes": 16,
        "records_codec": "deflate-base64",
        "records_b64": base64.b64encode(compressed).decode("ascii"),
        "read_duration_ns": 100,
        "copy_duration_ns": 200,
        "compression_duration_ns": 300,
        "freeze_duration_ns": 600,
    }
    event = {"epoch": 1, "seq": occurrence_sequence,
             "occurrence_id": occurrence_id, "frame": 10,
             "kind": "BUS_WRITE_PC", "pc": "0x000104",
             "native_snapshot": source,
             "native_snapshot_registers": {"A4": 0x1234}}
    return bytes(rom), event


class NativeSnapshotTest(unittest.TestCase):
    def test_actual_record_adapter_reaches_existing_resolver(self):
        rom, event = fixture()
        decoded = decode_native_snapshot(event, rom)
        self.assertEqual([item.sequence for item in decoded.records], [1, 2, 3])
        pool = SnapshotPool()
        snapshot = freeze_native_event(event, pool, rom)
        self.assertIsNotNone(snapshot)
        self.assertNotIn("native_snapshot", event)
        capture = to_predecessor_capture(snapshot, event, ("A4",))
        result = resolve(capture, rom, ["A4"])
        self.assertEqual(result["unresolved"], [])
        self.assertEqual(len(result["steps"]), 1)
        step = result["steps"][0]
        self.assertEqual(step["kind"], "REGISTER_REACHING_DEFINITION")
        self.assertEqual(step["producer_pc"], "0x000100")
        self.assertEqual(step["consumer_pc"], "0x000104")
        self.assertIs(step["evidence"]["intervening_register_write"], False)
        self.assertIn(1, [record.sequence for record in snapshot.native_records])

    def test_explicit_native_occurrence_mismatch_fails_closed(self):
        rom, event = fixture()
        event["native_snapshot"]["occurrence_id"] = "epoch=1:seq=8"
        with self.assertRaisesRegex(ValueError, "another occurrence"):
            decode_native_snapshot(event, rom)

    def test_bad_sequence_or_rom_opcode_is_rejected(self):
        rom, event = fixture()
        raw = b"".join(NATIVE_RECORD.pack(*row) for row in [
            (1, 0x100, 0x2840, 0), (4, 0x102, 0x4E71, 0),
            (3, 0x104, 0x4E71, 0)])
        event["native_snapshot"]["records_b64"] = _encode(raw)
        with self.assertRaisesRegex(ValueError, "not contiguous"):
            decode_native_snapshot(event, rom)
        rom, event = fixture()
        raw = b"".join(NATIVE_RECORD.pack(*row) for row in [
            (1, 0x100, 0xFFFF, 0), (2, 0x102, 0x4E71, 0),
            (3, 0x104, 0x4E71, 0)])
        event["native_snapshot"]["records_b64"] = _encode(raw)
        with self.assertRaisesRegex(ValueError, "differs from ROM"):
            decode_native_snapshot(event, rom)

    def test_capsule_lease_uses_exact_frozen_occurrence(self):
        rom, event = fixture()
        pool = SnapshotPool()
        frozen = freeze_native_event(event, pool, rom)
        self.assertIsNotNone(frozen)
        event["register_provenance_registers"] = ["A4"]
        capsules = CapsulePool(max_live=1, native_snapshot_pool=pool)
        capsule = capsules.claim(2, "INV-7", event)
        self.assertIsNotNone(capsule)
        capture = capsules.decode_predecessor(capsule.capsule_id,
                                              capsule.lease_id, "INV-7")
        self.assertEqual(capture.path, f"native-snapshot:{frozen.snapshot_identity}")
        self.assertEqual(capture.records[-1].pc, 0x104)
        event["snapshot_identity"] = "some-other-snapshot"
        with self.assertRaisesRegex(CapsuleFormatError, "mismatch"):
            capsules.claim(3, "INV-8", event)

    def test_real_dispatcher_lifecycle_releases_frozen_slot_after_idle(self):
        rom, event = fixture()
        pool = SnapshotPool()
        dispatcher = Dispatcher(worker_count=1, capacity=8,
                                processing_delay=0, rom=rom)
        admission = NativeSnapshotAdmission(dispatcher, pool, rom)
        dispatcher.start()
        try:
            admission.ingest(event)
            deadline = time.time() + 3
            while time.time() < deadline:
                if dispatcher.snapshot()["metrics"]["worker_returns"]:
                    break
                time.sleep(0.005)
            result = admission.snapshot()
            self.assertEqual(dispatcher.snapshot()["metrics"]["worker_returns"], 1)
            self.assertEqual(result["snapshots_completed"], 1)
            self.assertEqual(result["current_snapshot_depth"], 0)
            self.assertTrue(result["freeze_receipts"][0][
                "slot_released_after_worker_completion"])
        finally:
            dispatcher.stop()

    def test_native_ring_advance_keeps_frozen_hash_receipt_stable(self):
        rom, first = fixture(7)
        _, second = fixture(8, latest=4)
        pool = SnapshotPool()
        dispatcher = Dispatcher(worker_count=1, capacity=8,
                                processing_delay=0.05, rom=rom)
        admission = NativeSnapshotAdmission(dispatcher, pool, rom)
        dispatcher.start()
        try:
            admission.ingest(first)
            admission.ingest(second)
            time.sleep(0.1)
            receipts = admission.snapshot()["freeze_receipts"]
            first_receipt = next(item for item in receipts
                                 if item["occurrence_id"] == "epoch=1:seq=7")
            self.assertTrue(first_receipt["live_ring_advanced_after_freeze"])
            self.assertTrue(first_receipt[
                "frozen_snapshot_read_identical_at_worker_start"])
        finally:
            dispatcher.stop()


def _encode(raw):
    compressor = zlib.compressobj(wbits=-zlib.MAX_WBITS)
    return base64.b64encode(compressor.compress(raw) + compressor.flush()).decode("ascii")


if __name__ == "__main__":
    unittest.main()
