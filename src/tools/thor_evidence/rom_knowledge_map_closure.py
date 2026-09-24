"""Project exact multi-source fusion paths into canonical ROM object references."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from pathlib import Path
from typing import Any

try:
    from .rom_knowledge_fusion_query import logical_graph_hash
    from .rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes, stable_id
except ImportError:
    from rom_knowledge_fusion_query import logical_graph_hash
    from rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes, stable_id


ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3_145_728
GRAPH_SHA256 = "598c42312e80e1d87a871bca75412dfa10f0d7a2eb0d5e812fc2d8a2e3f29b65"


def _exact_fused_paths(store: KnowledgeStore) -> list[dict[str, Any]]:
    """Find exact joined paths from persisted derivations, without address rules."""
    paths = []
    for derivation in store.db.execute(
        "SELECT * FROM derivation WHERE rule_id='M14_2B_RAM_SHADOW_DMA_SAT_JOIN' "
        "AND output_type='relation_path' ORDER BY derivation_id"
    ):
        result = json.loads(derivation["result_json"])
        relations = result.get("relations", [])
        if len(relations) != 2 or result.get("truth") != "DERIVED_EXACT":
            continue
        first, second = [store.db.execute("SELECT * FROM relation WHERE relation_id=?", (rid,)).fetchone()
                         for rid in relations]
        if first is None or second is None or first["status"] != "DERIVED_EXACT" or \
                second["status"] != "DERIVED_EXACT" or \
                first["relation_type"] != "RAM_SHADOW_TO_DMA" or \
                second["relation_type"] != "DMA_TO_HARDWARE_SAT" or \
                first["target_object_id"] != second["source_object_id"]:
            continue
        left, right = json.loads(first["attributes_json"]), json.loads(second["attributes_json"])
        if (left.get("dma_source_address"), left.get("destination_start"), left.get("length_bytes")) != \
                (right.get("dma_source_address"), second["target_address"], right.get("length_bytes")):
            continue
        if left.get("source_truth") != "EXACT" or \
                right.get("source_truth") != "EXACT_DMA_TO_SAT_CHAIN":
            continue
        if derivation["rule_version"] != "1" or json.loads(derivation["assumptions_json"]) != []:
            continue
        object_row = store.db.execute("""SELECT o.object_id,o.object_type,r.range_id,r.start,r.end
          FROM rom_object o JOIN rom_range r USING(range_id) WHERE o.object_id=?""",
            (first["target_object_id"],)).fetchone()
        if object_row is None or object_row["object_id"] != result.get("join_identity"):
            continue
        source_rows = []
        for relation_id, wanted_type in ((relations[0], "M14_2B:GAMEPLAY"),
                                         (relations[1], "M14_2B:SPRITE")):
            rows = list(store.db.execute("""SELECT DISTINCT s.source_sha256,s.artifact_type,s.artifact_name,
                e.locator_json FROM evidence_ref e JOIN source_artifact s USING(source_sha256)
                WHERE e.subject_type='RELATION' AND e.subject_id=? ORDER BY s.source_sha256""",
                (relation_id,)))
            matching = [row for row in rows if row["artifact_type"] == wanted_type and
                        json.loads(row["locator_json"]).get("capture_id")]
            if not matching:
                source_rows = []
                break
            source = dict(matching[0])
            source.update({"relation_id": relation_id,
                "derivation_id": derivation["derivation_id"],
                "capture_id": json.loads(source["locator_json"])["capture_id"]})
            source_rows.append(source)
        if len(source_rows) != 2 or source_rows[0]["source_sha256"] == source_rows[1]["source_sha256"]:
            continue
        captures = {json.loads(row["locator_json"]).get("capture_id") for row in source_rows}
        if len(captures) != 1 or None in captures:
            continue
        source_truth = {"M14_2B:GAMEPLAY": "EXACT", "M14_2B:SPRITE": "EXACT_DMA_TO_SAT_CHAIN"}
        if any(json.loads(row["locator_json"]).get("original_truth") != source_truth[row["artifact_type"]]
               for row in source_rows):
            continue
        derivation_inputs = {row["subject_id"] for row in store.db.execute(
            "SELECT subject_id FROM derivation_input WHERE derivation_id=?", (derivation["derivation_id"],))}
        if not set(relations).issubset(derivation_inputs):
            continue
        paths.append({"object": dict(object_row), "relations": relations,
            "derivation_id": derivation["derivation_id"], "sources": source_rows,
            "truth": "DERIVED_EXACT"})
    return paths


def _operation(paths: list[dict[str, Any]], context: dict[str, str]) -> dict[str, Any]:
    if not paths or len({path["object"]["object_id"] for path in paths}) != 1:
        raise ValueError("STOP_MAP_CLOSURE_REFERENCE_IDENTITY_INVALID")
    obj = paths[0]["object"]
    sources_by_hash: dict[str, dict[str, Any]] = {}
    for path in paths:
        for source in path["sources"]:
            item = sources_by_hash.setdefault(source["source_sha256"], {
                "source_sha256": source["source_sha256"], "artifact_type": source["artifact_type"],
                "artifact_name": source["artifact_name"], "support": []})
            item["support"].append({"capture_id": source["capture_id"],
                "relation_id": source["relation_id"], "derivation_id": source["derivation_id"]})
    sources = sorted(sources_by_hash.values(), key=lambda row: (row["artifact_type"], row["source_sha256"]))
    relation_ids = sorted({relation_id for path in paths for relation_id in path["relations"]})
    derivation_ids = sorted({path["derivation_id"] for path in paths})
    captures = sorted({source["capture_id"] for item in sources for source in item["support"]})
    payload = {"object_id": obj["object_id"], "reference_kind": "FUSED_RUNTIME_DMA_EMITTER",
        "truth": "DERIVED_EXACT"}
    claim_id = stable_id("claim", payload)
    return {"operation": "ADD_REFERENCE", "range": [obj["start"], obj["end"]],
        "object_id": obj["object_id"], "preconditions": {"range_id": obj["range_id"],
            "object_type": obj["object_type"], "expected_map_owner": _emission_owner(obj)},
        "input_facts": relation_ids, "derivation_refs": derivation_ids,
        "truth": "DERIVED_EXACT", "source_count": len({row["artifact_type"] for row in sources}),
        "capture_count": len(captures), "capture_ids": captures,
        "source_artifacts": [{"source_sha256": row["source_sha256"],
            "artifact_type": row["artifact_type"], "artifact_name": row["artifact_name"],
            "support": sorted(row["support"], key=lambda item: (item["capture_id"], item["relation_id"]))}
            for row in sources],
        "why_no_single_source_was_sufficient":
            "GAMEPLAY proves the RAM-shadow-to-DMA edge; SPRITE proves the same DMA emitter reaches hardware SAT. The shared object and exact fusion derivation join both facts.",
        "claim_id": claim_id, "claim_value": payload, "proposal_context": context}


def _emission_owner(obj: dict[str, Any]) -> dict[str, Any] | None:
    # Filled by caller when it validates the operation against the emission partition.
    return None


def _partition(store: KnowledgeStore) -> list[tuple[Any, ...]]:
    return [tuple(row) for row in store.db.execute("SELECT * FROM emission ORDER BY start,end")]


def _class_metrics(rows: list[tuple[Any, ...]]) -> dict[str, int]:
    totals = {"asm_bytes": 0, "data_table_bytes": 0, "pointer_table_bytes": 0,
              "resource_bytes": 0}
    for row in rows:
        size, emission_type, classification = int(row[1])-int(row[0]), str(row[2]), str(row[3]).upper()
        if emission_type == "ASM":
            totals["asm_bytes"] += size
        if "POINTER_TABLE" in classification:
            totals["pointer_table_bytes"] += size
        elif any(token in classification for token in ("TABLE", "RECORD", "LOOKUP", "DESCRIPTOR")):
            totals["data_table_bytes"] += size
        if emission_type == "ASSET":
            totals["resource_bytes"] += size
    return totals


def _audit_partition(rows: list[tuple[Any, ...]]) -> dict[str, int]:
    cursor, gaps, overlaps = 0, 0, 0
    for row in rows:
        start, end = int(row[0]), int(row[1])
        if start > cursor:
            gaps += 1
        elif start < cursor:
            overlaps += 1
        cursor = end
    if cursor < ROM_SIZE:
        gaps += 1
    return {"ranges": len(rows), "gaps": gaps, "overlaps": overlaps}


def _unknown_ranking(store: KnowledgeStore, limit: int = 25) -> list[dict[str, Any]]:
    unknowns = [dict(row) for row in store.db.execute("""SELECT start,end FROM emission
        WHERE source_kind IN ('UNKNOWN','UNKNOWN_DATA','UNKNOWN_WITH_EVIDENCE') ORDER BY start,end""")]
    objects = [dict(row) for row in store.db.execute("""SELECT o.object_id,r.start,r.end FROM rom_object o
        JOIN rom_range r USING(range_id) ORDER BY r.start,r.end,o.object_id""")]
    evidence = list(store.db.execute("""SELECT e.subject_type,e.subject_id,s.source_sha256,
        e.fact_kind FROM evidence_ref e JOIN source_artifact s USING(source_sha256)"""))
    source_by_subject: dict[str, set[str]] = {}
    for row in evidence:
        source_by_subject.setdefault(str(row["subject_id"]), set()).add(str(row["source_sha256"]))
    degree: dict[str, int] = {}
    refs: dict[str, int] = {}
    consumers: dict[str, int] = {}
    for row in store.db.execute("SELECT relation_id,relation_type,source_object_id,target_object_id FROM relation"):
        relation_id = str(row["relation_id"])
        for object_id in {str(row["source_object_id"]), str(row["target_object_id"] or "")} - {""}:
            degree[object_id] = degree.get(object_id, 0) + 1
            source_by_subject.setdefault(object_id, set()).update(source_by_subject.get(relation_id, set()))
            if row["relation_type"] in {"RAM_SHADOW_TO_DMA", "DMA_TO_HARDWARE_SAT"}:
                consumers[object_id] = consumers.get(object_id, 0) + 1
    for row in store.db.execute("SELECT object_id,claim_type FROM claim"):
        if "REFERENCE" in str(row["claim_type"]):
            refs[str(row["object_id"])] = refs.get(str(row["object_id"]), 0) + 1
    ranked = []
    for interval in unknowns:
        start, end = int(interval["start"]), int(interval["end"])
        overlapping = [obj for obj in objects if int(obj["start"]) < end and start < int(obj["end"])]
        ids = {str(obj["object_id"]) for obj in overlapping}
        graph_degree = sum(degree.get(item, 0) for item in ids)
        sources = set().union(*(source_by_subject.get(item, set()) for item in ids)) if ids else set()
        known_refs = sum(refs.get(item, 0) for item in ids)
        runtime_consumers = sum(consumers.get(item, 0) for item in ids)
        if not (graph_degree or sources or known_refs or runtime_consumers):
            continue
        ranked.append({"rom_start": start, "rom_end": end, "size": end-start,
            "graph_degree": graph_degree, "independent_source_count": len(sources),
            "known_references": known_refs, "known_runtime_consumers": runtime_consumers,
            "blocker": "UNKNOWN emission has graph-connected objects but no exact map operation proof.",
            "missing_proof": "Exact byte boundary and a canonical DATA_TABLE, POINTER_TABLE, RESOURCE, or other supported class proof.",
            "next_generic_method_improvement": "Join independent exact consumer and structure evidence on canonical object identity; retain byte boundaries from source facts, not analysis windows."})
    ranked.sort(key=lambda item: (-item["graph_degree"], -item["independent_source_count"],
        -item["known_references"], -item["known_runtime_consumers"], item["rom_start"], item["rom_end"]))
    for index, item in enumerate(ranked[:limit], 1):
        item["rank"] = index
    return ranked[:limit]


def _apply_reference(store: KnowledgeStore, operation: dict[str, Any]) -> None:
    context = operation["proposal_context"]
    start, end = operation["range"]
    obj = store.db.execute("""SELECT o.object_type,r.range_id,r.start,r.end FROM rom_object o
        JOIN rom_range r USING(range_id) WHERE o.object_id=?""", (operation["object_id"],)).fetchone()
    if obj is None or (int(obj["start"]), int(obj["end"])) != (start, end) or \
            obj["range_id"] != operation["preconditions"]["range_id"] or \
            obj["object_type"] != operation["preconditions"]["object_type"]:
        raise ValueError("STOP_MAP_CLOSURE_OBJECT_PRECONDITION")
    owner = store.db.execute("SELECT * FROM emission WHERE start<=? AND ?<end", (start, start)).fetchone()
    if owner is None or dict(owner) != operation["preconditions"]["expected_map_owner"]:
        raise ValueError("STOP_MAP_CLOSURE_EMISSION_PRECONDITION")
    if operation["truth"] not in {"DERIVED_EXACT", "STATIC_VERIFIED"}:
        raise ValueError("STOP_MAP_CLOSURE_NON_EXACT_TRUTH")
    relation_rows = {str(row["relation_id"]): dict(row) for row in store.db.execute(
        "SELECT relation_id,relation_type,status FROM relation WHERE relation_id IN (%s)" %
        ",".join("?" for _ in operation["input_facts"]), operation["input_facts"])}
    if len(relation_rows) != len(operation["input_facts"]) or any(
            row["status"] != "DERIVED_EXACT" for row in relation_rows.values()) or \
            {row["relation_type"] for row in relation_rows.values()} != \
                {"RAM_SHADOW_TO_DMA", "DMA_TO_HARDWARE_SAT"}:
        raise ValueError("STOP_MAP_CLOSURE_PROOF_PRECONDITION")
    sources = operation["source_artifacts"]
    if len({source["source_sha256"] for source in sources}) != len(sources) or \
            {source["artifact_type"] for source in sources} != {"M14_2B:GAMEPLAY", "M14_2B:SPRITE"} or \
            operation["source_count"] != 2:
        raise ValueError("STOP_MAP_CLOSURE_SOURCE_PRECONDITION")
    capture_roles: dict[str, set[str]] = {}
    proof_relations: set[str] = set()
    proof_derivations: set[str] = set()
    for derivation_id in operation["derivation_refs"]:
        derivation = store.db.execute("SELECT * FROM derivation WHERE derivation_id=?",
                                      (derivation_id,)).fetchone()
        if derivation is None or derivation["rule_id"] != "M14_2B_RAM_SHADOW_DMA_SAT_JOIN" or \
                derivation["rule_version"] != "1" or json.loads(derivation["assumptions_json"]) != []:
            raise ValueError("STOP_MAP_CLOSURE_DERIVATION_PRECONDITION")
        result = json.loads(derivation["result_json"])
        derivation_relations = set(result.get("relations", []))
        if result.get("truth") != "DERIVED_EXACT" or result.get("join_identity") != operation["object_id"] or \
                len(derivation_relations) != 2 or not derivation_relations.issubset(relation_rows):
            raise ValueError("STOP_MAP_CLOSURE_DERIVATION_PRECONDITION")
        proof_relations.update(derivation_relations)
        proof_derivations.add(derivation_id)
    if proof_relations != set(operation["input_facts"]) or proof_derivations != set(operation["derivation_refs"]):
        raise ValueError("STOP_MAP_CLOSURE_DERIVATION_PRECONDITION")
    for source in sources:
        expected_relation = "RAM_SHADOW_TO_DMA" if source["artifact_type"] == "M14_2B:GAMEPLAY" \
            else "DMA_TO_HARDWARE_SAT"
        if not source["support"]:
            raise ValueError("STOP_MAP_CLOSURE_SOURCE_PRECONDITION")
        for support in source["support"]:
            relation_id = support["relation_id"]
            if relation_id not in relation_rows or relation_rows[relation_id]["relation_type"] != expected_relation or \
                    support["derivation_id"] not in operation["derivation_refs"]:
                raise ValueError("STOP_MAP_CLOSURE_SOURCE_PRECONDITION")
            evidence = store.db.execute("""SELECT locator_json FROM evidence_ref
                WHERE subject_type='RELATION' AND subject_id=? AND source_sha256=?""",
                (relation_id, source["source_sha256"])).fetchone()
            locator = json.loads(evidence[0]) if evidence else {}
            source_truth = "EXACT" if source["artifact_type"] == "M14_2B:GAMEPLAY" \
                else "EXACT_DMA_TO_SAT_CHAIN"
            if evidence is None or locator.get("capture_id") != support["capture_id"] or \
                    locator.get("original_truth") != source_truth:
                raise ValueError("STOP_MAP_CLOSURE_SOURCE_EVIDENCE_PRECONDITION")
            capture_roles.setdefault(support["capture_id"], set()).add(source["artifact_type"])
    if len(capture_roles) != operation["capture_count"] or \
            set(capture_roles) != set(operation["capture_ids"]) or any(
                roles != {"M14_2B:GAMEPLAY", "M14_2B:SPRITE"} for roles in capture_roles.values()):
        raise ValueError("STOP_MAP_CLOSURE_CAPTURE_JOIN_PRECONDITION")
    claim = {"claim_id": operation["claim_id"], "object_id": operation["object_id"],
        "claim_type": "GLOBAL_EVIDENCE_REFERENCE", "value_json": canonical(operation["claim_value"]),
        "status": "DERIVED_EXACT"}
    store.insert_rows("claim", [claim])
    refs = []
    for source in operation["source_artifacts"]:
        support = source["support"]
        capture_ids = sorted({item["capture_id"] for item in support})
        locator = {"analyzer": "m14-2c-global-map-closure-v1", "graph_hash": context["graph_hash"],
            "derivation_ids": sorted({item["derivation_id"] for item in support}),
            "relation_ids": sorted({item["relation_id"] for item in support}),
            "source_sha256": source["source_sha256"], "support": support,
            "capture_ids": capture_ids}
        if len(capture_ids) == 1:
            locator["capture_id"] = capture_ids[0]
        payload = {"subject_type": "CLAIM", "subject_id": claim["claim_id"],
            "source_sha256": source["source_sha256"], "support": support,
            "fact_kind": "FUSED_RUNTIME_DMA_EMITTER_REFERENCE"}
        refs.append({"ref_id": stable_id("evidence", payload), "subject_type": "CLAIM",
            "subject_id": claim["claim_id"], "source_sha256": source["source_sha256"],
            "fact_kind": payload["fact_kind"], "fact_count": 1, "locator_json": canonical(locator)})
    store.insert_rows("evidence_ref", refs)


def run_closure(base_db: Path, child_db: Path) -> dict[str, Any]:
    source = KnowledgeStore(base_db, ROM_SHA256, ROM_SIZE, read_only=True)
    base_meta, base_hashes = source.meta(), source.hashes()
    input_graph = logical_graph_hash(source)
    if input_graph != GRAPH_SHA256 or int(source.db.execute("SELECT COUNT(*) FROM emission").fetchone()[0]) != 2489:
        raise ValueError("STOP_MAP_CLOSURE_BASELINE_MISMATCH")
    before_partition = _partition(source)
    before_classes = _class_metrics(before_partition)
    before_audit = _audit_partition(before_partition)
    before_owned = source.metrics()["source_owned_bytes"]
    paths = _exact_fused_paths(source)
    context = {"base_generation": base_meta["generation_id"], "base_map_hash": base_hashes["map_hash"],
        "emission_hash": base_hashes["emission_hash"], "graph_hash": input_graph}
    grouped_paths: dict[str, list[dict[str, Any]]] = {}
    for path in paths:
        grouped_paths.setdefault(str(path["object"]["object_id"]), []).append(path)
    operations = []
    rejected_conflicts = 0
    for object_id in sorted(grouped_paths):
        op = _operation(grouped_paths[object_id], context)
        if source.db.execute("SELECT 1 FROM conflict WHERE start<? AND ?<end LIMIT 1",
                             (op["range"][1], op["range"][0])).fetchone():
            rejected_conflicts += 1
            continue
        owner_row = source.db.execute("SELECT * FROM emission WHERE start<=? AND ?<end",
                                      (op["range"][0], op["range"][0])).fetchone()
        op["preconditions"]["expected_map_owner"] = dict(owner_row) if owner_row else None
        operations.append(op)
    operations = sorted(operations, key=lambda op: (op["range"], op["claim_id"]))
    proposal_id = stable_id("map-proposal", {"context": context, "operations": operations})
    if child_db.exists():
        child_db.unlink()
    child_db.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(base_db, child_db)
    child = KnowledgeStore(child_db, ROM_SHA256, ROM_SIZE)
    child.create_map_proposal(proposal_id, context["base_generation"], context["base_map_hash"],
        context["graph_hash"], "m14.2c-closure-v1", operations)
    child.validate_map_proposal_parent(proposal_id, context["base_generation"])
    for op in operations:
        _apply_reference(child, op)
    child.set_generation_identity("gen-m14-2c-" + input_graph[:16], context["base_generation"])
    child.db.commit()
    after_partition = _partition(child)
    after_classes = _class_metrics(after_partition)
    after_audit = _audit_partition(after_partition)
    after_owned = child.metrics()["source_owned_bytes"]
    after_hashes = child.hashes()
    if before_partition != after_partition or before_owned != after_owned or before_audit != after_audit:
        raise ValueError("STOP_MAP_CLOSURE_EMISSION_OR_OWNERSHIP_CHANGED")
    try:
        child.validate_map_proposal_parent(proposal_id, context["base_generation"])
    except ValueError as error:
        stale_parent_rejected = str(error) == "STOP_MAP_PROPOSAL_STALE_PARENT"
    else:
        stale_parent_rejected = False
    if not stale_parent_rejected:
        raise ValueError("STOP_MAP_CLOSURE_STALE_PARENT_NOT_REJECTED")
    child.db.close()
    source.db.close()
    generation_hash = hashlib.sha256()
    with child_db.open("rb") as generation_file:
        for chunk in iter(lambda: generation_file.read(1024 * 1024), b""):
            generation_hash.update(chunk)
    generation_sha = generation_hash.hexdigest()
    source = KnowledgeStore(base_db, ROM_SHA256, ROM_SIZE, read_only=True)
    unknown_ranges = int(source.db.execute("""SELECT COUNT(*) FROM emission WHERE
        source_kind IN ('UNKNOWN','UNKNOWN_DATA','UNKNOWN_WITH_EVIDENCE')""").fetchone()[0])
    unknown_bytes = source.metrics()["unknown_bytes"]
    ranked_unknown_ranges = _unknown_ranking(source, 25)
    status_counts = {table: {str(row[0]): int(row[1]) for row in source.db.execute(
        f"SELECT status,COUNT(*) FROM {table} GROUP BY status ORDER BY status")}
        for table in ("claim", "relation")}
    exact_facts = sum(counts.get(status, 0) for counts in status_counts.values()
                      for status in ("DERIVED_EXACT", "STATIC_VERIFIED", "OBSERVED_RUNTIME"))
    ineligible = {"hypothesis_facts": sum(counts.get("HYPOTHESIS", 0) for counts in status_counts.values()),
        "unresolved_facts": sum(counts.get("UNRESOLVED", 0) for counts in status_counts.values()),
        "observed_runtime_without_exact_fusion_operation": status_counts["relation"].get("OBSERVED_RUNTIME", 0)}
    source.db.close()
    return {"status": "PASS" if operations else "NO_MAP_DELTA",
        "canonical_rom_identity": {"rom_sha256": ROM_SHA256, "rom_size": ROM_SIZE,
            "metadata_matches_requested_identity": True},
        "full_rom_partition_audit": {**before_audit, "rom_size": ROM_SIZE,
            "rom_sha256": ROM_SHA256},
        "exact_graph_facts": exact_facts, "exact_graph_fact_status_counts": status_counts,
        "map_eligible_facts": sum(len(op["input_facts"]) for op in operations),
        "map_ineligible_facts": ineligible,
        "input_graph_hash": input_graph, "proposal_id": proposal_id,
        "proposal_count": len(operations), "proposal_set_hash": after_hashes["proposal_set_hash"],
        "multi_source_map_proposals": sum(op["source_count"] > 1 for op in operations),
        "proposal_operations": operations, "proposals_valid": len(operations), "proposals_rejected": rejected_conflicts,
        "parent_context": context, "stale_parent_rejected": stale_parent_rejected,
        "child_generation_id": "gen-m14-2c-" + input_graph[:16],
        "map_before": before_audit, "map_after": after_audit,
        "unknown_ranges_before": unknown_ranges, "unknown_ranges_after": unknown_ranges,
        "unknown_bytes_before": unknown_bytes, "unknown_bytes_after": unknown_bytes,
        "source_owned_before": before_owned, "source_owned_after": after_owned,
        "source_owned_delta": after_owned-before_owned,
        "new_boundaries": 0, "new_references": len(operations),
        "new_symbols": 0, "new_classified_ranges": 0, "new_classified_bytes": 0,
        "class_metrics_before": before_classes, "class_metrics_after": after_classes,
        "emission_hash_before": base_hashes["emission_hash"],
        "emission_hash_after": after_hashes["emission_hash"],
        "map_hash_before": base_hashes["map_hash"], "map_hash_after": after_hashes["map_hash"],
        "generation_sha256": generation_sha, "ranked_unknown_ranges": ranked_unknown_ranges,
        "new_range_identities": [], "historical_split_lineage": [],
        "global_object_view_object_id": operations[0]["object_id"] if operations else None}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-db", required=True, type=Path)
    parser.add_argument("--child-db", required=True, type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    report = run_closure(args.base_db, args.child_db)
    payload = canonical(report)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(payload + "\n", encoding="utf-8")
    print(payload)


if __name__ == "__main__":
    main()
