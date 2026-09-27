"""Focused regression checks for compact Stage 5 evidence handling."""

from __future__ import annotations

import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from cartographer import Cartographer  # noqa: E402
from rom_knowledge_live_import import _lineage_count  # noqa: E402


class Stage5Tests(unittest.TestCase):
    def test_compact_lineage_count_preserves_occurrences(self) -> None:
        row = {"edge_id": "edge", "lineage": json.dumps([
            {"run_id": 7, "capture_id": 1, "generation": 1, "occurrence_count": 1000},
            {"run_id": 7, "capture_id": 2, "generation": 1, "occurrence_count": 4},
        ])}
        self.assertEqual(_lineage_count(row, 7), 1004)

    def test_lineage_count_rejects_wrong_run(self) -> None:
        row = {"edge_id": "edge", "lineage": json.dumps([
            {"run_id": 8, "capture_id": 1, "generation": 1, "occurrence_count": 2},
        ])}
        with self.assertRaises(ValueError):
            _lineage_count(row, 7)

    def test_cartographer_metadata_is_committed_before_merge(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "map.sqlite"
            graph = Cartographer(path, "a" * 64)
            graph.merge({"nodes": [], "edges": [], "frontiers": [],
                         "resolves_frontiers": []}, "empty", "source")
            graph.close()
            db = sqlite3.connect(path)
            try:
                self.assertEqual(db.execute("SELECT COUNT(*) FROM map_import").fetchone()[0], 1)
            finally:
                db.close()


if __name__ == "__main__":
    unittest.main()
