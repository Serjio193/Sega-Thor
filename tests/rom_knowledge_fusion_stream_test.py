"""Chunk-boundary and escape handling for bounded JSON array streaming."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))

from rom_knowledge_fusion_stream import _Stream, find_unique_array_value


class FusionStreamTests(unittest.TestCase):
    def test_skips_nested_secondary_arrays_and_chunk_split_strings(self):
        value = {"secondary": [{"text": ("braces {} [] slash \\ quote \" newline\n ,\"records\":[" * 10)}],
            "records": [{"ordinal": 0}, {"ordinal": 1}], "schema": "fixture.v1"}
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "fixture.json"
            path.write_text(json.dumps(value), encoding="utf-8")
            stream = _Stream(path, chunk=37)
            metadata = {}
            try:
                rows = list(stream.arrays({"records"}, {"schema"}, metadata))
            finally:
                stream.file.close()
        self.assertEqual(rows, [("records", 0, {"ordinal": 0}),
                                ("records", 1, {"ordinal": 1})])
        self.assertEqual(metadata, {"schema": "fixture.v1"})

    def test_producer_fast_seek_reads_primary_array_and_metadata_tail(self):
        value = {"instructions": [{"pc": 0x10}, {"pc": 0x12}],
            "records": [{"ordinal": 0}, {"ordinal": 1}],
            "capture_gaps": [], "schema": "fixture.v1", "sealed": True,
            "record_count": 2}
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "producer-order.json"
            path.write_text(json.dumps(value, separators=(",", ":")), encoding="utf-8")
            stream = _Stream(path, chunk=17, offset=find_unique_array_value(path, "records"))
            metadata = {}
            try:
                rows = list(stream.array_values())
                stream.top_level_tail({"schema", "sealed", "record_count"}, metadata)
            finally:
                stream.file.close()
        self.assertEqual(rows, [(0, {"ordinal": 0}), (1, {"ordinal": 1})])
        self.assertEqual(metadata, {"schema": "fixture.v1", "sealed": True,
                                    "record_count": 2})


if __name__ == "__main__":
    unittest.main()
