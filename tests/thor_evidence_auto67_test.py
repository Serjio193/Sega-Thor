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

    def test_known_replay_and_active_collision_do_not_lease(self):
        dispatcher = AUTO67.Dispatcher(2, capacity=8, processing_delay=0.05)
        dispatcher.start()
        try:
            dispatcher.ingest(event(1, address="0x200"))
            dispatcher.ingest(event(2, address="0x200"))
            dispatcher.ingest(event(3, address="0x200"))
            self.wait(dispatcher)
            dispatcher.ingest(event(4, address="0x200"))
            time.sleep(0.05)
            metrics = dispatcher.snapshot()["metrics"]
            self.assertEqual(metrics["worker_leases"], 1)
            self.assertGreaterEqual(metrics["active_collisions"], 1)
            self.assertTrue(metrics["same_session_known_replay"])
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
            self.assertEqual(worker["last_result"], "PROVEN")
        finally:
            dispatcher.stop()

    def test_live_snapshot_is_bounded_and_detaches_history(self):
        dispatcher = AUTO67.Dispatcher(16)
        for i in range(5000):
            investigation = {"id": str(i), "status": "BOUNDED_UNRESOLVED"}
            dispatcher.investigations[str(i)] = investigation
            dispatcher.recent_investigations.append(investigation)
        info = dispatcher.worker_info[0]
        for i in range(100):
            info["chain_fingerprint"] = str(i)
            dispatcher._transition(info, "LEASED", "KNOWN_CHECK")
        snapshot = dispatcher.snapshot(lightweight=True)
        self.assertEqual(len(snapshot["investigations"]), 16)
        self.assertEqual(len(snapshot["workers"][0]["transitions"]), 24)
        self.assertLess(len(json.dumps(snapshot)), 25000)
        self.assertEqual(len(dispatcher.snapshot()["investigations"]), 5000)
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


if __name__ == "__main__":
    unittest.main()
