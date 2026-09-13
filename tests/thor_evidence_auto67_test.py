import importlib.util
import time
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
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


if __name__ == "__main__":
    unittest.main()
