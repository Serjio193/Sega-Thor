import importlib.util
import time
import sys
import threading
import tempfile
import json
from unittest.mock import patch
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
SPEC = importlib.util.spec_from_file_location(
    "auto67_live", ROOT / "src/tools/thor_evidence/auto67_live.py")
AUTO67 = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(AUTO67)


def event(seq, pc="0x100", address="0x200", resolution="PROVEN"):
    return {"seq": seq, "frame": seq, "kind": "RAM_WRITE", "pc": pc,
            "address": address, "resolution": resolution}


class Auto67Test(unittest.TestCase):
    def wait(self, dispatcher):
        deadline = time.time() + 2
        while time.time() < deadline:
            if dispatcher.snapshot()["metrics"]["worker_returns"] >= 1:
                return
            time.sleep(0.01)
        self.fail("worker did not return")

    def wait_for_returns(self, dispatcher, count):
        deadline = time.time() + 5
        while time.time() < deadline:
            if dispatcher.snapshot()["metrics"]["worker_returns"] >= count:
                return
            time.sleep(0.01)
        self.fail("workers did not return the burst")

    def test_predecessor_ready_seed_is_selected_before_ordinary_work(self):
        dispatcher = AUTO67.Dispatcher(1)
        dispatcher.worker_states = ["IDLE"]
        ordinary = event(1, address="0x201")
        ready = event(2, address="0x202")
        ready["prehistory_path"] = "prehistory-00000001.o67p"
        dispatcher.window.append(ordinary)
        dispatcher.window.append(ready)
        chosen = dispatcher._choose_current()
        self.assertIsNotNone(chosen)
        self.assertEqual(chosen[1]["event"]["seq"], 2)

    def test_free_workers_claim_different_seeds(self):
        dispatcher = AUTO67.Dispatcher(2, capacity=8, processing_delay=0.01)
        dispatcher.start()
        try:
            dispatcher.ingest(event(1, address="0x200"))
            dispatcher.ingest(event(2, address="0x201"))
            self.wait(dispatcher)
            self.assertEqual(dispatcher.snapshot()["metrics"]["worker_leases"], 2)
        finally:
            dispatcher.stop()

    def test_identical_branch_occurrences_are_not_suppressed(self):
        dispatcher = AUTO67.Dispatcher(2, capacity=8, processing_delay=0.05)
        dispatcher.start()
        try:
            dispatcher.ingest(event(1, address="0x200"))
            dispatcher.ingest(event(2, address="0x200"))
            self.wait_for_returns(dispatcher, 2)
            metrics = dispatcher.snapshot()["metrics"]
            self.assertEqual(metrics["worker_leases"], 2)
            self.assertEqual(metrics["active_collisions"], 0)
            self.assertEqual(metrics["investigation_merges"], 0)
            self.assertEqual(metrics["known_rejected_before_dispatch"], 0)
        finally:
            dispatcher.stop()

    def test_waiting_runtime_releases_worker_and_window_is_bounded(self):
        dispatcher = AUTO67.Dispatcher(1, capacity=3, processing_delay=0)
        dispatcher.start()
        try:
            dispatcher.ingest(event(1, address="0x300", resolution="WAITING_RUNTIME"))
            self.wait(dispatcher)
            for seq in range(2, 20):
                dispatcher.ingest(event(seq, address=hex(0x300 + seq)))
            deadline = time.time() + 2
            while time.time() < deadline and dispatcher.snapshot()["metrics"]["workers_busy"]:
                time.sleep(0.01)
            snapshot = dispatcher.snapshot()
            self.assertEqual(snapshot["rolling_window"]["capacity"], 3)
            self.assertGreater(snapshot["rolling_window"]["overwrites"], 0)
            self.assertEqual(snapshot["metrics"]["workers_busy"], 0)
        finally:
            dispatcher.stop()

    def test_scale_choices_are_supported(self):
        for count in (1, 8, 16):
            dispatcher = AUTO67.Dispatcher(count)
            self.assertEqual(dispatcher.worker_count, count)

    def test_proven_worker_transition_is_visible_and_returns(self):
        dispatcher = AUTO67.Dispatcher(1, capacity=8, processing_delay=0)
        dispatcher.start()
        try:
            dispatcher.ingest(event(1, resolution="PROVEN"))
            self.wait(dispatcher)
            worker = dispatcher.snapshot()["workers"][0]
            states = [item["state"] for item in worker["transitions"]]
            self.assertIn("WORKING", states)
            self.assertIn("RETURNING", states)
            self.assertEqual(worker["state"], "IDLE")
            self.assertEqual(worker["last_result"], "EVIDENCE_OBSERVED")
        finally:
            dispatcher.stop()

    def test_live_snapshot_is_bounded_and_detaches_history(self):
        dispatcher = AUTO67.Dispatcher(16)
        for i in range(5000):
            investigation = {"id": str(i), "outcome": "EVIDENCE_OBSERVED"}
            dispatcher.recent_investigations.append(investigation)
        self.assertFalse(hasattr(dispatcher, "investigations"))
        info = dispatcher.worker_info[0]
        for i in range(100):
            info["chain_fingerprint"] = str(i)
            dispatcher._transition(info, "LEASED", "DISPATCH")
        snapshot = dispatcher.snapshot(lightweight=True)
        self.assertEqual(len(snapshot["investigations"]), 16)
        self.assertEqual(len(snapshot["workers"][0]["transitions"]), 24)
        self.assertLess(len(json.dumps(snapshot)), 25000)
        self.assertEqual(len(dispatcher.snapshot()["investigations"]), 16)
        dispatcher._transition(info, "WORKING", "CHAIN_BUILD")
        self.assertEqual(snapshot["workers"][0]["transitions"][-1]["state"], "LEASED")
        self.assertEqual(snapshot["workers"][0]["transitions"][-1]["chain"], "99")

    def test_live_snapshot_drops_when_dispatcher_lock_is_busy(self):
        dispatcher = AUTO67.Dispatcher(1)
        result = []
        with dispatcher.lock:
            reader = threading.Thread(target=lambda: result.append(
                dispatcher.snapshot(lightweight=True)))
            reader.start()
            reader.join(timeout=1)
            blocked = reader.is_alive()
        reader.join(timeout=1)
        self.assertFalse(blocked)
        self.assertEqual(result, [None])

    def test_slow_status_disk_does_not_hold_worker_claims(self):
        dispatcher = AUTO67.Dispatcher(1, processing_delay=0)
        dispatcher.start()
        entered, release = threading.Event(), threading.Event()

        def slow_write(*_):
            entered.set()
            release.wait(3)
            raise OSError("test unavailable disk")

        publisher = None
        try:
            with tempfile.TemporaryDirectory() as directory:
                with patch.object(Path, "write_bytes", slow_write):
                    publisher = AUTO67.StatusPublisher(
                        dispatcher, Path(directory) / "status.json", "")
                    self.assertTrue(entered.wait(1))
                    publisher.publish({"frame": 123, "events": [{"seq": 1}]})
                    self.assertNotIn("events", publisher.latest)
                    dispatcher.ingest(event(1))
                    self.wait(dispatcher)
                    self.assertEqual(dispatcher.snapshot()["metrics"]["workers_busy"], 0)
                    self.assertFalse(release.is_set())
                    release.set()
                    publisher.stop()
                    self.assertGreater(publisher.dropped, 0)
        finally:
            release.set()
            if publisher:
                publisher.stop()
            dispatcher.stop()

    def test_dispatch_profile_records_burst_without_large_dispatch_stall(self):
        dispatcher = AUTO67.Dispatcher(16, capacity=512, processing_delay=0)
        dispatcher.start()
        try:
            for index in range(512):
                dispatcher.ingest(event(index, address=f"0x{index + 0x400:X}"))
            self.wait_for_returns(dispatcher, 512)
            profile = dispatcher.snapshot()["dispatch_profile"]
            self.assertGreaterEqual(profile["sample_count"], 512)
            for stats in profile["stages_us"].values():
                self.assertEqual(set(stats), {"min", "p50", "p95", "p99", "max"})
            self.assertLess(profile["stages_us"][
                "T0_T8_dispatch_to_worker_start_us"]["p95"], 50000)
            self.assertLess(profile["claim_lock_us"]["p95"], 50000)
        finally:
            dispatcher.stop()


if __name__ == "__main__":
    unittest.main()
