"""R6 regression tests for the RAM-only Stage 5 session boundary."""
from __future__ import annotations

import hashlib
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from cartographer import Cartographer  # noqa: E402
from live_forward_archivist import archive_session_graph  # noqa: E402
from rom_knowledge_live_delta import import_archivist_graph  # noqa: E402
from rom_knowledge_map import KnowledgeStore  # noqa: E402


class Stage5InMemoryTests(unittest.TestCase):
    def test_empty_closed_graph_round_trips_without_session_path(self) -> None:
        rom = b"\0" * 8
        rom_sha = hashlib.sha256(rom).hexdigest()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            graph = Cartographer.in_memory(rom_sha)
            graph._put_meta("live_forward_session_schema", "oasis.m12.live-forward-session.v1")
            graph._put_meta("live_forward_session_id", "postrun-7")
            graph._put_meta("live_forward_instrumentation_identity", "a" * 64)
            graph._put_meta("live_forward_session_state", "CLOSED")
            graph._put_meta("live_forward_run_id", "7")
            graph._put_meta("live_forward_graph_sha256", graph.graph_hash())
            graph.db.commit()
            merge_master = root / "merge-master.sqlite"
            empty = Cartographer(merge_master, rom_sha)
            empty.close()
            source_sha = "b" * 64
            merged = archive_session_graph(merge_master, graph, rom_sha, source_sha)
            knowledge = root / "knowledge.sqlite"
            KnowledgeStore(knowledge, rom_sha, len(rom)).close()
            result = import_archivist_graph(graph, merge_master, knowledge, rom, rom_sha,
                                            merged["merge_receipt"], source_sha)
            self.assertEqual(result["status"], "PASS_IMPORTED")
            self.assertFalse((root / "session-rom-link.sqlite").exists())
            replay = import_archivist_graph(graph, merge_master, knowledge, rom, rom_sha,
                                             merged["merge_receipt"], source_sha)
            self.assertEqual(replay["status"], "PASS_IDEMPOTENT_NOOP")
            graph.close()


if __name__ == "__main__":
    unittest.main()
