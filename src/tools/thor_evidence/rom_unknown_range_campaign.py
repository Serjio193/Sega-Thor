"""Rank and screen UNKNOWN canonical ROM ranges using existing graph evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

try:
    from .rom_knowledge_fusion_query import logical_graph_hash
    from .rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes
except ImportError:
    from rom_knowledge_fusion_query import logical_graph_hash
    from rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes


ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3_145_728
UNKNOWN_KINDS = ("UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE")
ADDRESS_KEYS = {"rom_address", "rom_start", "rom_end", "target_rom_address",
                "source_rom_address", "pointer_target"}


def _walk_addresses(value: Any, key: str = "") -> list[int]:
    found: list[int] = []
    if isinstance(value, dict):
        for child_key, child in value.items():
            if child_key.lower() in ADDRESS_KEYS and isinstance(child, int):
                found.append(child)
            found.extend(_walk_addresses(child, child_key))
    elif isinstance(value, list):
        for child in value:
            found.extend(_walk_addresses(child, key))
    return found


def _unknown_rows(store: KnowledgeStore) -> list[dict[str, int]]:
    placeholders = ",".join("?" for _ in UNKNOWN_KINDS)
    return [dict(row) for row in store.db.execute(
        f"SELECT start,end FROM emission WHERE source_kind IN ({placeholders}) ORDER BY start,end",
        UNKNOWN_KINDS)]


def rank_unknown_ranges(store: KnowledgeStore) -> list[dict[str, Any]]:
    """Rank every UNKNOWN emission; rankings and hypothesis labels are not facts."""
    objects = [dict(row) for row in store.db.execute("""SELECT o.object_id,o.object_type,
        r.start,r.end FROM rom_object o JOIN rom_range r USING(range_id)
        ORDER BY r.start,r.end,o.object_id""")]
    object_by_id = {row["object_id"]: row for row in objects}
    evidence_by_subject: dict[str, list[dict[str, Any]]] = {}
    for row in store.db.execute("""SELECT e.subject_type,e.subject_id,e.fact_kind,s.source_sha256,
        s.artifact_type,e.locator_json FROM evidence_ref e JOIN source_artifact s USING(source_sha256)
        ORDER BY e.ref_id"""):
        evidence_by_subject.setdefault(str(row["subject_id"]), []).append(dict(row))
    relations = [dict(row) for row in store.db.execute("SELECT * FROM relation ORDER BY relation_id")]
    claims = [dict(row) for row in store.db.execute("SELECT * FROM claim ORDER BY claim_id")]
    claims_by_object: dict[str, list[dict[str, Any]]] = {}
    for claim in claims:
        claims_by_object.setdefault(str(claim["object_id"]), []).append(claim)

    ranked: list[dict[str, Any]] = []
    for interval in _unknown_rows(store):
        start, end = int(interval["start"]), int(interval["end"])
        overlapping = [obj for obj in objects if int(obj["start"]) < end and start < int(obj["end"])]
        ids = {str(obj["object_id"]) for obj in overlapping}
        relevant_relations = [rel for rel in relations if rel["source_object_id"] in ids or
                              rel["target_object_id"] in ids or
                              (rel["target_address"] is not None and start <= int(rel["target_address"]) < end)]
        source_hashes = {ev["source_sha256"] for obj_id in ids
                         for ev in evidence_by_subject.get(obj_id, [])}
        source_types = {ev["artifact_type"] for obj_id in ids
                        for ev in evidence_by_subject.get(obj_id, [])}
        for rel in relevant_relations:
            relation_evidence = evidence_by_subject.get(rel["relation_id"], [])
            source_hashes.update(ev["source_sha256"] for ev in relation_evidence)
            source_types.update(ev["artifact_type"] for ev in relation_evidence)
        executed = {str(obj["object_id"]) for obj in overlapping
                    if obj["object_type"] == "M68K_INSTRUCTION" and any(
                        claim["claim_type"] == "EXECUTED_FROM_ROM" and
                        claim["status"] == "OBSERVED_RUNTIME"
                        for claim in claims_by_object.get(str(obj["object_id"]), []))}
        incoming_address = {str(rel["relation_id"]) for rel in relations
                            if rel["target_address"] is not None and
                            start <= int(rel["target_address"]) < end}
        incoming_object = {str(rel["relation_id"]) for rel in relations
                           if rel["target_object_id"] in ids and rel["source_object_id"] not in ids}
        incoming_refs = incoming_address | incoming_object
        pointer_refs = set()
        for rel in relations:
            attrs = json.loads(rel["attributes_json"])
            if any(start <= address < end for address in _walk_addresses(attrs)):
                pointer_refs.add(str(rel["relation_id"]))
        consumers = {str(rel["relation_id"]) for rel in relevant_relations
                     if rel["relation_type"] in {"RAM_SHADOW_TO_DMA", "DMA_TO_HARDWARE_SAT"}}
        hypothesis_kinds: set[str] = set()
        for obj_id in ids:
            for claim in claims_by_object.get(obj_id, []):
                if claim["status"] == "HYPOTHESIS":
                    value = json.loads(claim["value_json"])
                    hypothesis_kinds.add(str(value.get("candidate_kind", "unspecified")))
        boundaries = {int(obj[key]) for obj in overlapping for key in ("start", "end")
                      if start < int(obj[key]) < end}
        adjacent = []
        for row in store.db.execute("SELECT start,end,source_kind FROM emission WHERE end=? OR start=?",
                                    (start, end)):
            adjacent.append(str(row["source_kind"]))
        kinds = {str(rel["relation_type"]) for rel in relevant_relations}
        if not overlapping:
            blocker = "NO_CANONICAL_OBJECT_OR_EXACT_BOUNDARY"
        elif any(obj["object_type"] == "M68K_INSTRUCTION" for obj in overlapping):
            blocker = "NO_CLOSED_CFG_OR_ROM_BYTE_ROUNDTRIP"
        elif consumers:
            blocker = "CONSUMER_PRESENT_BUT_TABLE_OR_RESOURCE_EXTENT_UNPROVEN"
        elif hypothesis_kinds:
            blocker = "HYPOTHESIS_ONLY_BOUNDARY_OR_FORMAT"
        else:
            blocker = "NO_EXACT_CONSUMER_OR_EXTENT_PROOF"
        missing: set[str] = set()
        if executed:
            missing.update({"CANONICAL_ROM_BYTES", "CLOSED_CONTROL_FLOW", "ASSEMBLER_ROUNDTRIP"})
        elif hypothesis_kinds:
            if hypothesis_kinds & {"decoder_eos", "resource_length_candidate"}:
                missing.update({"DECODER_OR_CONSUMER", "RESOURCE_LENGTH"})
            if hypothesis_kinds & {"count_times_stride", "monotonic_pointer_family"}:
                missing.update({"TABLE_COUNT", "ADDRESSING_INTERPRETATION"})
            if hypothesis_kinds & {"sentinel_termination"}:
                missing.add("EXACT_BOUNDARY")
            if hypothesis_kinds & {"uniform_alignment_padding"}:
                missing.add("STRUCTURAL_PADDING_PROOF")
            if not missing:
                missing.add("EXACT_BOUNDARY_OR_CONSUMER")
        else:
            missing.update({"EXACT_CONSUMER", "EXACT_EXTENT"})
        counts = {
            "executed_pc_count": len(executed),
            "rom_read_count": 0,
            "incoming_references": len(incoming_refs),
            "direct_branch_call_targets": 0,
            "outgoing_references": sum(rel["source_object_id"] in ids for rel in relations),
            "pointer_references": len(pointer_refs),
            "dma_resource_reads": sum(rel["relation_type"] in {"RAM_SHADOW_TO_DMA", "DMA_TO_HARDWARE_SAT"}
                                       for rel in relevant_relations),
            "audio_reads": 0,
            "known_consumers": len(consumers),
            "graph_degree": len(relevant_relations),
            "independent_source_count": len(source_hashes),
            "independent_source_types": sorted(source_types),
            "exact_candidate_boundaries": len(boundaries),
            "adjacent_verified_kinds": sorted(set(adjacent)),
            "hypothesis_kinds": sorted(hypothesis_kinds),
            "observed_relation_types": sorted(kinds),
            "missing_capabilities": sorted(missing),
        }
        fingerprint_payload = {"range": [start, end], "blocker": blocker,
                               "signals": counts}
        fingerprint = hashlib.sha256(canonical(fingerprint_payload).encode()).hexdigest()
        score = (counts["executed_pc_count"] * 8 + counts["incoming_references"] * 6 +
                 counts["pointer_references"] * 5 + counts["known_consumers"] * 4 +
                 counts["independent_source_count"] * 3 + counts["graph_degree"] +
                 counts["exact_candidate_boundaries"])
        ranked.append({"rom_start": start, "rom_end": end, "size": end - start,
                       "likely_generic_class": sorted(hypothesis_kinds) or ["UNCLASSIFIED"],
                       "exact_blocker": blocker, "blocker_fingerprint": fingerprint,
                       "priority_score": score, **counts})
    ranked.sort(key=lambda item: (-item["priority_score"], -item["executed_pc_count"],
                                  -item["incoming_references"], -item["graph_degree"],
                                  item["rom_start"], item["rom_end"]))
    for rank, item in enumerate(ranked, 1):
        item["rank"] = rank
    return ranked


def campaign_report(database: Path, top: int = 25) -> dict[str, Any]:
    store = KnowledgeStore(database, ROM_SHA256, ROM_SIZE, read_only=True)
    try:
        map_hashes = store.hashes()
        rankings = rank_unknown_ranges(store)
        unknown_bytes = sum(item["size"] for item in rankings)
        owned = store.metrics()["source_owned_bytes"]
        partition = {"ranges": int(store.db.execute("SELECT COUNT(*) FROM emission").fetchone()[0]),
                     "rom_bytes": int(store.db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission").fetchone()[0]),
                     "gaps": 0, "overlaps": 0}
        cursor = 0
        for row in store.db.execute("SELECT start,end FROM emission ORDER BY start,end"):
            if int(row["start"]) != cursor:
                if int(row["start"]) < cursor:
                    partition["overlaps"] += 1
                else:
                    partition["gaps"] += 1
            cursor = int(row["end"])
        if cursor != ROM_SIZE:
            partition["gaps"] += 1
        blockers: dict[str, int] = {}
        capabilities: dict[str, int] = {}
        for item in rankings:
            blockers[item["exact_blocker"]] = blockers.get(item["exact_blocker"], 0) + 1
            for capability in item["missing_capabilities"]:
                capabilities[capability] = capabilities.get(capability, 0) + 1
        result = {"schema": "oasis.m14.3.unknown-range-campaign.v1",
                  "status": "STOP_NO_EXACT_CLASSIFICATION" if rankings else "STOP_UNKNOWN_QUEUE_EMPTY",
                  "rom_sha256": ROM_SHA256, "rom_size": ROM_SIZE,
                  "generation_id": store.meta()["generation_id"],
                  "map_hash": map_hashes["map_hash"], "emission_hash": map_hashes["emission_hash"],
                  "graph_hash": logical_graph_hash(store), "map_partition": partition,
                  "unknown_ranges_before": len(rankings), "unknown_ranges_after": len(rankings),
                  "unknown_bytes_before": unknown_bytes, "unknown_bytes_after": unknown_bytes,
                  "new_classified_ranges": 0, "new_classified_bytes": 0,
                  "new_asm_bytes": 0, "new_data_table_bytes": 0,
                  "new_pointer_table_bytes": 0, "new_resource_bytes": 0,
                  "new_padding_bytes": 0, "new_boundaries": 0, "new_references": 0,
                  "new_symbols": 0, "source_owned_before": owned,
                  "source_owned_after": owned, "source_owned_delta": 0,
                  "classification_gate": "No exact map operations were generated; observed runtime edges and HYPOTHESIS claims are not exact extent proofs.",
                  "top_blockers": blockers,
                  "missing_capability_counts": capabilities,
                  "largest_expected_generic_improvement": "Provide the canonical ROM byte artifact to a generic decoder/CFG closure pass, then validate exact ASM extents by closed flow and assembler roundtrip; this resolves the missing byte and control-flow proof for runtime-seeded candidates.",
                  "ranked_unknown_ranges": rankings[:max(0, top)],
                  "ranked_unknown_count": len(rankings),
                  "ranking_sha256": sha256_bytes(canonical(rankings).encode())}
        return result
    finally:
        store.db.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", required=True, type=Path)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--top", type=int, default=25)
    args = parser.parse_args()
    report = campaign_report(args.database, args.top)
    payload = canonical(report)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(payload + "\n", encoding="utf-8")
    print(payload)


if __name__ == "__main__":
    main()
