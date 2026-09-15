import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from auto67_live import Dispatcher, RollingWindow


def event(sequence: int, address: str = "0x200") -> dict:
    return {"epoch": 1, "seq": sequence, "occurrence_id":
            f"epoch=1:seq={sequence}", "frame": sequence,
            "kind": "RAM_WRITE_SAMPLE", "pc": "0x100", "address": address}


class LuaTransportFixture:
    """Deterministic model of live_opportunistic.lua's transport ring."""

    def __init__(self, capacity: int):
        self.capacity = capacity
        self.ring = [None] * capacity
        self.start = 0
        self.count = 0
        self.overwritten = 0

    def append(self, item: dict) -> None:
        if self.count < self.capacity:
            self.ring[(self.start + self.count) % self.capacity] = item
            self.count += 1
        else:
            self.ring[self.start] = item
            self.start = (self.start + 1) % self.capacity
            self.overwritten += 1

    def snapshot(self) -> list[dict]:
        return [self.ring[(self.start + index) % self.capacity]
                for index in range(self.count)]


class Auto67RingCleanTest(unittest.TestCase):
    def test_a_python_ring_evicts_oldest_and_counts_overwrites(self):
        window = RollingWindow(4)
        for sequence in range(6):
            window.append(event(sequence))
        self.assertEqual([item["seq"] for item in window.items], [2, 3, 4, 5])
        self.assertEqual(window.overwrites, 2)

    def test_b_exact_leased_item_stays_marked_and_other_item_is_eligible(self):
        dispatcher = Dispatcher(2, capacity=4, processing_delay=0)
        dispatcher.worker_states = ["IDLE", "IDLE"]
        first, second, third = event(1), event(2), event(3)
        dispatcher.window.append(first)
        dispatcher.window.append(second)
        dispatcher.window.append(third)
        chosen = dispatcher._choose_current()
        self.assertIsNotNone(chosen)
        leased = next(item for item in dispatcher.window.items
                      if item.get("dispatch_state") == "LEASED")
        next_chosen = dispatcher._choose_current()
        self.assertIsNotNone(next_chosen)
        self.assertNotEqual(next_chosen[1]["event"]["seq"], leased["seq"])
        self.assertEqual(leased["dispatch_state"], "LEASED")

    def test_c_same_branch_occurrences_remain_independent(self):
        window = RollingWindow(16)
        for sequence in range(16):
            window.append(event(sequence, "0x200"))
        self.assertEqual(len(window.items), 16)
        self.assertEqual([item["occurrence_id"] for item in window.items],
                         [f"epoch=1:seq={i}" for i in range(16)])

    def test_d_python_ring_has_no_semantic_state(self):
        window = RollingWindow(4)
        self.assertEqual(set(vars(window)), {"capacity", "items", "overwrites"})
        for name in ("retained", "proven", "known", "duplicate", "frontier", "map_delta"):
            self.assertFalse(hasattr(window, name))
        self.assertFalse(hasattr(window, "current"))

    def test_e_lua_transport_fixture_is_bounded_ordered_and_identity_safe(self):
        transport = LuaTransportFixture(4)
        for sequence in range(6):
            transport.append(event(sequence))
        current = transport.snapshot()
        self.assertLessEqual(len(current), transport.capacity)
        self.assertEqual([item["seq"] for item in current], [2, 3, 4, 5])
        self.assertEqual([item["occurrence_id"] for item in current],
                         ["epoch=1:seq=2", "epoch=1:seq=3",
                          "epoch=1:seq=4", "epoch=1:seq=5"])
        self.assertEqual(transport.overwritten, 2)
        source = (ROOT / "src/tools/thor_evidence/capture/live_opportunistic.lua").read_text(
            encoding="utf-8")
        for token in ("local ring_start = 1", "local ring_count = 0",
                      "local overwritten = 0", "occurrence_id", "events_json"):
            self.assertIn(token, source)
        self.assertNotRegex(source, r"known|proven|frontier|map_delta")

    def test_f_two_rings_are_explicit_and_neither_is_a_backlog(self):
        source = (ROOT / "src/tools/thor_evidence/capture/live_opportunistic.lua").read_text(
            encoding="utf-8")
        architecture = (ROOT / "docs/ARCHITECTURE.md").read_text(encoding="utf-8")
        self.assertRegex(source, r"ring_count.*capacity|capacity.*ring_count")
        self.assertIn("event FIFO", architecture)
        self.assertIn("current, unleased `RollingWindow`", architecture)
        self.assertNotIn("raw-event backlog", source)


if __name__ == "__main__":
    unittest.main()
