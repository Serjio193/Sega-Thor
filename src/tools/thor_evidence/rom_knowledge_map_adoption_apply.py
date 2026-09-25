"""Apply validated M14.4 map operations to deterministic SQLite child generations."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
from typing import Any

from rom_knowledge_map import (KnowledgeStore, canonical, object_id, range_id,
                               sha256_bytes, stable_id)
from knowledge_generation import record_derivation
from rom_knowledge_fusion_query import global_object_view
from rom_knowledge_map_adoption_validation import _file_sha, _stage7_eligibility
import rom_generic_asm_closure as m14_4

ROM_SHA256 = m14_4.ROM_SHA256
ROM_SIZE = m14_4.ROM_SIZE
PROPOSAL_ID = "m14-4-proposal:8073082b1d516c4bb8ac47685ba605a5694bb9565ff5f45c1b4a2016445cd6f9"

def _insert_instruction_objects(store: KnowledgeStore, component: dict) -> dict[int, str]:
    result: dict[int, str] = {}
    for instruction, (start, end) in zip(component["instructions"], component["instruction_spans"]):
        rid, oid = range_id(ROM_SHA256, start, end), object_id(ROM_SHA256, start, end,
                                                              "M68K_INSTRUCTION")
        store.insert_rows("rom_range", [{"range_id": rid, "rom_sha256": ROM_SHA256,
            "start": start, "end": end}])
        existing = store.db.execute("SELECT object_type,range_id FROM rom_object WHERE object_id=?",
                                    (oid,)).fetchone()
        if existing and (existing["object_type"] != "M68K_INSTRUCTION" or
                         existing["range_id"] != rid):
            raise ValueError("STOP_M14_5_INSTRUCTION_IDENTITY_CONFLICT")
        if not existing:
            store.insert_rows("rom_object", [{"object_id": oid, "range_id": rid,
                "object_type": "M68K_INSTRUCTION", "attributes_json": canonical({
                    "opcode": int(instruction["opcode"]),
                    "operation": str(instruction["operation"]),
                    "raw_words": instruction["raw_words"]})}])
        result[start] = oid
    return result

def _apply_component(store: KnowledgeStore, component: dict, operations: list[dict],
                     report_sha: str, proposal_id: str, generation_dir: Path) -> dict:
    start, end = component["start"], component["end"]
    owner = store.db.execute("SELECT * FROM emission WHERE start<=? AND ?<end",
                             (start, start)).fetchone()
    if owner is None or int(owner["end"]) < end or owner["source_kind"] not in {
            "UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"}:
        raise ValueError("STOP_M14_5_SPLIT_PARENT_INVALID")
    old_emission = dict(owner)
    before = int(owner["start"]), int(owner["end"])
    store.db.execute("DELETE FROM emission WHERE start=? AND end=?", before)
    asm_dir = generation_dir / "asm"
    asm_dir.mkdir(parents=True, exist_ok=True)
    asm_name = f"{start:06X}_{end:06X}.asm"
    asm_path = asm_dir / asm_name
    shutil.copyfile(component["asm_source"], asm_path)
    artifact = f"asm/{asm_name}"
    segments = []
    if before[0] < start:
        segments.append({**old_emission, "end": start})
    segments.append({"start": start, "end": end, "emission_type": "ASM",
        "classification": "ASM_ROUNDTRIP_EXACT", "source_kind": "ASM_ROUNDTRIP_EXACT",
        "source_owned": 0, "artifact_type": "asm", "artifact": artifact})
    if end < before[1]:
        segments.append({**old_emission, "start": end})
    for row in segments:
        store.insert_rows("emission", [row])
        segment_range_id = range_id(ROM_SHA256, int(row["start"]), int(row["end"]))
        store.insert_rows("rom_range", [{"range_id": segment_range_id,
            "rom_sha256": ROM_SHA256, "start": int(row["start"]), "end": int(row["end"])}])
    old_emission_id = stable_id("emission", old_emission)
    lineage = {"parent_generation": store.meta()["generation_id"],
        "old_emission_id": old_emission_id, "old_emission": old_emission,
        "split_into": [{"start": row["start"], "end": row["end"],
            "range_id": range_id(ROM_SHA256, int(row["start"]), int(row["end"])),
            "classification": row["classification"], "artifact": row["artifact"]}
            for row in segments], "proposal_id": proposal_id,
        "proof_refs": component["proof_refs"]}
    ids = _insert_instruction_objects(store, component)
    entry_id = ids.get(start)
    if not entry_id:
        raise ValueError("STOP_M14_5_COMPONENT_ENTRY_MISSING")
    component_claim = {"claim_id": stable_id("claim", {"object_id": entry_id,
        "claim_type": "CANONICAL_ASM_EXTENT", "extent": [start, end],
        "cfg_sha256": component["cfg_sha256"]}), "object_id": entry_id,
        "claim_type": "CANONICAL_ASM_EXTENT", "value_json": canonical({
            "start": start, "end": end, "classification": "ASM_ROUNDTRIP_EXACT",
            "source_owned": False, "cfg_sha256": component["cfg_sha256"],
            "roundtrip_sha256": component["roundtrip_sha256"]}),
        "status": "DERIVED_EXACT"}
    store.insert_rows("claim", [component_claim])
    store.insert_rows("source_artifact", [{"source_sha256": report_sha,
        "checkpoint": "M14.4", "artifact_name": "THOR_M14_4_CANONICAL_ROM_GENERIC_ASM_CLOSURE.json",
        "artifact_type": "M14_4_EXACT_ASM_PROPOSAL"}])
    asm_sha = _file_sha(asm_path)
    store.insert_rows("source_artifact", [{"source_sha256": asm_sha,
        "checkpoint": "M14.4", "artifact_name": asm_name,
        "artifact_type": "M14_4_VASM_ROUNDTRIP_SOURCE"}])
    proof_evidence = {"ref_id": stable_id("evidence", {"claim_id": component_claim["claim_id"],
        "source_sha256": report_sha, "roundtrip": component["roundtrip_sha256"]}),
        "subject_type": "CLAIM", "subject_id": component_claim["claim_id"],
        "source_sha256": report_sha, "fact_kind": "M14_4_CFG_AND_ROUNDTRIP_PROOF",
        "fact_count": 1, "locator_json": canonical({"start": start, "end": end,
            "proposal_id": proposal_id, "cfg_sha256": component["cfg_sha256"],
            "roundtrip_sha256": component["roundtrip_sha256"],
            "proof_refs": component["proof_refs"], "asm_sha256": asm_sha})}
    store.insert_rows("evidence_ref", [proof_evidence])
    relation_ids = []
    for reference in component["accepted_references"]:
        source_id = ids[reference["source_pc"]]
        target_row = store.db.execute("""SELECT o.object_id,r.end FROM rom_object o
            JOIN rom_range r USING(range_id) WHERE o.object_type='M68K_INSTRUCTION'
            AND r.start=? ORDER BY r.end LIMIT 1""", (reference["target_pc"],)).fetchone()
        if target_row is None:
            raise ValueError("STOP_M14_5_REFERENCE_TARGET_MISSING")
        attrs = {"source_pc": reference["source_pc"], "target_pc": reference["target_pc"],
            "instruction_operation": reference["instruction_operation"],
            "component": [start, end], "cfg_sha256": component["cfg_sha256"],
            "roundtrip_sha256": component["roundtrip_sha256"]}
        relation_id = stable_id("relation", {"rom_sha256": ROM_SHA256,
            "relation_type": "ASM_CFG_EDGE", "source_object_id": source_id,
            "target_object_id": target_row["object_id"], "attributes": attrs})
        store.insert_rows("relation", [{"relation_id": relation_id,
            "relation_type": "ASM_CFG_EDGE", "source_object_id": source_id,
            "target_object_id": target_row["object_id"], "target_address": None,
            "status": "DERIVED_EXACT", "attributes_json": canonical(attrs)}])
        evidence = {"ref_id": stable_id("evidence", {"relation_id": relation_id,
            "source_sha256": report_sha, "kind": "M14_4_ASM_CFG_EDGE"}),
            "subject_type": "RELATION", "subject_id": relation_id,
            "source_sha256": report_sha, "fact_kind": "M14_4_ASM_CFG_EDGE",
            "fact_count": 1, "locator_json": canonical({**attrs,
                "proposal_id": proposal_id, "proof_refs": component["proof_refs"]})}
        store.insert_rows("evidence_ref", [evidence])
        relation_ids.append(relation_id)
    result = {"extent": [start, end], "emission_lineage": lineage,
        "entry_object_id": entry_id, "claim_id": component_claim["claim_id"],
        "evidence_ref": proof_evidence["ref_id"], "relation_ids": relation_ids,
        "asm_artifact": artifact, "asm_sha256": asm_sha,
        "cfg_sha256": component["cfg_sha256"],
        "roundtrip_sha256": component["roundtrip_sha256"]}
    implementation_hash = sha256_bytes(Path(__file__).read_bytes().replace(b"\r\n", b"\n"))
    inputs = [{"subject_type": "claim", "subject_id": component_claim["claim_id"],
        "role": "canonical_map_classification"}]
    inputs.extend({"subject_type": "relation", "subject_id": relation_id,
        "role": "decoded_control_flow"} for relation_id in relation_ids)
    inputs.extend({"subject_type": "evidence_ref", "subject_id": ref,
        "role": "M14_4_exact_seed_provenance"} for ref in component["proof_refs"])
    derivation_id = record_derivation(store, "M14_5_EXACT_ASM_MAP_ADOPTION", "1",
        implementation_hash, sha256_bytes(canonical({"extent": [start, end],
            "proposal_id": proposal_id}).encode()), inputs, "CANONICAL_ASM_EXTENT",
        entry_id, {**result, "emission_artifact": artifact})
    result["derivation_id"] = derivation_id
    return result

def _generation_hash(path: Path) -> str:
    return _file_sha(path)

def _partition(store: KnowledgeStore) -> tuple[list[tuple], dict]:
    rows = [tuple(row) for row in store.db.execute("SELECT * FROM emission ORDER BY start,end")]
    return rows, m14_4.canonical_partition_audit(store)

def apply_generation(proposal_db: Path, base_db: Path, audit: dict,
                     output_dir: Path, generation_name: str) -> dict[str, Any]:
    if output_dir.exists():
        raise ValueError("STOP_M14_5_GENERATION_OUTPUT_EXISTS")
    output_dir.mkdir(parents=True)
    child_path = output_dir / "canonical.sqlite"
    shutil.copy2(proposal_db, child_path)
    proposal_store = KnowledgeStore(child_path, ROM_SHA256, ROM_SIZE)
    try:
        parent_generation = audit["identity"]["base_generation"]
        proposal_store.validate_map_proposal_parent(PROPOSAL_ID, parent_generation)
        before_partition, before_audit = _partition(proposal_store)
        before_metrics = proposal_store.metrics()
        components = audit["valid_components"]
        accepted = set(audit["accepted_operation_ordinals"])
        operations = audit["operations"]
        # Verify every split and its exact classification before replacing any row.
        for component in components:
            component_operations = [op for index, op in enumerate(operations)
                if index in accepted and op["operation"] == "SPLIT_RANGE" and
                (int(op["start"]), int(op["end"])) ==
                    (component["start"], component["end"])]
            if len(component_operations) != 1:
                raise ValueError("STOP_M14_5_ACCEPTED_SPLIT_CARDINALITY")
        for component in components:
            applied = _apply_component(proposal_store, component, operations,
                audit["receipt_sha256"], PROPOSAL_ID, output_dir)
            component["application"] = applied
        proposal_store.db.execute("UPDATE map_proposal SET status='APPLIED' WHERE proposal_id=?",
                                  (PROPOSAL_ID,))
        map_hash = proposal_store.hashes()["map_hash"]
        child_generation = generation_name + "-" + map_hash[:16]
        proposal_store.set_generation_identity(child_generation, parent_generation)
        proposal_store.db.commit()
        after_partition, after_audit = _partition(proposal_store)
        after_metrics = proposal_store.metrics()
        after_hashes = proposal_store.hashes()
        if after_audit["gaps"] or after_audit["overlaps"] or after_audit["rom_bytes"] != ROM_SIZE:
            raise ValueError("STOP_M14_5_PARTITION_INVALID")
        if (after_metrics["source_owned_bytes"] != before_metrics["source_owned_bytes"] or
                before_metrics["source_owned_bytes"] != 1_487_672):
            raise ValueError("STOP_M14_5_SOURCE_OWNED_CHANGED")
        expected_removed = sum(item["end"] - item["start"] for item in components)
        if before_metrics["unknown_bytes"] - after_metrics["unknown_bytes"] != expected_removed:
            raise ValueError("STOP_M14_5_UNKNOWN_BYTE_DELTA_MISMATCH")
        stale_path = output_dir / "stale-parent.sqlite"
        proposal_store.db.close()
        shutil.copy2(proposal_db, stale_path)
        stale = KnowledgeStore(stale_path, ROM_SHA256, ROM_SIZE)
        stale.db.execute("UPDATE emission SET classification=classification||'_STALE' "
                         "WHERE start=(SELECT MIN(start) FROM emission)")
        stale.db.commit()
        try:
            stale.validate_map_proposal_parent(PROPOSAL_ID, parent_generation)
        except ValueError as error:
            stale_rejected = str(error) == "STOP_MAP_PROPOSAL_STALE_PARENT"
        else:
            stale_rejected = False
        stale.db.close()
        if not stale_rejected:
            raise ValueError("STOP_M14_5_STALE_PARENT_NOT_REJECTED")
        views = []
        child = KnowledgeStore(child_path, ROM_SHA256, ROM_SIZE, read_only=True)
        try:
            for component in components:
                entry_id = component["application"]["entry_object_id"]
                view = global_object_view(child, entry_id)
                if view["canonical_map_owner"] is None or \
                        view["canonical_map_owner"]["classification"] != "ASM_ROUNDTRIP_EXACT" or \
                        not any(item["claim_type"] == "CANONICAL_ASM_EXTENT" for item in view["claims"]) or \
                        not view["derivations"] or not view["runtime_occurrences"] or \
                        not view["relations"] or not any(item["fact_kind"] ==
                            "M14_4_CFG_AND_ROUNDTRIP_PROOF" for item in view["evidence"]):
                    raise ValueError("STOP_M14_5_GLOBAL_OBJECT_VIEW_RECONCILIATION")
                views.append({"entry_object_id": entry_id,
                    "classification": view["canonical_map_owner"]["classification"],
                    "runtime_occurrences": len(view["runtime_occurrences"]),
                    "relations": len(view["relations"]), "evidence": len(view["evidence"]),
                    "derivations": [row["derivation_id"] for row in view["derivations"]],
                    "roundtrip_proof_visible": any(item["fact_kind"] ==
                        "M14_4_CFG_AND_ROUNDTRIP_PROOF" for item in view["evidence"])})
        finally:
            child.db.close()
        generation_digest = _generation_hash(child_path)
        result = {"database": child_path.name, "generation_id": child_generation,
            "map_hash": map_hash, "emission_hash": after_hashes["emission_hash"],
            "generation_sha256": generation_digest, "before_partition": before_audit,
            "after_partition": after_audit, "before_metrics": before_metrics,
            "after_metrics": after_metrics, "stale_parent_rejected": stale_rejected,
            "application_lineage": [item["application"]["emission_lineage"]
                for item in components], "global_object_views": views,
            "components": [{key: item[key] for key in
                ("start", "end", "size", "cfg_sha256", "roundtrip_sha256", "proof_refs",
                 "stage7", "application")} for item in components]}
        return result
    except Exception:
        if proposal_store.db:
            proposal_store.db.close()
        raise

def _logical_generation(path: Path) -> tuple:
    store = KnowledgeStore(path, ROM_SHA256, ROM_SIZE, read_only=True)
    try:
        tables = ("rom_range", "rom_object", "claim", "relation", "emission",
                  "source_artifact", "evidence_ref", "conflict", "derivation",
                  "derivation_input", "map_proposal", "map_proposal_operation")
        return tuple((table, tuple(tuple(row) for row in store.db.execute(
            f"SELECT * FROM {table} ORDER BY 1,2"))) for table in tables)
    finally:
        store.db.close()
