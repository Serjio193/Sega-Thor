"""Combined object, path, and derivation queries for the M14.2B graph."""

from __future__ import annotations

import json
from typing import Any

try:
    from .rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes
except ImportError:
    from rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes


def global_object_view(store: KnowledgeStore, address_or_object: int | str) -> dict[str, Any]:
    if isinstance(address_or_object, int):
        emission = store.db.execute("SELECT * FROM emission WHERE start<=? AND ?<end",
                                    (address_or_object, address_or_object)).fetchone()
        row = store.db.execute("""SELECT o.object_id FROM rom_object o JOIN rom_range r USING(range_id)
            WHERE r.rom_sha256=? AND r.start<=? AND ?<r.end
            ORDER BY r.end-r.start,o.object_id LIMIT 1""",
            (store.meta()["rom_sha256"], address_or_object, address_or_object)).fetchone()
        object_id = str(row[0]) if row else None
    else:
        object_id = address_or_object
        emission = store.db.execute("""SELECT e.* FROM emission e JOIN rom_object o
            ON o.object_id=? JOIN rom_range r ON r.range_id=o.range_id
            WHERE e.start<=r.start AND r.start<e.end""", (object_id,)).fetchone()
    if object_id is None:
        raise ValueError("STOP_GLOBAL_OBJECT_NOT_FOUND")
    obj = store.db.execute("""SELECT o.*,r.start,r.end FROM rom_object o JOIN rom_range r USING(range_id)
        WHERE o.object_id=?""", (object_id,)).fetchone()
    relations = [dict(r) for r in store.db.execute("""SELECT * FROM relation
        WHERE source_object_id=? OR target_object_id=? OR
        (CAST(json_extract(attributes_json,'$.component[0]') AS INTEGER)<=? AND
         ?<CAST(json_extract(attributes_json,'$.component[1]') AS INTEGER))
        ORDER BY relation_id""", (object_id, object_id, int(obj["start"]), int(obj["start"])))]
    for relation in relations:
        relation["attributes"] = json.loads(relation.pop("attributes_json"))
    claims = [dict(r) for r in store.db.execute("SELECT * FROM claim WHERE object_id=? ORDER BY claim_id", (object_id,))]
    relation_ids = [item["relation_id"] for item in relations]
    evidence_sql = """SELECT e.*,s.artifact_name,s.artifact_type,s.checkpoint
        FROM evidence_ref e JOIN source_artifact s USING(source_sha256)
        WHERE e.subject_id=?
        OR e.subject_id IN (SELECT claim_id FROM claim WHERE object_id=?)"""
    evidence_args: tuple[Any, ...] = (object_id, object_id)
    if relation_ids:
        evidence_sql += " OR e.subject_id IN (" + ",".join("?" for _ in relation_ids) + ")"
        evidence_args += tuple(relation_ids)
    evidence_sql += " ORDER BY e.ref_id"
    evidence = [dict(r) for r in store.db.execute(evidence_sql, evidence_args)]
    for item in evidence:
        item["locator"] = json.loads(item.pop("locator_json"))
    derivation_sql = """SELECT DISTINCT d.* FROM derivation d
        LEFT JOIN derivation_input i USING(derivation_id) WHERE d.output_id=?"""
    derivation_args: tuple[Any, ...] = (object_id,)
    if relation_ids:
        placeholders = ",".join("?" for _ in relation_ids)
        derivation_sql += f" OR d.output_id IN ({placeholders}) OR i.subject_id IN ({placeholders})"
        derivation_args += tuple(relation_ids) + tuple(relation_ids)
    derivation_sql += " ORDER BY d.derivation_id"
    derivations = [dict(r) for r in store.db.execute(derivation_sql, derivation_args)]
    owned = store.db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0]
    return {"object": dict(obj), "canonical_map_owner": dict(emission) if emission else None,
        "source_owned_total": int(owned), "claims": claims, "relations": relations,
        "evidence": evidence, "derivations": derivations,
        "incoming_relations": [r for r in relations if r["target_object_id"] == object_id],
        "outgoing_relations": [r for r in relations if r["source_object_id"] == object_id],
        "runtime_occurrences": [e for e in evidence if
            e["fact_kind"].startswith("RUNTIME_OCCURRENCE:") or
            e["fact_kind"] == "RUNTIME_INSTRUCTION_OCCURRENCE"],
        "supporting_artifacts": sorted({e["artifact_name"] for e in evidence}),
        "supporting_analyzers": sorted({e["locator"].get("analyzer", e["artifact_type"])
                                         for e in evidence}),
        "supporting_captures": sorted({capture for e in evidence
            for capture in ([e["locator"]["capture_id"]] if e["locator"].get("capture_id")
                            else e["locator"].get("capture_ids", [])) if capture}),
        "hypotheses": [c for c in claims if c["status"] == "HYPOTHESIS"],
        "conflicts": [dict(r) for r in store.db.execute("SELECT * FROM conflict WHERE start<? AND ?<end",
            (int(obj["end"]), int(obj["start"]))) ]}


def why(store: KnowledgeStore, relation_id: str) -> dict[str, Any]:
    row = store.db.execute("""SELECT DISTINCT d.* FROM derivation d LEFT JOIN derivation_input i
        USING(derivation_id) WHERE d.output_id=? OR i.subject_id=?
        ORDER BY d.derivation_id LIMIT 1""", (relation_id, relation_id)).fetchone()
    if row is None:
        raise ValueError("STOP_WHY_DERIVATION_NOT_FOUND")
    derivation = dict(row)
    derivation["inputs"] = [dict(r) for r in store.db.execute("SELECT * FROM derivation_input WHERE derivation_id=? ORDER BY ordinal",
                                                             (row["derivation_id"],))]
    derivation["source_evidence"] = [dict(r) for r in store.db.execute("""SELECT e.*,s.artifact_name
        FROM evidence_ref e JOIN source_artifact s USING(source_sha256) WHERE e.subject_id IN
        (SELECT subject_id FROM derivation_input WHERE derivation_id=?) ORDER BY e.ref_id""",
        (row["derivation_id"],))]
    return derivation


def logical_graph_hash(store: KnowledgeStore) -> str:
    tables = ("rom_object", "claim", "relation", "derivation", "derivation_input", "conflict", "map_proposal", "map_proposal_operation")
    data = {table: [tuple(row) for row in store.db.execute(f"SELECT * FROM {table} ORDER BY 1,2")]
            for table in tables}
    return sha256_bytes(canonical(data).encode())


def witness_path(store: KnowledgeStore, capture_id: str) -> dict[str, Any]:
    rows = list(store.db.execute("""SELECT r.*,s.artifact_type FROM relation r
      JOIN evidence_ref e ON e.subject_type='RELATION' AND e.subject_id=r.relation_id
      JOIN source_artifact s USING(source_sha256)
      WHERE json_extract(e.locator_json,'$.capture_id')=?
      AND r.relation_type IN ('RAM_SHADOW_TO_DMA','DMA_TO_HARDWARE_SAT')
      ORDER BY r.relation_type,r.relation_id""", (capture_id,)))
    shadow = [row for row in rows if row["relation_type"] == "RAM_SHADOW_TO_DMA"]
    sats = [row for row in rows if row["relation_type"] == "DMA_TO_HARDWARE_SAT"]
    for first in shadow:
        for second in sats:
            if first["target_object_id"] != second["source_object_id"]:
                continue
            left, right = json.loads(first["attributes_json"]), json.loads(second["attributes_json"])
            if (left["ram_address"], left["destination_start"], left["length_bytes"]) != \
                    (right["dma_source_address"], second["target_address"], right["length_bytes"]):
                continue
            emitter = store.db.execute("SELECT object_type FROM rom_object WHERE object_id=?",
                                       (second["source_object_id"],)).fetchone()
            return {"capture_id": capture_id,
                "nodes": [{"kind": "RAM_SHADOW", "address": left["ram_address"]},
                    {"kind": "CANONICAL_ROM_OBJECT", "object_id": first["target_object_id"],
                     "object_type": str(emitter[0]) if emitter else "UNKNOWN"},
                    {"kind": "HARDWARE_SAT", "vram_address": second["target_address"],
                     "entries": right["sat_entries"]}],
                "relations": [first["relation_id"], second["relation_id"]],
                "derivations": [str(row[0]) for row in store.db.execute("""SELECT DISTINCT d.derivation_id
                    FROM derivation d JOIN derivation_input i USING(derivation_id)
                    WHERE i.subject_id IN (?,?) ORDER BY d.derivation_id""",
                    (first["relation_id"], second["relation_id"]))],
                "truth": [first["status"], second["status"]],
                "sources": sorted({str(first["artifact_type"]), str(second["artifact_type"])}),
                "connected": True}
    raise ValueError("STOP_FUSION_WITNESS_PATH_NOT_FOUND")
