import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


def load(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


GRAPH = load("auto64_provenance", "src/tools/thor_evidence/auto64_provenance.py")
ANALYZE = load("auto64_analyze", "src/tools/thor_evidence/auto64_provenance_analyze.py")


class Auto64AnalyzeTest(unittest.TestCase):
    def test_caller_requires_temporal_stack_compatibility(self):
        events = [{"kind": "TARGET_ENTRY_CONTEXT", "seq": 4, "epoch": 1, "frame": 2120,
                   "data": {"instruction_pc": 0xAF00, "previous_pc": 0x1234,
                            "stack_return_long": 0x1236,
                            "previous_pc_ring": [{"pc": 0x1234}]}}]
        result = ANALYZE.prove_dynamic_caller(events)
        self.assertEqual(result["status"], "PROVEN")

    def test_equal_register_value_does_not_prove_a6_origin(self):
        events = [{"kind": "TARGET_ENTRY_CONTEXT", "seq": 4, "epoch": 1, "frame": 2120,
                   "data": {"instruction_pc": 0xAF00, "previous_pc": 0xAF22,
                            "stack_return_long": 0x2000, "previous_pc_ring": []}}]
        self.assertEqual(ANALYZE.prove_dynamic_caller(events)["status"], "NOT_PROVEN")

    def test_proven_caller_creates_automatic_origin_child(self):
        coverage = {"schema": "test", "derived_frontiers": [{"id": "INV-AUTO64-A6",
            "created_from": "test", "entity": "PC:test", "question": "q", "why_open": "w"}]}
        graph = GRAPH.initial_graph({}, [], coverage)
        events = [{"kind": "TARGET_ENTRY_CONTEXT", "seq": 4, "epoch": 1, "frame": 2120,
                   "data": {"instruction_pc": 0xAF00, "previous_pc": 0x1234,
                            "stack_return_long": 0x1236,
                            "previous_pc_ring": [{"pc": 0x1234}]}}]
        updated = ANALYZE.advance(graph, events)
        self.assertIn("INV-AUTO64-A6-ORIGIN-001234", updated["nodes"])
        self.assertEqual(updated["nodes"]["INV-AUTO64-A6-CALLER"]["status"], "RESOLVED")

    def test_runtime_coverage_update_does_not_change_ownership(self):
        coverage = {"entities": {"PC:0000AF00-0000AF22":
            {"dimensions": {"SOURCE_OWNED": "NO", "EXECUTION": "UNSEEN",
                             "CALLER": "UNSEEN", "POINTER_SOURCE": "UNSEEN"},
             "provenance": []}}, "source_artifacts": []}
        result = ANALYZE.update_coverage(coverage, Path(__file__),
                                         {"caller_proof": {"status": "NOT_PROVEN"},
                                          "a6_candidates": []})
        self.assertEqual(result["entities"]["PC:0000AF00-0000AF22"]["dimensions"]["SOURCE_OWNED"], "NO")


if __name__ == "__main__":
    unittest.main()
