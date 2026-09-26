"""Runtime paths group events by capture identity, not per-event raw offsets."""

import json
import sqlite3
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from runtime_path_view import iter_runtime_paths


class RuntimePathViewTests(unittest.TestCase):
    def test_per_event_offsets_do_not_split_one_capture_window(self):
        db = sqlite3.connect(":memory:")
        self.addCleanup(db.close)
        db.execute("CREATE TABLE live_forward_runtime_occurrence("
                    "occurrence_id TEXT PRIMARY KEY,event_json TEXT NOT NULL)")
        window = {"worker_id": 2, "capture_id": 91, "generation": 4,
            "segment_sha256": "a" * 64, "source_raw_sha256": "b" * 64,
            "source_index_sha256": "c" * 64, "raw_offset": 480,
            "entry_stream_sequence": 10, "exit_stream_sequence": 12,
            "record_count": 2}
        for index, (sequence, pc, offset) in enumerate(((10, 0x100, 480),
                                                        (11, 0x102, 528))):
            event = {"occurrence_id": f"occ-{index}", "run_id": 7, "epoch": 3,
                "cpu_id": "M68K", "event_kind": "INSTRUCTION",
                "address_space": "FLOW_DOMAIN_1", "pc": pc, "address": pc + 2,
                "value": 0x4E71, "flags": 3, "instruction_sequence": 20 + index,
                "native_sequence": sequence, "windows": [{**window,
                    "source_offset": offset}]}
            db.execute("INSERT INTO live_forward_runtime_occurrence VALUES (?,?)",
                       (event["occurrence_id"], json.dumps(event)))
        paths = list(iter_runtime_paths(db))
        self.assertEqual(len(paths), 1)
        self.assertEqual(paths[0]["pcs"], [0x100, 0x102])
        self.assertEqual(paths[0]["native_sequences"], [10, 11])
        self.assertEqual(paths[0]["instruction_sequences"], [20, 21])
        self.assertEqual(paths[0]["window"]["capture_id"], 91)


if __name__ == "__main__":
    unittest.main()
