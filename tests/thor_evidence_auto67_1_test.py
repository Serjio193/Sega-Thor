import importlib.util
import sys
import tempfile
import time
import threading
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
spec = importlib.util.spec_from_file_location(
    "auto67_live", ROOT / "src/tools/thor_evidence/auto67_live.py")
auto67 = importlib.util.module_from_spec(spec)
assert spec.loader
spec.loader.exec_module(auto67)
from auto67_capsule import CAPSULE_SIZE, CapsulePool


def seed(seq: int, **context) -> dict:
    return {"seq": seq, "frame": seq, "kind": "RAM_WRITE",
            "pc": "0x100", "address": "0x200", **context}


class Auto671CapsuleTest(unittest.TestCase):
    def wait(self, dispatcher):
        deadline = time.time() + 2
        while time.time() < deadline:
            if dispatcher.snapshot()["metrics"]["worker_returns"] >= 1:
                return
            time.sleep(0.01)
        self.fail("worker did not return")

    def test_exact_pool_shape_and_fixed_capacity(self):
        pool = CapsulePool()
        try:
            snapshot = pool.snapshot()
            self.assertEqual(snapshot["configured"], 16)
            self.assertEqual(snapshot["capacity"], 131072)
            self.assertEqual(len(snapshot["items"]), 16)
            self.assertTrue(all(item["capacity"] == CAPSULE_SIZE
                                for item in snapshot["items"]))
        finally:
            pool.stop()

    def test_live_limit_and_reuse_after_release(self):
        pool = CapsulePool(max_live=4)
        try:
            claimed = [pool.claim(i, f"INV-{i}", seed(i)) for i in range(5)]
            self.assertEqual(sum(item is not None for item in claimed), 4)
            self.assertEqual(pool.snapshot()["active_live"], 4)
            first = claimed[0]
            assert first is not None
            pool.sync([{"capsule_id": first.capsule_id, "state": "FROZEN",
                        "bytes_used": 320, "event_count": 16}])
            self.assertTrue(pool.wait_frozen(first.capsule_id, first.lease_id,
                                             threading.Event()))
            pool.release(first.capsule_id, "KNOWN")
            replacement = pool.claim(9, "INV-9", seed(9))
            self.assertIsNotNone(replacement)
            self.assertEqual(pool.snapshot()["metrics"]["capsules_reused"], 1)
        finally:
            pool.stop()

    def test_dispatcher_waits_for_freeze_then_returns_without_backlog(self):
        pool = CapsulePool(max_live=2)
        dispatcher = auto67.Dispatcher(2, capacity=8, processing_delay=0,
                                       capsule_pool=pool)
        dispatcher.start()
        try:
            dispatcher.ingest(seed(1))
            deadline = time.time() + 2
            while time.time() < deadline and pool.snapshot()["active_live"] < 1:
                time.sleep(0.01)
            active = [item for item in pool.snapshot()["items"]
                      if item["state"] in {"CLAIMED", "CAPTURING"}]
            self.assertTrue(active)
            pool.sync([{"capsule_id": item["capsule_id"], "state": "FROZEN",
                        "bytes_used": 256, "event_count": 12}
                       for item in active])
            deadline = time.time() + 2
            while time.time() < deadline:
                if dispatcher.snapshot()["metrics"]["worker_returns"] >= 1:
                    break
                time.sleep(0.01)
            snapshot = dispatcher.snapshot()
            self.assertEqual(snapshot["metrics"]["workers_busy"], 0)
            self.assertGreaterEqual(snapshot["metrics"]["worker_returns"], 1)
            self.assertEqual(snapshot["capsules"]["active_live"], 0)
        finally:
            dispatcher.stop()
            pool.stop()

    def test_command_stream_is_bounded_and_has_no_raw_events(self):
        with tempfile.TemporaryDirectory() as directory:
            pool = CapsulePool(command_path=Path(directory) / "commands.txt")
            try:
                for i in range(80):
                    item = pool.claim(0, f"INV-{i}", seed(i))
                    if item is not None:
                        pool.release(item.capsule_id, "BOUNDED_UNRESOLVED")
                self.assertLessEqual(len(pool.commands), 64)
                self.assertNotIn("events", "\n".join(pool.commands))
            finally:
                pool.stop()

    def test_unresolved_context_can_reactivate_without_known_poisoning(self):
        dispatcher = auto67.Dispatcher(1, capacity=8, processing_delay=0)
        dispatcher.start()
        try:
            dispatcher.ingest(seed(1, epoch=1, caller_pc="0x300"))
            self.wait(dispatcher)
            dispatcher.ingest(seed(2, epoch=1, caller_pc="0x300"))
            dispatcher.ingest(seed(3, epoch=2, caller_pc="0x301"))
            self.wait(dispatcher)
            metrics = dispatcher.snapshot()["metrics"]
            self.assertEqual(metrics["known_rejected_before_dispatch"], 0)
            self.assertGreaterEqual(metrics["worker_leases"], 2)
            self.assertNotIn("REJECT_SAME_EXACT_CONTEXT", metrics["dispatch_reasons"])
        finally:
            dispatcher.stop()

    def test_proven_result_does_not_poison_future_dispatch(self):
        dispatcher = auto67.Dispatcher(1, capacity=8, processing_delay=0)
        dispatcher.start()
        try:
            item = seed(1, epoch=3, scene="room-a", resolution="PROVEN")
            dispatcher.ingest(item)
            self.wait(dispatcher)
            dispatcher.ingest(seed(2, epoch=3, scene="room-a", resolution="PROVEN"))
            time.sleep(0.03)
            metrics = dispatcher.snapshot()["metrics"]
            self.assertEqual(metrics["worker_leases"], 2)
            self.assertNotIn("REJECT_KNOWN_COMPLETE", metrics["dispatch_reasons"])
        finally:
            dispatcher.stop()

    def test_acceptance_snapshot_has_no_knowledge_yield_classifier(self):
        dispatcher = auto67.Dispatcher(1, capacity=8, processing_delay=0)
        dispatcher.start()
        try:
            snapshot = dispatcher.snapshot()
            self.assertNotIn("knowledge_yield", snapshot)
            self.assertIn("chain_store", snapshot)
        finally:
            dispatcher.stop()

    def test_frozen_capsules_do_not_block_hunt_workers(self):
        pool = CapsulePool(max_live=4)
        dispatcher = auto67.Dispatcher(1, capsule_pool=pool, processing_delay=0)
        dispatcher.start()
        try:
            for item in pool.capsules:
                item.state = "FROZEN"
            dispatcher.ingest(seed(1, epoch=99))
            self.wait(dispatcher)
            snapshot = dispatcher.snapshot()
            self.assertEqual(snapshot["capsules"]["capsules_free"], 0)
            self.assertFalse(snapshot["metrics"]["dispatch_starved_no_free_capsule"])
            self.assertEqual(snapshot["metrics"]["focused_capture_slot_waits"], 1)
            self.assertEqual(snapshot["metrics"]["worker_leases"], 1)
        finally:
            dispatcher.stop()
            pool.stop()

    def test_single_focused_slot_keeps_other_hunts_running(self):
        pool = CapsulePool(max_live=1)
        dispatcher = auto67.Dispatcher(4, capacity=32, processing_delay=0,
                                       capsule_pool=pool)
        dispatcher.start()
        try:
            for i in range(8):
                dispatcher.ingest(seed(i + 1, epoch=i + 1,
                                      address=f"0x{0x200 + i:x}"))
            deadline = time.time() + 2
            while time.time() < deadline:
                if dispatcher.snapshot()["metrics"]["worker_returns"] >= 7:
                    break
                time.sleep(0.01)
            time.sleep(0.25)
            metrics = dispatcher.snapshot()["metrics"]
            self.assertGreaterEqual(metrics["worker_leases"], 8)
            self.assertGreaterEqual(metrics["focused_capture_slot_waits"], 1)
            self.assertGreaterEqual(metrics["hunt_attempts"], 1)
        finally:
            dispatcher.stop()
            pool.stop()

    def test_sixteen_sequential_capsules_reuse_slot_zero(self):
        pool = CapsulePool(max_live=1)
        try:
            ids = []
            for i in range(16):
                item = pool.claim(0, f"INV-{i}", seed(i))
                self.assertIsNotNone(item)
                assert item is not None
                ids.append(item.capsule_id)
                pool.release(item.capsule_id, "BOUNDED_UNRESOLVED")
            self.assertEqual(ids, [0] * 16)
            self.assertEqual(pool.snapshot()["capsules_free"], 16)
            self.assertEqual(pool.snapshot()["metrics"]["capsules_reused"], 16)
        finally:
            pool.stop()


if __name__ == "__main__":
    unittest.main()
