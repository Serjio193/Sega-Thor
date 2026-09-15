import hashlib
import os
import sqlite3
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0, str(ROOT / "src/tools"))
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))

import map_merge
from auto67_cartographer import candidate_bundle, local_chain
from auto67_persistence import LiveMapSink
from auto67_transport import PreDispatchTransport
from cartographer import Cartographer


def _step(producer, consumer, register="A5"):
    return {"kind": "REGISTER_REACHING_DEFINITION", "register": register,
            "producer_pc": producer, "consumer_pc": consumer,
            "producer_occurrence": {"epoch": 1, "sequence": 3},
            "consumer_occurrence": {"epoch": 1, "sequence": 8},
            "evidence": {"complete_interval": True,
                         "intervening_register_write": False}}


def _chain(producer="0x2234", consumer="0x27EC"):
    return local_chain({"epoch": 1, "seq": 8, "occurrence_id": "epoch=1:seq=8",
                        "window_item_id": 8},
                       {"chain_steps": [_step(producer, consumer)]}, "INV", "LEASE")


def _write_bundle(path, bundle, rom="rom"):
    graph = Cartographer(path, rom)
    try:
        graph.merge(bundle, "seed:" + graph.graph_hash(), rom)
    finally:
        graph.close()


def _save_session(path, chains, rom="rom"):
    graph = Cartographer.in_memory(rom)
    try:
        for chain in chains:
            bundle, stable = candidate_bundle(chain)
            graph.merge(bundle, "auto67-live:" + stable, rom)
        target = sqlite3.connect(path)
        try:
            graph.db.backup(target)
            target.commit()
        finally:
            target.close()
    finally:
        graph.close()


def _hash(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


class RamSessionMapTest(unittest.TestCase):
    def test_a_sink_keeps_global_closed_until_offline_merge(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            global_path = root / "global.sqlite"
            seed, _ = candidate_bundle(_chain("0x100", "0x200"))
            _write_bundle(global_path, seed)
            before = _hash(global_path)
            sink = LiveMapSink(global_path, source_sha256="rom")
            sink.start()
            self.assertTrue(sink.submit(_chain("0x300", "0x400")))
            sink.stop()
            self.assertEqual(_hash(global_path), before)
            self.assertEqual(sink.snapshot()["session_save_status"], "PASS")
            self.assertTrue(Path(sink.snapshot()["session_map"]).exists())
            self.assertFalse(global_path.with_name("global.merge.tmp.sqlite").exists())

    def test_b_empty_global_merge_is_positive(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            session = root / "session.sqlite"
            bundle, _ = candidate_bundle(_chain())
            _save_session(session, [_chain()])
            result = map_merge.merge_session_map(root / "global.sqlite", session)
            self.assertEqual(result["status"], "PASS")
            session_graph = Cartographer(session, "rom")
            try:
                self.assertEqual(result["session_graph_hash"], session_graph.graph_hash())
            finally:
                session_graph.close()
            self.assertEqual(result["global_merge_delta"]["new_edges"], 1)
            self.assertTrue((root / "global.sqlite").exists())

    def test_c_mixed_old_new_and_duplicate_global_are_idempotent(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            global_path, session = root / "global.sqlite", root / "session.sqlite"
            old, _ = candidate_bundle(_chain("0x100", "0x200"))
            _write_bundle(global_path, old)
            _save_session(session, [_chain("0x100", "0x200"), _chain("0x300", "0x400")])
            first = map_merge.merge_session_map(global_path, session)
            self.assertEqual(first["global_merge_delta"]["new_edges"], 1)
            second = map_merge.merge_session_map(global_path, session)
            self.assertEqual(second["global_merge_delta"]["new_nodes"], 0)
            self.assertEqual(second["global_merge_delta"]["new_edges"], 0)
            self.assertEqual(first["global_graph_hash_after"], second["global_graph_hash_after"])
            graph = Cartographer(global_path, "rom")
            try:
                self.assertEqual(graph.db.execute("SELECT COUNT(*) FROM map_import").fetchone()[0], 1)
            finally:
                graph.close()

    def test_d_failed_replace_leaves_global_byte_exact(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            global_path, session = root / "global.sqlite", root / "session.sqlite"
            _write_bundle(global_path, candidate_bundle(_chain())[0])
            _save_session(session, [_chain("0x300", "0x400")])
            before = _hash(global_path)
            with mock.patch.object(map_merge.os, "replace", side_effect=OSError("blocked")):
                with self.assertRaises(OSError):
                    map_merge.merge_session_map(global_path, session)
            self.assertEqual(_hash(global_path), before)
            self.assertTrue(session.exists())

    def test_e_ram_and_disk_graphs_have_identical_semantics(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "disk.sqlite"
            chain = _chain()
            bundle, stable = candidate_bundle(chain)
            ram = Cartographer.in_memory("rom")
            disk = Cartographer(path, "rom")
            try:
                ram.merge(bundle, "auto67-live:" + stable, "rom")
                disk.merge(bundle, "auto67-live:" + stable, "rom")
                self.assertEqual(ram.metrics(), disk.metrics())
                self.assertEqual(ram.export_bundle(), disk.export_bundle())
            finally:
                ram.close()
                disk.close()

    def test_f_shutdown_after_final_save_is_bounded(self):
        with tempfile.TemporaryDirectory() as directory:
            sink = LiveMapSink(Path(directory) / "global.sqlite", source_sha256="rom")
            sink.start()
            sink.submit(_chain())
            started = time.monotonic()
            sink.stop()
            self.assertLess(time.monotonic() - started, 2.0)
            self.assertIsNotNone(sink.thread)
            self.assertFalse(sink.thread.is_alive())

    def test_g_state_option_audit_is_mechanical(self):
        runner = (ROOT / "src/tools/thor_evidence/auto67_runner.py").read_text(encoding="utf-8")
        live = (ROOT / "src/tools/thor_evidence/capture/live_opportunistic.lua").read_text(
            encoding="utf-8")
        self.assertNotIn("--state", runner)
        self.assertNotIn("OASIS_LIVE_STATE", runner)
        self.assertNotIn("OASIS_LIVE_STATE", live)

    def test_h_runner_orders_final_ingest_stop_then_merge(self):
        source = (ROOT / "src/tools/thor_evidence/auto67_runner.py").read_text(encoding="utf-8")
        self.assertIn("for event in transport.consume(lua_final):", source)
        self.assertLess(source.index("dispatcher.ingest(event)"), source.index("dispatcher.stop()"))
        self.assertLess(source.index("dispatcher.stop()"), source.index("map_sink.stop()"))
        self.assertLess(source.index("map_sink.stop()"), source.index("merge_session_map("))
        sink_source = (ROOT / "src/tools/thor_evidence/auto67_persistence.py").read_text(
            encoding="utf-8")
        self.assertIn("Cartographer.in_memory(self.source_sha256)", sink_source)
        self.assertNotIn("Cartographer(self.map_db", sink_source)

    def test_i_final_only_transport_events_are_ingested_once(self):
        transport = PreDispatchTransport()
        ingested = []
        periodic = {"epoch": 1, "events": [{"epoch": 1, "seq": n,
                   "occurrence_id": f"epoch=1:seq={n}"} for n in (18, 19, 20)]}
        final = {"epoch": 1, "events": [{"epoch": 1, "seq": n,
                "occurrence_id": f"epoch=1:seq={n}"} for n in (19, 20, 21, 22)]}
        ingested.extend(event["seq"] for event in transport.consume(periodic))
        ingested.extend(event["seq"] for event in transport.consume(final))
        self.assertEqual(ingested, [18, 19, 20, 21, 22])


if __name__ == "__main__":
    unittest.main()
