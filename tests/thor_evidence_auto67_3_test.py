import json
import sys
import tempfile
import time
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
from thor_evidence.auto67_persistence import LivePersistenceSink, chain_descriptor
from thor_evidence.store import Store


def run_one(database, seq, epoch, context):
    sink = LivePersistenceSink(database)
    sink.start()
    try:
        item = chain_descriptor({"seq": seq, "frame": seq, "epoch": epoch,
                                 "kind": "RAM_WRITE", "pc": "0x100", "address": "0x200"},
                                "BOUNDED_UNRESOLVED", seq, 0, "L1", "INV-1")
        assert sink.submit(item)
    finally:
        sink.stop()
    return sink.snapshot()


class Auto673PersistenceTest(unittest.TestCase):
    def test_temporal_fields_do_not_change_durable_identity(self):
        first = chain_descriptor({"seq": 1, "frame": 10, "epoch": 1,
                                  "kind": "RAM_WRITE", "pc": "0x100", "address": "0x200",
                                  "chain_steps": [{"pc": "0x100"}]},
                                 "BOUNDED_UNRESOLVED", 10, 1, "L1", "INV-1")
        replay = chain_descriptor({"seq": 99, "frame": 900, "epoch": 9,
                                   "kind": "RAM_WRITE", "pc": "0x100", "address": "0x200",
                                   "chain_steps": [{"pc": "0x100"}]},
                                  "PROVEN", 900, 2, "L2", "INV-2")
        self.assertEqual(first["chain_hash"], replay["chain_hash"])

    def test_same_sidecar_replay_deduplicates_observation(self):
        with tempfile.TemporaryDirectory() as directory:
            database = Path(directory) / "knowledge.sqlite"
            first = run_one(database, 1, 1, "context-a")
            replay = run_one(database, 99, 9, "context-b")
            self.assertEqual(first["dropped"], 0)
            self.assertEqual(replay["dropped"], 0)
            self.assertEqual(first["monitor"]["new_unique_chains_this_session"], 1)
            self.assertEqual(replay["monitor"]["exact_duplicates_rejected"], 1)
            store = Store(database)
            try:
                self.assertEqual(store.connection.execute(
                    "SELECT COUNT(*) FROM live_chain").fetchone()[0], 1)
                session = store.connection.execute(
                    "SELECT payload FROM live_session WHERE id=?",
                    (replay["session_id"],)).fetchone()
                values = json.loads(session[0])
                self.assertEqual(values["unique_chain_inserts"], 0)
                self.assertEqual(values["exact_duplicate_observations"], 1)
            finally:
                store.close()

    def test_different_and_unresolved_chain_bodies_survive_restart(self):
        with tempfile.TemporaryDirectory() as directory:
            database = Path(directory) / "chains.sqlite"
            sink = LivePersistenceSink(database)
            sink.start()
            try:
                unresolved = chain_descriptor(
                    {"kind": "RAM_WRITE", "pc": "0x100", "address": "0x200",
                     "chain_steps": [{"pc": "0x100"}],
                     "unresolved_frontier": {"next": "UNKNOWN"}},
                    "BOUNDED_UNRESOLVED", 1, 0, "L1", "INV-1")
                rooted = chain_descriptor(
                    {"kind": "RAM_WRITE", "pc": "0x100", "address": "0x200",
                     "chain_steps": [{"pc": "0x100"}, {"pc": "0x180"}],
                     "terminal_root": "0x180"},
                    "PROVEN", 2, 1, "L2", "INV-2")
                self.assertTrue(sink.submit(unresolved))
                self.assertTrue(sink.submit(rooted))
            finally:
                sink.stop()
            monitor = sink.snapshot()["monitor"]
            self.assertEqual(monitor["total_unique_chains"], 2)
            self.assertEqual(monitor["unresolved_chains"], 1)
            self.assertEqual(monitor["rooted_chains"], 1)
            store = Store(database)
            try:
                self.assertEqual(store.connection.execute(
                    "SELECT COUNT(*) FROM live_chain").fetchone()[0], 2)
            finally:
                store.close()


if __name__ == "__main__":
    unittest.main()
