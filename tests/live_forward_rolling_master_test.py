"""Deterministic compact rolling-master checks for the 2I post-run path."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from live_forward_rolling_master import compact  # noqa: E402
from live_forward_scaling_audit import RECORD  # noqa: E402


class RollingMasterTests(unittest.TestCase):
    def test_a_sealed_run_updates_one_atomic_compact_pointer(self) -> None:
        rom_path = ROOT / "local-roms" / "Beyond Oasis (USA).md"
        if not rom_path.is_file():
            rom_path = ROOT / "build" / "reference" / "Beyond Oasis (USA).bin"
        if not rom_path.is_file():
            self.skipTest("canonical ROM is not available")
        rom = rom_path.read_bytes()
        pc = 0x100
        first_opcode = int.from_bytes(rom[pc:pc + 2], "big")
        second_opcode = int.from_bytes(rom[pc + 2:pc + 4], "big")
        rows = [RECORD.pack(1, 1, 100, pc, pc + 2, first_opcode, 3, 0, 2, 0, 0, 0),
                RECORD.pack(2, 2, 101, pc + 2, pc + 4, second_opcode, 3, 0, 2, 0, 0, 0)]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            spool = root / "spool"
            spool.mkdir()
            raw = spool / "flow-v1-records.bin"
            index = spool / "flow-v1-segments.jsonl"
            data = b"".join(rows)
            raw.write_bytes(data)
            segment = {"valid": True, "ready_for_cartographer": True, "run_id": 7,
                       "epoch": 1, "worker_id": 0, "capture_id": 7_000_001,
                       "generation": 1, "record_count": 2}
            index.write_text(json.dumps({"segment": segment, "raw_offset": 0,
                "raw_length": len(data), "raw_sha256": hashlib.sha256(data).hexdigest()}) + "\n",
                encoding="utf-8")
            receipt = root / "receipt.json"
            receipt.write_text(json.dumps({"checkpoint": "M12-LIVE-WORKER-INTERACTIVE",
                "source_owned_delta": 0, "runtime": {"run_id": 7,
                "outcome": "STOPPED_FRAME_LIMIT"}, "raw_segment_spool": {
                "raw_path": str(raw), "index_path": str(index), "raw_sha256": hashlib.sha256(data).hexdigest(),
                "index_sha256": hashlib.sha256(index.read_bytes()).hexdigest()}}), encoding="utf-8")
            report = compact(receipt, root / "rolling-master", rom_path,
                             report_path=root / "report.json", status_path=root / "status.json")
            self.assertEqual(report["status"], "PASS_END_GAME_ROLLING_MASTER_COMPACT_V1")
            self.assertEqual(report["pipeline_state"], "ANALYSIS STOPPED ✗")
            pointer = json.loads((root / "rolling-master" / "current.json").read_text())
            self.assertEqual(pointer["master_sha256"], report["master_sha256"])
            self.assertEqual(report["unique_instruction_ranges"], 2)
            self.assertIn("stage_results", report)
            status = json.loads((root / "status.json").read_text())
            self.assertEqual(status["overall_state"], "ANALYSIS STOPPED ✗")
            self.assertEqual(status["stages"]["REFRESHING MAP"]["state"], "STOP")


if __name__ == "__main__":
    unittest.main(verbosity=2)
