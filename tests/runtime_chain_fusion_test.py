"""Full-path diversity, exact witness retention and replay acceptance."""

from contextlib import closing
import json
import sqlite3
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "src/tools/thor_evidence"), str(ROOT / "tests")]
from live_forward_cartographer_test import ROM, INSTRUMENTATION, _rows, _segment
from live_forward_cartographer import LiveForwardCartographer
from live_forward_archivist import archive_session
from cartographer import Cartographer
from map_merge import merge_session_map
from runtime_occurrence_merge import occurrence_hash
from runtime_path_view import iter_runtime_paths
from auto65_chain import _candidate, classify
from auto67_cartographer import candidate_bundle
from thor_evidence_auto67_cartographer_seam_test import chain, step


class RuntimeChainFusionTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.master = self.root / "master.sqlite"

    def session(self, paths, run=77, opcodes=None, flags=None):
        session = LiveForwardCartographer(ROM, INSTRUMENTATION, f"run-{run}")
        self.addCleanup(session.close)
        for index, pcs in enumerate(paths):
            rows = _rows([(pc, (opcodes or {}).get(pc, 0x4e71),
                           (flags or {}).get((index, pc), 3),
                           pcs[n + 1] if n + 1 < len(pcs) else 0x800)
                          for n, pc in enumerate(pcs)],
                         stream_start=100 + index * 100, instruction_start=50 + index * 100)
            segment, blob = _segment(rows, run=run, capture=index + 1)
            session.admit(segment, rows, blob)
        path = self.root / f"run-{run}.sqlite"
        session.save_closed(path, len(paths))
        return path

    def read(self):
        graph = Cartographer(self.master, ROM)
        self.addCleanup(graph.close)
        return graph

    def check_paths(self, paths, **kwargs):
        session = self.session(paths, **kwargs)
        archive_session(self.master, session, ROM)
        graph = self.read()
        view = list(iter_runtime_paths(graph.db))
        self.assertEqual(sorted(p["pcs"] for p in view), sorted(paths))
        actual = {(json.loads(s)["attributes"]["pc"], json.loads(t)["attributes"]["pc"])
                  for s, t in graph.db.execute("SELECT s.body,t.body FROM map_edge e "
                      "JOIN map_node s ON s.node_id=e.source_id "
                      "JOIN map_node t ON t.node_id=e.target_id")}
        self.assertEqual(actual, {pair for path in paths for pair in zip(path, path[1:])})
        self.assertEqual(graph.metrics()["source_owned_bytes"], 0)
        self.assertEqual(graph.metrics()["proven_edges"], 0)
        return graph, view

    def test_01_same_start_different_next(self):
        self.check_paths([[0x100, 0x200], [0x100, 0x300]])

    def test_02_same_first_edge_diverge_later(self):
        self.check_paths([[0x100, 0x102, 0x104, 0x200], [0x100, 0x102, 0x104, 0x300]])

    def test_03_same_path_distinct_occurrences(self):
        graph, paths = self.check_paths([[0x100, 0x102, 0x104]] * 2)
        self.assertEqual(graph.metrics()["nodes"], 3)
        self.assertEqual(graph.metrics()["edges"], 2)
        self.assertEqual(len({p["structural_path_id"] for p in paths}), 1)
        self.assertNotEqual(paths[0]["occurrence_ids"], paths[1]["occurrence_ids"])

    def test_04_hundred_executions_no_structural_bloat(self):
        graph, paths = self.check_paths([[0x100, 0x102, 0x104]] * 100)
        self.assertEqual((graph.metrics()["nodes"], graph.metrics()["edges"]), (3, 2))
        self.assertEqual(len(paths), 100)
        count = graph.db.execute("SELECT COUNT(*) FROM live_forward_runtime_occurrence "
            "WHERE json_extract(event_json,'$.event_kind')='INSTRUCTION' "
            "AND json_extract(event_json,'$.pc')=256").fetchone()[0]
        self.assertEqual(count, 100)

    def test_05_conditional_branch_outcomes(self):
        graph, _ = self.check_paths([[0x100, 0x200], [0x100, 0x102]],
            opcodes={0x100: 0x6602}, flags={(0, 0x100): 3 | 8 | 16, (1, 0x100): 3 | 8 | 32})
        outcomes = {v for row in graph.db.execute("SELECT lineage FROM map_edge")
                    for item in json.loads(row[0]) for v in item["control_flow_outcomes"]}
        self.assertEqual(outcomes, {"branch_taken", "branch_not_taken"})
        masks = {item["kind_flags_or"] for row in graph.db.execute("SELECT lineage FROM map_edge")
                 for item in json.loads(row[0])}
        self.assertEqual(masks, {27, 43})

    def test_06_indirect_targets(self):
        self.check_paths([[0x100, 0x200], [0x100, 0x300]], opcodes={0x100: 0x4ed0})

    def test_07_return_targets(self):
        for opcode in (0x4e75, 0x4e73):
            with self.subTest(opcode=opcode):
                # One independent master/session per opcode.
                self.master = self.root / f"master-{opcode}.sqlite"
                self.check_paths([[0x100, 0x200], [0x100, 0x300]],
                                 run=opcode, opcodes={0x100: opcode})

    def test_08_shared_prefix_structural_reuse(self):
        graph, _ = self.check_paths([[0x100, 0x102, 0x104, 0x200],
                                    [0x100, 0x102, 0x104, 0x300]])
        self.assertEqual((graph.metrics()["nodes"], graph.metrics()["edges"]), (5, 4))
        self.assertEqual(len(list(iter_runtime_paths(graph.db, (0x100, 0x102, 0x104)))), 2)

    def test_09_session_merge_distinct_runs(self):
        a = self.session([[0x100, 0x102, 0x200]], run=77)
        b = self.session([[0x100, 0x102, 0x300]], run=78)
        merge_session_map(self.master, a)
        merge_session_map(self.master, b)
        graph = self.read()
        self.assertEqual({p["run_id"] for p in iter_runtime_paths(graph.db)}, {77, 78})
        self.assertEqual(graph.metrics()["edges"], 3)
        reverse = self.root / "reverse.sqlite"
        merge_session_map(reverse, b)
        merge_session_map(reverse, a)
        with closing(sqlite3.connect(reverse)) as db:
            self.assertEqual(occurrence_hash(graph.db), occurrence_hash(db))

    def test_10_replay_idempotence(self):
        source = self.session([[0x100, 0x102, 0x104]])
        first = merge_session_map(self.master, source)
        replay = merge_session_map(self.master, source)
        self.assertEqual(replay["global_merge_delta"]["new_nodes"], 0)
        self.assertEqual(replay["global_merge_delta"]["new_edges"], 0)
        self.assertEqual(replay["runtime_occurrence_delta"]["new_occurrences"], 0)
        self.assertEqual(first["runtime_occurrence_delta"]["occurrence_hash"],
                         replay["runtime_occurrence_delta"]["occurrence_hash"])

    def test_dbcc_loop_multiplicity_not_edge_set_identity(self):
        _, paths = self.check_paths([[0x100, 0x100, 0x102], [0x100, 0x100, 0x100, 0x102]],
                                    opcodes={0x100: 0x51c8})
        self.assertEqual(len({p["structural_path_id"] for p in paths}), 2)

    def test_conflicting_occurrence_does_not_replace_master(self):
        source = self.session([[0x100, 0x102]])
        merge_session_map(self.master, source)
        before = self.master.read_bytes()
        with closing(sqlite3.connect(source)) as db:
            db.execute("UPDATE live_forward_runtime_occurrence SET event_json="
                       "json_set(event_json,'$.address',999)")
            db.commit()
        with self.assertRaisesRegex(ValueError, "OCCURRENCE_HASH_MISMATCH"):
            merge_session_map(self.master, source)
        self.assertEqual(before, self.master.read_bytes())
        with self.assertRaisesRegex(ValueError, "OCCURRENCE_HASH_MISMATCH"):
            archive_session(self.master, source, ROM)
        with closing(sqlite3.connect(source)) as db:
            db.execute("DELETE FROM map_meta WHERE key='live_forward_occurrence_sha256'")
            db.commit()
        with self.assertRaisesRegex(ValueError, "IDENTITY_CONFLICT"):
            merge_session_map(self.master, source)
        self.assertEqual(before, self.master.read_bytes())

    def test_auto65_full_structure_not_prefix(self):
        def candidate(pcs):
            nodes = [{"id": str(pc), "kind": "ROOT"} for pc in pcs]
            edges = [{"source": str(a), "target": str(b), "kind": "EXECUTION"}
                     for a, b in zip(pcs, pcs[1:])]
            return _candidate({"id": "fixture"}, nodes, edges, {}, "fixture")
        a = candidate([1, 2, 3, 4])
        b = candidate([1, 2, 3, 5])
        self.assertEqual(classify(a, [a]), "KNOWN_NEW_INSTANCE")
        self.assertEqual(classify(b, [a]), "NEW_BRANCH")
        self.assertNotEqual(a["fingerprint"], b["fingerprint"])

    def test_sealed_occurrence_table_cannot_disappear(self):
        source = self.session([[0x100, 0x102]])
        with closing(sqlite3.connect(source)) as db:
            db.execute("DROP TABLE live_forward_runtime_occurrence")
            db.commit()
        with self.assertRaisesRegex(ValueError, "OCCURRENCE_HASH_MISMATCH"):
            merge_session_map(self.master, source)
        self.assertFalse(self.master.exists())

    def test_auto67_identical_dependencies_keep_hundred_witnesses(self):
        graph = Cartographer.in_memory(ROM)
        self.addCleanup(graph.close)
        for i in range(100):
            bundle, identity = candidate_bundle(chain(f"epoch=1:seq={i}"))
            graph.merge(bundle, identity)
            graph.merge(bundle, identity)
        lineage = json.loads(graph.db.execute("SELECT lineage FROM map_edge").fetchone()[0])
        witnesses = [v for v in lineage if v["source"] == "AUTO67_LOCAL_CHAIN_OCCURRENCE"]
        self.assertEqual(len(witnesses), 100)
        self.assertEqual((graph.metrics()["nodes"], graph.metrics()["edges"]), (2, 1))

    def test_auto67_shared_dependency_different_tail(self):
        graph = Cartographer.in_memory(ROM)
        self.addCleanup(graph.close)
        for target in ("0x300", "0x400"):
            bundle, identity = candidate_bundle(chain(target, steps=[
                step("0x100", "0x200"), step("0x200", target)]))
            graph.merge(bundle, identity)
        self.assertEqual((graph.metrics()["nodes"], graph.metrics()["edges"]), (4, 3))

    def test_canonical_bridge_retains_paths_and_emission(self):
        from rom_knowledge_pipeline_test import ArchivistCanonicalPipelineTests
        fixture = ArchivistCanonicalPipelineTests()
        fixture.setUp()
        try:
            a = fixture.session("path-a", 501, instructions=(0x10, 0x12, 0x14))
            b = fixture.session("path-b", 502, instructions=(0x10, 0x12, 0x16))
            first = fixture.run_pipeline(a)
            second = fixture.run_pipeline(b)
            with closing(sqlite3.connect(Path(first["generation_dir"]) / "knowledge.sqlite")) as db:
                emission = db.execute("SELECT * FROM emission ORDER BY start").fetchall()
            with closing(sqlite3.connect(Path(second["generation_dir"]) / "knowledge.sqlite")) as db:
                paths = list(iter_runtime_paths(db, (0x10, 0x12)))
                self.assertEqual({tuple(p["pcs"]) for p in paths},
                                 {(0x10, 0x12, 0x14), (0x10, 0x12, 0x16)})
                self.assertEqual(db.execute("SELECT COUNT(*) FROM relation "
                    "WHERE relation_type='EXECUTED_NEXT'").fetchone()[0], 3)
                self.assertEqual(db.execute("SELECT * FROM emission ORDER BY start").fetchall(), emission)
                self.assertEqual({r[0] for r in db.execute("SELECT status FROM relation "
                    "WHERE relation_type='EXECUTED_NEXT'")}, {"OBSERVED_RUNTIME"})
            replay = fixture.run_pipeline(b)
            self.assertEqual(replay["replay_status"], "PASS_IDEMPOTENT_NOOP")
        finally:
            fixture.tearDown()

    def test_overlapping_windows_union_native_identity_and_bounded_view(self):
        session = LiveForwardCartographer(ROM, INSTRUMENTATION, "overlap")
        self.addCleanup(session.close)
        rows = _rows([(0x100, 0x4e71, 3, 0x102), (0x102, 0x4e71, 3, 0x104)])
        for capture in (1, 2):
            segment, blob = _segment(rows, capture=capture)
            session.admit(segment, rows, blob)
        path = self.root / "overlap.sqlite"
        session.save_closed(path, 2)
        merge_session_map(self.master, path)
        graph = self.read()
        events = [json.loads(row[0]) for row in graph.db.execute(
            "SELECT event_json FROM live_forward_runtime_occurrence")]
        self.assertEqual(len(events), 3)
        self.assertTrue(all(e["capture_ids"] == [1, 2] for e in events))
        paths = list(iter_runtime_paths(graph.db))
        self.assertEqual(len(paths), 2)  # two windows, one native execution
        self.assertEqual(paths[0]["occurrence_ids"], paths[1]["occurrence_ids"])
        with self.assertRaisesRegex(ValueError, "WINDOW_LIMIT"):
            list(iter_runtime_paths(graph.db, max_window_events=1))


if __name__ == "__main__":
    unittest.main()
