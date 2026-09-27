"""M13 live generic-closure stage contract tests."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
import live_forward_generic_closure_stage as stage  # noqa: E402
import generic_closure_handoff as handoff  # noqa: E402


class GenericClosureStageTests(unittest.TestCase):
    def test_large_artifact_hashing_does_not_require_read_bytes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "artifact.json"
            value = {"records": [{"address": n, "cpu": "M68K"} for n in range(2000)]}
            stage._write_json(path, value)
            with path.open("rb") as stream:
                expected_file_sha = hashlib.sha256(stream.read()).hexdigest()
            with patch.object(Path, "read_bytes", side_effect=AssertionError("whole-file read")):
                self.assertEqual(stage._sha(path), expected_file_sha)
            expected = hashlib.sha256(json.dumps(value, sort_keys=True,
                separators=(",", ":")).encode()).hexdigest()
            self.assertEqual(stage._canonical_json_sha(value), expected)

    def test_handoff_preserves_exception_type_in_nonfatal_stage_report(self) -> None:
        class Progress:
            def start(self, *args, **kwargs): pass
            def finish(self, *args, **kwargs): self.detail = kwargs.get("detail")

        progress = Progress()
        result = {"stages": {}}
        with patch.object(handoff, "run_generic_closure", side_effect=MemoryError()):
            stage_result, stop = handoff.run_generic_closure_handoff(
                {}, Path("."), Path("rom"), 1, progress, result)
        self.assertEqual(stop, "STOP_GENERIC_CLOSURE_EXCEPTION:MemoryError")
        self.assertEqual(stage_result["exception_type"], "MemoryError")
        self.assertEqual(result["stages"]["GENERIC RECURSIVE CLOSURE"]["stop"], stop)
        self.assertIn("MemoryError", progress.detail)

    def test_missing_sealed_flow_is_skipped_without_promotion(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            result = stage.run_generic_closure({"runtime": {"run_id": 1}}, Path(directory), Path(directory) / "rom")
            self.assertEqual(result["status"], "SKIPPED_NOT_APPLICABLE")
            self.assertEqual(result["source_owned_delta"], 0)

    def test_sealed_corpus_has_identity_and_deterministic_gap_plan(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw = root / "flow.bin"
            index = root / "flow.jsonl"
            record = struct.Struct("<QQQIIIHBBHHI").pack(1, 1, 1, 0x100, 0xFF1000,
                                                           0x4E75, 1, 0, 0, 0, 0, 0)
            raw.write_bytes(record * 2)
            index.write_text(json.dumps({"raw_offset": 0, "raw_length": len(record) * 2,
                                         "raw_sha256": hashlib.sha256(record * 2).hexdigest(),
                                         "segment": {"run_id": 7, "entry_frame": 10,
                                                     "record_count": 2,
                                                     "records_sha256": hashlib.sha256(record * 2).hexdigest()}}) + "\n")
            rom = root / "rom.bin"
            rom.write_bytes(b"\0" * 32)
            receipt = {"runtime": {"run_id": 7}, "raw_segment_spool": {
                "raw_path": str(raw), "index_path": str(index), "raw_bytes": len(record) * 2,
                "records": 2, "segments": 1,
                "raw_sha256": hashlib.sha256(record * 2).hexdigest(),
                "index_sha256": hashlib.sha256(index.read_bytes()).hexdigest()}}
            digest = hashlib.sha256(rom.read_bytes()).hexdigest()
            with patch.object(stage, "ROM_SIZE", 32), patch.object(stage, "ROM_SHA256", digest):
                result = stage.run_generic_closure(receipt, root / "analysis", rom)
            self.assertEqual(result["status"], "NO_DELTA")
            self.assertTrue(result["sealed"])
            self.assertEqual(result["corpus_records"], 2)
            self.assertEqual(result["flow_metrics"]["instruction_records"], 2)
            self.assertEqual(result["deterministic_replay_status"], "PASS")
            self.assertEqual(result["gap_ranking_status"], "PASS")
            generic = root / "analysis" / "generic-recursive-closure"
            corpus = json.loads((generic / "normalized_generic_corpus.json").read_text())
            self.assertEqual(corpus["schema"], "oasis.m13.normalized-generic-corpus.v2")
            self.assertEqual(len(corpus["instructions"]), 2)
            self.assertIn("memory", corpus)
            self.assertIn("rom_reads", corpus)
            self.assertEqual(corpus["schema_coverage"]["INSTRUCTIONS_WITH_CPU_ID"], 2)
            self.assertEqual(corpus["schema_coverage"]["INSTRUCTIONS_WITH_REGISTER_REF"], 0)
            closure = json.loads((generic / "postrun_generic_closure_receipt.json").read_text())
            self.assertEqual(closure["deterministic_replay_status"], "PASS")
            self.assertIn("total_facts", closure)
            self.assertTrue((root / "analysis" / "generic-recursive-closure" /
                             "postrun_capture_gap_ranking.json").is_file())

    def test_claimed_corpus_hash_mismatch_is_fatal(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw, index, rom = root / "flow.bin", root / "flow.jsonl", root / "rom.bin"
            raw.write_bytes(b"corrupt")
            index.write_text("{}\n", encoding="utf-8")
            rom.write_bytes(b"rom")
            receipt = {"flow_handoff": {"raw_path": str(raw), "index_path": str(index),
                "segments": 1, "raw_sha256": "0" * 64,
                "index_sha256": hashlib.sha256(index.read_bytes()).hexdigest()}}
            digest = hashlib.sha256(rom.read_bytes()).hexdigest()
            with patch.object(stage, "ROM_SIZE", len(rom.read_bytes())), \
                    patch.object(stage, "ROM_SHA256", digest), \
                    self.assertRaisesRegex(ValueError, "STOP_GENERIC_CLOSURE_CORPUS_RAW_HASH_MISMATCH"):
                stage.run_generic_closure(receipt, root / "analysis", rom)

    def test_claimed_corpus_with_wrong_rom_identity_is_fatal(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw, index, rom = root / "flow.bin", root / "flow.jsonl", root / "rom.bin"
            raw.write_bytes(b"x")
            index.write_text("{}\n", encoding="utf-8")
            rom.write_bytes(b"wrong rom")
            receipt = {"flow_handoff": {"raw_path": str(raw), "index_path": str(index),
                "segments": 1}}
            with self.assertRaisesRegex(ValueError, "STOP_GENERIC_CLOSURE_ROM_IDENTITY_MISMATCH"):
                stage.run_generic_closure(receipt, root / "analysis", rom)


if __name__ == "__main__":
    unittest.main(verbosity=2)
