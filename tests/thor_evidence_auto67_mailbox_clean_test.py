import json
import sys
import time
import unittest
from pathlib import Path
from types import SimpleNamespace


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from auto67_live import Dispatcher
from auto67_capsule import CapsulePool
from auto67_capsule_codec import CapsuleFormatError


def event(seq: int) -> dict:
    return {"epoch": 1, "seq": seq, "occurrence_id": f"epoch=1:seq={seq}",
            "window_item_id": seq, "frame": seq, "kind": "FRAME_PC", "pc": "0x100"}


class RecordingPool:
    max_live = 1

    def __init__(self):
        self.claims = []
        self.waits = []
        self.releases = []

    def claim(self, worker_id, investigation_id, item):
        self.claims.append((worker_id, investigation_id, dict(item)))
        return SimpleNamespace(capsule_id=0, lease_id="LEASE-1", predecessor_enabled=False)

    def wait_frozen(self, capsule_id, lease_id, stop_event):
        self.waits.append((capsule_id, lease_id))
        return False

    def analyzing(self, _capsule_id):
        return None

    def release(self, capsule_id):
        self.releases.append(capsule_id)

    def snapshot(self):
        return {"capsules_free": 16, "active_live": 0, "max_simultaneous_live": 1,
                "items": [], "metrics": {"capsules_frozen": 0}}


class RecordingSink:
    def __init__(self):
        self.items = []

    def submit(self, item):
        self.items.append(item)
        return True

    def set_runtime_leases(self, _count):
        return None

    def snapshot(self):
        return {"available": False}


class Auto67MailboxCleanTest(unittest.TestCase):
    def test_a_mailbox_has_only_the_minimal_lease_message(self):
        dispatcher = Dispatcher(1)
        dispatcher.worker_states = ["IDLE"]
        dispatcher.window.append(event(1))
        chosen = dispatcher._choose_current()
        self.assertIsNotNone(chosen)
        task = chosen[1]
        self.assertTrue({"event", "investigation_id", "lease_id", "dispatch_trace"}
                        <= set(task))
        for field in ("worker_id", "occurrence_id", "seed", "branch", "context",
                      "capsule_lease", "assigned_ns"):
            self.assertNotIn(field, task)

    def test_b_event_copy_preserves_identity_and_detaches_from_window(self):
        dispatcher = Dispatcher(1)
        dispatcher.worker_states = ["IDLE"]
        source = event(7)
        dispatcher.window.append(source)
        task = dispatcher._choose_current()[1]
        self.assertEqual(task["event"]["epoch"], 1)
        self.assertEqual(task["event"]["seq"], 7)
        self.assertEqual(task["event"]["occurrence_id"], "epoch=1:seq=7")
        self.assertEqual(task["event"]["window_item_id"], 7)
        source["seq"] = 99
        source["occurrence_id"] = "epoch=1:seq=99"
        self.assertEqual(task["event"]["seq"], 7)
        self.assertEqual(task["event"]["occurrence_id"], "epoch=1:seq=7")

    def test_c_investigation_and_lease_ids_are_single_threaded_to_persistence(self):
        pool, sink = RecordingPool(), RecordingSink()
        dispatcher = Dispatcher(1, capsule_pool=pool, chain_sink=sink, processing_delay=0)
        dispatcher.start()
        try:
            dispatcher.ingest(event(3))
            deadline = time.time() + 2
            while time.time() < deadline and not sink.items:
                time.sleep(0.01)
            self.assertTrue(sink.items)
            investigation_id = pool.claims[0][1]
            provenance = json.loads(sink.items[0]["provenance"])
            self.assertEqual(provenance["investigation_id"], investigation_id)
            self.assertEqual(provenance["lease_id"], "LEASE-1")
            self.assertEqual(pool.waits, [(0, "LEASE-1")])
            self.assertEqual(dispatcher.recent_investigations[-1]["investigation_id"],
                             investigation_id)
            self.assertEqual(dispatcher.recent_investigations[-1]["lease_id"], "LEASE-1")
        finally:
            dispatcher.stop()

    def test_d_capsule_mismatch_fails_closed(self):
        pool = CapsulePool(max_live=1)
        try:
            capsule = pool.claim(0, "INV-1", event(1))
            self.assertIsNotNone(capsule)
            assert capsule is not None
            with self.assertRaises(CapsuleFormatError):
                pool.decode_predecessor(capsule.capsule_id, "WRONG", "INV-1")
        finally:
            pool.stop()

    def test_e_no_mailbox_backlog_or_history_map_exists(self):
        dispatcher = Dispatcher(16)
        self.assertEqual(len(dispatcher.mailboxes), 16)
        self.assertFalse(hasattr(dispatcher, "task_queue"))
        self.assertFalse(hasattr(dispatcher, "pending_tasks"))
        self.assertFalse(hasattr(dispatcher, "history_map"))

    def test_f_state_option_audit_is_mechanical(self):
        runner = (ROOT / "src/tools/thor_evidence/auto67_runner.py").read_text(encoding="utf-8")
        live = (ROOT / "src/tools/thor_evidence/capture/live_opportunistic.lua").read_text(
            encoding="utf-8")
        self.assertNotIn("--state", runner)
        self.assertNotIn("OASIS_LIVE_STATE", runner)
        self.assertNotIn("OASIS_LIVE_STATE", live)

    def test_g_shutdown_after_final_ingest_releases_workers(self):
        pool = CapsulePool(max_live=1)
        dispatcher = Dispatcher(1, capsule_pool=pool, processing_delay=0)
        dispatcher.start()
        try:
            dispatcher.ingest(event(22))
            started = time.monotonic()
            dispatcher.stop()
            self.assertLess(time.monotonic() - started, 2.0)
            self.assertTrue(all(not thread.is_alive() for thread in dispatcher.threads))
        finally:
            pool.stop()


if __name__ == "__main__":
    unittest.main()
