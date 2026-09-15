import importlib.util
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
SPEC = importlib.util.spec_from_file_location(
    "auto67_live", ROOT / "src/tools/thor_evidence/auto67_live.py")
AUTO67 = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(AUTO67)

from auto67_capsule import CAPSULE_COUNT, MAX_LIVE_CAPTURES, CapsulePool
from auto67_persistence import descriptor


def event(seq: int) -> dict:
    return {"epoch": 1, "seq": seq, "occurrence_id": f"epoch=1:seq={seq}",
            "frame": seq, "kind": "BUS_WRITE", "pc": "0x1200",
            "address": "0xFF1000", "resolution": "BOUNDED_UNRESOLVED"}


class DispatcherOneTest(unittest.TestCase):
    def test_a_same_branch_different_occurrences_get_distinct_workers(self):
        dispatcher = AUTO67.Dispatcher(2, capacity=8)
        dispatcher.worker_states = ["IDLE", "IDLE"]
        dispatcher.window.append(event(100))
        dispatcher.window.append(event(101))
        dispatcher._dispatch_current()
        tasks = [task for task in dispatcher.mailboxes if task is not None]
        self.assertEqual(len(tasks), 2)
        self.assertEqual({dispatcher.mailboxes.index(task) for task in tasks}, {0, 1})
        self.assertEqual({task["event"]["occurrence_id"] for task in tasks},
                         {"epoch=1:seq=100", "epoch=1:seq=101"})
        self.assertEqual(len({task["investigation_id"] for task in tasks}), 2)
        self.assertEqual(len({task["lease_id"] for task in tasks}), 2)
        self.assertEqual(dispatcher.metrics["active_collisions"], 0)

    def test_b_same_stored_event_is_leased_once(self):
        dispatcher = AUTO67.Dispatcher(2, capacity=8)
        dispatcher.worker_states = ["IDLE", "IDLE"]
        stored = event(100)
        dispatcher.window.append(stored)
        first = dispatcher._choose_current()
        self.assertIsNotNone(first)
        worker_id, _ = first
        dispatcher.worker_states[worker_id] = "IDLE"
        self.assertIsNone(dispatcher._choose_current())
        self.assertEqual(stored["dispatch_state"], "LEASED")

    def test_c_busy_worker_cannot_receive_second_mailbox(self):
        dispatcher = AUTO67.Dispatcher(2, capacity=8)
        dispatcher.worker_states = ["WORKING", "IDLE"]
        existing = {"occurrence_id": "epoch=1:seq=1"}
        dispatcher.mailboxes[0] = existing
        dispatcher.window.append(event(2))
        dispatcher._dispatch_current()
        self.assertIs(dispatcher.mailboxes[0], existing)
        self.assertIsNotNone(dispatcher.mailboxes[1])
        self.assertEqual(dispatcher.worker_states, ["WORKING", "LEASED"])

    def test_d_sixteen_same_branch_occurrences_are_all_leased(self):
        dispatcher = AUTO67.Dispatcher(16, capacity=32)
        dispatcher.worker_states = ["IDLE"] * 16
        for seq in range(16):
            dispatcher.window.append(event(seq))
        dispatcher._dispatch_current()
        self.assertEqual(sum(task is not None for task in dispatcher.mailboxes), 16)
        self.assertEqual(dispatcher.metrics["worker_leases"], 16)
        self.assertEqual(dispatcher.metrics["active_collisions"], 0)

    def test_e_occurrence_identity_is_distinct_in_investigation_and_persistence(self):
        dispatcher = AUTO67.Dispatcher(2, capacity=8)
        dispatcher.worker_states = ["IDLE", "IDLE"]
        dispatcher.window.append(event(10))
        dispatcher.window.append(event(11))
        dispatcher._dispatch_current()
        tasks = [task for task in dispatcher.mailboxes if task is not None]
        self.assertEqual(len({task["event"]["occurrence_id"] for task in tasks}), 2)
        self.assertEqual(len({task["investigation_id"] for task in tasks}), 2)
        self.assertEqual(len({task["lease_id"] for task in tasks}), 2)
        records = [descriptor(task["event"], "BOUNDED_UNRESOLVED", task["event"]["frame"],
                              dispatcher.mailboxes.index(task), task["lease_id"],
                              task["investigation_id"])
                    for task in tasks]
        self.assertNotEqual(records[0]["provenance"], records[1]["provenance"])

    def test_capsule_limits_remain_resource_limits(self):
        pool = CapsulePool()
        self.assertEqual(CAPSULE_COUNT, 16)
        self.assertEqual(MAX_LIVE_CAPTURES, 4)
        claimed = [pool.claim(i, f"INV-{i}", event(i)) for i in range(4)]
        self.assertTrue(all(claimed))
        self.assertIsNone(pool.claim(4, "INV-4", event(4)))
        self.assertEqual(pool.capacity_state(), (4, 12))


if __name__ == "__main__":
    unittest.main()
