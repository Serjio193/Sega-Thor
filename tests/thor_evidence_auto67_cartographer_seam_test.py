import json
import queue
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
from auto67_cartographer import (LOCAL_CHAIN_SCHEMA, PROOF_CONTRACT,
                                 candidate_bundle, local_chain)
from auto67_persistence import LivePersistenceSink, chain_descriptor
from cartographer import Cartographer


def step(producer="0x002234", consumer="0x0027EC", register="A5",
         complete=True, intervening=False, epoch=1):
    return {
        "kind": "REGISTER_REACHING_DEFINITION", "register": register,
        "producer_pc": producer, "consumer_pc": consumer,
        "producer_occurrence": {"epoch": epoch, "sequence": 3, "frame": 30},
        "consumer_occurrence": {"epoch": epoch, "sequence": 8, "frame": 80},
        "evidence": {"complete_interval": complete,
                     "intervening_register_write": intervening},
    }


def chain(item, investigation="INV-1", lease="L-1", steps=None):
    return local_chain({"epoch": 1, "seq": 8, "occurrence_id": item,
                        "window_item_id": 8, "frame": 80},
                       {"chain_steps": steps if steps is not None else [step()]},
                       investigation, lease)


class Auto67CartographerSeamTest(unittest.TestCase):
    def test_a_worker_result_is_local_chain_without_global_classification(self):
        result = chain("epoch=1:seq=8")
        self.assertEqual(result["local_chain_schema"], LOCAL_CHAIN_SCHEMA)
        self.assertIn("chain_steps", result)
        text = json.dumps(result)
        for forbidden in ("NEW", "DUPLICATE", "KNOWN", "CONFLICT", "MAP_DELTA"):
            self.assertNotIn(forbidden, text)

    def test_b_valid_reaching_definition_emits_stable_proven_bundle(self):
        bundle, stable_hash = candidate_bundle(chain("epoch=1:seq=8"))
        self.assertTrue(stable_hash)
        self.assertEqual(len(bundle["nodes"]), 2)
        self.assertEqual(bundle["edges"][0]["relation"],
                         "REGISTER_REACHING_DEFINITION:A5")
        self.assertEqual(bundle["edges"][0]["status"], "PROVEN")
        self.assertEqual(bundle["frontiers"], [])
        self.assertEqual(bundle["resolves_frontiers"], [])

    def test_b_multi_step_bundle_keeps_all_valid_edges_and_nodes(self):
        steps = [step(producer="0x2234", consumer="0x27EC", register="A5"),
                 step(producer="0x27BE", consumer="0x27EC", register="A4")]
        bundle, _ = candidate_bundle(chain("epoch=1:seq=8", steps=steps))
        self.assertEqual(len(bundle["nodes"]), 3)
        self.assertEqual(len(bundle["edges"]), 2)
        self.assertEqual(
            {edge["relation"] for edge in bundle["edges"]},
            {"REGISTER_REACHING_DEFINITION:A4", "REGISTER_REACHING_DEFINITION:A5"})

    def test_c_incomplete_intervening_and_malformed_steps_are_not_promoted(self):
        for item in (step(complete=False), step(intervening=True),
                     {"kind": "REGISTER_REACHING_DEFINITION"},
                     step(register="D0"), step(epoch=1) | {
                         "consumer_occurrence": {"epoch": 2, "sequence": 8}}):
            self.assertIsNone(candidate_bundle({"chain_steps": [item]}))

    def test_d_first_merge_positive_and_exact_replay_zero(self):
        with tempfile.TemporaryDirectory() as directory:
            graph = Cartographer(Path(directory) / "map.sqlite", "rom")
            try:
                bundle, stable_hash = candidate_bundle(chain("epoch=1:seq=8"))
                first = graph.merge(bundle, "auto67-live:" + stable_hash, "rom")
                before = graph.graph_hash()
                replay = graph.merge(bundle, "auto67-live:" + stable_hash, "rom")
                self.assertGreater(first.new_nodes + first.new_edges, 0)
                self.assertEqual(replay.new_nodes + replay.new_edges, 0)
                self.assertEqual(before, replay.graph_hash)
            finally:
                graph.close()

    def test_e_different_occurrence_same_chain_has_zero_delta(self):
        first, first_hash = candidate_bundle(chain("epoch=1:seq=8", "INV-1", "L-1"))
        second, second_hash = candidate_bundle(chain("epoch=1:seq=80", "INV-2", "L-2"))
        self.assertEqual(first_hash, second_hash)
        self.assertEqual(first["nodes"], second["nodes"])
        self.assertEqual(first["edges"], second["edges"])

    def test_f_distinct_register_or_pc_is_new_knowledge(self):
        first = chain("epoch=1:seq=8")
        distinct = local_chain(first["occurrence"], {"chain_steps": [step(register="A4")]},
                               "INV-2", "L-2")
        with tempfile.TemporaryDirectory() as directory:
            graph = Cartographer(Path(directory) / "map.sqlite", "rom")
            try:
                b1, h1 = candidate_bundle(first)
                b2, h2 = candidate_bundle(distinct)
                graph.merge(b1, "auto67-live:" + h1, "rom")
                delta = graph.merge(b2, "auto67-live:" + h2, "rom")
                self.assertNotEqual(h1, h2)
                self.assertGreater(delta.new_edges, 0)
            finally:
                graph.close()

    def test_g_unproven_local_chain_is_retained_without_map_growth(self):
        result = local_chain({"epoch": 1, "seq": 8, "occurrence_id": "epoch=1:seq=8"},
                             {"chain_steps": [step(complete=False)]}, "INV-1", "L-1")
        self.assertEqual(result["chain_steps"][0]["evidence"]["complete_interval"], False)
        self.assertIsNone(candidate_bundle(result))

    def test_g_mixed_validity_keeps_invalid_step_local(self):
        valid = step(register="A5")
        invalid = step(producer="0x27BE", register="A4", complete=False)
        result = chain("epoch=1:seq=8", steps=[valid, invalid])
        bundle, _ = candidate_bundle(result)
        self.assertEqual(len(bundle["edges"]), 1)
        self.assertEqual(len(result["chain_steps"]), 2)
        self.assertEqual(bundle["frontiers"], [])

    def test_g_order_independent_complete_hash_and_import(self):
        a4 = step(producer="0x27BE", consumer="0x27EC", register="A4")
        a5 = step(producer="0x2234", consumer="0x27EC", register="A5")
        first, first_hash = candidate_bundle(chain("epoch=1:seq=8", steps=[a4, a5]))
        second, second_hash = candidate_bundle(chain("epoch=9:seq=88", steps=[a5, a4]))
        self.assertEqual(first, second)
        self.assertEqual(first_hash, second_hash)
        self.assertEqual("auto67-live:" + first_hash, "auto67-live:" + second_hash)

    def test_h_repeated_stable_chain_has_no_occurrence_bloat(self):
        with tempfile.TemporaryDirectory() as directory:
            graph = Cartographer(Path(directory) / "map.sqlite", "rom")
            try:
                for number in range(10):
                    item, stable_hash = candidate_bundle(
                        chain(f"epoch={number + 1}:seq={number + 8}",
                              f"INV-{number}", f"L-{number}"))
                    graph.merge(item, "auto67-live:" + stable_hash, "rom")
                metrics = graph.metrics()
                self.assertEqual((metrics["nodes"], metrics["edges"]), (2, 1))
                self.assertEqual(graph.db.execute("SELECT COUNT(*) FROM map_import").fetchone()[0], 1)
                self.assertEqual(metrics["open_frontiers"], 0)
            finally:
                graph.close()

    def test_i_sink_supports_map_only_and_compatibility_modes(self):
        item = chain_descriptor({"kind": "RAM_WRITE", "pc": "0x100", "address": "0x200"},
                                "BOUNDED_UNRESOLVED", 8, 0, "L-1", "INV-1")
        item["local_chain"] = chain("epoch=1:seq=8")
        with tempfile.TemporaryDirectory() as directory:
            sink = LivePersistenceSink(None, map_db=Path(directory) / "map-only.sqlite",
                                       source_sha256="rom")
            sink.start()
            self.assertTrue(sink.submit(item))
            sink.stop()
            snapshot = sink.snapshot()
            self.assertEqual(snapshot["map_new_edges"], 1)
            self.assertEqual(sink.snapshot()["map_write_errors"], 0)
            self.assertTrue(snapshot["graph_hash"])

            legacy = LivePersistenceSink(Path(directory) / "legacy.sqlite",
                                         map_db=Path(directory) / "compatible-map.sqlite",
                                         source_sha256="rom")
            legacy.start()
            self.assertTrue(legacy.submit(item))
            legacy.stop()
            self.assertEqual(legacy.snapshot()["map_new_edges"], 1)
            self.assertGreaterEqual(legacy.snapshot()["persisted"], 1)

    def test_i_no_proof_is_not_counted_as_map_drop(self):
        item = chain_descriptor({"kind": "RAM_WRITE", "pc": "0x100"},
                                "BOUNDED_UNRESOLVED", 8)
        item["local_chain"] = local_chain(
            {"epoch": 1, "seq": 8, "occurrence_id": "epoch=1:seq=8"},
            {"chain_steps": [step(complete=False)]}, "INV-1", "L-1")
        with tempfile.TemporaryDirectory() as directory:
            sink = LivePersistenceSink(None, map_db=Path(directory) / "map.sqlite",
                                        source_sha256="rom")
            sink.start()
            self.assertTrue(sink.submit(item))
            sink.stop()
            snapshot = sink.snapshot()
            self.assertEqual(snapshot["chains_without_accepted_proof"], 1)
            self.assertEqual(snapshot["map_fragments_dropped"], 0)

    def test_j_full_queue_counts_local_chain_drop_without_backlog(self):
        item = chain_descriptor({"kind": "RAM_WRITE", "pc": "0x100"},
                                "BOUNDED_UNRESOLVED", 8)
        item["local_chain"] = chain("epoch=1:seq=8")
        sink = LivePersistenceSink(None)
        sink.items = queue.Queue(maxsize=1)
        self.assertTrue(sink.submit(item))
        self.assertFalse(sink.submit(item))
        self.assertEqual(sink.snapshot()["map_fragments_dropped"], 1)

    def test_k_worker_path_has_no_cartographer_or_map_lookup(self):
        source = (ROOT / "src/tools/thor_evidence/auto67_live.py").read_text(encoding="utf-8")
        self.assertNotIn("map_lookup", source)
        self.assertNotIn("Cartographer", source)
        self.assertNotIn("MAP_DELTA", source)

    def test_l_existing_sink_owns_the_only_output_queue_and_writer(self):
        source = (ROOT / "src/tools/thor_evidence/auto67_persistence.py").read_text(
            encoding="utf-8")
        self.assertEqual(source.count("queue.Queue("), 1)
        self.assertEqual(source.count('name="auto67-chain-writer"'), 1)
        self.assertNotIn("check_same_thread=False", source)
        self.assertNotIn("self.cartographer", source)
        snapshot_start = source.index("    def snapshot(self)")
        self.assertNotIn("cartographer.graph_hash()", source[snapshot_start:])

    def test_m_cartographer_is_constructed_and_closed_in_writer(self):
        source = (ROOT / "src/tools/thor_evidence/auto67_persistence.py").read_text(
            encoding="utf-8")
        run_start = source.index("    def _run(self)")
        run_end = source.index("    def stop(self)")
        run_source = source[run_start:run_end]
        self.assertIn("cartographer = Cartographer(self.map_db", run_source)
        self.assertIn("cartographer.merge(", run_source)
        self.assertIn("cartographer.close()", run_source)
        runner = (ROOT / "src/tools/thor_evidence/auto67_runner.py").read_text(
            encoding="utf-8")
        self.assertNotIn("Cartographer(", runner)


if __name__ == "__main__":
    unittest.main()
