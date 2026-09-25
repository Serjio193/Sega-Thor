"""M14.5 non-owning exact ASM adoption contract checks."""

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from rom_knowledge_map import KnowledgeStore, range_id
from rom_knowledge_map_adoption_validation import _stage7_eligibility

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"


class AdoptionContractTest(unittest.TestCase):
    def test_exact_classification_does_not_bypass_stage7_gates(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = KnowledgeStore(Path(temporary) / "map.sqlite", ROM_SHA, 3_145_728)
            store.set_generation_identity("gen-fixture", None)
            rid = range_id(ROM_SHA, 0x100, 0x104)
            store.insert_rows("rom_range", [{"range_id": rid, "rom_sha256": ROM_SHA,
                "start": 0x100, "end": 0x104}])
            store.insert_rows("rom_object", [{"object_id": "object:entry", "range_id": rid,
                "object_type": "M68K_INSTRUCTION", "attributes_json": "{}"}])
            store.insert_rows("emission", [{"start": 0x100, "end": 0x104,
                "emission_type": "ASM", "classification": "ASM_ROUNDTRIP_EXACT",
                "source_kind": "ASM_ROUNDTRIP_EXACT", "source_owned": 0,
                "artifact_type": "asm", "artifact": "asm/entry.asm"}])
            result = _stage7_eligibility(store,
                {"start": 0x100, "end": 0x104, "valid": True}, {})
            self.assertTrue(result["exact_extent"])
            self.assertTrue(result["closed_cfg"])
            self.assertTrue(result["roundtrip_exact"])
            self.assertFalse(result["reconstruction_proof"])
            self.assertFalse(result["ownership_preconditions"]["static_caller_or_entry_support"])
            self.assertFalse(result["stage7_eligible"])
            self.assertEqual(result["exact_blocker"],
                "MISSING_STATIC_CALLER_OR_ENTRY_AND_WHOLE_ROM_PROMOTION_PROOF")
            self.assertEqual(store.metrics()["source_owned_bytes"], 0)
            store.db.close()

    def test_invalid_component_cannot_report_exact_stage7_evidence(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = KnowledgeStore(Path(temporary) / "map.sqlite", ROM_SHA, 3_145_728)
            store.set_generation_identity("gen-fixture", None)
            result = _stage7_eligibility(store,
                {"start": 0x200, "end": 0x204, "valid": False}, {})
            self.assertFalse(result["exact_extent"])
            self.assertFalse(result["closed_cfg"])
            self.assertFalse(result["roundtrip_exact"])
            self.assertFalse(result["emitter_available"])
            self.assertFalse(result["stage7_eligible"])
            store.db.close()


if __name__ == "__main__":
    unittest.main()
