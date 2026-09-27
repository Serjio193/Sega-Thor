"""Safety contract for Stage 9 absorbed-run permanent reclaim."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
import struct
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
sys.path.insert(0, str(ROOT / "src" / "tools"))

import live_forward_absorption_cleanup as cleanup  # noqa: E402


class AbsorptionFixture:
    def __init__(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.campaign = self.root / "campaign"
        self.post = self.campaign / "post-run-analysis"
        self.master_root = self.root / "rolling-master"
        self.run_analysis = self.master_root / "post-run-analysis" / "run-7-test"
        self.canonical_root = self.root / "canonical"
        self.canonical = self.canonical_root / "generations" / "gen-1"
        for path in (self.post, self.run_analysis / "stage7", self.canonical,
                     self.campaign / "continuous-runtime-evidence", self.campaign / "rom-link" / "count-1"):
            path.mkdir(parents=True, exist_ok=True)
        self.rom = self.campaign / "rom.bin"
        self.rom.write_bytes(b"\x4e\x75\x60\x00")
        self.patcher = mock.patch.multiple(cleanup, ROM_SHA=hashlib.sha256(self.rom.read_bytes()).hexdigest(), ROM_SIZE=4)
        self.patcher.start()
        record = struct.pack("<QQQIIIHBBHHI", 0, 0, 0, 0, 4, 0x4E75, 1, 0, 2, 0, 0, 0)
        self.raw = self.campaign / "continuous-runtime-evidence" / "flow-v1-records.bin"
        self.index = self.campaign / "continuous-runtime-evidence" / "flow-v1-segments.jsonl"
        self.raw.write_bytes(record)
        segment = {"run_id": 7, "epoch": 1, "worker_id": 0, "capture_id": 1, "generation": 1,
                   "entry_stream_sequence": 1, "exit_stream_sequence": 1, "record_count": 1}
        self.index.write_text(json.dumps({"segment": segment, "raw_offset": 0, "raw_length": len(record),
                                          "raw_sha256": hashlib.sha256(record).hexdigest()}) + "\n", encoding="utf-8")
        self.receipt = self.campaign / "live-worker-interactive-receipt.json"
        self.receipt.write_text(json.dumps({"runtime": {"run_id": 7, "rom_sha256": cleanup.ROM_SHA,
            "audited_segments": 1}, "raw_segment_spool": {"raw_path": str(self.raw),
            "index_path": str(self.index), "segments": 1}}), encoding="utf-8")
        for name in ("report.json", "status.json"):
            (self.post / name).write_text("{}", encoding="utf-8")
        (self.campaign / "rom-link" / "count-1" / "receipt.json").write_text("{}", encoding="utf-8")
        (self.campaign / "rom-link" / "count-1" / "raw.log").write_bytes(b"redundant")
        self._make_canonical()
        self._make_master()
        (self.run_analysis / "stage6-control-provenance.json").write_text("{}", encoding="utf-8")
        (self.run_analysis / "stage7" / "stage7-result.json").write_text("{}", encoding="utf-8")
        self.stage_results = {
            "REFRESHING MAP": {"state": "PASS", "generation_dir": str(self.canonical), "source_owned_after": 0},
            "CONTROL PROVENANCE": {"status": "NO_DELTA"},
            "GENERIC RECURSIVE CLOSURE": {"state": "NO_DELTA",
                "deterministic_replay_status": "PASS", "gap_ranking_status": "PASS",
                "deterministic_replay_sha256": "a" * 64},
            "ASM CLOSURE": {"state": "NO_DELTA"},
            "FULL ROM AUDIT": {"state": "PASS", "status": "PASS_POSTRUN_FULL_ROM_AUDIT_V1"},
        }
        generic = self.run_analysis / "generic-recursive-closure"
        generic.mkdir(parents=True)
        (generic / "postrun_generic_closure_receipt.json").write_text(json.dumps({
            "deterministic_replay_status": "PASS", "gap_ranking_status": "PASS",
            "deterministic_replay_sha256": "a" * 64}), encoding="utf-8")
        (generic / "postrun_capture_gap_ranking.json").write_text(json.dumps({
            "schema": "oasis.m13.generic-gap-ranking.v1", "gaps": []}), encoding="utf-8")
        ranking_hash = cleanup._sha(generic / "postrun_capture_gap_ranking.json")
        (generic / "postrun_recursive_closure_receipt.json").write_text(json.dumps({
            "status": "NO_DELTA", "output_hashes": {
                "postrun_capture_gap_ranking.json": ranking_hash}}), encoding="utf-8")

    def _make_canonical(self) -> None:
        db = sqlite3.connect(self.canonical / "knowledge.sqlite")
        db.execute("CREATE TABLE emission(start INTEGER,end INTEGER,source_owned INTEGER)")
        db.execute("INSERT INTO emission VALUES (0,4,0)")
        db.commit(); db.close()
        pointer = {"generation_dir": "generations/gen-1", "knowledge_sha256": cleanup._sha(self.canonical / "knowledge.sqlite")}
        (self.canonical_root / "current.json").write_text(json.dumps(pointer), encoding="utf-8")

    def _make_master(self) -> None:
        generation = self.master_root / "generations" / "master-1"
        generation.mkdir(parents=True)
        db = sqlite3.connect(generation / "master.sqlite")
        db.executescript("""
            CREATE TABLE run(run_id INTEGER PRIMARY KEY,receipt_sha256 TEXT,raw_sha256 TEXT,index_sha256 TEXT,
              segments INTEGER,raw_records INTEGER,unique_pcs INTEGER,unique_edges INTEGER,terminal_facts INTEGER,status TEXT);
            CREATE TABLE audit(run_id INTEGER PRIMARY KEY,raw_records_read INTEGER,accepted_runtime_occurrences INTEGER,
              unique_instruction_ranges INTEGER,unique_control_edges INTEGER,terminal_facts INTEGER,rejected_facts INTEGER,
              conflicts INTEGER,occurrence_totals INTEGER,witness_totals INTEGER);
            CREATE TABLE instruction(pc INTEGER,opcode INTEGER,bytes_sha256 TEXT,range_end INTEGER,range_status TEXT,
              occurrences INTEGER,witness_runs INTEGER,first_run INTEGER,last_run INTEGER,PRIMARY KEY(pc,opcode));
            CREATE TABLE edge(source_pc INTEGER,relation TEXT,target_pc INTEGER,occurrences INTEGER,witness_runs INTEGER,
              first_run INTEGER,last_run INTEGER,PRIMARY KEY(source_pc,relation,target_pc));
            CREATE TABLE meta(key TEXT PRIMARY KEY,value TEXT);
        """)
        db.execute("INSERT INTO run VALUES (7,'r',?,?,1,1,1,1,1,'STOPPED_END_GAME')",
                   (hashlib.sha256(self.raw.read_bytes()).hexdigest(), hashlib.sha256(self.index.read_bytes()).hexdigest()))
        db.execute("INSERT INTO audit VALUES (7,1,1,1,1,1,0,0,1,1)")
        db.execute("INSERT INTO instruction VALUES (0,20085,'x',2,'PASS',1,1,7,7)")
        db.execute("INSERT INTO edge VALUES (0,'OBSERVED_NEXT_PC',4,1,1,7,7)")
        db.commit(); db.close()
        report = generation / "report.json"; report.write_text("{}", encoding="utf-8")
        pointer = {"generation_dir": "generations/master-1", "master_sha256": cleanup._sha(generation / "master.sqlite"),
                   "report_sha256": cleanup._sha(report)}
        (self.master_root / "current.json").write_text(json.dumps(pointer), encoding="utf-8")

    def close(self) -> None:
        self.patcher.stop(); self.temp.cleanup()


class AbsorptionCleanupTests(unittest.TestCase):
    def setUp(self) -> None:
        self.fx = AbsorptionFixture()

    def tearDown(self) -> None:
        self.fx.close()

    def call(self, stages=None):
        return cleanup.reclaim_absorbed_run(self.fx.receipt, {"conflicts": 0},
                                            stages or self.fx.stage_results, self.fx.master_root,
                                            self.fx.run_analysis, self.fx.rom)

    def test_e_fully_absorbed_run_generates_manifest_and_deletes_raw(self) -> None:
        result = self.call()
        self.assertEqual(result["status"], cleanup.ABSORPTION_STATUS)
        self.assertEqual(result["semantic_equivalence"], "PASS")
        self.assertGreater(result["raw_bytes_deleted"], 0)
        self.assertFalse(self.fx.raw.exists()); self.assertFalse(self.fx.index.exists())
        self.assertEqual(result["deletion_mode"], "PERMANENT_NO_RECYCLE_BIN")
        self.assertTrue((self.fx.post / "absorbed-run-7.json").is_file())

    def test_e_coordinator_outputs_may_be_published_after_reclaim(self) -> None:
        (self.fx.post / "report.json").unlink()
        (self.fx.post / "status.json").unlink()
        result = self.call()
        self.assertEqual(result["status"], cleanup.ABSORPTION_STATUS)
        self.assertFalse(self.fx.raw.exists())

    def test_a_run_missing_from_lineage_keeps_raw(self) -> None:
        value = json.loads(self.fx.receipt.read_text()); value["runtime"]["run_id"] = 8
        self.fx.receipt.write_text(json.dumps(value), encoding="utf-8")
        with self.assertRaisesRegex(cleanup.CleanupStop, cleanup.STOP_NOT_ABSORBED): self.call()
        self.assertTrue(self.fx.raw.exists())

    def test_b_and_c_incomplete_stage_stop_and_keep(self) -> None:
        for stage in ("CONTROL PROVENANCE", "FULL ROM AUDIT"):
            stages = dict(self.fx.stage_results); stages[stage] = dict(stages[stage]); stages[stage]["status"] = "STOP"
            stages[stage]["state"] = "STOP"
            with self.assertRaisesRegex(cleanup.CleanupStop, cleanup.STOP_NOT_ABSORBED): self.call(stages)
            self.assertTrue(self.fx.raw.exists())

    def test_generic_gap_ranking_and_deterministic_replay_gate_raw_deletion(self) -> None:
        stages = dict(self.fx.stage_results)
        stages["GENERIC RECURSIVE CLOSURE"] = {"state": "NO_DELTA",
            "deterministic_replay_status": "PENDING", "gap_ranking_status": "PASS"}
        with self.assertRaisesRegex(cleanup.CleanupStop, cleanup.STOP_NOT_ABSORBED):
            self.call(stages)
        self.assertTrue(self.fx.raw.exists())
        ranking = self.fx.run_analysis / "generic-recursive-closure" / "postrun_capture_gap_ranking.json"
        ranking.unlink()
        with self.assertRaisesRegex(cleanup.CleanupStop, cleanup.STOP_NOT_ABSORBED): self.call()
        self.assertTrue(self.fx.raw.exists())

    def test_d_semantic_equivalence_failure_deletes_nothing(self) -> None:
        self.fx.raw.write_bytes(b"broken")
        with self.assertRaisesRegex(cleanup.CleanupStop, cleanup.STOP_NOT_ABSORBED): self.call()
        self.assertTrue(self.fx.raw.exists())

    def test_f_g_h_i_lock_and_replay_contract(self) -> None:
        original = self.fx.raw
        calls = {"count": 0}
        unlink = Path.unlink
        def flaky(path, missing_ok=False):
            if path == original and calls["count"] < 2:
                calls["count"] += 1
                error = OSError("locked"); error.winerror = 32; raise error
            return unlink(path, missing_ok=missing_ok)
        with mock.patch.object(Path, "unlink", flaky):
            result = self.call()
        self.assertGreaterEqual(calls["count"], 2); self.assertEqual(result["raw_bytes_retained"], 0)
        replay = self.call()
        self.assertEqual(replay["raw_bytes_deleted"], result["raw_bytes_deleted"])

    def test_h_permanently_locked_required_file_stops(self) -> None:
        error = OSError("locked"); error.winerror = 32
        with mock.patch.object(cleanup, "_delete", side_effect=cleanup.CleanupStop(cleanup.STOP_DELETE_FAILED, "locked")):
            with self.assertRaisesRegex(cleanup.CleanupStop, cleanup.STOP_DELETE_FAILED): self.call()

    def test_j_protected_master_path_rejected(self) -> None:
        roots = (self.fx.campaign.resolve(), self.fx.run_analysis.resolve())
        protected = self.fx.master_root / "generations" / "master-1" / "master.sqlite"
        with self.assertRaisesRegex(cleanup.CleanupStop, cleanup.STOP_NOT_ABSORBED):
            cleanup._validate_manifest(cleanup._manifest([protected], 7), 7, roots)

    def test_k_and_l_post_delete_integrity_and_retention_are_fail_closed(self) -> None:
        with mock.patch.object(cleanup, "_canonical_integrity", side_effect=cleanup.CleanupStop(cleanup.STOP_NOT_ABSORBED, "bad")):
            with self.assertRaises(cleanup.CleanupStop): self.call()
        self.assertTrue(self.fx.raw.exists())
        with mock.patch.object(cleanup, "_delete", return_value=0):
            with self.assertRaisesRegex(cleanup.CleanupStop, cleanup.STOP_DELETE_FAILED): self.call()
        self.assertTrue(self.fx.raw.exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
