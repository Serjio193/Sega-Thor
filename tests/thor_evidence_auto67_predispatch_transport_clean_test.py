import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from auto67_transport import PreDispatchTransport


def snapshot(epoch, sequences, key="events"):
    return {"epoch": epoch, key: [{"epoch": epoch, "seq": seq, "kind": "FRAME_PC",
                                    "pc": "0x100"} for seq in sequences]}


class Auto67PredispatchTransportCleanTest(unittest.TestCase):
    def test_a_replaceable_snapshot_cursor_accepts_only_new_identity(self):
        transport = PreDispatchTransport()
        self.assertEqual([item["seq"] for item in transport.consume(snapshot(1, [0, 1]))], [0, 1])
        self.assertEqual([item["seq"] for item in transport.consume(snapshot(1, [1, 2]))], [2])
        self.assertEqual(transport.snapshot()["last_identity"], (1, 2, "epoch=1:seq=2"))

    def test_b_epoch_change_is_not_lost_by_sequence_cursor(self):
        transport = PreDispatchTransport()
        transport.consume(snapshot(4, [99]))
        accepted = transport.consume(snapshot(5, [0]))
        self.assertEqual([item["seq"] for item in accepted], [0])
        self.assertEqual(transport.snapshot()["epoch_changes"], 1)

    def test_c_malformed_and_stale_transport_items_are_dropped(self):
        transport = PreDispatchTransport()
        transport.consume(snapshot(1, [3]))
        accepted = transport.consume({"epoch": 1, "events": [
            {"epoch": 1, "seq": 3, "kind": "FRAME_PC"},
            {"epoch": 1, "seq": 2, "kind": "FRAME_PC"},
            {"epoch": 1, "kind": "FRAME_PC"},
            "bad",
        ]})
        self.assertEqual(accepted, [])
        stats = transport.snapshot()
        self.assertEqual(stats["duplicate"], 1)
        self.assertEqual(stats["stale"], 1)
        self.assertEqual(stats["invalid"], 2)

    def test_d_legacy_discovery_key_is_transport_only_compatibility(self):
        transport = PreDispatchTransport()
        accepted = transport.consume(snapshot(1, [7], key="discovery"))
        self.assertEqual(accepted[0]["occurrence_id"], "epoch=1:seq=7")

    def test_e_no_raw_event_backlog_or_semantic_dispatch_state(self):
        transport = PreDispatchTransport()
        transport.consume(snapshot(1, range(200)))
        state = transport.snapshot()
        self.assertNotIn("events", state)
        self.assertNotIn("discovery", state)
        self.assertNotIn("known", state)
        self.assertNotIn("proven", state)
        self.assertNotIn("frontier", state)
        self.assertNotIn("queue", state)
        source = (ROOT / "src/tools/thor_evidence/auto67_runner.py").read_text(encoding="utf-8")
        self.assertNotIn("seen_sequence", source)
        self.assertIn("PreDispatchTransport", source)


if __name__ == "__main__":
    unittest.main()
