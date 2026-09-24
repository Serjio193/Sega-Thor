"""Apply one accepted MAP-1 session as a canonical delta."""

from __future__ import annotations

import sqlite3
import hashlib
import json
from pathlib import Path
from typing import Any

try:
    from .rom_knowledge_map import canonical, runtime_occurrence_id, stable_id
    from .rom_knowledge_map import SCHEMA
    from .rom_knowledge_live_import import (CONTROL_RELATIONS, KnowledgeImportStop,
        KnowledgeStore, _add_claim, _add_evidence, _add_object, _add_relation,
        _check_archivist_chain, _hash_json, _import_control_relation,
        _instruction_data, _lineage_count, _owned_bytes, _read_edge, _read_node,
        _sha_bytes, _table_rows, sha256_file, _check_archivist_chain_graph)
except ImportError:
    from rom_knowledge_map import canonical, runtime_occurrence_id, stable_id
    from rom_knowledge_map import SCHEMA
    from rom_knowledge_live_import import (CONTROL_RELATIONS, KnowledgeImportStop,
        KnowledgeStore, _add_claim, _add_evidence, _add_object, _add_relation,
        _check_archivist_chain, _hash_json, _import_control_relation,
        _instruction_data, _lineage_count, _owned_bytes, _read_edge, _read_node,
        _sha_bytes, _table_rows, sha256_file, _check_archivist_chain_graph)


def import_archivist_session(session_path: Path, master_path: Path,
                             knowledge_path: Path, rom: bytes, rom_sha256: str,
                             merge_receipt: dict[str, Any], session_graph: Any = None,
                             session_source_sha256: str | None = None) -> dict[str, Any]:
    """Import one accepted MAP-1 session as a stable, replay-safe canonical delta."""
    if session_graph is None:
        session_db, master_db, nodes, edges, session_graph_hash, session_sha, run_id = \
            _check_archivist_chain(session_path, master_path, rom_sha256, merge_receipt)
    else:
        session_sha = session_source_sha256 or ""
        session_db, master_db, nodes, edges, session_graph_hash, session_sha, run_id = \
            _check_archivist_chain_graph(session_graph, master_path, rom_sha256,
                                         merge_receipt, session_sha)
    bundle = _table_rows()
    store = KnowledgeStore(knowledge_path, rom_sha256, len(rom))
    try:
        before_hashes, before_counts = store.hashes(), store.counts()
        meta = store.meta()
        if meta.get("schema") != SCHEMA or \
                meta.get("rom_sha256") != rom_sha256 or int(meta.get("rom_size", -1)) != len(rom):
            raise KnowledgeImportStop("STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH")
        session_id = str(merge_receipt["session_id"])
        import_key = "M12-ARCHIVIST-SESSION:" + session_id
        input_hash = _hash_json({"rom_sha256": rom_sha256, "session_id": session_id,
            "session_graph_hash": session_graph_hash, "source_artifact_sha256": session_sha})
        existing = store.db.execute("SELECT input_hash FROM map_import WHERE import_key=?",
                                    (import_key,)).fetchone()
        if existing:
            if str(existing[0]) != input_hash:
                raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_NONIDEMPOTENT")
            return {"status": "PASS_IDEMPOTENT_NOOP", "import_key": import_key,
                "input_hash": input_hash, "objects_added": 0, "claims_added": 0,
                "relations_added": 0, "evidence_refs_added": 0,
                "counts_before": before_counts, "counts_after": before_counts,
                "hashes_before": before_hashes, "hashes_after": before_hashes,
                "session_graph_hash": session_graph_hash, "session_source_sha256": session_sha}

        occurrences = _runtime_occurrences(session_db, run_id)

        for row in store.db.execute("SELECT source_sha256 FROM source_artifact"):
            if len(str(row[0])) != 64:
                raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
        instruction_ranges, executed = _instruction_data(
            nodes, edges, rom, rom_sha256, run_id, store.db)
        objects_by_instruction: dict[str, str] = {}
        for node_id, (start, end, data) in instruction_ranges.items():
            oid = _add_object(bundle, rom_sha256, start, end, "M68K_INSTRUCTION",
                {"opcode": int.from_bytes(data[:2], "big"), "length": end - start,
                 "bytes_sha256": _sha_bytes(data),
                 "source_owned_bytes": _owned_bytes(store.db, start, end)})
            objects_by_instruction[node_id] = oid
        session_file_sha = session_sha
        master_file_sha = sha256_file(master_path)
        bundle["source_artifact"].extend([
            {"source_sha256": session_file_sha, "checkpoint": "M12-ARCHIVIST-CANONICAL-KNOWLEDGE-2G",
             "artifact_name": f"closed-map1-session-{session_id}.sqlite",
             "artifact_type": "ARCHIVIST_ACCEPTED_SESSION"},
            {"source_sha256": master_file_sha, "checkpoint": "M12-ARCHIVIST-CANONICAL-KNOWLEDGE-2G",
             "artifact_name": "accepted-archivist-master.sqlite",
             "artifact_type": "ARCHIVIST_ACCEPTED_MASTER"}])
        session_locator = {"session_id": session_id,
            "session_graph_sha256": session_graph_hash,
            "master_graph_sha256": merge_receipt["master_graph_hash_after"],
            "run_id": run_id}
        master_locator = {**session_locator, "archivist_receipt_sha256":
                          merge_receipt["receipt_sha256"]}
        relation_rows: dict[str, str] = {}
        relation_sources: dict[str, tuple[str, str, str]] = {}
        claims_by_instruction: dict[str, str] = {}
        for fact in executed:
            node_id, edge_id, count = fact["source_node_id"], fact["edge_id"], fact["occurrences"]
            oid = objects_by_instruction[node_id]
            claim_id = _add_claim(bundle, oid, "EXECUTED_FROM_ROM")
            claims_by_instruction[node_id] = claim_id
            start, end, _ = instruction_ranges[node_id]
            locator = {**session_locator, "table": "map_edge", "edge_id": edge_id,
                       "range_node_id": fact["range_node_id"], "start": start, "end": end}
            _add_evidence(bundle, "CLAIM", claim_id, session_file_sha,
                          "RUNTIME_INSTRUCTION_RANGE", 1, locator)
        included_relations = excluded_exception_edges = excluded_exception_occurrences = 0
        for edge_id, edge in edges.items():
            source, target, relation, scope, status, _ = _read_edge(edge)
            if relation == "EXECUTED_FROM_ROM":
                continue
            if relation in CONTROL_RELATIONS:
                rid, fact_kind, count = _import_control_relation(edge_id, edge, nodes,
                    objects_by_instruction, rom, rom_sha256, run_id, bundle, store.db)
                relation_rows[edge_id] = rid
                relation_sources[edge_id] = (source, target, relation)
            elif relation == "EXECUTED_NEXT":
                if source not in nodes or target not in nodes:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
                source_kind = str(nodes[source]["kind"])
                target_kind = str(nodes[target]["kind"])
                if source_kind != "M68K_INSTRUCTION" or target_kind != "M68K_INSTRUCTION":
                    if "EXCEPTION_EVENT" not in source_kind + target_kind:
                        raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                                  unmapped_fact_types=[relation + ":" + source_kind + ":" + target_kind])
                    excluded_exception_edges += 1
                    excluded_exception_occurrences += _lineage_count(edge, run_id)
                    continue
                if source not in objects_by_instruction or target not in objects_by_instruction:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
                count = _lineage_count(edge, run_id)
                rid = _add_relation(bundle, relation, objects_by_instruction[source],
                                    objects_by_instruction[target], None)
                relation_rows[edge_id] = rid
                relation_sources[edge_id] = (source, target, relation)
                fact_kind = "RUNTIME_NEXT_OCCURRENCE"
            elif relation == "OBSERVED_NEXT_PC":
                if source not in objects_by_instruction or target not in nodes:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
                target_kind, _, target_scope, _, target_attrs = _read_node(nodes[target])
                if target_kind != "M68K_TARGET_ADDRESS" or target_scope != "rom:" + rom_sha256:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                              unmapped_fact_types=[relation + ":TARGET"])
                address = int(target_attrs.get("raw_next_pc", -1))
                if address < 0:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
                count = _lineage_count(edge, run_id)
                rid = _add_relation(bundle, relation, objects_by_instruction[source], None, address)
                relation_rows[edge_id] = rid
                relation_sources[edge_id] = (source, target, relation)
                fact_kind = "RUNTIME_TERMINAL_FACT"
            elif relation == "ROM_LINK_UNRESOLVED" and status == "UNRESOLVED":
                raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                          unmapped_fact_types=[relation])
            else:
                if status in {"OBSERVED", "PROVEN"}:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                              unmapped_fact_types=[relation])
                continue
            included_relations += 1
        _admit_occurrence_evidence(bundle, occurrences, session_file_sha,
                                   claims_by_instruction, relation_rows,
                                   relation_sources, nodes)
        bundle["map_import"].append({"import_key": import_key, "input_hash": input_hash})
        counts_before = store.counts()
        hashes_before = store.hashes()
        # Read-only validation helpers may leave a driver transaction open on
        # some SQLite builds; close it before the single atomic write phase.
        if store.db.in_transaction:
            store.db.rollback()
        store.db.execute("BEGIN IMMEDIATE")
        try:
            for table, rows in bundle.items():
                if table == "evidence_ref":
                    for row in rows:
                        _insert_evidence_ref(store.db, row)
                else:
                    store.insert_rows(table, rows)
            store.db.commit()
        except Exception:
            store.db.rollback()
            raise
        counts_after, hashes_after = store.counts(), store.hashes()
        delta = {table: counts_after[table] - counts_before[table] for table in counts_before}
        return {"status": "PASS_IMPORTED", "import_key": import_key,
            "input_hash": input_hash, "objects_added": delta["rom_object"],
            "claims_added": delta["claim"], "relations_added": delta["relation"],
            "evidence_refs_added": delta["evidence_ref"], "source_artifacts_added": delta["source_artifact"],
            "map_import_rows_added": delta["map_import"], "counts_before": counts_before,
            "counts_after": counts_after, "hashes_before": hashes_before,
            "hashes_after": hashes_after, "session_graph_hash": session_graph_hash,
            "session_source_sha256": session_sha,
            "master_source_sha256": master_file_sha,
            "executed_instruction_edges": len(executed),
            "instruction_occurrences": sum(row["occurrences"] for row in executed),
            "canonical_executed_next_relations": sum(1 for edge in edges.values()
                if edge["relation"] == "EXECUTED_NEXT" and edge["source_id"] in objects_by_instruction
                and edge["target_id"] in objects_by_instruction),
            "canonical_terminal_facts": sum(1 for edge in edges.values()
                if edge["relation"] == "OBSERVED_NEXT_PC"),
            "canonical_control_relations": {key: sum(1 for row in edges.values()
                if row["relation"] == key) for key in CONTROL_RELATIONS},
            "exception_flow_edges_excluded": excluded_exception_edges,
            "exception_flow_occurrences_excluded": excluded_exception_occurrences,
            "imported_relation_edges": included_relations}
    finally:
        store.close()
        if session_graph is None:
            session_db.close()
            master_db.close()


def _runtime_occurrences(session_db: sqlite3.Connection,
                         run_id: int) -> list[dict[str, Any]]:
    present = session_db.execute("SELECT 1 FROM sqlite_master WHERE type='table' "
                                 "AND name='live_forward_runtime_occurrence'").fetchone()
    if not present:
        raise KnowledgeImportStop("STOP_RUNTIME_OCCURRENCE_TABLE_MISSING")
    found = []
    for occurrence_id, encoded in session_db.execute(
            "SELECT occurrence_id,event_json FROM live_forward_runtime_occurrence "
            "ORDER BY occurrence_id"):
        try:
            event = json.loads(encoded)
            expected = runtime_occurrence_id(capture_id=event["capture_id"],
                epoch=int(event["epoch"]), cpu=event["cpu_id"],
                address_space=event["address_space"],
                native_sequence=int(event["native_sequence"]),
                event_kind=event["event_kind"], run_id=int(event["run_id"]),
                instruction_sequence=int(event["instruction_sequence"]))
        except (KeyError, TypeError, ValueError, json.JSONDecodeError) as exc:
            raise KnowledgeImportStop("STOP_RUNTIME_OCCURRENCE_IDENTITY_INVALID") from exc
        if expected != occurrence_id or int(event["run_id"]) != run_id or \
                not event.get("capture_ids") or not event.get("windows"):
            raise KnowledgeImportStop("STOP_RUNTIME_OCCURRENCE_IDENTITY_INVALID")
        found.append(event)
    if not found:
        raise KnowledgeImportStop("STOP_RUNTIME_OCCURRENCE_EVIDENCE_MISSING")
    return found


def _runtime_evidence_row(bundle: dict[str, list[dict[str, Any]]],
                          event: dict[str, Any], subject_type: str,
                          subject_id: str, session_sha: str) -> None:
    occurrence_id = event["occurrence_id"]
    stable_content = {key: value for key, value in event.items()
                      if key not in {"capture_ids", "windows"}}
    source_sha = hashlib.sha256(canonical(stable_content).encode()).hexdigest()
    bundle["source_artifact"].append({"source_sha256": source_sha,
        "checkpoint": "M14.2A-SCOPED-RUNTIME-OCCURRENCE-V1",
        "artifact_name": "occurrence-" + occurrence_id.rsplit(":", 1)[-1],
        "artifact_type": "NORMALIZED_RUNTIME_OCCURRENCE"})
    locator = {**event, "session_source_sha256s": [session_sha]}
    ref_id = stable_id("runtime-evidence", {"occurrence_id": occurrence_id,
        "subject_type": subject_type, "subject_id": subject_id})
    bundle["evidence_ref"].append({"ref_id": ref_id,
        "subject_type": subject_type, "subject_id": subject_id,
        "source_sha256": source_sha,
        "fact_kind": "RUNTIME_OCCURRENCE:" + event["event_kind"],
        "fact_count": 1, "locator_json": canonical(locator)})


def _admit_occurrence_evidence(bundle: dict[str, list[dict[str, Any]]],
                               occurrences: list[dict[str, Any]], session_sha: str,
                               claims: dict[str, str], relations: dict[str, str],
                               relation_sources: dict[str, tuple[str, str, str]],
                               nodes: dict[str, dict[str, Any]]) -> None:
    by_node: dict[str, list[dict[str, Any]]] = {}
    for event in occurrences:
        node_id = event.get("instruction_node_id")
        if node_id:
            by_node.setdefault(str(node_id), []).append(event)
        _runtime_evidence_row(bundle, event, "RUNTIME_OCCURRENCE",
                              event["occurrence_id"], session_sha)
        if node_id in claims:
            _runtime_evidence_row(bundle, event, "CLAIM", claims[node_id], session_sha)
        edge_id = event.get("edge_id")
        if event["event_kind"] == "EXECUTED_NEXT" and edge_id in relations:
            _runtime_evidence_row(bundle, event, "RELATION", relations[edge_id], session_sha)
    for edge_id, relation_id in relations.items():
        source, target, relation = relation_sources[edge_id]
        if relation == "EXECUTED_NEXT":
            supported = [event for event in occurrences
                         if event.get("event_kind") == "EXECUTED_NEXT" and
                         event.get("edge_id") == edge_id]
        else:
            supported = by_node.get(source, [])
            if not supported and relation in CONTROL_RELATIONS:
                target_node = nodes.get(target)
                if target_node is not None:
                    try:
                        start = int(json.loads(target_node["body"]).get("attributes", {}).get(
                            "start_offset", -1))
                    except (KeyError, TypeError, ValueError, json.JSONDecodeError):
                        start = -1
                    supported = [event for event in occurrences
                        if event.get("event_kind") == "INSTRUCTION" and
                        int(event.get("pc", -2)) == start]
        if not supported:
            raise KnowledgeImportStop("STOP_RUNTIME_RELATION_OCCURRENCE_MISSING",
                                      edge_id=edge_id)
        for event in supported:
            _runtime_evidence_row(bundle, event, "RELATION", relation_id, session_sha)


def _insert_evidence_ref(db: sqlite3.Connection, row: dict[str, Any]) -> None:
    existing = db.execute("SELECT subject_type,subject_id,source_sha256,fact_kind,"
        "fact_count,locator_json FROM evidence_ref WHERE ref_id=?", (row["ref_id"],)).fetchone()
    values = (row["subject_type"], row["subject_id"], row["source_sha256"],
              row["fact_kind"], row["fact_count"])
    if existing is None:
        db.execute("INSERT INTO evidence_ref VALUES (?,?,?,?,?,?,?)",
            (row["ref_id"], *values, row["locator_json"]))
        return
    if tuple(existing[:5]) != values:
        raise KnowledgeImportStop("STOP_RUNTIME_OCCURRENCE_IDENTITY_CONFLICT")
    old_locator, new_locator = json.loads(existing[5]), json.loads(row["locator_json"])
    for key in ("capture_ids", "windows", "session_source_sha256s"):
        values_union = list(old_locator.get(key, [])) + list(new_locator.get(key, []))
        old_locator[key] = sorted({canonical(item): item for item in values_union}.values(),
                                  key=canonical)
    for key, value in new_locator.items():
        if key not in {"capture_ids", "windows", "session_source_sha256s"} and \
                key not in old_locator:
            old_locator[key] = value
    old_locator["session_source_sha256s"] = sorted(set(
        old_locator.get("session_source_sha256s", [])) |
        {str(old_locator.pop("session_source_sha256", "")),
         str(new_locator.pop("session_source_sha256", ""))} - {""})
    db.execute("UPDATE evidence_ref SET locator_json=? WHERE ref_id=?",
               (canonical(old_locator), row["ref_id"]))


def import_archivist_graph(session_graph: Any, master_path: Path, knowledge_path: Path,
                           rom: bytes, rom_sha256: str, merge_receipt: dict[str, Any],
                           session_source_sha256: str) -> dict[str, Any]:
    """RAM Stage 5 adapter; no session SQLite path is created."""
    return import_archivist_session(Path("<ram-session>"), master_path, knowledge_path,
                                    rom, rom_sha256, merge_receipt, session_graph,
                                    session_source_sha256)
