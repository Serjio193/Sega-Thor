"""R3 MASTER V2 provenance, outcomes and absorption authority tests."""
from __future__ import annotations

import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
import master_outcome_view as outcome  # noqa: E402
import master_v2_shadow as v2  # noqa: E402
from rom_knowledge_map import KnowledgeStore  # noqa: E402


def db(path: Path, schema: dict[str, str], rows: dict[str, list[tuple]]) -> None:
    conn = sqlite3.connect(path)
    for table, definition in schema.items():
        conn.execute(f"CREATE TABLE {table}({definition})")
        for row in rows.get(table, []):
            conn.execute(f"INSERT INTO {table} VALUES ({','.join('?' for _ in row)})", row)
    conn.commit(); conn.close()


class OutcomeAuthorityTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory(); root = Path(self.temp.name)
        self.roll = root / "rolling"; self.canon = root / "canonical"; self.campaign = root / "campaign"
        rg = self.roll / "generations" / "master-1"; cg = self.canon / "generations" / "gen-1"
        rg.mkdir(parents=True); cg.mkdir(parents=True)
        (self.campaign / "post-run-analysis").mkdir(parents=True)
        db(rg / "master.sqlite", {
            "meta": "key TEXT PRIMARY KEY,value TEXT", "run": "run_id INTEGER PRIMARY KEY,status TEXT",
            "audit": "run_id INTEGER PRIMARY KEY,records INTEGER", "instruction": "pc INTEGER,opcode INTEGER,occurrences INTEGER,PRIMARY KEY(pc,opcode)",
            "edge": "source_pc INTEGER,relation TEXT,target_pc INTEGER,PRIMARY KEY(source_pc,relation,target_pc)"},
           {"meta": [("rom_sha256", v2.ROM_SHA)], "run": [(7, "STOPPED_END_GAME")],
            "audit": [(7, 1)], "instruction": [(0, 0x4E75, 1)], "edge": []})
        db(cg / "master.sqlite", {name: f"id TEXT PRIMARY KEY" for name in
            ("map_meta", "map_import", "map_node", "map_edge", "map_frontier", "map_conflict")}, {})
        KnowledgeStore(cg / "knowledge.sqlite", v2.ROM_SHA, v2.ROM_SIZE).close()
        (self.roll / "current.json").write_text(json.dumps({
            "generation_dir": "generations/master-1", "master_sha256": v2._sha(rg / "master.sqlite")}), encoding="utf-8")
        (self.canon / "current.json").write_text(json.dumps({
            "generation_dir": "generations/gen-1", "master_sha256": v2._sha(cg / "master.sqlite"),
            "knowledge_sha256": v2._sha(cg / "knowledge.sqlite")}), encoding="utf-8")
        post = self.campaign / "post-run-analysis"
        (post / "report.json").write_text(json.dumps({"run_id": 7, "pipeline_state": "ANALYSIS COMPLETE ✓"}), encoding="utf-8")
        (post / "absorbed-run-7.json").write_text(json.dumps({"run_id": 7, "status": "PASS_ABSORBED_RAW_PERMANENT_RECLAIM_V1"}), encoding="utf-8")
        self.state = v2.LegacyState(self.roll, self.canon, self.campaign, 7,
                                     json.loads((self.roll / "current.json").read_text()),
                                     json.loads((self.canon / "current.json").read_text()), rg, cg,
                                     self.roll / "post-run-analysis" / "run-7")
        self.state.run_analysis.mkdir(parents=True)
        (self.state.run_analysis / "stage6-control-provenance.json").write_text(json.dumps({
            "run_id": 7, "status": "NO_DELTA", "raw_flow_sha256": "flow",
            "segments_processed": 1, "segments_total": 1}), encoding="utf-8")

    def tearDown(self) -> None:
        self.temp.cleanup()

    def test_projection_round_trip_and_read_authority(self) -> None:
        path = Path(self.temp.name) / "master-v2"
        v2.write_master_v2(self.state, path)
        checked = outcome.verify_projection(self.state, path)
        self.assertEqual(checked["status"], "PASS")
        view = outcome.open_outcome_authority(path)
        self.assertEqual(view.run_id, 7)
        self.assertEqual(view.provenance.status, "NO_DELTA")
        self.assertEqual(view.require_stage("STAGE_9")["status"],
                         "PASS_ABSORBED_RAW_PERMANENT_RECLAIM_V1")

    def test_corrupt_or_missing_required_outcome_fails_closed(self) -> None:
        path = Path(self.temp.name) / "master-v2"
        v2.write_master_v2(self.state, path)
        data = bytearray(path.read_bytes()); marker = b'"outcomes"'
        offset = data.find(marker)
        self.assertGreaterEqual(offset, 0)
        data[offset + len(marker) + 20] ^= 1; path.write_bytes(data)
        with self.assertRaisesRegex(ValueError, outcome.STOP_UNAVAILABLE):
            outcome.open_outcome_authority(path)

    def test_missing_required_section_has_no_legacy_fallback(self) -> None:
        path = Path(self.temp.name) / "master-v2"
        v2.write_master_v2(self.state, path)
        data = path.read_bytes().replace(b'"outcomes"', b'"xutcomes"', 1)
        path.write_bytes(data)
        with self.assertRaisesRegex(ValueError, outcome.STOP_UNAVAILABLE):
            outcome.open_outcome_authority(path)


if __name__ == "__main__":
    unittest.main(verbosity=2)
