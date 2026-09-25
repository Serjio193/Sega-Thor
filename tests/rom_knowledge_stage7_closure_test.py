import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "src" / "tools"
EVIDENCE = TOOLS / "thor_evidence"
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(EVIDENCE))

from re_m12_auto_promote import selected
from rom_knowledge_map import KnowledgeStore, range_id, sha256_bytes
from rom_knowledge_stage7_closure import (
    _caller_count,
    _reference_relation,
    _stage7_map_admits,
    ROM_SHA,
)


class Stage7ClosureContractTest(unittest.TestCase):
    def test_existing_selector_requires_exactness_and_verified_caller(self):
        assets = {"records": []}
        row = {"start": 0x100, "end": 0x104, "exact": True, "called_by": 0}
        self.assertEqual(selected([row], assets)[0], [])
        self.assertEqual(len(selected([{**row, "called_by": 1}], assets)[0]), 1)
        self.assertEqual(selected([{**row, "exact": False, "called_by": 1}], assets)[0], [])

    def test_map_adapter_preserves_unknown_only_split_contract(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = KnowledgeStore(Path(temporary) / "map.sqlite", ROM_SHA, 0x1000)
            store.set_generation_identity("fixture", None)
            for start, kind in ((0x100, "UNKNOWN"), (0x200, "ASM_ROUNDTRIP_EXACT")):
                store.insert_rows("emission", [{"start": start, "end": start + 4,
                    "emission_type": "ASM", "classification": kind,
                    "source_kind": kind, "source_owned": 0,
                    "artifact_type": "asm", "artifact": "component.asm"}])
            self.assertTrue(_stage7_map_admits(store, 0x100, 0x104))
            self.assertFalse(_stage7_map_admits(store, 0x200, 0x204))
            store.db.close()

    def test_only_static_verified_relations_count_as_callers(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = KnowledgeStore(Path(temporary) / "map.sqlite", ROM_SHA, 0x1000)
            store.set_generation_identity("fixture", None)
            for start, object_id in ((0x300, "caller"), (0x400, "entry")):
                rid = range_id(ROM_SHA, start, start + 2)
                store.insert_rows("rom_range", [{"range_id": rid, "rom_sha256": ROM_SHA,
                    "start": start, "end": start + 2}])
                store.insert_rows("rom_object", [{"object_id": object_id, "range_id": rid,
                    "object_type": "M68K_INSTRUCTION", "attributes_json": "{}"}])
            store.insert_rows("relation", [{"relation_id": "observed", "relation_type": "STATIC_CALLER",
                "source_object_id": "caller", "target_object_id": "entry",
                "target_address": None, "status": "OBSERVED", "attributes_json": "{}"},
                {"relation_id": "verified", "relation_type": "STATIC_CALLER",
                "source_object_id": "caller", "target_object_id": "entry",
                "target_address": None, "status": "STATIC_VERIFIED", "attributes_json": "{}"}])
            self.assertEqual(_caller_count(store, "entry"), 1)
            store.db.close()

    def test_exact_cfg_reference_reconciliation_is_idempotent_and_byte_bound(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = KnowledgeStore(Path(temporary) / "map.sqlite", ROM_SHA, 0x1000)
            store.set_generation_identity("fixture", None)
            rom = bytearray(0x1000)
            rom[0x100:0x102] = b"\x4e\x75"
            rom[0x200:0x202] = b"\x4e\x75"
            for start, object_id in ((0x100, "source"), (0x200, "target")):
                rid = range_id(ROM_SHA, start, start + 2)
                store.insert_rows("rom_range", [{"range_id": rid, "rom_sha256": ROM_SHA,
                    "start": start, "end": start + 2}])
                raw = bytes(rom[start:start + 2])
                store.insert_rows("rom_object", [{"object_id": object_id, "range_id": rid,
                    "object_type": "M68K_INSTRUCTION", "attributes_json":
                    '{"bytes_sha256":"' + sha256_bytes(raw) +
                    '","length":2,"opcode":20085}'}])
            report_sha = "f" * 64
            store.insert_rows("source_artifact", [{"source_sha256": report_sha,
                "checkpoint": "M14.4", "artifact_name": "proposal.json",
                "artifact_type": "M14_4_EXACT_ASM_PROPOSAL"}])
            store.insert_rows("evidence_ref", [{"ref_id": "proof-ref",
                "subject_type": "CLAIM", "subject_id": "proof",
                "source_sha256": report_sha, "fact_kind": "CFG_PROOF",
                "fact_count": 1, "locator_json": "{}"}])
            component = {"start": 0x100, "end": 0x102,
                "cfg_sha256": "a" * 64, "roundtrip_sha256": "b" * 64}
            operation = {"reference_kind": "M68K_CONTROL_FLOW",
                "cfg_proof": component["cfg_sha256"],
                "roundtrip_proof": component["roundtrip_sha256"],
                "proof_refs": ["proof-ref"], "source_pc": 0x100,
                "target_pc": 0x200, "instruction_operation": "RTS"}
            first, existed = _reference_relation(store, component, operation,
                bytes(rom), report_sha)
            second, replayed = _reference_relation(store, component, operation,
                bytes(rom), report_sha)
            self.assertFalse(existed)
            self.assertTrue(replayed)
            self.assertEqual(first["relation_id"], second["relation_id"])
            self.assertEqual(store.db.execute("SELECT COUNT(*) FROM relation").fetchone()[0], 1)
            rom[0x100] ^= 1
            with self.assertRaisesRegex(ValueError, "EXACT_INSTRUCTION_BYTES_"):
                _reference_relation(store, component, operation, bytes(rom), report_sha)
            store.db.close()


if __name__ == "__main__":
    unittest.main()
