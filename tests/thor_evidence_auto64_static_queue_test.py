import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "auto64_static_queue", ROOT / "src/tools/thor_evidence/auto64_static_queue.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class Auto64StaticQueueTest(unittest.TestCase):
    def test_scheduler_consumes_open_items_deterministically(self):
        graph = {"nodes": {
            "INV-AUTO64-D3": {"id": "INV-AUTO64-D3", "status": "OPEN", "priority": 80,
                              "raw_witnesses": []},
            "INV-AUTO64-GAP-1": {"id": "INV-AUTO64-GAP-1", "status": "OPEN", "priority": 65,
                                 "raw_witnesses": []}}, "queue_history": []}
        updated = MODULE.consume(graph, {"instructions": []}, Path("static.json"))
        self.assertEqual(updated["nodes"]["INV-AUTO64-D3"]["status"], "BOUNDED_UNRESOLVED")
        self.assertEqual(updated["nodes"]["INV-AUTO64-GAP-1"]["status"], "BOUNDED_UNRESOLVED")
        self.assertEqual([x["frontier"] for x in updated["queue_history"]],
                         ["INV-AUTO64-D3", "INV-AUTO64-GAP-1"])


if __name__ == "__main__":
    unittest.main()
