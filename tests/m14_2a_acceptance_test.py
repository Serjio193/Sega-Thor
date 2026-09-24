"""Focused additive-schema, identity, conflict, and proposal acceptance."""
from __future__ import annotations

import hashlib
import shutil
import sqlite3
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / "src" / "tools" / "thor_evidence"
sys.path.insert(0, str(EVIDENCE))
from cartographer import Cartographer  # noqa: E402
from rom_knowledge_map import (  # noqa: E402
    LEGACY_SCHEMA, SCHEMA, SCHEMA_SQL, KnowledgeStore, canonical,
    range_id, object_id, runtime_occurrence_id,
)

ROM = "a" * 64
SIZE = 16


def make_store(path: Path, generation: str = "child") -> KnowledgeStore:
    store = KnowledgeStore(path, ROM, SIZE)
    store.db.execute("INSERT INTO emission VALUES (?,?,?,?,?,?,?,?)",
                     (0, SIZE, "INCBIN", "UNKNOWN", "UNKNOWN", 0, "blob", "fixture"))
    store.set_generation_identity(generation, "parent")
    store.db.commit()
    return store


class M142Acceptance(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def test_additive_migration_fresh_and_copied_generation_preserves_emission(self) -> None:
        source = self.root / "legacy.sqlite"
        store = make_store(source)
        emission_before = store.hashes()["emission_hash"]
        for table in ("map_proposal_operation", "map_proposal", "derivation_input", "derivation"):
            store.db.execute(f"DROP TABLE {table}")
        store.db.execute("UPDATE map_meta SET value=? WHERE key='schema'", (LEGACY_SCHEMA,))
        store.db.commit(); store.close()
        copied = self.root / "copied.sqlite"
        shutil.copy2(source, copied)
        migrated = KnowledgeStore(copied, ROM, SIZE)
        try:
            self.assertEqual(migrated.meta()["schema"], SCHEMA)
            self.assertEqual(migrated.hashes()["emission_hash"], emission_before)
            self.assertEqual(migrated.counts()["map_proposal"], 0)
            self.assertTrue(all(migrated.db.execute(
                "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?", (table,)).fetchone()
                for table in ("derivation", "derivation_input", "map_proposal", "map_proposal_operation")))
        finally:
            migrated.close()
        fresh = KnowledgeStore(self.root / "fresh.sqlite", ROM, SIZE)
        try:
            self.assertEqual(fresh.meta()["schema"], SCHEMA)
        finally:
            fresh.close()

    def _admit(self, path: Path, order: tuple[str, ...]) -> tuple[str, str, str]:
        store = make_store(path)
        try:
            start = store.hashes()
            rid = range_id(ROM, 0, 2)
            oid = object_id(ROM, 0, 2, "M68K_INSTRUCTION")
            store.insert_rows("rom_range", [{"range_id": rid, "rom_sha256": ROM,
                "start": 0, "end": 2}])
            store.insert_rows("rom_object", [{"object_id": oid, "range_id": rid,
                "object_type": "M68K_INSTRUCTION", "attributes_json": canonical({"pc": 0})}])
            store.insert_rows("claim", [{"claim_id": "static-claim", "object_id": oid,
                "claim_type": "EXECUTED_FROM_ROM", "value_json": canonical(True),
                "status": "OBSERVED_RUNTIME"}])
            for capture in order:
                occurrence = runtime_occurrence_id(capture_id=capture, epoch=1,
                    cpu="M68K", address_space="ROM", native_sequence=9,
                    event_kind="INSTRUCTION")
                store.insert_rows("source_artifact", [{"source_sha256": hashlib.sha256(
                    capture.encode()).hexdigest(), "checkpoint": "fixture",
                    "artifact_name": capture, "artifact_type": "capture"}])
                store.insert_rows("evidence_ref", [{"ref_id": occurrence,
                    "subject_type": "rom_object", "subject_id": oid,
                    "source_sha256": hashlib.sha256(capture.encode()).hexdigest(),
                    "fact_kind": "RUNTIME_INSTRUCTION_OCCURRENCE", "fact_count": 1,
                    "locator_json": canonical({"capture_id": capture, "epoch": 1,
                        "cpu": "M68K", "address_space": "ROM", "native_sequence": 9,
                    "event_kind": "INSTRUCTION"})}])
            store.db.commit()
            graph_only = store.hashes()
            self.assertEqual(start["emission_hash"], graph_only["emission_hash"])
            self.assertEqual(store.counts()["rom_object"], 1)
            self.assertEqual(store.counts()["evidence_ref"], 2)
            duplicate_id = runtime_occurrence_id(capture_id="capture-a", epoch=1,
                cpu="M68K", address_space="ROM", native_sequence=9,
                event_kind="INSTRUCTION")
            duplicate_sha = hashlib.sha256(b"capture-a").hexdigest()
            store.insert_rows("evidence_ref", [{"ref_id": duplicate_id,
                "subject_type": "rom_object", "subject_id": oid,
                "source_sha256": duplicate_sha,
                "fact_kind": "RUNTIME_INSTRUCTION_OCCURRENCE", "fact_count": 1,
                "locator_json": canonical({"capture_id": "capture-a", "epoch": 1,
                    "cpu": "M68K", "address_space": "ROM", "native_sequence": 9,
                    "event_kind": "INSTRUCTION"})}])
            self.assertEqual(store.counts()["evidence_ref"], 2)
            store.insert_rows("relation", [{"relation_id": "relation-fixture",
                "relation_type": "OBSERVED_NEXT_PC", "source_object_id": oid,
                "target_object_id": None, "target_address": 4,
                "status": "OBSERVED_RUNTIME", "attributes_json": "{}"}])
            relation_hashes = store.hashes()
            self.assertEqual(relation_hashes["emission_hash"], start["emission_hash"])
            # Exact duplicate insertions are ignored and retain the logical hash.
            before = relation_hashes.copy()
            store.insert_rows("rom_object", [{"object_id": oid, "range_id": rid,
                "object_type": "M68K_INSTRUCTION", "attributes_json": canonical({"pc": 0})}])
            self.assertEqual(store.hashes(), before)
            graph_hash = relation_hashes["graph_structure_hash"]
            proposal_hash = store.create_map_proposal("proposal", "child",
                relation_hashes["map_hash"], "f" * 64, "validator-v1",
                [{"kind": "annotate", "object_id": oid}])
            proposed = store.hashes()
            self.assertNotEqual(proposal_hash, "")
            self.assertEqual(proposed["graph_structure_hash"], graph_hash)
            self.assertEqual(proposed["emission_hash"], start["emission_hash"])
            store.validate_map_proposal_parent("proposal", "child")
            with self.assertRaisesRegex(ValueError, "STOP_MAP_PROPOSAL_STALE_PARENT"):
                store.validate_map_proposal_parent("proposal", "old-parent")
            return oid, graph_hash, proposed["map_hash"]
        finally:
            store.close()

    def test_scoped_cpu_identity_and_capture_order_independence(self) -> None:
        a = runtime_occurrence_id(capture_id="c", epoch=1, cpu="M68K",
            address_space="ROM", native_sequence=4, event_kind="INSTRUCTION")
        b = runtime_occurrence_id(capture_id="c", epoch=1, cpu="Z80",
            address_space="Z80_BUS", native_sequence=4, event_kind="INSTRUCTION")
        self.assertNotEqual(a, b)
        forward = self._admit(self.root / "ab.sqlite", ("capture-a", "capture-b"))
        reverse = self._admit(self.root / "ba.sqlite", ("capture-b", "capture-a"))
        self.assertEqual(forward, reverse)

    def test_conflicting_proven_node_stays_conflict_after_reopen(self) -> None:
        path = self.root / "cartographer.sqlite"
        graph = Cartographer(path, ROM)
        seed = {"nodes": [{"kind": "M68K_INSTRUCTION", "key": "pc:0", "scope": ROM,
            "status": "PROVEN", "attributes": {"opcode": 1}, "lineage": ["capture-a"]}],
            "edges": [], "frontiers": []}
        graph.merge(seed, "seed", "1" * 64)
        conflict = {"nodes": [{"kind": "M68K_INSTRUCTION", "key": "pc:0", "scope": ROM,
            "status": "PROVEN", "attributes": {"opcode": 2}, "lineage": ["capture-b"]}],
            "edges": [], "frontiers": []}
        result = graph.merge(conflict, "conflict", "2" * 64)
        self.assertEqual(result.new_conflicts, 1)
        row = graph.db.execute("SELECT status FROM map_node").fetchone()
        self.assertEqual(row[0], "CONFLICT")
        self.assertEqual(graph.db.execute("SELECT COUNT(*) FROM map_conflict").fetchone()[0], 1)
        graph.close()
        reopened = Cartographer(path, ROM)
        try:
            self.assertEqual(reopened.db.execute("SELECT status FROM map_node").fetchone()[0], "CONFLICT")
            self.assertEqual(reopened.db.execute("SELECT COUNT(*) FROM map_conflict").fetchone()[0], 1)
        finally:
            reopened.close()


if __name__ == "__main__":
    unittest.main(verbosity=2)
