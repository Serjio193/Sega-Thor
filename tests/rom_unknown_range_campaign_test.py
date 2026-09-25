"""Deterministic all-range M14.3 ranking and fail-closed screening tests."""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from rom_knowledge_map import KnowledgeStore, canonical, stable_id  # noqa: E402
from rom_unknown_range_campaign import ROM_SIZE, campaign_report, rank_unknown_ranges  # noqa: E402


class UnknownRangeCampaignTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.db = Path(self.temp.name) / "knowledge.sqlite"
        self.store = KnowledgeStore(self.db,
            "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263", ROM_SIZE)
        self.store.set_generation_identity("fixture-generation", None)
        self.store.insert_rows("emission", [
            {"start": 0, "end": 32, "emission_type": "INCBIN", "classification": "UNKNOWN",
             "source_kind": "UNKNOWN", "source_owned": 0, "artifact_type": "blob", "artifact": "a"},
            {"start": 32, "end": ROM_SIZE, "emission_type": "INCBIN", "classification": "UNKNOWN",
             "source_kind": "UNKNOWN", "source_owned": 0, "artifact_type": "blob", "artifact": "b"},
        ])

    def tearDown(self) -> None:
        self.store.db.close()
        self.temp.cleanup()

    def test_all_unknown_ranges_are_ranked_and_order_is_deterministic(self) -> None:
        first = rank_unknown_ranges(self.store)
        second = rank_unknown_ranges(self.store)
        self.assertEqual(len(first), 2)
        self.assertEqual(first, second)
        self.assertEqual([(row["rom_start"], row["rom_end"]) for row in first],
                         [(0, 32), (32, ROM_SIZE)])
        self.assertTrue(all(row["exact_blocker"] == "NO_CANONICAL_OBJECT_OR_EXACT_BOUNDARY" for row in first))

    def test_hypothesis_and_observation_signals_never_classify_map_bytes(self) -> None:
        rid = stable_id("range", {"fixture": "candidate"})
        oid = stable_id("object", {"fixture": "candidate"})
        self.store.insert_rows("rom_range", [{"range_id": rid,
            "rom_sha256": "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263",
            "start": 4, "end": 8}])
        self.store.insert_rows("rom_object", [{"object_id": oid, "range_id": rid,
            "object_type": "ROM_RANGE", "attributes_json": "{}"}])
        self.store.insert_rows("claim", [{"claim_id": "hypothesis", "object_id": oid,
            "claim_type": "FORMAT_HYPOTHESIS",
            "value_json": canonical({"candidate_kind": "uniform_alignment_padding",
                                      "exact_boundary": True}), "status": "HYPOTHESIS"}])
        ranked = rank_unknown_ranges(self.store)
        candidate = next(row for row in ranked if row["rom_start"] == 0)
        self.assertEqual(candidate["likely_generic_class"], ["uniform_alignment_padding"])
        self.assertEqual(candidate["exact_blocker"], "HYPOTHESIS_ONLY_BOUNDARY_OR_FORMAT")
        self.store.db.commit()
        report = campaign_report(self.db, top=2)
        self.assertEqual(report["new_classified_bytes"], 0)
        self.assertEqual(report["source_owned_delta"], 0)
        self.assertEqual(report["unknown_ranges_before"], report["unknown_ranges_after"])


if __name__ == "__main__":
    unittest.main()
