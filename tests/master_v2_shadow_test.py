"""MASTER V2 shadow container and legacy round-trip contract."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
import master_v2_shadow as v2  # noqa: E402
from rom_knowledge_map import KnowledgeStore  # noqa: E402
from cartographer import Cartographer  # noqa: E402
import master_canonical_view as canonical_view  # noqa: E402


def make_db(path: Path, tables: dict[str, str], rows: dict[str, list[tuple]]) -> None:
    db = sqlite3.connect(path)
    for table, schema in tables.items():
        db.execute(f"CREATE TABLE {table}({schema})")
        for row in rows.get(table, []):
            db.execute(f"INSERT INTO {table} VALUES ({','.join('?' for _ in row)})", row)
    db.commit(); db.close()


class MasterV2Tests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(); root = Path(self.temp.name)
        self.roll = root / "rolling"; self.canon = root / "canonical"; self.campaign = root / "campaign"
        rg = self.roll / "generations" / "master-1"; cg = self.canon / "generations" / "gen-1"
        (rg).mkdir(parents=True); (cg).mkdir(parents=True); (self.campaign / "post-run-analysis").mkdir(parents=True)
        make_db(rg / "master.sqlite", {
            "meta": "key TEXT PRIMARY KEY,value TEXT", "run": "run_id INTEGER PRIMARY KEY,status TEXT",
            "audit": "run_id INTEGER PRIMARY KEY,records INTEGER", "instruction": "pc INTEGER,opcode INTEGER,occurrences INTEGER,PRIMARY KEY(pc,opcode)",
            "edge": "source_pc INTEGER,relation TEXT,target_pc INTEGER,PRIMARY KEY(source_pc,relation,target_pc)"},
            {"meta": [("rom_sha256", v2.ROM_SHA)], "run": [(7, "STOPPED_END_GAME")], "audit": [(7, 1)],
             "instruction": [(0, 0x4E75, 1)], "edge": [(0, "OBSERVED_NEXT_PC", 4)]})
        make_db(cg / "master.sqlite", canonical_view.MASTER_SCHEMA,
            {"map_meta": [("source_owned_bytes", "0")],
             "map_import": [("import-1", "e" * 64, "{}")],
             "map_node": [("n", "M68K_INSTRUCTION", "pc:0", v2.ROM_SHA,
                           "PROVEN", "{}", "[]")],
             "map_edge": [("e", "n", "n", "OBSERVED_NEXT_PC", v2.ROM_SHA,
                           "PROVEN", "{}", "[]")]})
        graph = Cartographer(cg / "master.sqlite", v2.ROM_SHA)
        try:
            for opcode, import_ref in ((0x4E75, "conflict-seed"), (0x4E71, "conflict-second")):
                graph.merge({"nodes": [{"kind": "M68K_INSTRUCTION", "key": "conflict-pc",
                    "scope": v2.ROM_SHA, "status": "PROVEN", "attributes": {"opcode": opcode},
                    "lineage": [import_ref]}], "edges": [], "frontiers": []},
                    import_ref, hashlib.sha256(import_ref.encode()).hexdigest())
            self.assertEqual(graph.db.execute(
                "SELECT status FROM map_node WHERE node_key='conflict-pc'").fetchone()[0], "CONFLICT")
        finally:
            graph.close()
        knowledge_path = cg / "knowledge.sqlite"
        knowledge = KnowledgeStore(knowledge_path, v2.ROM_SHA, v2.ROM_SIZE)
        try:
            knowledge.set_generation_identity("gen-1", "master-parent")
            range_key = "range-fixture"
            object_key = "object-1"
            knowledge.insert_rows("rom_range", [{"range_id": range_key, "rom_sha256": v2.ROM_SHA,
                "start": 8, "end": 12}])
            knowledge.insert_rows("rom_object", [{"object_id": object_key, "range_id": range_key,
                "object_type": "ROM_DATA", "attributes_json": "{}"}])
            knowledge.insert_rows("claim", [{"claim_id": "claim-fixture", "object_id": object_key,
                "claim_type": "FIXTURE_FACT", "value_json": "true", "status": "OBSERVED_RUNTIME"}])
            knowledge.insert_rows("relation", [{"relation_id": "relation-fixture",
                "relation_type": "FIXTURE_LINK", "source_object_id": object_key,
                "target_object_id": object_key, "target_address": None,
                "status": "OBSERVED_RUNTIME", "attributes_json": "{}"}])
            knowledge.insert_rows("conflict", [{"conflict_id": "conflict-fixture", "start": 8,
                "end": 12, "conflict_type": "FIXTURE_CONFLICT", "detail_json": "{}"}])
            knowledge.db.execute("INSERT INTO emission VALUES (?,?,?,?,?,?,?,?)",
                (0, 4, "INCBIN", "UNKNOWN", "UNKNOWN", 0, "blob", "fixture.bin"))
            source = "a" * 64
            knowledge.db.execute("INSERT INTO source_artifact VALUES (?,?,?,?)",
                (source, "fixture", "fixture.json", "json"))
            knowledge.insert_rows("evidence_ref", [{"ref_id": "evidence-fixture",
                "subject_type": "rom_object", "subject_id": object_key, "source_sha256": source,
                "fact_kind": "FIXTURE_FACT", "fact_count": 1, "locator_json": "{}"}])
            knowledge.insert_rows("map_import", [{"import_key": "fixture-import", "input_hash": "f" * 64}])
            derivation = knowledge.record_derivation("fixture-rule", "1", "b" * 64,
                "c" * 64, [{"subject_type": "rom_object", "subject_id": "object-1", "role": "input"}],
                "claim", "claim-1", {"exact": True})
            knowledge.create_map_proposal("proposal-1", "gen-1", knowledge.hashes()["map_hash"],
                "d" * 64, "validator-1", [{"operation": "annotate", "target": "object-1"}])
            knowledge.db.commit()
        finally:
            knowledge.close()
        (self.roll / "current.json").write_text(json.dumps({"generation_dir": "generations/master-1", "master_sha256": v2._sha(rg / "master.sqlite")}), encoding="utf-8")
        (self.canon / "current.json").write_text(json.dumps({"generation_dir": "generations/gen-1", "master_sha256": v2._sha(cg / "master.sqlite"), "knowledge_sha256": v2._sha(cg / "knowledge.sqlite")}), encoding="utf-8")
        post = self.campaign / "post-run-analysis"
        (post / "report.json").write_text(json.dumps({"run_id": 7, "source_owned_delta": 0, "stage_results": {"CLEANUP": {"status": "PASS"}}}), encoding="utf-8")
        (post / "status.json").write_text(json.dumps({"stage": "CLEANUP", "stage_index": 9}), encoding="utf-8")
        (post / "absorbed-run-7.json").write_text(json.dumps({"run_id": 7, "status": "PASS_ABSORBED_RAW_PERMANENT_RECLAIM_V1"}), encoding="utf-8")
        self.state = v2.LegacyState(self.roll, self.canon, self.campaign, 7,
                                     json.loads((self.roll / "current.json").read_text()),
                                     json.loads((self.canon / "current.json").read_text()), rg, cg,
                                     self.roll / "post-run-analysis" / "run-7-test")
        self.state.run_analysis.mkdir(parents=True)

    def tearDown(self) -> None:
        self.temp.cleanup()

    def test_a_b_e_g_h_i_round_trip_and_determinism(self) -> None:
        first = Path(self.temp.name) / "one.master-v2"; second = Path(self.temp.name) / "two.master-v2"
        a = v2.write_master_v2(self.state, first); b = v2.write_master_v2(self.state, second)
        self.assertEqual(first.read_bytes(), second.read_bytes()); self.assertEqual(a["bytes"], b["bytes"])
        decoded = v2.decode_master_v2(first, materialize=True)
        self.assertEqual(decoded["overall_sha256"], a["overall_sha256"])
        self.assertEqual({x["name"] for x in decoded["sections"]}, set(v2.SECTION_NAMES))
        self.assertEqual(v2.legacy_section_hashes(self.state, v2._meta_payload(self.state, v2._file_manifest(self.state), a["generation_id"])),
                         {x["name"]: {"sha256": x["sha256"], "bytes": x["bytes"]} for x in a["sections"]})
        table_rows = decoded["payloads"]["canonical_knowledge"]["tables"]
        self.assertEqual(len(table_rows["derivation"]), 1)
        self.assertEqual(len(table_rows["derivation_input"]), 1)
        self.assertEqual(len(table_rows["map_proposal"]), 1)
        self.assertEqual(len(table_rows["map_proposal_operation"]), 1)
        for table in ("rom_range", "rom_object", "claim", "relation", "source_artifact",
                      "evidence_ref", "conflict", "map_import", "emission"):
            self.assertTrue(table_rows[table], f"{table} must survive the MASTER V2 round trip")
        materialized = canonical_view.MasterCanonicalView(first).materialize(
            Path(self.temp.name) / "roundtrip")
        graph_db = sqlite3.connect(materialized / "master.sqlite")
        self.assertEqual(graph_db.execute(
            "SELECT status FROM map_node WHERE node_key='conflict-pc'").fetchone()[0], "CONFLICT")
        self.assertEqual(graph_db.execute("SELECT COUNT(*) FROM map_conflict").fetchone()[0], 1)
        graph_db.close()
        reopened = KnowledgeStore(materialized / "knowledge.sqlite", v2.ROM_SHA,
                                  v2.ROM_SIZE, read_only=True)
        try:
            self.assertEqual(reopened.hashes(), decoded["payloads"]["meta"]["canonical_logical_hashes"])
            self.assertEqual(len(reopened.db.execute("SELECT * FROM derivation").fetchall()), 1)
            self.assertEqual(len(reopened.db.execute("SELECT * FROM derivation_input").fetchall()), 1)
            self.assertEqual(len(reopened.db.execute("SELECT * FROM map_proposal").fetchall()), 1)
            self.assertEqual(len(reopened.db.execute("SELECT * FROM map_proposal_operation").fetchall()), 1)
            for table in ("rom_range", "rom_object", "claim", "relation", "source_artifact",
                          "evidence_ref", "conflict", "map_import", "emission"):
                self.assertEqual(reopened.db.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0], 1)
        finally:
            reopened.close()

    def test_c_corruption_and_d_missing_section_rejected(self) -> None:
        path = Path(self.temp.name) / "master-v2"; v2.write_master_v2(self.state, path)
        data = bytearray(path.read_bytes()); data[-1] ^= 1; path.write_bytes(data)
        with self.assertRaises(ValueError): v2.decode_master_v2(path)
        path.unlink(); v2.write_master_v2(self.state, path)
        data = path.read_bytes().replace(b"canonical_knowledge", b"missing_knowledge", 1); path.write_bytes(data)
        with self.assertRaises(ValueError): v2.decode_master_v2(path)

    def test_f_source_owned_emission_and_legacy_authority_unchanged(self) -> None:
        before = {(p, v2._sha(p)) for p in (self.roll / "current.json", self.canon / "current.json")}
        path = Path(self.temp.name) / "master-v2"; result = v2.write_master_v2(self.state, path)
        self.assertEqual(result["legacy_persistent_bytes"], sum(x["bytes"] for x in v2._file_manifest(self.state)))
        self.assertEqual(before, {(p, v2._sha(p)) for p, _ in before})


if __name__ == "__main__":
    unittest.main(verbosity=2)
