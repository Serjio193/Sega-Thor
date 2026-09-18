import json
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "src/tools"), str(ROOT / "src/tools/thor_evidence")]
from rom_knowledge_map import KnowledgeStore, canonical, object_id, range_id, stable_id


def claim(object_key, kind, value, status):
    value_json = canonical(value)
    payload = {"object_id": object_key, "claim_type": kind, "value": value, "status": status}
    return {"claim_id": stable_id("claim", payload), "object_id": object_key,
            "claim_type": kind, "value_json": value_json, "status": status}


def evidence(subject_type, subject_id, source_hash, kind, count, locator):
    payload = {"subject_type": subject_type, "subject_id": subject_id,
               "source_sha256": source_hash, "fact_kind": kind,
               "fact_count": count, "locator": locator}
    return {"ref_id": stable_id("evidence", payload), "subject_type": subject_type,
            "subject_id": subject_id, "source_sha256": source_hash, "fact_kind": kind,
            "fact_count": count, "locator_json": canonical(locator)}


class RomKnowledgeMapTest(unittest.TestCase):
    def test_canonical_identity_is_rom_range_and_type_only(self):
        digest = "a" * 64
        first = object_id(digest, 0x10, 0x14, "M68K_INSTRUCTION")
        self.assertEqual(first, object_id(digest, 0x10, 0x14, "M68K_INSTRUCTION"))
        self.assertNotEqual(first, object_id(digest, 0x10, 0x14, "ROM_DATA"))
        self.assertEqual(range_id(digest, 0x10, 0x14), range_id(digest, 0x10, 0x14))

    def test_queries_deduplication_and_compact_export(self):
        with tempfile.TemporaryDirectory() as directory:
            digest, source_hash = "a" * 64, "b" * 64
            store = KnowledgeStore(Path(directory) / "knowledge.sqlite", digest, 8)
            code_id = object_id(digest, 0, 4, "ROM_RANGE")
            instruction_id = object_id(digest, 4, 6, "M68K_INSTRUCTION")
            unknown_id = object_id(digest, 6, 8, "UNKNOWN")
            code_claim = claim(code_id, "SOURCE_CLASS", {
                "source_kind": "CODE_VERIFIED", "classification": "CODE_VERIFIED",
                "confidence": "HIGH"}, "STATIC_VERIFIED")
            ownership_claim = claim(code_id, "SOURCE_OWNED", True, "STATIC_VERIFIED")
            execution_claim = claim(instruction_id, "EXECUTED_FROM_ROM", True, "OBSERVED_RUNTIME")
            hypothesis_claim = claim(unknown_id, "FORMAT_HYPOTHESIS", {"kind": "candidate"}, "HYPOTHESIS")
            relation_payload = {"relation_type": "OBSERVED_NEXT_PC",
                "source_object_id": instruction_id, "target_object_id": None,
                "target_address": 6, "status": "OBSERVED_RUNTIME"}
            relation_id = stable_id("relation", relation_payload)
            locator = {"table": "map_edge", "edge_id": "edge-1", "run_id": "run-1"}
            rows = {
                "rom_range": [
                    {"range_id": range_id(digest, 0, 4), "rom_sha256": digest, "start": 0, "end": 4},
                    {"range_id": range_id(digest, 4, 6), "rom_sha256": digest, "start": 4, "end": 6},
                    {"range_id": range_id(digest, 6, 8), "rom_sha256": digest, "start": 6, "end": 8}],
                "rom_object": [
                    {"object_id": code_id, "range_id": range_id(digest, 0, 4), "object_type": "ROM_RANGE",
                     "attributes_json": '{"source_owned_bytes":4}'},
                    {"object_id": instruction_id, "range_id": range_id(digest, 4, 6),
                     "object_type": "M68K_INSTRUCTION", "attributes_json": '{"source_owned_bytes":0}'},
                    {"object_id": unknown_id, "range_id": range_id(digest, 6, 8), "object_type": "UNKNOWN",
                     "attributes_json": '{"source_owned_bytes":0}'}],
                "claim": [code_claim, ownership_claim, execution_claim, hypothesis_claim],
                "relation": [{"relation_id": relation_id, **relation_payload, "attributes_json": "{}"}],
                "emission": [
                    {"start": 0, "end": 4, "emission_type": "ASM", "classification": "CODE_VERIFIED",
                     "source_kind": "CODE_VERIFIED", "source_owned": 1,
                     "artifact_type": "asm", "artifact": "asm/code.asm"},
                    {"start": 4, "end": 8, "emission_type": "INCBIN", "classification": "UNKNOWN",
                     "source_kind": "UNKNOWN", "source_owned": 0,
                     "artifact_type": "blob", "artifact": "blobs/000004_000008.bin"}],
                "source_artifact": [{"source_sha256": source_hash, "checkpoint": "fixture",
                    "artifact_name": "fixture.json", "artifact_type": "TEST"}],
                "evidence_ref": [evidence("RELATION", relation_id, source_hash,
                    "RUNTIME_TERMINAL_FACT", 1, locator)],
                "conflict": [],
                "map_import": [{"import_key": "fixture", "input_hash": "c" * 64}],
            }
            try:
                store.db.execute("BEGIN IMMEDIATE")
                for table, data in rows.items():
                    store.insert_rows(table, data)
                store.db.commit()
                first_hashes, first_counts = store.hashes(), store.counts()
                store.db.execute("BEGIN IMMEDIATE")
                for table, data in rows.items():
                    store.insert_rows(table, data)
                store.db.commit()
                self.assertEqual(first_hashes, store.hashes())
                self.assertEqual(first_counts, store.counts())
                metrics = store.metrics()
                self.assertEqual(metrics["unknown_bytes"], 4)
                self.assertEqual(metrics["source_owned_code_ranges_unobserved"], 1)
                self.assertEqual(metrics["source_owned_code_bytes_unobserved"], 4)
                self.assertEqual(metrics["executed_not_fully_owned_objects"], 1)
                self.assertEqual(len(store.query("unknown")), 1)
                self.assertEqual(len(store.query("executed")), 1)
                self.assertEqual(len(store.query("executed-not-owned")), 1)
                self.assertEqual(len(store.query("owned-code-unseen")), 1)
                self.assertEqual(len(store.query("conflicts")), 0)
                self.assertEqual(len(store.query("hypotheses")), 1)
                self.assertEqual(len(store.query("emission-at", address=7)), 1)
                self.assertEqual(len(store.query("objects-in-range", start=4, end=5)), 1)
                self.assertEqual(len(store.query("evidence", subject_id=relation_id)), 1)
                exported = store.export({}, {}, {})
                self.assertIsInstance(exported["claims"][0]["value"], (bool, dict))
                self.assertIsInstance(exported["evidence_refs"][0]["locator"], dict)
                self.assertNotIn("bytes_hex", json.dumps(exported))
            finally:
                store.close()


if __name__ == "__main__":
    unittest.main()
