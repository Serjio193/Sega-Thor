"""Record accounting, provenance, replay, and streaming hash regressions."""

from __future__ import annotations

import hashlib
import json
import sqlite3
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "src/tools"), str(ROOT / "src/tools/thor_evidence")]
sys.path.insert(0, str(ROOT / "tests"))

from normalized_v2_canonical_adapter import apply_path
from rom_knowledge_hash_stream import canonical_hash, grouped_rows_hash
from rom_knowledge_map import KnowledgeStore, object_id, range_id
import rom_knowledge_pipeline_test as pipeline_fixture
import rom_knowledge_pipeline as pipeline


class NormalizedV2AdapterTests(unittest.TestCase):
    def test_primary_records_are_accounted_once_and_replay_is_noop(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            rom = bytearray(0x100)
            rom[0x10:0x12] = b"\x4e\x71"
            rom_bytes = bytes(rom)
            rom_sha = hashlib.sha256(rom_bytes).hexdigest()
            store_path = root / "map.sqlite"
            store = KnowledgeStore(store_path, rom_sha, len(rom_bytes))
            rid = range_id(rom_sha, 0x10, 0x12)
            oid = object_id(rom_sha, 0x10, 0x12, "M68K_INSTRUCTION")
            store.insert_rows("rom_range", [{"range_id": rid, "rom_sha256": rom_sha,
                "start": 0x10, "end": 0x12}])
            store.insert_rows("rom_object", [{"object_id": oid, "range_id": rid,
                "object_type": "M68K_INSTRUCTION",
                "attributes_json": json.dumps({"opcode": 0x4E71}, sort_keys=True,
                                                separators=(",", ":"))}])
            store.close()
            records = [
                {"run_id": 42, "epoch": 1, "frame": None, "stream_sequence": 1,
                 "instruction_sequence": 1, "cpu_id": "M68K", "pc": 0x10,
                 "address": 0x12, "opcode": 0x4E71, "kind": "instruction",
                 "opcode_verification": "ROM_OPCODE_EXACT"},
                {"run_id": 42, "epoch": 1, "frame": None, "stream_sequence": 2,
                 "instruction_sequence": 1, "cpu_id": "M68K", "pc": 0x10,
                 "address": 0xC00000, "opcode": 3, "kind": "bus"},
                {"run_id": 42, "epoch": 1, "frame": 1, "stream_sequence": 3,
                 "instruction_sequence": 2, "cpu_id": "M68K", "kind": "frame_boundary"},
                {"kind": "instruction", "cpu_id": "M68K"},
            ]
            artifact = root / "normalized.json"
            artifact.write_text(json.dumps({"instructions": [{"ignored": True}],
                "records": records, "schema": "oasis.m13.normalized-generic-corpus.v2",
                "corpus_id": "run-42-test", "record_count": len(records) + 1,
                "sealed": True, "raw_sha256": "a" * 64}), encoding="utf-8")
            digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
            first = apply_path(store_path, artifact, digest, rom_bytes, rom_sha,
                               len(rom_bytes))
            self.assertEqual(first["input_records"], 4)
            self.assertEqual(first["producer_record_count"], 5)
            self.assertEqual(first["producer_deduplicated_records"], 1)
            self.assertEqual(first["outcomes"], {"MERGED": 1, "ALREADY_KNOWN": 0,
                "UNRESOLVED": 2, "REJECTED": 1})
            self.assertEqual(sum(first["outcomes"].values()), first["input_records"])
            self.assertEqual(first["unaccounted_records"], 0)
            replay = apply_path(store_path, artifact, digest, rom_bytes, rom_sha,
                                len(rom_bytes), verify_only=True)
            self.assertEqual(replay["outcomes"]["ALREADY_KNOWN"], 4)
            self.assertEqual(replay["provenance_rows_inserted"], 0)

    def test_grouped_hash_matches_materialized_canonical_json(self):
        with tempfile.TemporaryDirectory() as temporary:
            store = KnowledgeStore(Path(temporary) / "map.sqlite", "b" * 64, 32)
            tables = ("rom_range", "rom_object", "claim", "relation", "conflict")
            columns = {name: __import__("rom_knowledge_map").TABLE_COLUMNS[name]
                       for name in tables}
            materialized = {name: [tuple(row) for row in store.db.execute(
                f"SELECT {','.join(columns[name])} FROM {name} "
                f"ORDER BY {','.join(columns[name])}")] for name in tables}
            self.assertEqual(grouped_rows_hash(store.db, tables, columns),
                             canonical_hash(materialized))
            store.close()

    def test_supplemental_adapter_runs_inside_canonical_publisher_transaction(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            rom_path = root / "fixture.rom"
            rom_path.write_bytes(pipeline_fixture.ROM_BYTES)
            knowledge = pipeline_fixture._base_knowledge(root / "base.sqlite")
            master = root / "master.sqlite"
            output = root / "published"
            session = pipeline_fixture._make_session(root / "session.sqlite",
                "normalized-adapter-fixture", 601, instructions=(0x10,))

            def publish_extra(staged_path):
                store = KnowledgeStore(staged_path, pipeline_fixture.ROM_SHA,
                    len(pipeline_fixture.ROM_BYTES))
                try:
                    digest = "d" * 64
                    store.insert_rows("source_artifact", [{"source_sha256": digest,
                        "checkpoint": "fixture", "artifact_name": "normalized.json",
                        "artifact_type": "NORMALIZED_GENERIC_CORPUS_V2"}])
                    store.insert_rows("evidence_ref", [{"ref_id": "fixture:normalized",
                        "subject_type": "NORMALIZED_RECORD", "subject_id": "fixture:0",
                        "source_sha256": digest, "fact_kind": "NORMALIZED_V2_RECORD:UNRESOLVED",
                        "fact_count": 1, "locator_json": "{}"}])
                finally:
                    store.close()
                return {"status": "PASS_NORMALIZED_V2_ACCOUNTED", "input_records": 1}

            result = pipeline.archive_and_refresh_knowledge(session, master, knowledge,
                rom_path, output, expected_source_owned=0,
                expected_rom_sha256=pipeline_fixture.ROM_SHA,
                expected_rom_size=len(pipeline_fixture.ROM_BYTES),
                post_import_hook=publish_extra)
            self.assertEqual(result["import"]["supplemental_evidence"]["input_records"], 1)
            self.assertEqual(result["independent_audit"]["status"],
                "PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1")
            final = sqlite3.connect(Path(result["generation_dir"]) / "knowledge.sqlite")
            try:
                self.assertEqual(final.execute("SELECT COUNT(*) FROM evidence_ref WHERE "
                    "ref_id='fixture:normalized'").fetchone()[0], 1)
            finally:
                final.close()

    def test_normalized_evidence_publisher_commits_gc_compatible_generation(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            rom_path = root / "fixture.rom"
            rom_path.write_bytes(pipeline_fixture.ROM_BYTES)
            knowledge = pipeline_fixture._base_knowledge(root / "base.sqlite")
            master, output = root / "master.sqlite", root / "published"
            session = pipeline_fixture._make_session(root / "session.sqlite",
                "normalized-v2-publisher-fixture", 701, instructions=(0x10,))
            seed = pipeline.archive_and_refresh_knowledge(session, master, knowledge,
                rom_path, output, expected_source_owned=0,
                expected_rom_sha256=pipeline_fixture.ROM_SHA,
                expected_rom_size=len(pipeline_fixture.ROM_BYTES))
            artifact = root / "normalized.json"
            record = {"run_id": 701, "epoch": 1, "frame": None,
                "stream_sequence": 1, "instruction_sequence": 1, "cpu_id": "M68K",
                "pc": 0x10, "address": 0x12, "opcode": 0x4E71,
                "kind": "instruction", "opcode_verification": "ROM_OPCODE_EXACT"}
            artifact.write_text(json.dumps({"records": [record],
                "schema": "oasis.m13.normalized-generic-corpus.v2",
                "corpus_id": "run-701-fixture", "record_count": 1,
                "sealed": True, "raw_sha256": "a" * 64}), encoding="utf-8")
            artifact_sha = hashlib.sha256(artifact.read_bytes()).hexdigest()
            result = pipeline.publish_normalized_v2_evidence(artifact, artifact_sha,
                rom_path, output, seed["knowledge_after"]["hashes"]["map_hash"], 0,
                expected_rom_sha256=pipeline_fixture.ROM_SHA,
                expected_rom_size=len(pipeline_fixture.ROM_BYTES))
            self.assertEqual(result["status"], "PASS_NORMALIZED_V2_CANONICAL_PUBLISH_V1")
            self.assertEqual(result["independent_audit"]["status"],
                "PASS_NORMALIZED_V2_CANONICAL_AUDIT_V1")
            self.assertEqual(result["import"]["supplemental_evidence"]["producer_deduplicated_records"], 0)
            plan = __import__("knowledge_generation_gc").build_plan(output)
            self.assertEqual(plan["status"], "PASS")
            pointer = json.loads((output / "current.json").read_text())
            check, _ = __import__("knowledge_generation_gc")._self_check_current(output, pointer)
            self.assertEqual(check["status"], "PASS")


if __name__ == "__main__":
    unittest.main()
