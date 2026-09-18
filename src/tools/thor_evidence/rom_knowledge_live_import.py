"""Translate accepted closed MAP-1 runtime deltas into canonical 2D facts."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from pathlib import Path
from typing import Any

try:
    from .cartographer import canonical as map1_json, digest as map1_digest
    from .rom_knowledge_map import (KnowledgeStore, canonical, object_id, range_id,
                                   stable_id)
except ImportError:
    from cartographer import canonical as map1_json, digest as map1_digest
    from rom_knowledge_map import KnowledgeStore, canonical, object_id, range_id, stable_id


CONTROL_RELATIONS = {
    "OBSERVED_CODE_POINTER_TO": "RUNTIME_CODE_POINTER_PROVENANCE",
    "OBSERVED_CODE_OFFSET_TO": "RUNTIME_CODE_OFFSET_PROVENANCE",
    "OBSERVED_JUMP_TABLE_ENTRY_TO": "RUNTIME_JUMP_TABLE_ENTRY_PROVENANCE",
}
SUPPORTED_RELATIONS = {"EXECUTED_FROM_ROM", "EXECUTED_NEXT", "OBSERVED_NEXT_PC",
                       *CONTROL_RELATIONS}
CONTROL_SOURCES = {
    "OBSERVED_CODE_POINTER_TO": ("ROM_CODE_POINTER_SOURCE", "ROM_DATA"),
    "OBSERVED_CODE_OFFSET_TO": ("ROM_CODE_OFFSET_SOURCE", "ROM_DATA"),
    "OBSERVED_JUMP_TABLE_ENTRY_TO": ("ROM_JUMP_TABLE_ENTRY", "TABLE_ENTRY"),
}


class KnowledgeImportStop(ValueError):
    """A fail-closed MAP-1 import error with an explicit report payload."""

    def __init__(self, code: str, **report: Any):
        super().__init__(code)
        self.code = code
        self.report = {"status": code, **report}


def _sha_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _hash_json(value: Any) -> str:
    return _sha_bytes(canonical(value).encode("utf-8"))


def _readonly(path: Path) -> sqlite3.Connection:
    db = sqlite3.connect(Path(path).resolve().as_uri() + "?mode=ro&immutable=1", uri=True)
    db.row_factory = sqlite3.Row
    return db


def _map1_hash(db: sqlite3.Connection) -> str:
    graph = {key: [dict(row) for row in db.execute(
        f"SELECT * FROM {table} ORDER BY {order}")] for key, table, order in (
            ("nodes", "map_node", "node_id"), ("edges", "map_edge", "edge_id"),
            ("frontiers", "map_frontier", "frontier_id"),
            ("conflicts", "map_conflict", "conflict_id"))}
    return map1_digest(graph)


def _metadata(db: sqlite3.Connection) -> dict[str, str]:
    return {str(row[0]): str(row[1]) for row in db.execute("SELECT key,value FROM map_meta")}


def _node_rows(db: sqlite3.Connection) -> dict[str, dict[str, Any]]:
    return {str(row["node_id"]): dict(row) for row in db.execute(
        "SELECT * FROM map_node ORDER BY node_id")}


def _edge_rows(db: sqlite3.Connection) -> dict[str, dict[str, Any]]:
    return {str(row["edge_id"]): dict(row) for row in db.execute(
        "SELECT * FROM map_edge ORDER BY edge_id")}


def _lineage(row: dict[str, Any], run_id: int) -> list[dict[str, Any]]:
    try:
        values = json.loads(str(row["lineage"]))
    except (KeyError, TypeError, json.JSONDecodeError) as exc:
        raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH") from exc
    if not isinstance(values, list) or not values:
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING",
                                  edge_id=row.get("edge_id"))
    for item in values:
        if not isinstance(item, dict) or int(item.get("run_id", -1)) != run_id or \
                int(item.get("capture_id", 0)) <= 0 or int(item.get("generation", 0)) <= 0:
            raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING",
                                      edge_id=row.get("edge_id"))
    return values


def _lineages_contained(session_value: str, master_value: str) -> bool:
    try:
        session_items, master_items = json.loads(session_value), json.loads(master_value)
    except (TypeError, json.JSONDecodeError):
        return False
    return isinstance(session_items, list) and isinstance(master_items, list) and \
        {map1_json(item) for item in session_items} <= {map1_json(item) for item in master_items}


def _read_node(row: dict[str, Any]) -> tuple[str, str, str, str, dict[str, Any]]:
    kind, key, scope = (str(row[name]) for name in ("kind", "node_key", "scope"))
    if map1_digest({"kind": kind, "key": key, "scope": scope}) != str(row["node_id"]):
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT",
                                  node_id=row["node_id"])
    try:
        body = json.loads(str(row["body"]))
        attrs = body.get("attributes", {})
    except (TypeError, json.JSONDecodeError) as exc:
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT",
                                  node_id=row["node_id"]) from exc
    if not isinstance(attrs, dict):
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT",
                                  node_id=row["node_id"])
    return kind, key, scope, str(row["status"]), attrs


def _read_edge(row: dict[str, Any]) -> tuple[str, str, str, str, str, dict[str, Any]]:
    source, target, relation, scope = (str(row[name]) for name in
                                       ("source_id", "target_id", "relation", "scope"))
    expected = map1_digest({"source": source, "target": target,
                            "relation": relation, "scope": scope})
    if expected != str(row["edge_id"]):
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT",
                                  edge_id=row["edge_id"])
    try:
        body = json.loads(str(row["body"]))
    except (TypeError, json.JSONDecodeError) as exc:
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT",
                                  edge_id=row["edge_id"]) from exc
    return source, target, relation, scope, str(row["status"]), body


def _receipt_fields(merge_receipt: dict[str, Any]) -> dict[str, Any]:
    payload = {key: value for key, value in merge_receipt.items() if key != "receipt_sha256"}
    if merge_receipt.get("receipt_sha256") != _hash_json(payload):
        raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
    required = ("rom_sha256", "session_id", "session_graph_hash", "master_graph_hash_after",
                "source_artifact_sha256", "merge_mode", "conflict_count")
    if any(key not in merge_receipt for key in required):
        raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
    if int(merge_receipt["conflict_count"]) != 0:
        raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
    return payload


def _check_archivist_chain(session_path: Path, master_path: Path, rom_sha256: str,
                           merge_receipt: dict[str, Any]) -> tuple[sqlite3.Connection,
                           sqlite3.Connection, dict[str, dict[str, Any]],
                           dict[str, dict[str, Any]], str, str, int]:
    receipt = _receipt_fields(merge_receipt)
    if receipt["rom_sha256"] != rom_sha256:
        raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH")
    session_sha = sha256_file(session_path)
    if session_sha != receipt["source_artifact_sha256"]:
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
    source = _readonly(session_path)
    master = _readonly(master_path)
    try:
        if source.execute("PRAGMA integrity_check").fetchone()[0] != "ok" or \
                master.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        smeta, mmeta = _metadata(source), _metadata(master)
        if smeta.get("schema") != "m12.map1.v1" or mmeta.get("schema") != "m12.map1.v1":
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        if smeta.get("rom_sha256") != rom_sha256 or mmeta.get("rom_sha256") != rom_sha256:
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH")
        if smeta.get("live_forward_session_state") != "CLOSED":
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        if int(smeta.get("source_owned_bytes", "-1")) != 0:
            raise KnowledgeImportStop("STOP_RUNTIME_SOURCE_OWNED_MUTATION")
        conflicts = int(source.execute("SELECT COUNT(*) FROM map_conflict").fetchone()[0])
        if conflicts or int(master.execute("SELECT COUNT(*) FROM map_conflict").fetchone()[0]):
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH",
                                      unmapped_fact_types=["MAP1_CONFLICT"])
        if int(source.execute("SELECT COUNT(*) FROM map_frontier").fetchone()[0]):
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH",
                                      unmapped_fact_types=["MAP1_FRONTIER"])
        session_hash, master_hash = _map1_hash(source), _map1_hash(master)
        if session_hash != smeta.get("live_forward_graph_sha256") or \
                session_hash != receipt["session_graph_hash"] or \
                master_hash != receipt["master_graph_hash_after"]:
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        run_id = int(smeta.get("live_forward_run_id", "0"))
        if run_id <= 0 or smeta.get("live_forward_session_id") != receipt["session_id"]:
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        nodes, edges = _node_rows(source), _edge_rows(source)
        master_nodes, master_edges = _node_rows(master), _edge_rows(master)
        for node_id, node in nodes.items():
            if node_id not in master_nodes:
                raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
            merged = master_nodes[node_id]
            for field in ("kind", "node_key", "scope", "body"):
                if node[field] != merged[field]:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT",
                                              node_id=node_id)
            if not _lineages_contained(node["lineage"], merged["lineage"]):
                raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        for edge_id, edge in edges.items():
            if edge_id not in master_edges:
                raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
            merged = master_edges[edge_id]
            for field in ("source_id", "target_id", "relation", "scope", "body"):
                if edge[field] != merged[field]:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT",
                                              edge_id=edge_id)
            if not _lineages_contained(edge["lineage"], merged["lineage"]):
                raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
        return source, master, nodes, edges, session_hash, session_sha, run_id
    except Exception:
        source.close()
        master.close()
        raise


def _table_rows() -> dict[str, list[dict[str, Any]]]:
    return {name: [] for name in ("rom_range", "rom_object", "claim", "relation",
        "source_artifact", "evidence_ref", "map_import")}


def _owned_bytes(db: sqlite3.Connection, start: int, end: int) -> int:
    return int(db.execute("SELECT COALESCE(SUM(MIN(end,?)-MAX(start,?)),0) FROM emission "
        "WHERE source_owned=1 AND start<? AND end>?", (end, start, end, start)).fetchone()[0])


def _add_object(bundle: dict[str, list[dict[str, Any]]], rom_sha: str, start: int,
                end: int, kind: str, attrs: dict[str, Any]) -> str:
    rid, oid = range_id(rom_sha, start, end), object_id(rom_sha, start, end, kind)
    bundle["rom_range"].append({"range_id": rid, "rom_sha256": rom_sha,
                                "start": start, "end": end})
    bundle["rom_object"].append({"object_id": oid, "range_id": rid, "object_type": kind,
                                 "attributes_json": canonical(attrs)})
    return oid


def _add_claim(bundle: dict[str, list[dict[str, Any]]], oid: str, kind: str,
               value: Any = True) -> str:
    payload = {"object_id": oid, "claim_type": kind, "value": value,
               "status": "OBSERVED_RUNTIME"}
    cid = stable_id("claim", payload)
    bundle["claim"].append({"claim_id": cid, "object_id": oid, "claim_type": kind,
        "value_json": canonical(value), "status": "OBSERVED_RUNTIME"})
    return cid


def _add_relation(bundle: dict[str, list[dict[str, Any]]], kind: str,
                  source_id: str, target_id: str | None,
                  target_address: int | None) -> str:
    payload = {"relation_type": kind, "source_object_id": source_id,
        "target_object_id": target_id, "target_address": target_address,
        "status": "OBSERVED_RUNTIME"}
    rid = stable_id("relation", payload)
    bundle["relation"].append({"relation_id": rid, **payload, "attributes_json": "{}"})
    return rid


def _add_evidence(bundle: dict[str, list[dict[str, Any]]], subject_type: str,
                  subject_id: str, source_sha: str, fact_kind: str,
                  count: int, locator: dict[str, Any]) -> None:
    payload = {"subject_type": subject_type, "subject_id": subject_id,
        "source_sha256": source_sha, "fact_kind": fact_kind,
        "fact_count": count, "locator": locator}
    bundle["evidence_ref"].append({"ref_id": stable_id("evidence", payload),
        "subject_type": subject_type, "subject_id": subject_id,
        "source_sha256": source_sha, "fact_kind": fact_kind,
        "fact_count": count, "locator_json": canonical(locator)})


def _range_attributes(kind: str, key: str, scope: str, status: str,
                      attrs: dict[str, Any], rom: bytes, rom_sha: str) -> tuple[int, int, bytes]:
    try:
        start, end = int(attrs["start_offset"]), int(attrs["end_offset_exclusive"])
        data = bytes.fromhex(str(attrs["bytes_hex"]))
        digest = str(attrs["bytes_sha256"])
    except (KeyError, TypeError, ValueError) as exc:
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING") from exc
    if kind != "ROM_INSTRUCTION_RANGE" or scope != rom_sha or status != "OBSERVED" or \
            attrs.get("rom_sha256") != rom_sha or start < 0 or end <= start or end > len(rom) or \
            len(data) != end - start or rom[start:end] != data or _sha_bytes(data) != digest:
        raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH")
    expected_key = f"{start:08X}:{end:08X}:{data.hex().upper()}"
    if key != expected_key or int(attrs.get("opcode", -1)) != int.from_bytes(data[:2], "big"):
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT")
    return start, end, data


def _instruction_data(nodes: dict[str, dict[str, Any]], edges: dict[str, dict[str, Any]],
                      rom: bytes, rom_sha: str, run_id: int,
                      base_db: sqlite3.Connection) -> tuple[
                      dict[str, tuple[int, int, bytes]], list[dict[str, Any]]]:
    instruction_ranges: dict[str, tuple[int, int, bytes]] = {}
    executed: list[dict[str, Any]] = []
    for edge_id, edge in edges.items():
        source, target, relation, scope, status, _ = _read_edge(edge)
        if relation != "EXECUTED_FROM_ROM":
            continue
        if status != "OBSERVED" or source not in nodes or target not in nodes:
            raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT",
                                      edge_id=edge_id)
        source_kind, source_key, source_scope, _, source_attrs = _read_node(nodes[source])
        target_kind, target_key, target_scope, target_status, target_attrs = _read_node(nodes[target])
        if source_kind != "M68K_INSTRUCTION" or source_scope != "rom:" + rom_sha:
            raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                      unmapped_fact_types=["EXECUTED_FROM_ROM_ENDPOINT"])
        start, end, data = _range_attributes(target_kind, target_key, target_scope,
                                              target_status, target_attrs, rom, rom_sha)
        try:
            pc_text, opcode_text = source_key.split(":", 1)
            pc, opcode = int(pc_text, 16), int(opcode_text, 16)
        except ValueError as exc:
            raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT") from exc
        if pc != start or opcode != int.from_bytes(data[:2], "big") or \
                source_attrs.get("pc") != pc or source_attrs.get("opcode") != opcode:
            raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT")
        current = instruction_ranges.get(source)
        if current and current != (start, end, data):
            raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT")
        instruction_ranges[source] = (start, end, data)
        lineage = _lineage(edge, run_id)
        executed.append({"edge_id": edge_id, "source_node_id": source,
                         "range_node_id": target, "occurrences": len(lineage)})
    return instruction_ranges, executed


def _import_control_relation(edge_id: str, edge: dict[str, Any],
                             nodes: dict[str, dict[str, Any]],
                             instruction_objects: dict[str, str], rom: bytes,
                             rom_sha: str, run_id: int,
                             bundle: dict[str, list[dict[str, Any]]],
                             emission_db: sqlite3.Connection) -> tuple[str, str, int]:
    source, target, relation, scope, status, _ = _read_edge(edge)
    expected_source, object_type = CONTROL_SOURCES[relation]
    if status != "OBSERVED" or scope != rom_sha or source not in nodes or target not in nodes:
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                  unmapped_fact_types=[relation])
    source_kind, source_key, source_scope, source_status, source_attrs = _read_node(nodes[source])
    if source_kind != expected_source or source_scope != rom_sha or source_status != "OBSERVED":
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                  unmapped_fact_types=[relation + ":SOURCE"])
    try:
        start, end = int(source_attrs["start_offset"]), int(source_attrs["end_offset_exclusive"])
        source_bytes = bytes.fromhex(str(source_attrs["bytes_hex"]))
        target_kind, target_key, target_scope, target_status, target_attrs = _read_node(nodes[target])
    except (KeyError, TypeError, ValueError) as exc:
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING") from exc
    if source_attrs.get("rom_sha256") != rom_sha or start < 0 or end <= start or end > len(rom) or \
            len(source_bytes) != end - start or rom[start:end] != source_bytes or \
            _sha_bytes(source_bytes) != source_attrs.get("bytes_sha256"):
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_IDENTITY_CONFLICT")
    if relation == "OBSERVED_JUMP_TABLE_ENTRY_TO" and \
            (not isinstance(source_attrs.get("selected_index"), int) or
             source_attrs.get("selected") is not True):
        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
    target_range = _range_attributes(target_kind, target_key, target_scope,
                                     target_status, target_attrs, rom, rom_sha)
    target_object = _add_object(bundle, rom_sha, *target_range[:2], "M68K_INSTRUCTION",
        {"opcode": int.from_bytes(target_range[2][:2], "big"),
         "length": target_range[1] - target_range[0],
         "bytes_sha256": _sha_bytes(target_range[2]),
         "source_owned_bytes": _owned_bytes(emission_db, *target_range[:2])})
    source_object = _add_object(bundle, rom_sha, start, end, object_type,
        {"bytes_sha256": _sha_bytes(source_bytes), "length": end - start,
         "source_owned_bytes": _owned_bytes(emission_db, start, end)})
    relation_id = _add_relation(bundle, relation, source_object, target_object, None)
    evidence_count = len(_lineage(edge, run_id))
    return relation_id, CONTROL_RELATIONS[relation], evidence_count


def import_archivist_session(session_path: Path, master_path: Path,
                             knowledge_path: Path, rom: bytes, rom_sha256: str,
                             merge_receipt: dict[str, Any]) -> dict[str, Any]:
    """Apply an accepted Archivist session through the isolated delta writer."""
    try:
        from .rom_knowledge_live_delta import import_archivist_session as apply_delta
    except ImportError:
        from rom_knowledge_live_delta import import_archivist_session as apply_delta
    return apply_delta(session_path, master_path, knowledge_path, rom,
                       rom_sha256, merge_receipt)
