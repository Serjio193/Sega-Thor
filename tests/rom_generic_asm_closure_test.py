"""M14.4 exact seed admission and parent-bound proposal persistence."""

import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from rom_generic_asm_closure import exact_cfg_seeds, persist_proposals
from rom_knowledge_map import KnowledgeStore, object_id, range_id

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"


class GenericAsmClosureTest(unittest.TestCase):
    def test_only_exact_runtime_instruction_claims_seed_cfg(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = KnowledgeStore(Path(temporary) / "knowledge.sqlite", ROM_SHA, 3_145_728)
            store.set_generation_identity("gen-fixture", None)
            rid = range_id(ROM_SHA, 0x100, 0x102)
            oid = object_id(ROM_SHA, 0x100, 0x102, "M68K_INSTRUCTION")
            target_rid = range_id(ROM_SHA, 0x102, 0x104)
            target_oid = object_id(ROM_SHA, 0x102, 0x104, "M68K_INSTRUCTION")
            store.insert_rows("rom_range", [{"range_id": rid, "rom_sha256": ROM_SHA,
                "start": 0x100, "end": 0x102}])
            store.insert_rows("rom_object", [{"object_id": oid, "range_id": rid,
                "object_type": "M68K_INSTRUCTION", "attributes_json": "{}"}])
            store.insert_rows("rom_range", [{"range_id": target_rid, "rom_sha256": ROM_SHA,
                "start": 0x102, "end": 0x104}])
            store.insert_rows("rom_object", [{"object_id": target_oid, "range_id": target_rid,
                "object_type": "M68K_INSTRUCTION", "attributes_json": "{}"}])
            store.insert_rows("claim", [{"claim_id": "claim:runtime", "object_id": oid,
                "claim_type": "EXECUTED_FROM_ROM", "value_json": "true",
                "status": "OBSERVED_RUNTIME"}])
            store.insert_rows("claim", [{"claim_id": "claim:hypothesis", "object_id": oid,
                "claim_type": "FORMAT_HYPOTHESIS", "value_json": "{}",
                "status": "HYPOTHESIS"}])
            store.insert_rows("claim", [{"claim_id": "claim:canonical-reference", "object_id": oid,
                "claim_type": "GLOBAL_EVIDENCE_REFERENCE", "value_json": "{}",
                "status": "DERIVED_EXACT"}])
            store.insert_rows("source_artifact", [{"source_sha256": "a" * 64,
                "checkpoint": "fixture", "artifact_name": "capture", "artifact_type": "FLOW"}])
            store.insert_rows("source_artifact", [{"source_sha256": "b" * 64,
                "checkpoint": "fixture-2", "artifact_name": "capture-2", "artifact_type": "FLOW"}])
            store.insert_rows("evidence_ref", [{"ref_id": "evidence:runtime",
                "subject_type": "CLAIM", "subject_id": "claim:runtime",
                "source_sha256": "a" * 64, "fact_kind": "EXECUTED_PC",
                "fact_count": 1, "locator_json": "{\"pc\":256}"}])
            store.insert_rows("evidence_ref", [{"ref_id": "evidence:canonical-reference",
                "subject_type": "CLAIM", "subject_id": "claim:canonical-reference",
                "source_sha256": "b" * 64, "fact_kind": "CANONICAL_REFERENCE",
                "fact_count": 1, "locator_json": "{\"pc\":256}"}])
            store.insert_rows("relation", [{"relation_id": "relation:asm-edge",
                "relation_type": "ASM_CFG_EDGE", "source_object_id": oid,
                "target_object_id": target_oid, "target_address": None,
                "status": "DERIVED_EXACT", "attributes_json": "{}"}])
            store.insert_rows("evidence_ref", [{"ref_id": "evidence:asm-edge",
                "subject_type": "RELATION", "subject_id": "relation:asm-edge",
                "source_sha256": "b" * 64, "fact_kind": "ASM_CFG_EDGE",
                "fact_count": 1, "locator_json": "{\"pc\":258}"}])
            seeds = exact_cfg_seeds(store, 0x100, 0x104)
            self.assertEqual([seed["pc"] for seed in seeds], [0x100, 0x102])
            self.assertEqual(len(seeds[0]["claim_ids"]), 2)
            self.assertEqual(seeds[0]["seed_kinds"], ["EXECUTED_FROM_ROM", "GLOBAL_EVIDENCE_REFERENCE"])
            self.assertEqual({ref["ref_id"] for ref in seeds[0]["evidence_refs"]},
                {"evidence:runtime", "evidence:canonical-reference"})
            self.assertEqual(seeds[1]["relation_ids"], ["relation:asm-edge"])
            store.db.close()

    def test_exact_proof_is_persisted_as_parent_bound_operations(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            db_path = root / "parent.sqlite"
            store = KnowledgeStore(db_path, ROM_SHA, 3_145_728)
            store.set_generation_identity("gen-fixture", None)
            hashes = store.hashes()
            store.db.commit()
            result = {"start": 0x100, "end": 0x120, "exact_extent_proofs": [{
                "start": 0x108, "end": 0x10A, "cfg_sha256": "b" * 64,
                "roundtrip_sha256": "c" * 64, "proof_refs": ["evidence:runtime"],
                "references": [{"source_pc": 0x108, "target_pc": 0x10A,
                                "instruction_operation": "bra"}]}]}
            proposal = persist_proposals(db_path, root, [result], "a" * 64,
                hashes["map_hash"], hashes["emission_hash"], "gen-fixture")
            self.assertEqual(proposal["operation_count"], 5)
            child = KnowledgeStore(root / proposal["database"], ROM_SHA, 3_145_728, read_only=True)
            operations = [json.loads(row[0]) for row in child.db.execute(
                "SELECT operation_json FROM map_proposal_operation ORDER BY ordinal")]
            self.assertEqual(len(operations), 5)
            self.assertEqual(operations[1]["operation"], "ADD_BOUNDARY")
            self.assertEqual(operations[3]["operation"], "CLASSIFY_RANGE")
            self.assertTrue(any(operation["operation"] == "ADD_REFERENCE"
                                for operation in operations))
            self.assertEqual(child.hashes()["map_hash"], hashes["map_hash"])
            self.assertEqual(child.hashes()["emission_hash"], hashes["emission_hash"])
            child.db.close()
            store.db.close()


if __name__ == "__main__":
    unittest.main()
