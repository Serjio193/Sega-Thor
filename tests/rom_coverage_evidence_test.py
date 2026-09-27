from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
import struct
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1] / "tools" / "bizhawk-native-ring"
sys.path.insert(0, str(TOOLS))

from rom_coverage_evidence import (CanonicalEvidenceIndex, MASTER_POINTER_SCHEMA,
                                  POINTER_SCHEMA, resolve_knowledge_pointer)
import master_v2_shadow as master_v2


ROM_SHA = "a" * 64
ROM_SIZE = 16
MASTER_ROM_SIZE = master_v2.ROM_SIZE


def _make_generation(root: Path, generation_id: str = "gen-1") -> Path:
    generation = root / "generations" / generation_id
    generation.mkdir(parents=True)
    database = generation / "knowledge.sqlite"
    connection = sqlite3.connect(database)
    connection.executescript("""
        CREATE TABLE map_meta(key TEXT PRIMARY KEY,value TEXT NOT NULL);
        CREATE TABLE rom_range(range_id TEXT PRIMARY KEY,rom_sha256 TEXT,start INTEGER,end INTEGER);
        CREATE TABLE rom_object(object_id TEXT PRIMARY KEY,range_id TEXT,object_type TEXT,attributes_json TEXT);
        CREATE TABLE claim(claim_id TEXT PRIMARY KEY,object_id TEXT,claim_type TEXT,value_json TEXT,status TEXT);
        CREATE TABLE conflict(conflict_id TEXT PRIMARY KEY,start INTEGER,end INTEGER);
    """)
    connection.execute("INSERT INTO map_meta VALUES ('rom_sha256',?)", (ROM_SHA,))
    ranges = (("r1", 2, 10), ("r2", 6, 12), ("r3", 8, 14))
    connection.executemany("INSERT INTO rom_range VALUES (?,?,?,?)",
                           ((rid, ROM_SHA, start, end) for rid, start, end in ranges))
    objects = (("o1", "r1", "GRAPHICS_STREAM", '{"name":"waterfall"}'),
               ("o2", "r2", "ROM_DATA", "{}"),
               ("o3", "r3", "AUDIO_DATA", "{}"))
    connection.executemany("INSERT INTO rom_object VALUES (?,?,?,?)", objects)
    claims = (("c1", "o1", "ASSET_KIND", '"tile stream"', "STATIC_VERIFIED"),
              ("c2", "o2", "CONSUMER", '"menu"', "HYPOTHESIS"),
              ("c3", "o3", "AUDIO_ROLE", '"music"', "OBSERVED_RUNTIME"))
    connection.executemany("INSERT INTO claim VALUES (?,?,?,?,?)", claims)
    connection.execute("INSERT INTO conflict VALUES ('x1',9,11)")
    connection.commit()
    connection.close()
    return database


def _publish_pointer(root: Path, database: Path, generation_id: str) -> Path:
    pointer = root / "current.json"
    pointer.write_text(json.dumps({
        "schema": POINTER_SCHEMA,
        "generation_id": generation_id,
        "generation_dir": f"generations/{generation_id}",
        "knowledge_sha256": hashlib.sha256(database.read_bytes()).hexdigest(),
    }), encoding="utf-8")
    return pointer


def _make_master_v2(root: Path, generation_id: str = "master-gen-1") -> Path:
    master_path = root / "master-v2.bin"
    meta = {"schema": master_v2.SCHEMA, "generation_id": generation_id,
            "parent_generation": "master-parent", "parent_master_sha256": "1" * 64,
            "canonical_master_sha256": "2" * 64,
            "canonical_knowledge_sha256": "3" * 64,
            "rom": {"sha256": master_v2.ROM_SHA, "size": MASTER_ROM_SIZE}}
    tables = {name: [] for name in (
        "map_meta", "source_artifact", "rom_range", "rom_object", "claim",
        "relation", "evidence_ref", "emission", "conflict", "map_import",
        "derivation", "derivation_input", "map_proposal", "map_proposal_operation")}
    tables["rom_range"] = [["r1", master_v2.ROM_SHA, 2, 10],
                           ["r2", master_v2.ROM_SHA, 10, 14]]
    tables["rom_object"] = [
        ["o1", "r1", "GRAPHICS_STREAM", '{"name":"waterfall"}'],
        ["o2", "r2", "M68K_INSTRUCTION", '{"pc":10}'],
    ]
    tables["claim"] = [
        ["c1", "o1", "ASSET_KIND", '"stream"', "STATIC_VERIFIED"],
        ["c2", "o1", "RUNTIME_STATE", '"observed"', "OBSERVED"],
    ]
    locator = json.dumps({"capture_id": "capture-7", "run_id": 7,
        "epoch": 2, "cpu_id": "M68K", "event_kind": "EXECUTED_NEXT",
        "pc": 8, "native_sequence": 101, "occurrence_id": "occ-7-101"})
    tables["relation"] = [["rel-1", "EXECUTED_NEXT", "o1", "o2", 10,
                           "OBSERVED_RUNTIME", "{}"]]
    tables["source_artifact"] = [["d" * 64, "M14-TRACE-TEST",
                                   "capture-7.json", "NORMALIZED_RUNTIME"]]
    tables["evidence_ref"] = [["ev-1", "RELATION", "rel-1", "d" * 64,
                                "RUNTIME_NEXT_OCCURRENCE", 1, locator]]
    tables["derivation"] = [["der-1", "asset-extract", "v1", "e" * 64,
        "f" * 64, "EXTRACTED_ASSET", "asset-1",
        json.dumps({"path": "assets/waterfall.png", "sha256": "c" * 64}), "[]"]]
    tables["derivation_input"] = [["der-1", 0, "RELATION", "rel-1", "source"]]
    tables["emission"] = [[2, 10, "ASSET", "ASSET_EXACT", "ROM", 0,
                           "resource", "assets/waterfall.png"]]
    tables["conflict"] = [["x1", 9, 11, "CLAIM_CONFLICT", "{}"]]
    payloads = {"meta": master_v2._json_bytes(meta),
                "canonical_knowledge": master_v2._json_bytes({"tables": tables})}
    records = []
    header = master_v2._json_bytes({"schema": master_v2.SCHEMA,
                                    "sections": master_v2.SECTION_NAMES})
    body = bytearray(master_v2.MAGIC + struct.pack("<Q", len(header)) + header)
    for name in master_v2.SECTION_NAMES:
        data = payloads.get(name, b"{}")
        digest = hashlib.sha256(data).hexdigest()
        encoded_name = name.encode("ascii")
        body.extend(struct.pack("<H", len(encoded_name)) + encoded_name)
        body.extend(struct.pack("<Q", len(data)) + bytes.fromhex(digest) + data)
        records.append({"name": name, "bytes": len(data), "sha256": digest})
    logical_hash = hashlib.sha256(master_v2._json_bytes(
        {"schema": master_v2.SCHEMA, "sections": records})).hexdigest()
    footer = master_v2._json_bytes({"sections": records,
                                    "overall_sha256": logical_hash})
    body.extend(master_v2.FOOTER_MAGIC + struct.pack("<Q", len(footer)) + footer)
    master_path.write_bytes(body)
    pointer = {
        "schema": MASTER_POINTER_SCHEMA, "generation_id": generation_id,
        "path": master_path.name, "sha256": hashlib.sha256(body).hexdigest(),
        "bytes": len(body), "logical_sha256": logical_hash,
        "rom_sha256": master_v2.ROM_SHA, "rom_size": MASTER_ROM_SIZE,
        "master_schema": master_v2.SCHEMA,
        "sections": {record["name"]: record["sha256"] for record in records},
    }
    (root / "current.json").write_text(json.dumps(pointer), encoding="utf-8")
    return root / "current.json"


class CanonicalEvidenceIndexTests(unittest.TestCase):
    def test_default_pointer_prefers_promoted_master_and_keeps_override(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            master = root / "build/thor-evidence/master-v2/current.json"
            m14 = root / "build/thor-evidence/archivist-knowledge-pipeline-2g-canonical-verified-20260918/current.json"
            m14.parent.mkdir(parents=True)
            m14.write_text("{}", encoding="utf-8")
            self.assertEqual(resolve_knowledge_pointer(root), m14)
            master.parent.mkdir(parents=True)
            master.write_text("{}", encoding="utf-8")
            self.assertEqual(resolve_knowledge_pointer(root), master)
            explicit = root / "custom-current.json"
            self.assertEqual(resolve_knowledge_pointer(root, explicit), explicit)

    def test_range_summary_unions_each_status_and_preserves_overlap(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            database = _make_generation(root)
            pointer = _publish_pointer(root, database, "gen-1")
            before = hashlib.sha256(database.read_bytes()).hexdigest()
            index = CanonicalEvidenceIndex.load(pointer, ROM_SHA, ROM_SIZE)

            summary = index.summarize(0, ROM_SIZE)
            self.assertEqual(summary.state, "CONFLICT")
            self.assertEqual(summary.counts["STATIC_VERIFIED"], 8)
            self.assertEqual(summary.counts["HYPOTHESIS"], 6)
            self.assertEqual(summary.counts["OBSERVED_RUNTIME"], 6)
            self.assertEqual(summary.counts["CONFLICT"], 2)
            self.assertGreater(sum(summary.counts.values()), ROM_SIZE)
            self.assertTrue(any("GRAPHICS_STREAM: waterfall" in fact
                                for fact in summary.facts))
            self.assertTrue(any("no linked, evidence-backed execution edge" in line
                                for line in summary.trace))
            self.assertTrue(any("no linked derivation" in line for line in summary.trace))
            self.assertEqual(hashlib.sha256(database.read_bytes()).hexdigest(), before)

    def test_redraw_state_query_matches_details_without_iterating_facts(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            pointer = _make_master_v2(Path(directory))
            index = CanonicalEvidenceIndex.load(pointer, master_v2.ROM_SHA,
                                                MASTER_ROM_SIZE)
            expected = {
                (0, 2): "NONE",
                (2, 8): "MIXED",
                (8, 10): "CONFLICT",
                (10, 11): "CONFLICT",
                (11, 12): "NONE",
            }
            for (start, end), state in expected.items():
                self.assertEqual(index.state_for_range(start, end), state)
                self.assertEqual(index.state_for_range(start, end),
                                 index.summarize(start, end).state)

            class FactsMustNotBeScanned:
                def __iter__(self):
                    raise AssertionError("redraw query iterated canonical facts")

            index.facts = FactsMustNotBeScanned()
            self.assertEqual(index.state_for_range(2, 8), "MIXED")

    def test_mismatched_rom_identity_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pointer = _publish_pointer(root, _make_generation(root), "gen-1")
            with self.assertRaisesRegex(ValueError, "ROM SHA-256 mismatch"):
                CanonicalEvidenceIndex.load(pointer, "b" * 64, ROM_SIZE)

    def test_pointer_rejects_generation_escape(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            parent = Path(directory)
            root = parent / "map"
            root.mkdir()
            external = parent / "outside"
            database = _make_generation(external)
            pointer = root / "current.json"
            pointer.write_text(json.dumps({
                "schema": POINTER_SCHEMA,
                "generation_id": "gen-1",
                "generation_dir": "../outside/generations/gen-1",
                "knowledge_sha256": hashlib.sha256(database.read_bytes()).hexdigest(),
            }), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "escapes"):
                CanonicalEvidenceIndex.load(pointer, ROM_SHA, ROM_SIZE)

    def test_pointer_hash_must_match_database(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            database = _make_generation(root)
            pointer = _publish_pointer(root, database, "gen-1")
            payload = json.loads(pointer.read_text(encoding="utf-8"))
            payload["knowledge_sha256"] = "0" * 64
            pointer.write_text(json.dumps(payload), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "database hash"):
                CanonicalEvidenceIndex.load(pointer, ROM_SHA, ROM_SIZE)

    def test_new_pointer_generation_is_read_as_new_accumulated_map(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            first = _make_generation(root, "gen-1")
            pointer = _publish_pointer(root, first, "gen-1")
            first_index = CanonicalEvidenceIndex.load(pointer, ROM_SHA, ROM_SIZE)
            second = _make_generation(root, "gen-2")
            _publish_pointer(root, second, "gen-2")
            second_index = CanonicalEvidenceIndex.load(pointer, ROM_SHA, ROM_SIZE)
            self.assertEqual(first_index.generation_id, "gen-1")
            self.assertEqual(second_index.generation_id, "gen-2")

    def test_master_v2_startup_pointer_loads_verified_range_claims(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            pointer = _make_master_v2(Path(directory))
            index = CanonicalEvidenceIndex.load(pointer, master_v2.ROM_SHA,
                                                MASTER_ROM_SIZE)
            summary = index.summarize(0, 8)
            hover = index.summarize(0, 8, include_trace=False)
            self.assertEqual(index.authority, "MASTER V2 STARTUP")
            self.assertEqual(index.generation_id, "master-gen-1")
            self.assertEqual(summary.counts["STATIC_VERIFIED"], 6)
            self.assertEqual(summary.counts["OTHER_CLAIM"], 6)
            self.assertEqual(summary.state, "MIXED")
            self.assertTrue(any("OBSERVED" in fact for fact in summary.facts))
            self.assertTrue(any("EXECUTED_NEXT" in line for line in summary.trace))
            self.assertTrue(any("runtime witness" in line and "pc=0x8" in line
                                for line in summary.trace))
            self.assertTrue(any("RUNTIME PATH SEGMENT" in line and "o1" in line and
                                "o2" in line for line in summary.trace))
            self.assertTrue(any("RECORDED EXTRACTION OUTPUT REFERENCE" in line and
                                "assets/waterfall.png" in line and "c" * 64 in line
                                for line in summary.trace))
            self.assertTrue(any("CANONICAL EMISSION" in line
                                for line in summary.trace))
            self.assertTrue(any("INPUT RELATION:rel-1 role=source" in line
                                for line in summary.trace))
            self.assertEqual(hover.trace, ())

    def test_master_v2_rejects_wrong_rom_and_path_escape(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            outer = Path(directory)
            root = outer / "map"
            root.mkdir()
            pointer = _make_master_v2(root)
            with self.assertRaisesRegex(ValueError, "ROM identity mismatch"):
                CanonicalEvidenceIndex.load(pointer, "b" * 64, MASTER_ROM_SIZE)

            outside = outer / "outside"
            outside.mkdir()
            external_pointer = _make_master_v2(outside)
            external_value = json.loads(external_pointer.read_text(encoding="utf-8"))
            escaped_root = root / "nested"
            escaped_root.mkdir()
            escaped_value = dict(external_value, path="../../outside/" +
                                 str(external_value["path"]))
            escaped_pointer = escaped_root / "current.json"
            escaped_pointer.write_text(json.dumps(escaped_value), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "escapes pointer directory"):
                CanonicalEvidenceIndex.load(escaped_pointer, master_v2.ROM_SHA,
                                            MASTER_ROM_SIZE)

    def test_master_v2_rejects_pointer_section_hash_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            pointer = _make_master_v2(Path(directory))
            value = json.loads(pointer.read_text(encoding="utf-8"))
            value["sections"]["canonical_knowledge"] = "0" * 64
            pointer.write_text(json.dumps(value), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "section hashes mismatch"):
                CanonicalEvidenceIndex.load(pointer, master_v2.ROM_SHA,
                                            MASTER_ROM_SIZE)


if __name__ == "__main__":
    unittest.main()
