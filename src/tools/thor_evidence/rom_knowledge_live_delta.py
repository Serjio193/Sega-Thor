"""Apply one accepted MAP-1 session as a canonical delta."""

from __future__ import annotations

import sqlite3
from pathlib import Path
from typing import Any

try:
    from .rom_knowledge_live_import import (CONTROL_RELATIONS, KnowledgeImportStop,
        KnowledgeStore, _add_claim, _add_evidence, _add_object, _add_relation,
        _check_archivist_chain, _hash_json, _import_control_relation,
        _instruction_data, _lineage, _owned_bytes, _read_edge, _read_node,
        _sha_bytes, _table_rows, sha256_file)
except ImportError:
    from rom_knowledge_live_import import (CONTROL_RELATIONS, KnowledgeImportStop,
        KnowledgeStore, _add_claim, _add_evidence, _add_object, _add_relation,
        _check_archivist_chain, _hash_json, _import_control_relation,
        _instruction_data, _lineage, _owned_bytes, _read_edge, _read_node,
        _sha_bytes, _table_rows, sha256_file)


def import_archivist_session(session_path: Path, master_path: Path,
                             knowledge_path: Path, rom: bytes, rom_sha256: str,
                             merge_receipt: dict[str, Any]) -> dict[str, Any]:
    """Import one accepted MAP-1 session as a stable, replay-safe canonical delta."""
    session_db, master_db, nodes, edges, session_graph_hash, session_sha, run_id = \
        _check_archivist_chain(session_path, master_path, rom_sha256, merge_receipt)
    bundle = _table_rows()
    store = KnowledgeStore(knowledge_path, rom_sha256, len(rom))
    try:
        before_hashes, before_counts = store.hashes(), store.counts()
        meta = store.meta()
        if meta.get("schema") != "oasis.m12.canonical-rom-knowledge.v1" or \
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
        for fact in executed:
            node_id, edge_id, count = fact["source_node_id"], fact["edge_id"], fact["occurrences"]
            oid = objects_by_instruction[node_id]
            claim_id = _add_claim(bundle, oid, "EXECUTED_FROM_ROM")
            start, end, _ = instruction_ranges[node_id]
            locator = {**session_locator, "table": "map_edge", "edge_id": edge_id,
                       "range_node_id": fact["range_node_id"], "start": start, "end": end}
            _add_evidence(bundle, "CLAIM", claim_id, session_file_sha,
                          "RUNTIME_INSTRUCTION_RANGE", 1, locator)
            _add_evidence(bundle, "CLAIM", claim_id, session_file_sha,
                          "RUNTIME_INSTRUCTION_OCCURRENCE", count, locator)
            _add_evidence(bundle, "CLAIM", claim_id, master_file_sha,
                          "ARCHIVIST_ACCEPTED_RUNTIME_OCCURRENCE", count, master_locator | {
                              "table": "map_edge", "edge_id": edge_id})
        included_relations = excluded_exception_edges = excluded_exception_occurrences = 0
        for edge_id, edge in edges.items():
            source, target, relation, scope, status, _ = _read_edge(edge)
            if relation == "EXECUTED_FROM_ROM":
                continue
            if relation in CONTROL_RELATIONS:
                rid, fact_kind, count = _import_control_relation(edge_id, edge, nodes,
                    objects_by_instruction, rom, rom_sha256, run_id, bundle, store.db)
                relation_rows[edge_id] = rid
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
                    excluded_exception_occurrences += len(_lineage(edge, run_id))
                    continue
                if source not in objects_by_instruction or target not in objects_by_instruction:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING")
                count = len(_lineage(edge, run_id))
                rid = _add_relation(bundle, relation, objects_by_instruction[source],
                                    objects_by_instruction[target], None)
                relation_rows[edge_id] = rid
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
                count = len(_lineage(edge, run_id))
                rid = _add_relation(bundle, relation, objects_by_instruction[source], None, address)
                relation_rows[edge_id] = rid
                fact_kind = "RUNTIME_TERMINAL_FACT"
            elif relation == "ROM_LINK_UNRESOLVED" and status == "UNRESOLVED":
                raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                          unmapped_fact_types=[relation])
            else:
                if status in {"OBSERVED", "PROVEN"}:
                    raise KnowledgeImportStop("STOP_KNOWLEDGE_IMPORT_UNSUPPORTED_FACT",
                                              unmapped_fact_types=[relation])
                continue
            locator = {**session_locator, "table": "map_edge", "edge_id": edge_id,
                       "relation": relation}
            _add_evidence(bundle, "RELATION", rid, session_file_sha, fact_kind, count, locator)
            _add_evidence(bundle, "RELATION", rid, master_file_sha,
                "ARCHIVIST_ACCEPTED_" + fact_kind, count, master_locator | {
                    "table": "map_edge", "edge_id": edge_id})
            included_relations += 1
        bundle["map_import"].append({"import_key": import_key, "input_hash": input_hash})
        counts_before = store.counts()
        hashes_before = store.hashes()
        store.db.execute("BEGIN IMMEDIATE")
        try:
            for table, rows in bundle.items():
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
        session_db.close()
        master_db.close()
