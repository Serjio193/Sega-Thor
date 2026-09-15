import json
import struct
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from auto67_capsule import CapsulePool
from auto67_capsule_codec import decode_capsule
from auto67_live import Dispatcher
from auto67_materializer import materialize


def seed(seq: int, **extra) -> dict:
    return {"seq": seq, "frame": seq, "kind": "RAM_WRITE",
            "pc": "0x100", "address": f"0x{0x200 + seq:X}", **extra}


def write_v2(path: Path, records, capsule_id=0):
    logical = 64 + len(records) * 20
    payload = bytearray(b"O67V")
    payload.extend(struct.pack("<IIIII", 2, capsule_id, 100, logical, len(records)))
    for record in records:
        payload.extend(struct.pack("<IIIII", *record))
    path.write_bytes(payload)


class FailingSink:
    def submit(self, _item):
        raise OSError("persistence unavailable")

    def set_runtime_leases(self, _count):
        raise OSError("persistence unavailable")

    def snapshot(self):
        return {"available": False}


class Auto67WorkerCleanTest(unittest.TestCase):
    @staticmethod
    def wait_for(dispatcher, count):
        deadline = time.time() + 5
        while time.time() < deadline:
            if dispatcher.snapshot()["metrics"]["worker_returns"] >= count:
                return
            time.sleep(0.01)
        raise AssertionError("worker did not complete expected tasks")

    def test_a_worker_history_stays_bounded_after_many_completions(self):
        dispatcher = Dispatcher(1, capacity=128, processing_delay=0)
        dispatcher.start()
        try:
            for sequence in range(40):
                dispatcher.ingest(seed(sequence))
            self.wait_for(dispatcher, 40)
            snapshot = dispatcher.snapshot()
            self.assertFalse(hasattr(dispatcher, "investigations"))
            self.assertEqual(len(dispatcher.recent_investigations), 16)
            self.assertEqual(len(snapshot["investigations"]), 16)
            self.assertLess(len(json.dumps(snapshot)), 100000)
        finally:
            dispatcher.stop()

    def test_b_known_during_work_is_not_a_worker_semantic_override(self):
        dispatcher = Dispatcher(2, processing_delay=0)
        dispatcher.start()
        try:
            dispatcher.ingest(seed(1, known_during_work=True))
            dispatcher.ingest(seed(2, known_during_work=True))
            self.wait_for(dispatcher, 2)
            outcomes = [item["outcome"] for item in dispatcher.recent_investigations]
            self.assertEqual(len(outcomes), 2)
            self.assertTrue(all(outcome == "EVIDENCE_OBSERVED" for outcome in outcomes))
            self.assertNotIn("known_found_during_work", dispatcher.snapshot()["metrics"])
        finally:
            dispatcher.stop()

    def test_c_capsule_release_only_releases_resources(self):
        pool = CapsulePool(max_live=4)
        try:
            capsule = pool.claim(0, "INV-1", seed(1))
            self.assertIsNotNone(capsule)
            assert capsule is not None
            pool.sync([{"capsule_id": capsule.capsule_id, "state": "FROZEN",
                        "bytes_used": 20, "event_count": 1}])
            self.assertTrue(pool.wait_frozen(capsule.capsule_id, capsule.lease_id,
                                              threading.Event()))
            pool.release(capsule.capsule_id)
            snapshot = pool.snapshot()
            self.assertEqual(snapshot["active_live"], 0)
            self.assertEqual(snapshot["items"][0]["state"], "FREE")
            for field in ("known", "merged", "proven", "bounded_unresolved"):
                self.assertNotIn(field, snapshot["items"][0])
            self.assertIsNotNone(pool.claim(0, "INV-2", seed(2)))
        finally:
            pool.stop()

    def test_d_live_materialization_keeps_diagnostics_without_frontier(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capsule-00-L00000001.bin"
            write_v2(path, [(1, 100, 0xC00004, 0x100, 1)])
            capsule = decode_capsule(path, 0, "L00000001")
            result = materialize({"kind": "BUS_WRITE_PC", "pc": "0x000000",
                                  "address": "0xC00004"}, capsule,
                                 bytes.fromhex("4A3900FF0010"), live_worker=True)
            self.assertNotIn("unresolved_frontier", result)
            self.assertIn("STATIC_DECODE", result["capture_diagnostics"]["missing_evidence"])
            self.assertNotIn('"next"', json.dumps(result))

    def test_e_proven_causal_facts_survive_live_materialization(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capsule-00-L00000001.bin"
            write_v2(path, [(1, 100, 0xC00004, 0x100, 1)])
            capsule = decode_capsule(path, 0, "L00000001")
            rom = bytearray(0x200)
            rom[0x100:0x104] = bytes.fromhex("3955FFFC")
            result = materialize({"kind": "BUS_WRITE_PC", "pc": "0x000100",
                                  "address": "0xC00004"}, capsule, bytes(rom),
                                 live_worker=True)
            self.assertTrue(any(fact["kind"] == "INSTRUCTION_SOURCE_MEMORY"
                                for fact in result["causal_facts"]))
            self.assertNotIn("unresolved_frontier", result)

    def test_f_dispatcher_keeps_occurrence_identity_and_exact_item_guard(self):
        dispatcher = Dispatcher(2, processing_delay=0)
        dispatcher.start()
        try:
            first, second = seed(1), seed(2)
            first["address"] = second["address"] = "0x200"
            dispatcher.ingest(first)
            dispatcher.ingest(second)
            self.wait_for(dispatcher, 2)
            snapshot = dispatcher.snapshot()
            self.assertEqual(snapshot["metrics"]["worker_leases"], 2)
            identities = [(item["occurrence_id"], item["window_item_id"])
                          for item in snapshot["investigations"]]
            self.assertEqual(len({item[0] for item in identities}), 2)
            self.assertEqual(len({item[1] for item in identities}), 2)
        finally:
            dispatcher.stop()

    def test_g_persistence_failure_cannot_block_worker_return_or_release(self):
        sink = FailingSink()
        dispatcher = Dispatcher(1, processing_delay=0, chain_sink=sink)
        dispatcher.start()
        try:
            dispatcher.ingest(seed(1))
            self.wait_for(dispatcher, 1)
            snapshot = dispatcher.snapshot()
            self.assertEqual(snapshot["workers"][0]["state"], "IDLE")
            self.assertEqual(snapshot["metrics"]["worker_returns"], 1)
            self.assertGreaterEqual(snapshot["metrics"]["persistence_submit_errors"], 1)
        finally:
            dispatcher.stop()


if __name__ == "__main__":
    unittest.main()
