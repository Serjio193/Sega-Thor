"""Deterministic MAP-1 → Archivist → canonical-map contract tests (2G)."""

from __future__ import annotations

import hashlib
import json
import sqlite3
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "src/tools"), str(ROOT / "src/tools/thor_evidence")]

import rom_knowledge_pipeline as pipeline
from cartographer import Cartographer, canonical, digest
from live_forward_archivist import archive_session
from rom_knowledge_live_import import KnowledgeImportStop, import_archivist_session
from rom_knowledge_map import KnowledgeStore, runtime_occurrence_id


ROM = bytearray(0x100)
ROM[0x10:0x16] = bytes.fromhex("4e714e714e75")
ROM[0x80:0x82] = bytes.fromhex("0010")
ROM[0x82:0x84] = bytes.fromhex("0002")
ROM[0x84:0x88] = bytes.fromhex("00000012")
ROM[0x88:0x8C] = bytes.fromhex("00000010")
ROM[0x8C:0x90] = bytes.fromhex("00000012")
ROM_BYTES = bytes(ROM)
ROM_SHA = hashlib.sha256(ROM_BYTES).hexdigest()
INSTRUMENTATION = "b" * 64


def _lineage(run_id: int, capture: int = 1, generation: int = 1) -> dict[str, int]:
    return {"run_id": run_id, "epoch": 1, "worker_id": 0,
            "capture_id": capture, "generation": generation, "occurrence_count": 1}


def _node(kind: str, key: str, scope: str, attrs: dict, lineage: dict) -> dict:
    return {"kind": kind, "key": key, "scope": scope, "status": "OBSERVED",
            "attributes": attrs, "lineage": [lineage]}


def _instruction(offset: int, run_id: int, capture: int) -> tuple[dict, dict]:
    data = ROM_BYTES[offset:offset + 2]
    opcode = int.from_bytes(data, "big")
    lineage = _lineage(run_id, capture)
    node = _node("M68K_INSTRUCTION", f"{offset:08X}:{opcode:04X}",
        "rom:" + ROM_SHA, {"cpu": "M68K", "pc": offset, "opcode": opcode}, lineage)
    range_attrs = {"rom_sha256": ROM_SHA, "start_offset": offset,
        "end_offset_exclusive": offset + 2, "length": 2, "opcode": opcode,
        "bytes_hex": data.hex().upper(), "bytes_sha256": hashlib.sha256(data).hexdigest()}
    range_node = _node("ROM_INSTRUCTION_RANGE",
        f"{offset:08X}:{offset + 2:08X}:{data.hex().upper()}", ROM_SHA,
        range_attrs, lineage)
    return node, range_node


def _edge(source: dict, target: dict, relation: str, run_id: int,
          capture: int = 1, generation: int = 1) -> dict:
    return {"source": digest({"kind": source["kind"], "key": source["key"],
                              "scope": source["scope"]}),
        "target": digest({"kind": target["kind"], "key": target["key"],
                          "scope": target["scope"]}),
        "relation": relation, "scope": ROM_SHA, "status": "OBSERVED",
        "rule": "2G deterministic fixture", "assumptions": [],
        "lineage": [_lineage(run_id, capture, generation)]}


def _make_session(path: Path, session_id: str, run_id: int,
                  instructions: tuple[int, ...] = (0x10, 0x12),
                  terminal: int | None = 0x14, control: tuple[str, ...] = (),
                  exception: bool = False, unsupported: bool = False,
                  conflict: bool = False, source_owned: int = 0) -> Path:
    graph = Cartographer.in_memory(ROM_SHA)
    scope = "rom:" + ROM_SHA
    lineage = _lineage(run_id)
    nodes: list[dict] = []
    edges: list[dict] = []
    instruction_nodes: dict[int, dict] = {}
    range_nodes: dict[int, dict] = {}
    for offset in instructions:
        ins, ran = _instruction(offset, run_id, 1)
        nodes.extend((ins, ran))
        instruction_nodes[offset], range_nodes[offset] = ins, ran
        edges.append(_edge(ins, ran, "EXECUTED_FROM_ROM", run_id))
    for left, right in zip(instructions, instructions[1:]):
        edges.append(_edge(instruction_nodes[left], instruction_nodes[right],
                           "EXECUTED_NEXT", run_id))
    if terminal is not None:
        target = _node("M68K_TARGET_ADDRESS", f"{terminal:08X}", scope,
                       {"raw_next_pc": terminal}, lineage)
        nodes.append(target)
        edges.append(_edge(instruction_nodes[instructions[-1]], target,
                           "OBSERVED_NEXT_PC", run_id))
    for relation in control:
        sources = {
            "OBSERVED_CODE_POINTER_TO": ("ROM_CODE_POINTER_SOURCE", 0x80, 0x82, "pointer"),
            "OBSERVED_CODE_OFFSET_TO": ("ROM_CODE_OFFSET_SOURCE", 0x82, 0x84, "offset"),
            "OBSERVED_JUMP_TABLE_ENTRY_TO": ("ROM_JUMP_TABLE_ENTRY", 0x84, 0x88, "table"),
        }
        kind, start, end, key = sources[relation]
        attrs = {"rom_sha256": ROM_SHA, "start_offset": start,
            "end_offset_exclusive": end, "bytes_hex": ROM_BYTES[start:end].hex().upper(),
            "bytes_sha256": hashlib.sha256(ROM_BYTES[start:end]).hexdigest()}
        if relation == "OBSERVED_JUMP_TABLE_ENTRY_TO":
            attrs.update({"selected": True, "selected_index": 0})
        source = _node(kind, f"{start:08X}:{end:08X}:{key}", ROM_SHA, attrs, lineage)
        nodes.append(source)
        if relation == "OBSERVED_JUMP_TABLE_ENTRY_TO":
            unused_start, unused_end = 0x8C, 0x90
            unused_attrs = {"rom_sha256": ROM_SHA, "start_offset": unused_start,
                "end_offset_exclusive": unused_end,
                "bytes_hex": ROM_BYTES[unused_start:unused_end].hex().upper(),
                "bytes_sha256": hashlib.sha256(ROM_BYTES[unused_start:unused_end]).hexdigest(),
                "selected": False, "selected_index": 1}
            nodes.append(_node(kind, f"{unused_start:08X}:{unused_end:08X}:table-unused",
                               ROM_SHA, unused_attrs, lineage))
        edges.append(_edge(source, range_nodes[instructions[-1]], relation, run_id))
    if exception:
        event = _node("M68K_EXCEPTION_EVENT", "event:1", scope,
            {"pc": instructions[-1], "next_pc": instructions[0]}, lineage)
        nodes.append(event)
        edges.extend((_edge(instruction_nodes[instructions[-1]], event,
                           "EXECUTED_NEXT", run_id),
                      _edge(event, instruction_nodes[instructions[0]],
                           "EXECUTED_NEXT", run_id)))
    if unsupported:
        edges.append(_edge(instruction_nodes[instructions[0]], instruction_nodes[instructions[-1]],
                           "UNSUPPORTED_RUNTIME_FACT", run_id))
    graph.merge({"nodes": nodes, "edges": edges, "frontiers": [],
                 "resolves_frontiers": []}, "fixture:" + session_id, ROM_SHA)
    graph.db.execute("CREATE TABLE live_forward_runtime_occurrence("
        "occurrence_id TEXT PRIMARY KEY,event_json TEXT NOT NULL)")
    capture_scope = f"native-run:{run_id}:epoch:1"
    runtime_events = []
    for index, offset in enumerate(instructions):
        node_id = graph._node_id(instruction_nodes[offset])
        event = {"capture_id": capture_scope, "capture_ids": [1], "run_id": run_id,
            "epoch": 1, "cpu_id": "M68K", "address_space": "FLOW_DOMAIN_0",
            "native_sequence": 100 + index, "instruction_sequence": 50 + index,
            "event_kind": "INSTRUCTION", "pc": offset,
            "address": instructions[index + 1] if index + 1 < len(instructions) else
                (terminal if terminal is not None else offset + 2),
            "value": int.from_bytes(ROM_BYTES[offset:offset + 2], "big"), "width": 2,
            "flags": 1, "instruction_node_id": node_id, "edge_id": None,
            "target_node_id": None, "windows": [{"worker_id": 0, "capture_id": 1,
                "generation": 1, "segment_sha256": "c" * 64}]}
        event["occurrence_id"] = runtime_occurrence_id(capture_id=capture_scope,
            epoch=1, cpu="M68K", address_space="FLOW_DOMAIN_0",
            native_sequence=100 + index, event_kind="INSTRUCTION", run_id=run_id,
            instruction_sequence=50 + index)
        runtime_events.append(event)
    for index, (left, right) in enumerate(zip(instructions, instructions[1:])):
        source, target = graph._node_id(instruction_nodes[left]), graph._node_id(instruction_nodes[right])
        edge_id = graph.db.execute("SELECT edge_id FROM map_edge WHERE source_id=? "
            "AND target_id=? AND relation='EXECUTED_NEXT'", (source, target)).fetchone()[0]
        event = {"capture_id": capture_scope, "capture_ids": [1], "run_id": run_id,
            "epoch": 1, "cpu_id": "M68K", "address_space": "FLOW_DOMAIN_0",
            "native_sequence": 100 + index, "instruction_sequence": 50 + index,
            "event_kind": "EXECUTED_NEXT", "pc": left, "address": right,
            "value": int.from_bytes(ROM_BYTES[left:left + 2], "big"), "width": 2,
            "flags": 1, "instruction_node_id": source, "edge_id": edge_id,
            "target_node_id": target, "windows": [{"worker_id": 0, "capture_id": 1,
                "generation": 1, "segment_sha256": "c" * 64}]}
        event["occurrence_id"] = runtime_occurrence_id(capture_id=capture_scope,
            epoch=1, cpu="M68K", address_space="FLOW_DOMAIN_0",
            native_sequence=100 + index, event_kind="EXECUTED_NEXT", run_id=run_id,
            instruction_sequence=50 + index)
        runtime_events.append(event)
    graph.db.executemany("INSERT INTO live_forward_runtime_occurrence VALUES (?,?)",
        [(event["occurrence_id"], canonical(event)) for event in runtime_events])
    if conflict:
        graph._record_conflict("node", "fixture-conflict", ["left", "right"], [lineage])
    now = "2026-09-18T00:00:00Z"
    metadata = {"live_forward_session_schema": "oasis.m12.live-forward-session.v1",
        "live_forward_session_id": session_id, "live_forward_instrumentation_identity": INSTRUMENTATION,
        "live_forward_run_id": str(run_id), "live_forward_created_utc": now,
        "live_forward_closed_utc": now, "live_forward_session_state": "CLOSED",
        "live_forward_graph_sha256": graph.graph_hash(), "source_owned_bytes": str(source_owned)}
    graph.db.executemany("INSERT OR REPLACE INTO map_meta VALUES (?,?)", metadata.items())
    graph.db.commit()
    target_db = sqlite3.connect(path)
    try:
        graph.db.backup(target_db)
    finally:
        target_db.close()
        graph.close()
    return path


def _base_knowledge(path: Path) -> Path:
    store = KnowledgeStore(path, ROM_SHA, len(ROM_BYTES))
    try:
        store.db.execute("INSERT INTO emission VALUES (?,?,?,?,?,?,?,?)",
            (0, len(ROM_BYTES), "INCBIN", "UNKNOWN", "UNKNOWN", 0, "blob", "fixture.bin"))
        store.db.commit()
    finally:
        store.close()
    return path


class ArchivistCanonicalPipelineTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.rom_path = self.root / "fixture.rom"
        self.rom_path.write_bytes(ROM_BYTES)
        self.master = self.root / "bootstrap-master.sqlite"
        self.knowledge = _base_knowledge(self.root / "knowledge.sqlite")
        self.output = self.root / "published"

    def tearDown(self):
        self.temp.cleanup()

    def session(self, name: str, run_id: int, **kwargs) -> Path:
        return _make_session(self.root / (name + ".sqlite"), name, run_id, **kwargs)

    def run_pipeline(self, session: Path, campaign_receipt: Path | None = None):
        return pipeline.archive_and_refresh_knowledge(session, self.master, self.knowledge,
            self.rom_path, self.output, campaign_receipt_path=campaign_receipt,
            expected_source_owned=0,
            expected_rom_sha256=ROM_SHA, expected_rom_size=len(ROM_BYTES))

    def relation_count(self, generation: Path, kind: str) -> int:
        db = sqlite3.connect(generation / "knowledge.sqlite")
        try:
            return int(db.execute("SELECT COUNT(*) FROM relation WHERE relation_type=?",
                                  (kind,)).fetchone()[0])
        finally:
            db.close()

    def evidence_count(self, generation: Path) -> int:
        db = sqlite3.connect(generation / "knowledge.sqlite")
        try:
            return int(db.execute("SELECT COUNT(*) FROM evidence_ref").fetchone()[0])
        finally:
            db.close()

    def test_a_seed_archivist_and_knowledge_generation(self):
        result = self.run_pipeline(self.session("seed", 101))
        self.assertEqual(result["status"], pipeline.PASS)
        self.assertEqual(result["archivist_merge"]["merge_mode"], "SEED")
        self.assertEqual(result["independent_audit"]["status"],
                         "PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1")

    def test_b_new_session_merges_only_new_canonical_facts(self):
        first = self.run_pipeline(self.session("first", 102, instructions=(0x10,)))
        before = first["knowledge_after"]["counts"]["rom_object"]
        second = self.run_pipeline(self.session("second", 103, instructions=(0x12,)))
        self.assertEqual(second["archivist_merge"]["merge_mode"], "MERGE")
        self.assertGreater(second["knowledge_after"]["counts"]["rom_object"], before)

    def test_c_duplicate_session_is_stable_noop(self):
        session = self.session("duplicate", 104)
        first = self.run_pipeline(session)
        second = self.run_pipeline(session)
        self.assertEqual(second["replay_status"], "PASS_IDEMPOTENT_NOOP")
        self.assertEqual(second["knowledge_after"]["hashes"], first["knowledge_after"]["hashes"])

    def test_d_overlapping_execution_reuses_object_and_adds_evidence(self):
        first = self.run_pipeline(self.session("overlap-a", 105, instructions=(0x10,)))
        generation_a = self.output / first["generation_dir"] if not Path(first["generation_dir"]).is_absolute() else Path(first["generation_dir"])
        old_evidence = self.evidence_count(generation_a)
        second = self.run_pipeline(self.session("overlap-b", 106, instructions=(0x10,)))
        self.assertEqual(second["import"]["objects_added"], 0)
        self.assertGreater(second["import"]["evidence_refs_added"], 0)
        self.assertGreater(self.evidence_count(Path(second["generation_dir"])), old_evidence)

    def test_e_executed_next_relation_is_canonical_once(self):
        result = self.run_pipeline(self.session("next", 107))
        generation = Path(result["generation_dir"])
        self.assertEqual(self.relation_count(generation, "EXECUTED_NEXT"), 1)

    def test_f_terminal_next_pc_remains_address_only(self):
        result = self.run_pipeline(self.session("terminal", 108, instructions=(0x14,), terminal=0x20))
        db = sqlite3.connect(Path(result["generation_dir"]) / "knowledge.sqlite")
        try:
            row = db.execute("SELECT target_object_id,target_address FROM relation "
                             "WHERE relation_type='OBSERVED_NEXT_PC'").fetchone()
            self.assertEqual(row, (None, 0x20))
        finally:
            db.close()

    def test_g_exception_endpoint_edges_do_not_become_instruction_adjacency(self):
        result = self.run_pipeline(self.session("exception", 109, exception=True))
        generation = Path(result["generation_dir"])
        self.assertEqual(self.relation_count(generation, "EXECUTED_NEXT"), 1)
        self.assertEqual(result["import"]["exception_flow_edges_excluded"], 2)

    def test_h_pointer_relation_fixture_imports(self):
        result = self.run_pipeline(self.session("pointer", 110, instructions=(0x10,),
            terminal=None, control=("OBSERVED_CODE_POINTER_TO",)))
        self.assertEqual(self.relation_count(Path(result["generation_dir"]),
                                             "OBSERVED_CODE_POINTER_TO"), 1)

    def test_i_offset_relation_fixture_imports(self):
        result = self.run_pipeline(self.session("offset", 111, instructions=(0x10,),
            terminal=None, control=("OBSERVED_CODE_OFFSET_TO",)))
        self.assertEqual(self.relation_count(Path(result["generation_dir"]),
                                             "OBSERVED_CODE_OFFSET_TO"), 1)

    def test_j_selected_jump_table_fixture_imports(self):
        result = self.run_pipeline(self.session("table", 112, instructions=(0x10,),
            terminal=None, control=("OBSERVED_JUMP_TABLE_ENTRY_TO",)))
        db = sqlite3.connect(Path(result["generation_dir"]) / "knowledge.sqlite")
        try:
            self.assertEqual(db.execute("SELECT COUNT(*) FROM relation WHERE "
                "relation_type='OBSERVED_JUMP_TABLE_ENTRY_TO'").fetchone()[0], 1)
            self.assertEqual(db.execute("SELECT COUNT(*) FROM rom_object WHERE "
                "object_type='TABLE_ENTRY'").fetchone()[0], 1)
        finally:
            db.close()

    def _campaign_receipt(self, path: Path, session: Path, valid: bool = True) -> Path:
        cycles = 1
        transitions = [cycles] * 4 if valid else [cycles, 0, cycles, cycles]
        receipt = {"checkpoint": "M12-ROM-RANGE-LINKAGE-2B",
            "status": "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE", "session_path": str(session.resolve()),
            "source_owned_delta": 0,
            "runtime": {"outcome": "PASS", "configured_count": 1,
                "required_cycles_per_worker": cycles, "required_completed_segments": 1,
                "audited_segments": 1, "workers": [{"worker_id": 0,
                    "capture_count": 1, "lifecycle_transition_counts": transitions}],
                "final_metrics": {"captures_dropped": 0, "captures_invalid": 0,
                    "identity_collisions": 0}},
            "saved_session": {"segments_admitted": 1, "segments_rejected": 0},
            "rom_projection": {"status": "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE",
                "rom_sha256": ROM_SHA, "instruction_occurrences": 2},
            "independent_audit": {"status": "PASS_INDEPENDENT_ROM_RANGE_AUDIT",
                "segments_audited": 1, "unique_capture_ids": 1,
                "workers_represented": [0], "worker_segment_counts": {"0": 1},
                "worker_generation_counts": {"0": 1}, "audited_range_occurrences": 2,
                "audited_terminal_next_pc_facts": 1, "terminal_next_pc_facts": 1,
                "identity_conflicts": 0, "opcode_mismatches": 0,
                "unresolved_instruction_occurrences": 0,
                "rom_unresolved_occurrences": 0, "decode_unsupported_occurrences": 0}}
        path.write_text(json.dumps(receipt), encoding="utf-8")
        return path

    def test_p_campaign_receipt_requires_and_summarizes_every_worker_cycle(self):
        session = self.session("campaign-contract", 122)
        campaign = self._campaign_receipt(self.root / "campaign.json", session)
        result = self.run_pipeline(session, campaign)
        self.assertEqual(result["campaign"]["worker_count"], 1)
        self.assertEqual(result["campaign"]["cycles_per_worker"], 1)
        self.assertEqual(result["campaign"]["audited_segments"], 1)

    def test_q_campaign_receipt_rejects_incomplete_worker_lifecycle(self):
        session = self.session("campaign-incomplete", 123)
        campaign = self._campaign_receipt(self.root / "campaign-incomplete.json", session,
                                           valid=False)
        with self.assertRaisesRegex(ValueError, "campaign receipt mismatch"):
            self.run_pipeline(session, campaign)
        self.assertFalse((self.output / "current.json").exists())

    def test_k_rom_mismatch_stops_import(self):
        session = self.session("rom-mismatch", 113)
        archived = archive_session(self.master, session, ROM_SHA)
        with self.assertRaises(KnowledgeImportStop) as caught:
            import_archivist_session(session, self.master, self.knowledge, ROM_BYTES,
                                     "0" * 64, archived["merge_receipt"])
        self.assertEqual(caught.exception.code, "STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH")

    def test_l_conflicted_session_stops_without_publication(self):
        good = self.run_pipeline(self.session("conflict-base", 114))
        pointer = (self.output / "current.json").read_bytes()
        bad = self.session("conflict", 115, conflict=True)
        with self.assertRaisesRegex(ValueError, "STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH"):
            self.run_pipeline(bad)
        self.assertEqual((self.output / "current.json").read_bytes(), pointer)
        self.assertEqual(json.loads(pointer)["generation_id"], good["generation_id"])

    def test_m_audit_failure_preserves_accepted_generation(self):
        accepted = self.run_pipeline(self.session("audit-base", 116))
        pointer = (self.output / "current.json").read_bytes()
        generation = Path(accepted["generation_dir"])
        before = hashlib.sha256((generation / "knowledge.sqlite").read_bytes()).hexdigest()
        with mock.patch.object(pipeline, "audit_pipeline", side_effect=RuntimeError("forced audit failure")):
            with self.assertRaisesRegex(ValueError, "STOP_KNOWLEDGE_AUDIT_FAILED"):
                self.run_pipeline(self.session("audit-fail", 117))
        self.assertEqual((self.output / "current.json").read_bytes(), pointer)
        self.assertEqual(hashlib.sha256((generation / "knowledge.sqlite").read_bytes()).hexdigest(), before)

    def test_n_source_owned_mutation_stops_with_specific_code(self):
        with self.assertRaisesRegex(ValueError, "STOP_RUNTIME_SOURCE_OWNED_MUTATION"):
            self.run_pipeline(self.session("owned", 118, source_owned=1))
        self.assertFalse((self.output / "current.json").exists())

    def test_o_emission_mutation_stops_and_old_generation_survives(self):
        accepted = self.run_pipeline(self.session("emission-base", 119))
        pointer = (self.output / "current.json").read_bytes()
        original = pipeline.audit_pipeline

        def corrupt_emission(*args, **kwargs):
            db = sqlite3.connect(args[3])
            db.execute("UPDATE emission SET classification='RUNTIME_MUTATED'")
            db.commit()
            db.close()
            return original(*args, **kwargs)

        with mock.patch.object(pipeline, "audit_pipeline", side_effect=corrupt_emission):
            with self.assertRaisesRegex(ValueError, "STOP_RUNTIME_EMISSION_MUTATION"):
                self.run_pipeline(self.session("emission-fail", 120))
        self.assertEqual((self.output / "current.json").read_bytes(), pointer)
        self.assertEqual(json.loads(pointer)["generation_id"], accepted["generation_id"])

    def test_unsupported_factual_relation_is_reported_and_stops(self):
        with self.assertRaisesRegex(KnowledgeImportStop, "STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT"):
            self.run_pipeline(self.session("unsupported", 121, unsupported=True))
        failure = json.loads((self.output / "last_failure.json").read_text(encoding="utf-8"))
        self.assertEqual(failure["unmapped_facts"], [
            {"status": "UNMAPPED_FACT_TYPE", "type": "UNSUPPORTED_RUNTIME_FACT"}])
        self.assertFalse((self.output / "current.json").exists())


if __name__ == "__main__":
    unittest.main()
