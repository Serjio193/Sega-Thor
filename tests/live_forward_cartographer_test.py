import hashlib
import json
import sqlite3
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
sys.path.insert(0, str(ROOT / "tools/bizhawk-native-ring"))

from cartographer import Cartographer
from live_forward_archivist import archive_session
from live_forward_cartographer import EVENT_SHIFT, FLAG_EVENT, LiveForwardCartographer
from live_forward_rom_link_audit import RECORD


ROM = "a" * 64
INSTRUMENTATION = "b" * 64


def _rows(instructions, stream_start=100, instruction_start=50):
    rows = []
    for index, (pc, opcode, flags, next_pc) in enumerate(instructions):
        rows.append((stream_start + index, instruction_start + index, 0, pc, next_pc,
                     opcode, flags, 0, 0, 0, 0, 0))
    return rows


def _segment(rows, worker=0, capture=1, generation=1, run=77, epoch=1, valid=True):
    records = b"".join(RECORD.pack(*row) for row in rows)
    identity = f"{run}:{epoch}:{worker}:{capture}:{generation}".encode()
    return ({"valid": valid, "ready_for_cartographer": True, "run_id": run,
        "epoch": epoch, "worker_id": worker,
        "capture_id": capture, "generation": generation,
        "entry_stream_sequence": rows[0][0], "exit_stream_sequence": rows[-1][0] + 1,
        "entry_instruction_sequence": rows[0][1],
        "exit_instruction_sequence": rows[-1][1] + 1,
        "record_count": len(rows), "configured_depth": 20,
        "segment_sha256": hashlib.sha256(identity + records).hexdigest(),
        "records_sha256": hashlib.sha256(records).hexdigest()}, records)


def _save_session(path, instructions, run=77, capture=1, generation=1):
    rows = _rows(instructions)
    segment, records = _segment(rows, capture=capture, generation=generation, run=run)
    session = LiveForwardCartographer(ROM, INSTRUMENTATION, f"session-{run}-{capture}")
    session.admit(segment, rows, records)
    receipt = session.save_closed(path, 1)
    session.close()
    return receipt


def _ids(path):
    graph = Cartographer(path, ROM)
    try:
        nodes = {row[0] for row in graph.db.execute("SELECT node_id FROM map_node")}
        edges = {row[0] for row in graph.db.execute("SELECT edge_id FROM map_edge")}
        statuses = {row[0]: row[1] for row in graph.db.execute(
            "SELECT edge_id, status FROM map_edge")}
        return nodes, edges, statuses, graph.graph_hash()
    finally:
        graph.close()


class LiveForwardCartographerTests(unittest.TestCase):
    def test_0_scaling_callback_is_after_validation_and_before_exact_ack(self):
        source = (ROOT / "tools/bizhawk-native-ring/live_forward_scaling_runtime.py").read_text(
            encoding="utf-8")
        validated = source.index("segment = validate_segment(")
        admitted = source.index("on_segment(ready_segment, list(RECORD.iter_unpack(data)), data)")
        acknowledged = source.index('with ack.open("a", encoding="ascii", newline="")')
        self.assertLess(validated, admitted)
        self.assertLess(admitted, acknowledged)

    def test_a_valid_segment_becomes_observed_ordered_execution_edges(self):
        rows = _rows([(0x100, 0x4E71, 3, 0x102), (0x102, 0x6602, 3 | 8 | 16, 0x106),
                      (0x106, 0x4E75, 3 | 8, 0x200), (0x200, 0x4E71, 3, 0x202)])
        segment, records = _segment(rows)
        session = LiveForwardCartographer(ROM, INSTRUMENTATION)
        try:
            session.admit(segment, rows, records)
            metrics = session.metrics()
            self.assertEqual((metrics["segments_admitted"], metrics["edges"]), (1, 3))
            self.assertEqual(metrics["source_owned_bytes"], 0)
            edge = session.graph.db.execute("SELECT status, body FROM map_edge LIMIT 1").fetchone()
            self.assertEqual(edge["status"], "OBSERVED")
            self.assertEqual(json.loads(edge["body"])["relation"], "EXECUTED_NEXT")
            pending = session.graph.db.execute(
                "SELECT lineage_json FROM live_forward_pending_lineage WHERE object_type='edge' "
                "LIMIT 1").fetchone()
            self.assertEqual(json.loads(pending[0])["profile"], "FLOW_V1")
            self.assertGreater(metrics["pending_lineage_records"], 0)
            with tempfile.TemporaryDirectory() as directory:
                session.save_closed(Path(directory) / "session.sqlite", 1)
                outcomes = {outcome for (encoded,) in session.graph.db.execute(
                    "SELECT lineage FROM map_edge") for item in json.loads(encoded)
                    for outcome in item["control_flow_outcomes"]}
                self.assertIn("branch_taken", outcomes)
                self.assertIn("return", outcomes)
        finally:
            session.close()

    def test_occurrences_keep_native_identity_and_deduplicate_overlapping_windows(self):
        rows = _rows([(0x100, 0x4E71, 3, 0x102), (0x102, 0x4E75, 3, 0x200)])
        first, first_blob = _segment(rows, worker=0, capture=1, generation=1)
        overlap, overlap_blob = _segment(rows, worker=1, capture=2, generation=1)
        session = LiveForwardCartographer(ROM, INSTRUMENTATION, "overlap-occurrences")
        try:
            session.admit(first, rows, first_blob)
            session.admit(overlap, rows, overlap_blob)
            events = [json.loads(row[0]) for row in session.graph.db.execute(
                "SELECT event_json FROM live_forward_runtime_occurrence ORDER BY occurrence_id")]
            self.assertEqual(len(events), 3)  # two instructions plus their transition
            instruction = [event for event in events if event["event_kind"] == "INSTRUCTION"]
            self.assertEqual(len(instruction), 2)
            self.assertTrue(all(event["capture_ids"] == [1, 2] for event in instruction))
            self.assertEqual(session.graph.db.execute("SELECT COUNT(*) FROM map_node").fetchone()[0], 2)
        finally:
            session.close()

    def test_same_numeric_bus_address_on_m68k_and_z80_has_distinct_occurrences(self):
        # EVENT_SHIFT is the bit position; subtype BUS_READ is 1.
        read_flags = FLAG_EVENT | (1 << EVENT_SHIFT)
        rows = [
            (10, 1, 0, 0x20, 0x1234, 0x5A, read_flags, 0, 1, 1, 0, 0),
            (11, 1, 0, 0x20, 0x1234, 0x5A, read_flags, 1, 1, 7, 0, 0),
        ]
        segment, blob = _segment(rows)
        session = LiveForwardCartographer(ROM, INSTRUMENTATION, "cross-cpu-occurrences")
        try:
            session.admit(segment, rows, blob)
            events = [json.loads(row[0]) for row in session.graph.db.execute(
                "SELECT event_json FROM live_forward_runtime_occurrence ORDER BY occurrence_id")]
            self.assertEqual(len(events), 2)
            self.assertEqual({event["address"] for event in events}, {0x1234})
            self.assertEqual({event["cpu_id"] for event in events}, {"M68K", "Z80"})
            self.assertEqual(len({event["occurrence_id"] for event in events}), 2)
        finally:
            session.close()

    def test_z80_instructions_remain_occurrences_without_m68k_rom_nodes(self):
        rows = [
            (10, 101, 7, 0x20, 0x22, 0x1234, 3, 1, 2, 7, 0, 0),
            (11, 102, 9, 0x22, 0x24, 0x5678, 3, 1, 2, 7, 0, 0),
        ]
        segment, blob = _segment(rows)
        session = LiveForwardCartographer(ROM, INSTRUMENTATION, "z80-occurrences-only")
        try:
            session.admit(segment, rows, blob)
            self.assertEqual(session.metrics()["nodes"], 0)
            self.assertEqual(session.metrics()["edges"], 0)
            events = [json.loads(row[0]) for row in session.graph.db.execute(
                "SELECT event_json FROM live_forward_runtime_occurrence")]
            self.assertEqual(len(events), 2)
            self.assertEqual({event["cpu_id"] for event in events}, {"Z80"})
            self.assertEqual([event["native_sequence"] for event in sorted(
                events, key=lambda item: item["native_sequence"])], [10, 11])
        finally:
            session.close()

    def test_sideband_is_preserved_in_raw_input_but_skipped_from_graph_edges(self):
        rows = [
            (100, 50, 0, 0x100, 0x102, 0x4E71, 3, 0, 0, 0, 0, 0),
            (101, 50, 77, 0x100, 0x00F00010, 0x1234,
             FLAG_EVENT | (1 << EVENT_SHIFT), 0, 0, 0, 9, 0xABC),
            (102, 51, 0, 0x102, 0x104, 0x4E71, 3, 0, 0, 0, 0, 0),
        ]
        segment, records = _segment(rows)
        segment.update({"source_raw_sha256": "c" * 64,
                        "source_index_sha256": "d" * 64, "raw_offset": 1000})
        session = LiveForwardCartographer(ROM, INSTRUMENTATION)
        try:
            session.admit(segment, rows, records)
            self.assertEqual((session.metrics()["nodes"], session.metrics()["edges"]),
                             (2, 1))
            lineage = session.graph.db.execute(
                "SELECT lineage_json FROM live_forward_pending_lineage "
                "WHERE object_type='edge' LIMIT 1").fetchone()[0]
            lineage = json.loads(lineage)
            self.assertEqual(lineage["first_stream_sequence"], 100)
            self.assertEqual(lineage["next_stream_sequence_first"], 102)
            event = next(json.loads(item[0]) for item in session.graph.db.execute(
                "SELECT event_json FROM live_forward_runtime_occurrence")
                if json.loads(item[0])["event_kind"] == "BUS_READ")
            self.assertEqual((event["master_time"], event["reserved"], event["auxiliary"]),
                             (77, 9, 0xABC))
            self.assertEqual(event["record_hex"], struct.pack(
                "<QQQIIIHBBHHI", *rows[1]).hex())
            self.assertEqual(event["windows"][0]["source_offset"], 1000 + 48)
            self.assertEqual(event["windows"][0]["source_raw_sha256"], "c" * 64)
            self.assertEqual(event["windows"][0]["source_index_sha256"], "d" * 64)
            self.assertEqual((event["windows"][0]["entry_stream_sequence"],
                              event["windows"][0]["exit_stream_sequence"],
                              event["windows"][0]["record_count"]), (100, 103, 3))
            transition = next(json.loads(item[0]) for item in session.graph.db.execute(
                "SELECT event_json FROM live_forward_runtime_occurrence")
                if json.loads(item[0])["event_kind"] == "EXECUTED_NEXT")
            self.assertEqual(transition["windows"][0]["source_offset"], 1000)
        finally:
            session.close()

    def test_b_invalid_segment_never_enters_the_graph(self):
        rows = _rows([(0x100, 0x4E71, 3, 0x102)])
        segment, records = _segment(rows, valid=False)
        session = LiveForwardCartographer(ROM, INSTRUMENTATION)
        try:
            with self.assertRaisesRegex(ValueError, "READY_FOR_CARTOGRAPHER"):
                session.admit(segment, rows, records)
            self.assertEqual(session.metrics()["nodes"], 0)
            self.assertEqual(session.metrics()["segments_rejected"], 1)
        finally:
            session.close()

    def test_c_overlap_deduplicates_and_alternate_branch_is_preserved(self):
        session = LiveForwardCartographer(ROM, INSTRUMENTATION)
        chains = [
            [(0x100, 0x4E71, 3, 0x102), (0x102, 0x4E71, 3, 0x104),
             (0x104, 0x4E71, 3, 0x106)],
            [(0x102, 0x4E71, 3, 0x104), (0x104, 0x4E71, 3, 0x106),
             (0x106, 0x4E75, 3 | 8, 0x200)],
            [(0x100, 0x4E71, 3, 0x108), (0x108, 0x4E75, 3 | 8, 0x200)],
        ]
        try:
            for index, instructions in enumerate(chains, 1):
                rows = _rows(instructions, stream_start=index * 100,
                             instruction_start=index * 10)
                segment, records = _segment(rows, capture=index, generation=index)
                session.admit(segment, rows, records)
            metrics = session.metrics()
            self.assertEqual(metrics["nodes"], 5)
            self.assertEqual(metrics["edges"], 4)
            self.assertGreater(metrics["duplicate_structural_edges"], 0)
            self.assertEqual(metrics["branch_alternatives"], 1)
            branch_source = session.graph.db.execute(
                "SELECT source_id FROM map_edge WHERE source_id IN "
                "(SELECT source_id FROM map_edge GROUP BY source_id HAVING COUNT(*) > 1) "
                "LIMIT 1").fetchone()[0]
            self.assertEqual(session.graph.db.execute(
                "SELECT COUNT(DISTINCT target_id) FROM map_edge WHERE source_id=?",
                (branch_source,)).fetchone()[0], 2)
            with tempfile.TemporaryDirectory() as directory:
                session.save_closed(Path(directory) / "session.sqlite", 3)
                lineage = json.loads(session.graph.db.execute(
                    "SELECT e.lineage FROM map_edge e JOIN map_node s "
                    "ON s.node_id=e.source_id JOIN map_node t ON t.node_id=e.target_id "
                    "WHERE s.node_key='00000102:4E71' AND t.node_key='00000104:4E71'"
                ).fetchone()[0])
                self.assertEqual({item["capture_id"] for item in lineage}, {1, 2})
                self.assertTrue(all(item["profile"] == "FLOW_V1" for item in lineage))
        finally:
            session.close()

    def test_d_replayed_identical_segment_is_idempotent(self):
        rows = _rows([(0x100, 0x4E71, 3, 0x102), (0x102, 0x4E75, 3 | 8, 0x200)])
        segment, records = _segment(rows)
        session = LiveForwardCartographer(ROM, INSTRUMENTATION)
        try:
            session.admit(segment, rows, records)
            before = session.graph.graph_hash()
            replay = session.admit(segment, rows, records)
            self.assertEqual(replay["new_edges"], 0)
            self.assertEqual(before, session.graph.graph_hash())
        finally:
            session.close()

    def test_d1_per_segment_merge_skips_only_redundant_full_graph_hash(self):
        rows = _rows([(0x100, 0x4E71, 3, 0x102), (0x102, 0x4E75, 3 | 8, 0x200)])
        segment, records = _segment(rows)
        session = LiveForwardCartographer(ROM, INSTRUMENTATION)
        try:
            with mock.patch.object(session.graph, "graph_hash",
                                   wraps=session.graph.graph_hash) as graph_hash:
                session.admit(segment, rows, records)
                graph_hash.assert_not_called()
            self.assertIsNone(session.metrics()["graph_hash"])
            with tempfile.TemporaryDirectory() as directory:
                saved = session.save_closed(Path(directory) / "session.sqlite", 1)
            self.assertTrue(saved["graph_hash"])
            self.assertEqual(saved["graph_hash"], session.metrics()["graph_hash"])
        finally:
            session.close()

    def test_e_ram_is_fresh_and_backup_preserves_graph_hash_and_lineage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            master = Cartographer(root / "old-master.sqlite", ROM)
            master.close()
            session = LiveForwardCartographer(ROM, INSTRUMENTATION, "ram-only")
            try:
                self.assertEqual(session.metrics()["nodes"], 0)
                rows = _rows([(0x100, 0x4E71, 3, 0x102), (0x102, 0x4E75, 3 | 8, 0x200)])
                segment, records = _segment(rows)
                session.admit(segment, rows, records)
                saved = session.save_closed(root / "session.sqlite", 1)
                self.assertEqual(saved["graph_hash"], session.graph.graph_hash())
                graph = Cartographer(root / "session.sqlite", ROM)
                try:
                    self.assertEqual(graph.graph_hash(), saved["graph_hash"])
                    lineage = json.loads(graph.db.execute(
                        "SELECT lineage FROM map_edge").fetchone()[0])[0]
                    for key in ("run_id", "epoch", "worker_id", "capture_id", "generation",
                                "segment_sha256", "entry_stream_sequence",
                                "exit_instruction_sequence", "profile"):
                        self.assertIn(key, lineage)
                finally:
                    graph.close()
            finally:
                session.close()

    def test_f_archivist_seeds_missing_master_then_merges_and_replays_idempotently(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            master, first, second = root / "master.sqlite", root / "first.sqlite", root / "second.sqlite"
            first_saved = _save_session(first, [(0x100, 0x4E71, 3, 0x102),
                                                (0x102, 0x4E71, 3, 0x104)], run=1)
            seeded = archive_session(master, first, ROM)
            self.assertEqual(seeded["mode"], "SEED")
            self.assertIsNone(seeded["warning"])
            self.assertEqual(seeded["master_graph_hash_after"], first_saved["graph_hash"])
            old_nodes, old_edges, _, _ = _ids(master)
            second_saved = _save_session(second, [(0x102, 0x4E71, 3, 0x104),
                                                  (0x104, 0x4E75, 3 | 8, 0x200)], run=2)
            merged = archive_session(master, second, ROM)
            new_nodes, new_edges, _, merged_hash = _ids(master)
            self.assertEqual(merged["mode"], "MERGE")
            self.assertTrue(old_nodes <= new_nodes and old_edges <= new_edges)
            self.assertGreater(len(new_edges), len(old_edges))
            self.assertTrue(_ids(second)[0] <= new_nodes)
            replay = archive_session(master, second, ROM)
            self.assertEqual(replay["master_graph_hash_after"], merged_hash)
            self.assertEqual(second_saved["graph_hash"], replay["session_graph_hash"])

    def test_g_observed_status_stays_observed_and_existing_proven_survives(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            session_path, master_path = root / "session.sqlite", root / "master.sqlite"
            rows = _rows([(0x100, 0x4E71, 3, 0x102), (0x102, 0x4E75, 3 | 8, 0x200)])
            segment, records = _segment(rows)
            session = LiveForwardCartographer(ROM, INSTRUMENTATION, "proof-status")
            session.admit(segment, rows, records)
            bundle = session.graph.export_bundle()
            session.save_closed(session_path, 1)
            session.close()
            graph = Cartographer(master_path, ROM)
            try:
                for node in bundle["nodes"]:
                    node["status"] = "PROVEN"
                for edge in bundle["edges"]:
                    edge["status"] = "PROVEN"
                graph.merge(bundle, "seed-proven", ROM)
            finally:
                graph.close()
            archive_session(master_path, session_path, ROM)
            graph = Cartographer(master_path, ROM)
            try:
                self.assertEqual(graph.db.execute(
                    "SELECT COUNT(*) FROM map_edge WHERE status='PROVEN'").fetchone()[0], 1)
                self.assertEqual(graph.db.execute(
                    "SELECT COUNT(*) FROM map_edge WHERE status='OBSERVED'").fetchone()[0], 0)
            finally:
                graph.close()
            observed_master = root / "observed.sqlite"
            archive_session(observed_master, session_path, ROM)
            graph = Cartographer(observed_master, ROM)
            try:
                self.assertEqual(graph.db.execute(
                    "SELECT DISTINCT status FROM map_edge").fetchone()[0], "OBSERVED")
            finally:
                graph.close()

    def test_h_rom_schema_and_conflict_failures_preserve_existing_master(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            session_path = root / "session.sqlite"
            _save_session(session_path, [(0x100, 0x4E71, 3, 0x102),
                                         (0x102, 0x4E75, 3 | 8, 0x200)], run=9)
            with self.assertRaisesRegex(ValueError, "ROM_MISMATCH"):
                archive_session(root / "none.sqlite", session_path, "c" * 64)
            wrong_rom = Cartographer(root / "wrong-rom.sqlite", "c" * 64)
            wrong_rom.close()
            before = (root / "wrong-rom.sqlite").read_bytes()
            with self.assertRaisesRegex(ValueError, "ROM_MISMATCH"):
                archive_session(root / "wrong-rom.sqlite", session_path, ROM)
            self.assertEqual((root / "wrong-rom.sqlite").read_bytes(), before)
            db = sqlite3.connect(session_path)
            try:
                db.execute("UPDATE map_meta SET value='wrong.schema' WHERE key='schema'")
                db.commit()
            finally:
                db.close()
            with self.assertRaisesRegex(ValueError, "SCHEMA_MISMATCH"):
                archive_session(root / "bad-schema.sqlite", session_path, ROM)

            conflicted_session = root / "conflict-session.sqlite"
            valid_session = _save_session(conflicted_session,
                [(0x300, 0x4E71, 3, 0x302)], run=10)
            conflict_master = root / "conflict-master.sqlite"
            rows = _rows([(0x300, 0x4E71, 3, 0x302)])
            segment, records = _segment(rows, run=11)
            candidate = LiveForwardCartographer(ROM, INSTRUMENTATION, "conflict-source")
            candidate.admit(segment, rows, records)
            conflicting = candidate.graph.export_bundle()
            candidate.close()
            conflicting["nodes"][0]["attributes"]["pc"] += 1
            graph = Cartographer(conflict_master, ROM)
            graph.merge(conflicting, "conflicting-seed", ROM)
            graph.close()
            before = hashlib.sha256(conflict_master.read_bytes()).hexdigest()
            with self.assertRaisesRegex(ValueError, "MERGE_CONFLICT"):
                archive_session(conflict_master, conflicted_session, ROM)
            after = hashlib.sha256(conflict_master.read_bytes()).hexdigest()
            self.assertEqual(after, before)
            self.assertEqual(valid_session["segments_admitted"], 1)
            self.assertTrue(conflict_master.exists())


if __name__ == "__main__":
    unittest.main()
