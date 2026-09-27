"""Focused Stage 6 lifecycle and contract tests."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
sys.path.insert(0, str(ROOT / "src" / "tools"))

import live_forward_control_provenance_stage as stage  # noqa: E402


class _Records:
    @staticmethod
    def iter_unpack(data: bytes):
        del data
        return iter([tuple(range(7))])


class _Progress:
    def __init__(self):
        self.updates = []
        self.heartbeats = []

    def update(self, processed, **values):
        self.updates.append((processed, values))

    def heartbeat(self, detail):
        self.heartbeats.append(detail)


class ControlProvenanceStageTests(unittest.TestCase):
    def test_zero_consumers_returns_no_delta_with_known_total(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw = root / "flow.bin"
            index = root / "flow.jsonl"
            raw.write_bytes(b"payload")
            index.write_text(json.dumps({"raw_offset": 0, "raw_length": 7,
                "segment": {"run_id": 7}}) + "\n", encoding="utf-8")
            rom = root / "rom.bin"
            rom.write_bytes(b"rom")
            receipt = {"runtime": {"run_id": 7}, "raw_segment_spool": {
                "raw_path": str(raw), "index_path": str(index), "segments": 1,
                "raw_sha256": hashlib.sha256(raw.read_bytes()).hexdigest(),
                "index_sha256": hashlib.sha256(index.read_bytes()).hexdigest()}}
            progress = _Progress()
            with patch.object(stage, "ROM_SHA", hashlib.sha256(b"rom").hexdigest()), \
                    patch.object(stage, "RECORD", _Records), \
                    patch.object(stage, "analyze_flow_segment",
                                 return_value={"status": "PASS", "consumers": []}):
                result = stage.run_control_provenance(receipt, rom, "gen-a", 1, progress)
            self.assertEqual(result["status"], "NO_DELTA")
            self.assertEqual(result["output_generation_or_same"], "gen-a")
            self.assertEqual(result["raw_flow_identity"]["raw_path"], str(raw))
            self.assertEqual(progress.updates[-1][0], 1)
            self.assertEqual(progress.updates[-1][1]["total"], 1)

    def test_unresolved_consumers_do_not_globally_stop(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw, index, rom = root / "f.bin", root / "f.jsonl", root / "r.bin"
            raw.write_bytes(b"payload")
            index.write_text(json.dumps({"raw_offset": 0, "raw_length": 7,
                "segment": {"run_id": 8}}) + "\n", encoding="utf-8")
            rom.write_bytes(b"rom")
            receipt = {"runtime": {"run_id": 8}, "raw_segment_spool": {
                "raw_path": str(raw), "index_path": str(index), "segments": 1}}
            diagnostic = root / "stage6-diagnostic.json"
            with patch.object(stage, "ROM_SHA", hashlib.sha256(b"rom").hexdigest()), \
                    patch.object(stage, "RECORD", _Records), \
                    patch.object(stage, "analyze_flow_segment", return_value={
                        "status": "PASS_CONTROL_PROVENANCE_V1", "consumers": [{
                            "status": "SOURCE_TRANSFORMED_UNSUPPORTED",
                            "required_registers": ["A4"],
                            "consumer_occurrence": {"record_index": 0, "pc": 2,
                                                     "opcode": 0x4E90}}]}):
                result = stage.run_control_provenance(receipt, rom, "gen-a", 1,
                                                      _Progress(), diagnostic)
            self.assertEqual(result["status"], "NO_DELTA")
            self.assertEqual(result["unresolved"], 1)
            self.assertEqual(result["unsupported"], 1)
            self.assertEqual(json.loads(diagnostic.read_text())["stop_code"], None)

    def test_flow_corruption_still_stops_with_diagnostic(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            raw, index, rom = root / "f.bin", root / "f.jsonl", root / "r.bin"
            raw.write_bytes(b"payload")
            index.write_text(json.dumps({"raw_offset": 0, "raw_length": 7,
                "segment": {"run_id": 9}}) + "\n", encoding="utf-8")
            rom.write_bytes(b"rom")
            receipt = {"runtime": {"run_id": 9}, "raw_segment_spool": {
                "raw_path": str(raw), "index_path": str(index), "segments": 1}}
            diagnostic = root / "stage6-diagnostic.json"
            with patch.object(stage, "ROM_SHA", hashlib.sha256(b"rom").hexdigest()), \
                    patch.object(stage, "RECORD", _Records), \
                    patch.object(stage, "analyze_flow_segment", return_value={
                        "status": "STOP_CONTROL_PROVENANCE_FLOW_MISMATCH",
                        "consumers": []}):
                result = stage.run_control_provenance(receipt, rom, "gen-a", 1,
                                                      _Progress(), diagnostic)
            self.assertEqual(result["status"], "STOP")
            self.assertEqual(result["stop_reason"], "STOP_CONTROL_PROVENANCE_FLOW_MISMATCH")
            self.assertEqual(json.loads(diagnostic.read_text())["FLOW_mismatches"], 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
