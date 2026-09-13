import importlib.util
import tempfile
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "auto64_provenance", ROOT / "src/tools/thor_evidence/auto64_provenance.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class Auto64ProvenanceTest(unittest.TestCase):
    def test_queue_is_priority_ordered_and_children_are_persisted(self):
        coverage = {"schema": "test", "derived_frontiers": [{"id": "INV-AUTO64-A6",
            "created_from": "test", "entity": "PC:test", "question": "q", "why_open": "w"}]}
        graph = MODULE.initial_graph({}, [], coverage)
        self.assertEqual(MODULE.next_open(graph)["id"], "INV-AUTO64-A6")
        self.assertIn("INV-AUTO64-A6-CALLER", graph["nodes"])
        self.assertIn("INV-AUTO64-D3", graph["nodes"])

    def test_direct_call_scan_does_not_invent_callers(self):
        rom = bytes(0x200)
        self.assertEqual(MODULE.direct_callers(rom, 0xAF00, 0xAF23), [])

    def test_request_preserves_temporal_provenance_rule(self):
        coverage = {"schema": "test", "derived_frontiers": [{"id": "INV-AUTO64-A6",
            "created_from": "test", "entity": "PC:test", "question": "q", "why_open": "w"}]}
        graph = MODULE.initial_graph({}, [], coverage)
        request = MODULE.make_request(graph, {}, [])
        self.assertEqual(request["frontier"], "INV-AUTO64-A6")
        self.assertIn("definition-before-use", request["runtime_if_needed"]["provenance_rule"])


if __name__ == "__main__":
    unittest.main()
