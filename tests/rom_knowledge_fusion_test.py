import json
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "src/tools"), str(ROOT / "src/tools/thor_evidence")]
from rom_knowledge_map import KnowledgeStore, runtime_occurrence_id
from rom_knowledge_fusion_conflicts import eligible_exact_operations, record_conflict
from rom_knowledge_fusion_stream import iter_json_arrays


class RomKnowledgeFusionTest(unittest.TestCase):
    def test_streaming_selected_arrays_preserves_nested_json(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "corpus.json"
            path.write_text(json.dumps({"ignored": [{"q": [1, 2]}],
                "instructions": [{"pc": 8, "nested": {"text": "x,]\\\""}},
                    {"pc": 9}]}, separators=(",", ":")), encoding="utf-8")
            rows = list(iter_json_arrays(path, {"instructions"}))
            self.assertEqual(rows, [("instructions", 0,
                {"pc": 8, "nested": {"text": 'x,]\\"'}}), ("instructions", 1, {"pc": 9})])

    def test_scoped_runtime_identity_never_aliases_capture_or_sequence(self):
        first = runtime_occurrence_id(capture_id="run:1", epoch=3, cpu="M68K",
            address_space="ROM", native_sequence=100, instruction_sequence=7,
            event_kind="INSTRUCTION", run_id=1)
        other_capture = runtime_occurrence_id(capture_id="run:2", epoch=3, cpu="M68K",
            address_space="ROM", native_sequence=100, instruction_sequence=7,
            event_kind="INSTRUCTION", run_id=2)
        other_sequence = runtime_occurrence_id(capture_id="run:1", epoch=3, cpu="M68K",
            address_space="ROM", native_sequence=101, instruction_sequence=8,
            event_kind="INSTRUCTION", run_id=1)
        self.assertEqual(len({first, other_capture, other_sequence}), 3)

    def test_conflict_suppresses_only_overlapping_exact_proposal(self):
        with tempfile.TemporaryDirectory() as directory:
            store = KnowledgeStore(Path(directory) / "knowledge.sqlite", "a" * 64, 64)
            try:
                record_conflict(store, 8, 16, "INCOMPATIBLE_EXACT_CLASSIFICATION", [
                    {"source": "A", "value": "CODE"}, {"source": "B", "value": "DATA"}])
                candidates = [
                    {"operation": "CLASSIFY_RANGE", "range": [8, 16], "truth": "DERIVED_EXACT"},
                    {"operation": "ADD_REFERENCE", "range": [24, 28], "truth": "DERIVED_EXACT"},
                    {"operation": "CLASSIFY_RANGE", "range": [40, 48], "truth": "HYPOTHESIS"}]
                eligible, suppressed = eligible_exact_operations(store, candidates)
                self.assertEqual([item["operation"] for item in eligible], ["ADD_REFERENCE"])
                self.assertEqual({item["reason"] for item in suppressed},
                                 {"OVERLAPS_CONFLICT", "NON_EXACT_TRUTH"})
                self.assertEqual(len(store.query("conflicts")), 1)
            finally:
                store.close()


if __name__ == "__main__":
    unittest.main()
