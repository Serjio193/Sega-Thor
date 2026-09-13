import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "auto64_coverage", ROOT / "src/tools/thor_evidence/auto64_knowledge_coverage.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class Auto64CoverageTest(unittest.TestCase):
    def test_import_preserves_multidimensional_fail_closed_state(self):
        coverage = MODULE.import_coverage(
            {"_path": "auto62.json", "_sha256": "x", "investigations_created": [{}]},
            {"_path": "auto63.json", "_sha256": "y", "selected": {"target_span": [1, 9]},
             "unresolved_frontiers": ["A6_INHERITED_AT_ENTRY"]}, None, None, 10, "test")
        entity = coverage["entities"]["ROM:000001-000009"]
        self.assertEqual(entity["dimensions"]["CONSUMER"], "PROVEN")
        self.assertEqual(entity["dimensions"]["SOURCE_OWNED"], "NO")
        self.assertEqual(MODULE.derive_frontiers(coverage)[0]["created_from"],
                         "AUTO63 unresolved_frontier")

    def test_unknown_frontiers_do_not_create_hardcoded_a6_task(self):
        coverage = {"unresolved_frontiers": ["OTHER_FRONTIER"]}
        self.assertEqual(MODULE.derive_frontiers(coverage), [])

    def test_authoritative_input_identity_is_retained_even_when_missing(self):
        coverage = MODULE.import_coverage(
            {"_path": "a", "_sha256": "x", "investigations_created": []},
            {"_path": "b", "_sha256": "y", "selected": {}, "unresolved_frontiers": []},
            None, None, 10, "test", [Path("missing.sqlite")])
        artifact = coverage["source_artifacts"][-1]
        self.assertEqual(artifact["kind"], "EVIDENCE_ENGINE_SQLITE")
        self.assertFalse(artifact["available"])


if __name__ == "__main__":
    unittest.main()
