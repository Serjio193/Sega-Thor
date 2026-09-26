"""Independent read-only audit for the 2G MAP-1 → Archivist → 2D chain."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from pathlib import Path
from typing import Any

try:
    from .rom_knowledge_map import runtime_occurrence_id
except ImportError:
    from rom_knowledge_map import runtime_occurrence_id

CONTROL_RELATIONS = {
    "OBSERVED_CODE_POINTER_TO": "RUNTIME_CODE_POINTER_PROVENANCE",
    "OBSERVED_CODE_OFFSET_TO": "RUNTIME_CODE_OFFSET_PROVENANCE",
    "OBSERVED_JUMP_TABLE_ENTRY_TO": "RUNTIME_JUMP_TABLE_ENTRY_PROVENANCE",
}
CONTROL_SOURCES = {
    "OBSERVED_CODE_POINTER_TO": ("ROM_CODE_POINTER_SOURCE", "ROM_DATA"),
    "OBSERVED_CODE_OFFSET_TO": ("ROM_CODE_OFFSET_SOURCE", "ROM_DATA"),
    "OBSERVED_JUMP_TABLE_ENTRY_TO": ("ROM_JUMP_TABLE_ENTRY", "TABLE_ENTRY"),
}


TABLES = {
    "rom_range": "range_id,rom_sha256,start,end",
    "rom_object": "object_id,range_id,object_type,attributes_json",
    "claim": "claim_id,object_id,claim_type,value_json,status",
    "relation": "relation_id,relation_type,source_object_id,target_object_id,target_address,status,attributes_json",
    "conflict": "conflict_id,start,end,conflict_type,detail_json",
    "source_artifact": "source_sha256,checkpoint,artifact_name,artifact_type",
    "evidence_ref": "ref_id,subject_type,subject_id,source_sha256,fact_kind,fact_count,locator_json",
    "emission": "start,end,emission_type,classification,source_kind,source_owned,artifact_type,artifact",
    "derivation": "derivation_id,rule_id,rule_version,implementation_hash,parameters_hash,output_type,output_id,result_json,assumptions_json",
    "derivation_input": "derivation_id,ordinal,subject_type,subject_id,role",
    "map_proposal": "proposal_id,base_generation,base_map_hash,graph_hash,validator_version,status,proposal_set_hash",
    "map_proposal_operation": "proposal_id,ordinal,operation_json",
}


def _json(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _hash_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def _hash_json(value: Any) -> str:
    return _hash_bytes(_json(value).encode("utf-8"))


def _file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _id(prefix: str, value: Any) -> str:
    return prefix + ":" + _hash_json(value)


def _open(path: Path) -> sqlite3.Connection:
    db = sqlite3.connect(Path(path).resolve().as_uri() + "?mode=ro&immutable=1", uri=True)
    db.row_factory = sqlite3.Row
    return db


def _map1_hash(db: sqlite3.Connection) -> str:
    graph = {key: [dict(row) for row in db.execute(f"SELECT * FROM {table} ORDER BY {order}")]
        for key, table, order in (("nodes", "map_node", "node_id"),
            ("edges", "map_edge", "edge_id"), ("frontiers", "map_frontier", "frontier_id"),
            ("conflicts", "map_conflict", "conflict_id"))}
    return _hash_json(graph)


def _database_hashes(db: sqlite3.Connection) -> dict[str, str]:
    rows = lambda table: [tuple(row) for row in db.execute(
        f"SELECT {TABLES[table]} FROM {table} ORDER BY {TABLES[table]}")]
    legacy = not db.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name='derivation'").fetchone()
    structure_tables = ("rom_range", "rom_object", "claim", "relation", "conflict")
    if not legacy:
        structure_tables += ("derivation", "derivation_input")
    structure = {table: rows(table) for table in structure_tables}
    evidence = {table: rows(table) for table in ("source_artifact", "evidence_ref")}
    emission = rows("emission")
    structure_hash, evidence_hash, emission_hash = map(_hash_json,
        (structure, evidence, emission))
    result = {"structure_hash": structure_hash, "evidence_index_hash": evidence_hash,
            "emission_hash": emission_hash,
            "map_hash": _hash_bytes((structure_hash + evidence_hash + emission_hash).encode())}
    if not legacy:
        proposals = {table: rows(table) for table in ("map_proposal", "map_proposal_operation")}
        result.update({"graph_structure_hash": structure_hash,
                       "proposal_set_hash": _hash_json(proposals)})
    return result


def _meta(db: sqlite3.Connection) -> dict[str, str]:
    return {str(row[0]): str(row[1]) for row in db.execute("SELECT key,value FROM map_meta")}


def _node(row: sqlite3.Row) -> tuple[str, str, str, str, dict[str, Any]]:
    kind, key, scope = str(row["kind"]), str(row["node_key"]), str(row["scope"])
    if _hash_json({"kind": kind, "key": key, "scope": scope}) != str(row["node_id"]):
        raise ValueError("STOP_KNOWLEDGE_AUDIT_IDENTITY_MISMATCH")
    attrs = json.loads(str(row["body"])).get("attributes", {})
    if not isinstance(attrs, dict):
        raise ValueError("STOP_KNOWLEDGE_AUDIT_IDENTITY_MISMATCH")
    return kind, key, scope, str(row["status"]), attrs


def _edge(row: sqlite3.Row) -> tuple[str, str, str, str, str]:
    values = tuple(str(row[name]) for name in ("source_id", "target_id", "relation", "scope"))
    if _hash_json({"source": values[0], "target": values[1],
                   "relation": values[2], "scope": values[3]}) != str(row["edge_id"]):
        raise ValueError("STOP_KNOWLEDGE_AUDIT_IDENTITY_MISMATCH")
    return (*values, str(row["status"]))


def _range(node: sqlite3.Row, rom: bytes, rom_sha: str) -> tuple[int, int, bytes]:
    kind, key, scope, status, attrs = _node(node)
    try:
        start, end = int(attrs["start_offset"]), int(attrs["end_offset_exclusive"])
        data = bytes.fromhex(str(attrs["bytes_hex"]))
    except (KeyError, TypeError, ValueError) as exc:
        raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING") from exc
    if kind != "ROM_INSTRUCTION_RANGE" or scope != rom_sha or status != "OBSERVED" or \
            attrs.get("rom_sha256") != rom_sha or start < 0 or end <= start or end > len(rom) or \
            len(data) != end - start or data != rom[start:end] or \
            _hash_bytes(data) != attrs.get("bytes_sha256") or \
            key != f"{start:08X}:{end:08X}:{data.hex().upper()}":
        raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISMATCH")
    return start, end, data


def _range_id(rom_sha: str, start: int, end: int) -> str:
    return _id("range", {"rom_sha256": rom_sha, "start": start, "end": end})


def _object_id(rom_sha: str, start: int, end: int, kind: str) -> str:
    return _id("object", {"rom_sha256": rom_sha, "start": start, "end": end,
                          "object_type": kind})


def _relation_id(kind: str, source: str, target: str | None,
                 address: int | None) -> str:
    return _id("relation", {"relation_type": kind, "source_object_id": source,
        "target_object_id": target, "target_address": address, "status": "OBSERVED_RUNTIME"})


def _evidence_id(subject_type: str, subject_id: str, source_sha: str,
                 fact_kind: str, count: int, locator: dict[str, Any]) -> str:
    return _id("evidence", {"subject_type": subject_type, "subject_id": subject_id,
        "source_sha256": source_sha, "fact_kind": fact_kind,
        "fact_count": count, "locator": locator})


def _expect_runtime_evidence(evidence: dict[str, tuple[str, str, str, str, int, str]],
                             event: dict[str, Any],
                             subject_type: str, subject_id: str,
                             session_sha: str) -> None:
    stable_content = {key: value for key, value in event.items()
                      if key not in {"capture_ids", "windows"}}
    source_sha = _hash_bytes(_json(stable_content).encode())
    locator = {**event, "session_source_sha256s": [session_sha]}
    ref_id = _id("runtime-evidence", {"occurrence_id": event["occurrence_id"],
        "subject_type": subject_type, "subject_id": subject_id})
    fact_kind = "RUNTIME_OCCURRENCE:" + event["event_kind"]
    evidence[ref_id] = (subject_type, subject_id, source_sha, fact_kind, 1, _json(locator))


def _lineage_count(edge: sqlite3.Row, run_id: int) -> int:
    values = json.loads(str(edge["lineage"]))
    if not isinstance(values, list) or not values:
        raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING")
    for row in values:
        if not isinstance(row, dict) or int(row.get("run_id", -1)) != run_id or \
                int(row.get("capture_id", 0)) <= 0 or int(row.get("generation", 0)) <= 0:
            raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING")
    return len(values)


def _expected_import(session: sqlite3.Connection, knowledge: sqlite3.Connection,
                     rom: bytes, rom_sha: str, merge: dict[str, Any],
                     session_sha: str, master_sha: str) -> dict[str, Any]:
    meta = _meta(session)
    run_id, session_id = int(meta["live_forward_run_id"]), str(meta["live_forward_session_id"])
    nodes = {str(row["node_id"]): row for row in session.execute("SELECT * FROM map_node")}
    edges = {str(row["edge_id"]): row for row in session.execute("SELECT * FROM map_edge")}
    instruction_obj: dict[str, str] = {}
    objects: dict[str, tuple[str, int, int, dict[str, Any]]] = {}
    claims: dict[str, tuple[str, str, str, str]] = {}
    relations: dict[str, tuple[str, str, str | None, int | None, str, str]] = {}
    evidence: dict[str, tuple[str, str, str, str, int, str]] = {}
    relation_ids: dict[str, str] = {}
    relation_sources: dict[str, tuple[str, str, str]] = {}

    def expect_evidence(subject_type: str, subject_id: str, source_sha: str,
                        fact_kind: str, count: int, locator: dict[str, Any]) -> None:
        ref_id = _evidence_id(subject_type, subject_id, source_sha, fact_kind, count, locator)
        evidence[ref_id] = (subject_type, subject_id, source_sha, fact_kind,
                            count, _json(locator))
    for edge_id, edge in edges.items():
        source, target, relation, scope, status = _edge(edge)
        if relation != "EXECUTED_FROM_ROM":
            continue
        if source not in nodes or target not in nodes or status != "OBSERVED":
            raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISMATCH")
        source_kind, source_key, source_scope, _, source_attrs = _node(nodes[source])
        start, end, data = _range(nodes[target], rom, rom_sha)
        try:
            pc_text, op_text = source_key.split(":", 1)
            pc, opcode = int(pc_text, 16), int(op_text, 16)
        except ValueError as exc:
            raise ValueError("STOP_KNOWLEDGE_AUDIT_IDENTITY_MISMATCH") from exc
        if source_kind != "M68K_INSTRUCTION" or source_scope != "rom:" + rom_sha or \
                pc != start or opcode != int.from_bytes(data[:2], "big") or \
                source_attrs.get("pc") != pc or source_attrs.get("opcode") != opcode:
            raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISMATCH")
        oid = _object_id(rom_sha, start, end, "M68K_INSTRUCTION")
        owned = int(knowledge.execute("SELECT COALESCE(SUM(MIN(end,?)-MAX(start,?)),0) FROM emission "
            "WHERE source_owned=1 AND start<? AND end>?", (end, start, end, start)).fetchone()[0])
        attrs = {"opcode": opcode, "length": end - start,
                 "bytes_sha256": _hash_bytes(data), "source_owned_bytes": owned}
        objects[oid] = ("M68K_INSTRUCTION", start, end, attrs)
        instruction_obj[source] = oid
        claim_id = _id("claim", {"object_id": oid, "claim_type": "EXECUTED_FROM_ROM",
            "value": True, "status": "OBSERVED_RUNTIME"})
        claims[claim_id] = (oid, "EXECUTED_FROM_ROM", "true", "OBSERVED_RUNTIME")
        loc = {"session_id": session_id, "session_graph_sha256": merge["session_graph_hash"],
            "master_graph_sha256": merge["master_graph_hash_after"], "run_id": run_id,
            "table": "map_edge", "edge_id": edge_id, "range_node_id": target,
            "start": start, "end": end}
        expect_evidence("CLAIM", claim_id, session_sha, "RUNTIME_INSTRUCTION_RANGE", 1, loc)

    for edge_id, edge in edges.items():
        source, target, relation, scope, status = _edge(edge)
        if relation == "EXECUTED_FROM_ROM":
            continue
        kind_id = None
        target_obj = address = None
        fact_kind = ""
        if relation == "EXECUTED_NEXT":
            if source not in nodes or target not in nodes:
                raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING")
            src_kind, _, _, _, _ = _node(nodes[source])
            dst_kind, _, _, _, _ = _node(nodes[target])
            if src_kind != "M68K_INSTRUCTION" or dst_kind != "M68K_INSTRUCTION":
                if "EXCEPTION_EVENT" in src_kind + dst_kind:
                    continue
                raise ValueError("STOP_KNOWLEDGE_AUDIT_UNMAPPED_FACT")
            if source not in instruction_obj or target not in instruction_obj:
                raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING")
            kind_id, target_obj, fact_kind = instruction_obj[source], instruction_obj[target], "RUNTIME_NEXT_OCCURRENCE"
        elif relation == "OBSERVED_NEXT_PC":
            if source not in instruction_obj or target not in nodes:
                raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING")
            target_kind, _, target_scope, _, attrs = _node(nodes[target])
            if target_kind != "M68K_TARGET_ADDRESS" or target_scope != "rom:" + rom_sha:
                raise ValueError("STOP_KNOWLEDGE_AUDIT_UNMAPPED_FACT")
            kind_id, address, fact_kind = instruction_obj[source], int(attrs["raw_next_pc"]), "RUNTIME_TERMINAL_FACT"
        elif relation in CONTROL_RELATIONS:
            if source not in nodes or target not in nodes:
                raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING")
            src_kind, _, src_scope, src_status, src_attrs = _node(nodes[source])
            expected_src, object_type = CONTROL_SOURCES[relation]
            if source not in nodes or target not in nodes or src_kind != expected_src or \
                    src_scope != rom_sha or src_status != "OBSERVED" or status != "OBSERVED":
                raise ValueError("STOP_KNOWLEDGE_AUDIT_UNMAPPED_FACT")
            start, end = int(src_attrs["start_offset"]), int(src_attrs["end_offset_exclusive"])
            data = bytes.fromhex(str(src_attrs["bytes_hex"]))
            if src_attrs.get("rom_sha256") != rom_sha or start < 0 or end <= start or \
                    end > len(rom) or data != rom[start:end] or \
                    _hash_bytes(data) != src_attrs.get("bytes_sha256"):
                raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISMATCH")
            if relation == "OBSERVED_JUMP_TABLE_ENTRY_TO" and \
                    (src_attrs.get("selected") is not True or not isinstance(src_attrs.get("selected_index"), int)):
                raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING")
            target_start, target_end, target_data = _range(nodes[target], rom, rom_sha)
            src_obj = _object_id(rom_sha, start, end, object_type)
            dst_obj = _object_id(rom_sha, target_start, target_end, "M68K_INSTRUCTION")
            objects[src_obj] = (object_type, start, end, {"bytes_sha256": _hash_bytes(data),
                "length": end - start, "source_owned_bytes": int(knowledge.execute(
                    "SELECT COALESCE(SUM(MIN(end,?)-MAX(start,?)),0) FROM emission "
                    "WHERE source_owned=1 AND start<? AND end>?", (end, start, end, start)).fetchone()[0])})
            objects[dst_obj] = ("M68K_INSTRUCTION", target_start, target_end,
                {"opcode": int.from_bytes(target_data[:2], "big"), "length": target_end - target_start,
                 "bytes_sha256": _hash_bytes(target_data), "source_owned_bytes": int(knowledge.execute(
                    "SELECT COALESCE(SUM(MIN(end,?)-MAX(start,?)),0) FROM emission "
                    "WHERE source_owned=1 AND start<? AND end>?",
                    (target_end, target_start, target_end, target_start)).fetchone()[0])})
            kind_id, target_obj, fact_kind = src_obj, dst_obj, CONTROL_RELATIONS[relation]
        elif relation == "ROM_LINK_UNRESOLVED" or status in {"OBSERVED", "PROVEN"}:
            raise ValueError("STOP_KNOWLEDGE_AUDIT_UNMAPPED_FACT")
        else:
            continue
        relation_id = _relation_id(relation, kind_id, target_obj, address)
        relations[relation_id] = (relation, kind_id, target_obj, address,
                                  "OBSERVED_RUNTIME", "{}")
        relation_ids[edge_id] = relation_id
        relation_sources[edge_id] = (source, target, relation)

    occurrence_table = session.execute("SELECT 1 FROM sqlite_master WHERE type='table' "
        "AND name='live_forward_runtime_occurrence'").fetchone()
    if not occurrence_table:
        raise ValueError("STOP_RUNTIME_OCCURRENCE_TABLE_MISSING")
    occurrence_rows = [json.loads(row[0]) for row in session.execute(
        "SELECT event_json FROM live_forward_runtime_occurrence ORDER BY occurrence_id")]
    by_node: dict[str, list[dict[str, Any]]] = {}
    for event in occurrence_rows:
        expected_id = runtime_occurrence_id(capture_id=event["capture_id"],
            epoch=int(event["epoch"]), cpu=event["cpu_id"],
            address_space=event["address_space"],
            native_sequence=int(event["native_sequence"]),
            event_kind=event["event_kind"], run_id=int(event["run_id"]),
            instruction_sequence=int(event["instruction_sequence"]))
        if event.get("occurrence_id") != expected_id or int(event["run_id"]) != run_id:
            raise ValueError("STOP_RUNTIME_OCCURRENCE_IDENTITY_INVALID")
        if event.get("instruction_node_id"):
            by_node.setdefault(event["instruction_node_id"], []).append(event)
        subjects = [("RUNTIME_OCCURRENCE", event["occurrence_id"])]
        if event.get("instruction_node_id") in instruction_obj:
            node = event["instruction_node_id"]
            subjects.append(("CLAIM", _id("claim", {"object_id": instruction_obj[node],
                "claim_type": "EXECUTED_FROM_ROM", "value": True,
                "status": "OBSERVED_RUNTIME"})))
        for subject_type, subject_id in subjects:
            _expect_runtime_evidence(evidence, event, subject_type,
                                     subject_id, session_sha)
        if event.get("event_kind") == "EXECUTED_NEXT" and event.get("edge_id") in relation_ids:
            _expect_runtime_evidence(evidence, event, "RELATION",
                relation_ids[event["edge_id"]], session_sha)
    for edge_id, relation_id in relation_ids.items():
        source, target, relation = relation_sources[edge_id]
        if relation == "EXECUTED_NEXT":
            supporting = [event for event in occurrence_rows if
                event.get("event_kind") == "EXECUTED_NEXT" and event.get("edge_id") == edge_id]
        else:
            supporting = by_node.get(source, [])
            if not supporting and relation in CONTROL_RELATIONS:
                attrs = _node(nodes[target])[4]
                start = int(attrs.get("start_offset", -1))
                supporting = [event for event in occurrence_rows if
                    event.get("event_kind") == "INSTRUCTION" and int(event.get("pc", -2)) == start]
        if not supporting:
            raise ValueError("STOP_RUNTIME_RELATION_OCCURRENCE_MISSING")
        for event in supporting:
            _expect_runtime_evidence(evidence, event, "RELATION", relation_id, session_sha)

    for oid, (kind, start, end, attrs) in objects.items():
        found = knowledge.execute("SELECT r.rom_sha256,r.start,r.end,o.object_type,o.attributes_json "
            "FROM rom_object o JOIN rom_range r USING(range_id) WHERE o.object_id=?", (oid,)).fetchone()
        if found is None or tuple(found[:4]) != (rom_sha, start, end, kind) or \
                str(found[4]) != _json(attrs):
            raise ValueError("STOP_KNOWLEDGE_AUDIT_CANONICAL_OBJECT_MISSING")
    for claim_id, expected in claims.items():
        row = knowledge.execute("SELECT object_id,claim_type,value_json,status FROM claim WHERE claim_id=?",
                                 (claim_id,)).fetchone()
        if row is None or tuple(row) != expected:
            raise ValueError("STOP_KNOWLEDGE_AUDIT_CANONICAL_CLAIM_MISSING")
    for relation_id, expected in relations.items():
        row = knowledge.execute("SELECT relation_type,source_object_id,target_object_id,target_address,status,"
            "attributes_json FROM relation WHERE relation_id=?", (relation_id,)).fetchone()
        if row is None or tuple(row) != expected:
            raise ValueError("STOP_KNOWLEDGE_AUDIT_CANONICAL_RELATION_MISSING")
    for ref_id, expected in evidence.items():
        row = knowledge.execute("SELECT subject_type,subject_id,source_sha256,fact_kind,fact_count,locator_json "
                                "FROM evidence_ref WHERE ref_id=?", (ref_id,)).fetchone()
        if row is None or tuple(row) != expected:
            raise ValueError("STOP_KNOWLEDGE_AUDIT_EVIDENCE_MISSING")
    return {"expected_instruction_objects": len(instruction_obj),
        "expected_claims": len(claims), "expected_relations": len(relations),
        "expected_evidence_refs": len(evidence)}


def audit_pipeline(session_path: Path, master_path: Path, base_knowledge_path: Path,
                   final_knowledge_path: Path, rom_path: Path, rom_sha256: str,
                   merge_receipt: dict[str, Any], import_report: dict[str, Any],
                   expected_source_owned: int | None = None) -> dict[str, Any]:
    """Recompute MAP-1/master lineage and check every newly canonicalized fact."""
    rom = Path(rom_path).read_bytes()
    if _hash_bytes(rom) != rom_sha256:
        raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH")
    merge_payload = {key: value for key, value in merge_receipt.items() if key != "receipt_sha256"}
    if merge_receipt.get("receipt_sha256") != _hash_json(merge_payload):
        raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
    session_sha, master_sha = _file_hash(session_path), _file_hash(master_path)
    if session_sha != merge_receipt.get("source_artifact_sha256") or \
            merge_receipt.get("rom_sha256") != rom_sha256:
        raise ValueError("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
    session, master = _open(session_path), _open(master_path)
    base, final = _open(base_knowledge_path), _open(final_knowledge_path)
    try:
        for db in (session, master, base, final):
            if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
                raise ValueError("STOP_KNOWLEDGE_AUDIT_FAILED")
        smeta, mmeta = _meta(session), _meta(master)
        if smeta.get("schema") != "m12.map1.v1" or mmeta.get("schema") != "m12.map1.v1" or \
                smeta.get("live_forward_session_state") != "CLOSED":
            raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        if smeta.get("rom_sha256") != rom_sha256 or mmeta.get("rom_sha256") != rom_sha256:
            raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH")
        if int(smeta.get("source_owned_bytes", "-1")) != 0:
            raise ValueError("STOP_RUNTIME_SOURCE_OWNED_MUTATION")
        session_graph_hash, master_graph_hash = _map1_hash(session), _map1_hash(master)
        if session_graph_hash != merge_receipt.get("session_graph_hash") or \
                session_graph_hash != smeta.get("live_forward_graph_sha256") or \
                master_graph_hash != merge_receipt.get("master_graph_hash_after"):
            raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        for table, column in (("map_node", "node_id"), ("map_edge", "edge_id")):
            old = {str(row[0]): tuple(row[1:]) for row in session.execute(f"SELECT * FROM {table}")}
            new = {str(row[0]): tuple(row[1:]) for row in master.execute(f"SELECT * FROM {table}")}
            for key, row in old.items():
                if key not in new:
                    raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
                columns = [d[0] for d in session.execute(
                    f"SELECT * FROM {table} LIMIT 0").description]
                base_row = dict(zip(columns[1:], row))
                merged_row = dict(zip(columns[1:], new[key]))
                for field in base_row:
                    if field not in {"lineage", "status"} and base_row[field] != merged_row[field]:
                        raise ValueError("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT")
                if not { _json(v) for v in json.loads(base_row["lineage"]) } <= \
                        { _json(v) for v in json.loads(merged_row["lineage"]) }:
                    raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        if _meta(base).get("schema") not in {"oasis.m12.canonical-rom-knowledge.v1",
                "oasis.m14.canonical-rom-knowledge.v2"} or \
                _meta(final).get("schema") != "oasis.m14.canonical-rom-knowledge.v2" or \
                _meta(final).get("rom_sha256") != rom_sha256:
            raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH")
        base_hashes, final_hashes = _database_hashes(base), _database_hashes(final)
        base_emission = [tuple(row) for row in base.execute(f"SELECT {TABLES['emission']} FROM emission ORDER BY start,end")]
        final_emission = [tuple(row) for row in final.execute(f"SELECT {TABLES['emission']} FROM emission ORDER BY start,end")]
        owned = int(final.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0])
        old_owned = int(base.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0])
        if owned != old_owned or (expected_source_owned is not None and
                                  owned != expected_source_owned):
            raise ValueError("STOP_RUNTIME_SOURCE_OWNED_MUTATION")
        if final_emission != base_emission or base_hashes["emission_hash"] != final_hashes["emission_hash"]:
            raise ValueError("STOP_RUNTIME_EMISSION_MUTATION")
        for table in ("rom_range", "rom_object", "claim", "relation", "conflict",
                      "source_artifact", "evidence_ref", "emission", "derivation",
                      "derivation_input", "map_proposal", "map_proposal_operation"):
            cols = TABLES[table]
            if not base.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name=?",
                                (table,)).fetchone():
                if final.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0]:
                    raise ValueError("STOP_KNOWLEDGE_AUDIT_FAILED")
                continue
            before = {tuple(row) for row in base.execute(f"SELECT {cols} FROM {table}")}
            after = {tuple(row) for row in final.execute(f"SELECT {cols} FROM {table}")}
            if not before <= after:
                raise ValueError("STOP_KNOWLEDGE_AUDIT_FAILED")
        if int(final.execute("SELECT COUNT(*) FROM conflict").fetchone()[0]) != 0:
            raise ValueError("STOP_KNOWLEDGE_AUDIT_FAILED")
        if import_report.get("session_source_sha256") != session_sha or \
                import_report.get("master_source_sha256") != master_sha:
            raise ValueError("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
        artifacts = {str(row[0]) for row in final.execute("SELECT source_sha256 FROM source_artifact")}
        if session_sha not in artifacts or master_sha not in artifacts:
            raise ValueError("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
        key = "M12-ARCHIVIST-SESSION:" + str(merge_receipt["session_id"])
        input_hash = _hash_json({"rom_sha256": rom_sha256, "session_id": merge_receipt["session_id"],
            "session_graph_hash": session_graph_hash, "source_artifact_sha256": session_sha})
        if tuple(final.execute("SELECT input_hash FROM map_import WHERE import_key=?", (key,)).fetchone() or ()) != (input_hash,):
            raise ValueError("STOP_KNOWLEDGE_IMPORT_NONIDEMPOTENT")
        expected = _expected_import(session, final, rom, rom_sha256,
                                    merge_receipt, session_sha, master_sha)
        counts = {table: int(final.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0])
                  for table in ("rom_object", "claim", "relation", "evidence_ref")}
        relation_types = {str(row[0]): int(row[1]) for row in final.execute(
            "SELECT relation_type,COUNT(*) FROM relation GROUP BY relation_type")}
        emission_bytes = {str(row[0]): int(row[1]) for row in final.execute(
            "SELECT emission_type,SUM(end-start) FROM emission GROUP BY emission_type")}
        if expected_source_owned is not None and owned != expected_source_owned:
            raise ValueError("STOP_RUNTIME_SOURCE_OWNED_MUTATION")
        return {"status": "PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1",
            "rom_sha256": rom_sha256, "session_graph_hash": session_graph_hash,
            "master_graph_hash_after": master_graph_hash,
            "source_artifact_hash": session_sha, "master_artifact_hash": master_sha,
            "expected_import": expected, "knowledge_counts": counts,
            "relation_counts_by_type": relation_types, "source_owned_bytes": owned,
            "source_owned_delta": owned - old_owned, "emission_bytes_by_type": emission_bytes,
            "hashes_before": base_hashes, "hashes_after": final_hashes,
            "emission_hash_unchanged": base_hashes["emission_hash"] == final_hashes["emission_hash"]}
    finally:
        for db in (session, master, base, final):
            db.close()
