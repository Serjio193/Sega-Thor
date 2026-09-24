"""Regression tests for exact-only global graph to canonical map projection."""

from __future__ import annotations

import copy
import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from rom_knowledge_fusion_query import global_object_view  # noqa: E402
from rom_knowledge_map import KnowledgeStore, canonical, stable_id  # noqa: E402
from rom_knowledge_map_closure import (  # noqa: E402
    _apply_reference, _exact_fused_paths, _operation,
)


class MapClosureTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.db = Path(self.temp.name) / "knowledge.sqlite"
        self.store = KnowledgeStore(self.db, "a" * 64, 64)
        self.store.set_generation_identity("base", None)
        self.store.insert_rows("emission", [{"start": 0, "end": 64,
            "emission_type": "ASM", "classification": "CODE_VERIFIED",
            "source_kind": "CODE_VERIFIED", "source_owned": 1,
            "artifact_type": "asm", "artifact": "fixture.asm"}])
        self.range_id = stable_id("range", {"fixture": "rom"})
        self.object_id = stable_id("object", {"fixture": "emitter"})
        self.store.insert_rows("rom_range", [{"range_id": self.range_id,
            "rom_sha256": "a" * 64, "start": 8, "end": 10}])
        self.store.insert_rows("rom_object", [{"object_id": self.object_id,
            "range_id": self.range_id, "object_type": "M68K_INSTRUCTION",
            "attributes_json": "{}"}])
        producer_range = stable_id("range", {"fixture": "producer"})
        self.store.insert_rows("rom_range", [{"range_id": producer_range,
            "rom_sha256": "a" * 64, "start": 12, "end": 14}])
        self.store.insert_rows("rom_object", [{"object_id": "object:producer",
            "range_id": producer_range, "object_type": "M68K_INSTRUCTION",
            "attributes_json": "{}"}])
        self.first_id, self.second_id = "rel:first", "rel:second"
        self.store.insert_rows("relation", [
            {"relation_id": self.first_id, "relation_type": "RAM_SHADOW_TO_DMA",
             "source_object_id": "object:producer", "target_object_id": self.object_id,
             "target_address": None, "status": "DERIVED_EXACT",
             "attributes_json": canonical({"dma_source_address": 100, "destination_start": 200,
                                            "length_bytes": 16, "source_truth": "EXACT"})},
            {"relation_id": self.second_id, "relation_type": "DMA_TO_HARDWARE_SAT",
             "source_object_id": self.object_id, "target_object_id": None,
             "target_address": 200, "status": "DERIVED_EXACT",
             "attributes_json": canonical({"dma_source_address": 100, "length_bytes": 16,
                                            "source_truth": "EXACT_DMA_TO_SAT_CHAIN"})},
        ])
        sources = [("b" * 64, "M14_2B:GAMEPLAY"), ("c" * 64, "M14_2B:SPRITE")]
        for index, (digest, artifact_type) in enumerate(sources):
            self.store.insert_rows("source_artifact", [{"source_sha256": digest,
                "checkpoint": "fixture", "artifact_name": f"{artifact_type}.json",
                "artifact_type": artifact_type}])
            relation_id = self.first_id if index == 0 else self.second_id
            self.store.insert_rows("evidence_ref", [{"ref_id": f"ev:{index}",
                "subject_type": "RELATION", "subject_id": relation_id,
                "source_sha256": digest, "fact_kind": "EXACT", "fact_count": 1,
                "locator_json": canonical({"capture_id": "capture:1",
                    "original_truth": "EXACT" if artifact_type.endswith("GAMEPLAY") else
                    "EXACT_DMA_TO_SAT_CHAIN"})}])
        self.derivation_id = self.store.record_derivation("M14_2B_RAM_SHADOW_DMA_SAT_JOIN", "1",
            "d" * 64, "e" * 64,
            [{"subject_type": "relation", "subject_id": self.first_id, "role": "gameplay"},
             {"subject_type": "relation", "subject_id": self.second_id, "role": "sprite"}],
            "relation_path", "path:fixture", {"relations": [self.first_id, self.second_id],
                "truth": "DERIVED_EXACT", "join_identity": self.object_id})
        self.store.db.commit()

    def tearDown(self) -> None:
        self.store.db.close()
        self.temp.cleanup()

    def test_exact_independent_path_generates_parent_bound_reference(self) -> None:
        paths = _exact_fused_paths(self.store)
        self.assertEqual(len(paths), 1)
        context = {"base_generation": "base", "base_map_hash": self.store.hashes()["map_hash"],
            "emission_hash": self.store.hashes()["emission_hash"], "graph_hash": "f" * 64}
        operation = _operation(paths, context)
        operation["preconditions"]["expected_map_owner"] = dict(self.store.db.execute(
            "SELECT * FROM emission WHERE start<=8 AND 8<end").fetchone())
        proposal_id = "proposal:fixture"
        self.store.create_map_proposal(proposal_id, "base", context["base_map_hash"],
            context["graph_hash"], "test-v1", [operation])
        self.store.validate_map_proposal_parent(proposal_id, "base")
        emission_before = self.store.hashes()["emission_hash"]
        owned_before = self.store.metrics()["source_owned_bytes"]
        _apply_reference(self.store, operation)
        self.store.db.commit()
        view = global_object_view(self.store, 8)
        self.assertTrue(any(row["claim_type"] == "GLOBAL_EVIDENCE_REFERENCE" for row in view["claims"]))
        self.assertEqual(len([item for item in view["evidence"]
                              if item["fact_kind"] == "FUSED_RUNTIME_DMA_EMITTER_REFERENCE"]), 2)
        self.assertEqual(view["supporting_captures"], ["capture:1"])
        self.assertEqual(self.store.hashes()["emission_hash"], emission_before)
        self.assertEqual(self.store.metrics()["source_owned_bytes"], owned_before)
        self.store.set_generation_identity("child", "base")
        with self.assertRaisesRegex(ValueError, "STOP_MAP_PROPOSAL_STALE_PARENT"):
            self.store.validate_map_proposal_parent(proposal_id, "base")

    def test_hypothesis_relation_never_becomes_canonical_reference(self) -> None:
        self.store.db.execute("UPDATE relation SET status='HYPOTHESIS' WHERE relation_id=?",
                              (self.second_id,))
        self.assertEqual(_exact_fused_paths(self.store), [])

    def test_capture_support_merges_under_one_stable_reference_identity(self) -> None:
        path = _exact_fused_paths(self.store)[0]
        second_capture = copy.deepcopy(path)
        second_capture["relations"] = ["rel:second-gameplay", "rel:second-sprite"]
        second_capture["derivation_id"] = "derivation:second-capture"
        for source in second_capture["sources"]:
            source["capture_id"] = "capture:2"
            source["derivation_id"] = second_capture["derivation_id"]
            source["source_sha256"] = hashlib.sha256(
                (source["source_sha256"] + "second").encode()).hexdigest()
            source["relation_id"] = "rel:second-gameplay" if source["artifact_type"].endswith("GAMEPLAY") \
                else "rel:second-sprite"
        context = {"base_generation": "base", "base_map_hash": self.store.hashes()["map_hash"],
            "emission_hash": self.store.hashes()["emission_hash"], "graph_hash": "f" * 64}
        first_operation = _operation([path], context)
        merged = _operation([path, second_capture], context)
        self.assertEqual(merged["claim_id"], first_operation["claim_id"])
        self.assertEqual(merged["source_count"], 2)
        self.assertEqual(merged["capture_count"], 2)
        self.assertEqual(len(merged["source_artifacts"]), 4)
        self.assertEqual(len(merged["input_facts"]), 4)


if __name__ == "__main__":
    unittest.main()
