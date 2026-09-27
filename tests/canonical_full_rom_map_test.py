import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[1] / "src" / "tools" / "thor_evidence"))
from canonical_full_rom_map import CanonicalRomMap, Range  # noqa: E402


class CanonicalFullRomMapTests(unittest.TestCase):
    def setUp(self):
        self.map = CanonicalRomMap([
            Range(0, 4, "HEADER", "STATIC_VERIFIED", 1, "header"),
            Range(4, 8, "UNKNOWN", "DERIVED_EXACT", 0),
        ], rom_size=8, rom_sha256="test")

    def test_lookup_covers_every_byte_and_boundaries_are_explicit(self):
        self.assertEqual(self.map.lookup(4)["range_start"], 4)
        self.assertEqual(self.map.lookup(4)["start_boundary"], "UNKNOWN_BOUNDARY")
        with self.assertRaises(ValueError):
            self.map.lookup(8)

    def test_closure_splits_and_classifies_without_losing_bytes(self):
        updated, delta = self.map.apply([
            {"op": "SPLIT_RANGE", "start": 4, "split": 6},
            {"op": "CLASSIFY_RANGE", "start": 4, "end_exclusive": 6,
             "class": "DATA_TABLE", "truth": "DERIVED_EXACT"},
        ])
        self.assertEqual(sum(item.size for item in updated.ranges), 8)
        self.assertEqual(delta["NEW_CLASSIFIED_BYTES"], 2)
        self.assertEqual(delta["SOURCE_OWNED_DELTA"], 0)

    def test_promotion_gate_rejects_heuristics(self):
        with self.assertRaisesRegex(ValueError, "STOP_MAP_PROMOTION_GATE"):
            self.map.apply([{"op": "PROMOTE_SOURCE_OWNED", "start": 4,
                             "end_exclusive": 8, "exact_extent": True}])

    def test_report_import_fails_closed_on_ownership_mismatch(self):
        report = {"rom": {"bytes": 0x300000,
                           "sha256": "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"},
                  "emission": [{"start": 0, "end": 0x300000, "source_owned": 1,
                                "emission_type": "UNKNOWN", "classification": "UNKNOWN"}]}
        with self.assertRaisesRegex(ValueError, "STOP_MAP_OWNERSHIP_ACCOUNTING_MISMATCH"):
            CanonicalRomMap.from_report(report)


if __name__ == "__main__":
    unittest.main(verbosity=2)
