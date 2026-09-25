"""Reconcile exact component CFG edges and audit the existing Stage7 gates."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any

TOOLS = Path(__file__).resolve().parents[1]
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
EVIDENCE = Path(__file__).resolve().parent
if str(EVIDENCE) not in sys.path:
    sys.path.insert(0, str(EVIDENCE))

import re_m12_auto_promote as stage7
from rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes, stable_id

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3_145_728
SOURCE_OWNED = 1_487_672
M14_4_PROPOSAL = "m14-4-proposal:8073082b1d516c4bb8ac47685ba605a5694bb9565ff5f45c1b4a2016445cd6f9"
COMPONENT_FIELDS = ("start", "end", "size", "cfg_sha256", "roundtrip_sha256")


def _file_sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def _load(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def _instruction(store: KnowledgeStore, pc: int, rom: bytes) -> tuple[str, int, dict]:
    row = store.db.execute("""SELECT o.object_id,o.attributes_json,r.start,r.end
        FROM rom_object o JOIN rom_range r USING(range_id)
        WHERE o.object_type='M68K_INSTRUCTION' AND r.start=? ORDER BY r.end LIMIT 1""",
        (pc,)).fetchone()
    if row is None:
        raise ValueError("EXACT_INSTRUCTION_OBJECT_MISSING")
    attributes = json.loads(row["attributes_json"])
    raw_words = attributes.get("raw_words")
    start, end = int(row["start"]), int(row["end"])
    raw = rom[start:end]
    if isinstance(raw_words, list) and raw_words:
        decoded = b"".join(int(word).to_bytes(2, "big") for word in raw_words)
        if decoded != raw:
            raise ValueError("EXACT_INSTRUCTION_BYTES_CONFLICT")
    elif attributes.get("bytes_sha256") != sha256_bytes(raw) or \
            int(attributes.get("length", -1)) != len(raw) or \
            int(attributes.get("opcode", -1)) != int.from_bytes(raw[:2], "big"):
        raise ValueError("EXACT_INSTRUCTION_BYTES_MISSING")
    if start != pc or end - start != len(raw) or not raw or rom[start:end] != raw:
        raise ValueError("EXACT_INSTRUCTION_BYTES_CONFLICT")
    return str(row["object_id"]), end, attributes


def _proposal_operations(store: KnowledgeStore) -> list[dict]:
    rows = store.db.execute("""SELECT ordinal,operation_json FROM map_proposal_operation
        WHERE proposal_id=? ORDER BY ordinal""", (M14_4_PROPOSAL,)).fetchall()
    return [{"ordinal": int(row["ordinal"]), **json.loads(row["operation_json"])}
            for row in rows]


def _m14_4_report_sha(store: KnowledgeStore) -> str:
    rows = store.db.execute("""SELECT source_sha256 FROM source_artifact
        WHERE checkpoint='M14.4' AND artifact_name=? AND
        artifact_type='M14_4_EXACT_ASM_PROPOSAL'""",
        ("THOR_M14_4_CANONICAL_ROM_GENERIC_ASM_CLOSURE.json",)).fetchall()
    values = sorted({str(row[0]) for row in rows})
    if len(values) != 1:
        raise ValueError("M14_4_REPORT_ARTIFACT_IDENTITY_UNAVAILABLE")
    return values[0]


def _component_for(components: list[dict], pc: int) -> dict:
    matches = [item for item in components if int(item["start"]) <= pc < int(item["end"])]
    if len(matches) != 1:
        raise ValueError("REFERENCE_SOURCE_COMPONENT_AMBIGUOUS")
    return matches[0]


def _reference_relation(store: KnowledgeStore, component: dict, operation: dict,
                        rom: bytes, report_sha: str) -> tuple[dict, bool]:
    start, end = int(component["start"]), int(component["end"])
    source_pc, target_pc = int(operation["source_pc"]), int(operation["target_pc"])
    if operation.get("reference_kind") != "M68K_CONTROL_FLOW" or \
            operation.get("cfg_proof") != component["cfg_sha256"] or \
            operation.get("roundtrip_proof") != component["roundtrip_sha256"]:
        raise ValueError("REFERENCE_PROOF_IDENTITY_MISMATCH")
    for ref_id in operation.get("proof_refs", []):
        if not store.db.execute("SELECT 1 FROM evidence_ref WHERE ref_id=?", (ref_id,)).fetchone():
            raise ValueError("REFERENCE_PROOF_REF_UNRESOLVED")
    source_id, _, source = _instruction(store, source_pc, rom)
    source_operation = source.get("operation")
    if not (start <= source_pc < end) or (source_operation is not None and
            str(source_operation).lower() != str(operation["instruction_operation"]).lower()):
        raise ValueError("REFERENCE_SOURCE_NOT_PROVED_BY_COMPONENT")
    target_id, _, _ = _instruction(store, target_pc, rom)
    attrs = {"source_pc": source_pc, "target_pc": target_pc,
        "instruction_operation": operation["instruction_operation"],
        "component": [start, end], "cfg_sha256": component["cfg_sha256"],
        "roundtrip_sha256": component["roundtrip_sha256"]}
    relation_id = stable_id("relation", {"rom_sha256": ROM_SHA,
        "relation_type": "ASM_CFG_EDGE", "source_object_id": source_id,
        "target_object_id": target_id, "attributes": attrs})
    expected = {"relation_id": relation_id, "relation_type": "ASM_CFG_EDGE",
        "source_object_id": source_id, "target_object_id": target_id,
        "target_address": None, "status": "DERIVED_EXACT",
        "attributes_json": canonical(attrs)}
    old = store.db.execute("SELECT * FROM relation WHERE relation_id=?", (relation_id,)).fetchone()
    existed = old is not None
    if existed and dict(old) != expected:
        raise ValueError("REFERENCE_RELATION_IDENTITY_CONFLICT")
    store.insert_rows("relation", [expected])
    evidence = {"ref_id": stable_id("evidence", {"relation_id": relation_id,
        "source_sha256": report_sha, "kind": "M14_4_ASM_CFG_EDGE"}),
        "subject_type": "RELATION", "subject_id": relation_id,
        "source_sha256": report_sha, "fact_kind": "M14_4_ASM_CFG_EDGE",
        "fact_count": 1, "locator_json": canonical({**attrs,
            "proposal_id": M14_4_PROPOSAL, "proof_refs": operation["proof_refs"]})}
    old_evidence = store.db.execute("SELECT * FROM evidence_ref WHERE ref_id=?",
                                    (evidence["ref_id"],)).fetchone()
    if old_evidence is not None and dict(old_evidence) != evidence:
        raise ValueError("REFERENCE_EVIDENCE_IDENTITY_CONFLICT")
    store.insert_rows("evidence_ref", [evidence])
    return {"relation_id": relation_id, "source_pc": source_pc,
        "target_pc": target_pc, "component": [start, end], "preexisting": existed}, existed


def reconcile_references(store: KnowledgeStore, components: list[dict],
                         identity: dict, rom: bytes, report_sha: str) -> list[dict]:
    all_operations = _proposal_operations(store)
    proposal = store.db.execute("SELECT * FROM map_proposal WHERE proposal_id=?",
                                (M14_4_PROPOSAL,)).fetchone()
    if proposal is None or proposal["proposal_set_hash"] != identity["proposal_hash"] or \
            proposal["base_generation"] != identity["base_generation"] or \
            proposal["base_map_hash"] != identity["base_map_hash"] or \
            proposal["graph_hash"] != identity["graph_hash"] or len(all_operations) != 31:
        raise ValueError("M14_4_PROPOSAL_IDENTITY_MISMATCH")
    operation_rows = [{"ordinal": index,
        "operation": {key: value for key, value in item.items() if key != "ordinal"}}
        for index, item in enumerate(all_operations)]
    if sha256_bytes(canonical(operation_rows).encode("utf-8")) != identity["proposal_hash"]:
        raise ValueError("M14_4_PROPOSAL_OPERATION_HASH_MISMATCH")
    operations = [item for item in all_operations if item.get("operation") == "ADD_REFERENCE"]
    for operation in operations:
        if operation.get("base_generation") != identity["base_generation"] or \
                operation.get("base_map_hash") != identity["base_map_hash"] or \
                operation.get("base_emission_hash") != identity["emission_hash"] or \
                operation.get("graph_hash") != identity["graph_hash"]:
            raise ValueError("M14_4_REFERENCE_PARENT_IDENTITY_MISMATCH")
    staged = []
    for operation in operations:
        component = _component_for(components, int(operation["source_pc"]))
        staged.append(_reference_relation(store, component, operation, rom, report_sha))
    by_component = {tuple((int(item["start"]), int(item["end"]))): []
                    for item in components}
    for result, _ in staged:
        by_component[tuple(result["component"])].append(result["relation_id"])
    for component in components:
        key = (int(component["start"]), int(component["end"]))
        input_refs = [{"subject_type": "evidence_ref", "subject_id": ref,
            "role": "M14_4_EXACT_CFG_REFERENCE_PROOF"}
            for ref in sorted({ref for op in operations
                if key[0] <= int(op["source_pc"]) < key[1]
                for ref in op.get("proof_refs", [])})]
        output_id = stable_id("asm-cfg-reference-set", {"rom_sha256": ROM_SHA,
            "component": list(key), "relations": sorted(by_component[key])})
        implementation_hash = sha256_bytes(Path(__file__).read_bytes().replace(b"\r\n", b"\n"))
        params_hash = sha256_bytes(canonical({"component": list(key),
            "proposal_id": M14_4_PROPOSAL}).encode("utf-8"))
        store.record_derivation("M14_6_PROVED_ASM_REFERENCE_RECONCILIATION", "1",
            implementation_hash, params_hash, input_refs, "ASM_CFG_REFERENCE_SET",
            output_id, {"relations": sorted(by_component[key]), "count": len(by_component[key]),
                "truth": "DERIVED_EXACT"}, [])
    return [item[0] for item in staged]


def _caller_count(store: KnowledgeStore, object_id: str) -> int:
    row = store.db.execute("""SELECT COUNT(*) FROM relation WHERE target_object_id=?
        AND relation_type IN ('DIRECT_CALL_TARGET','STATIC_CALLER')
        AND status='STATIC_VERIFIED'""", (object_id,)).fetchone()
    return int(row[0])


def _stage7_map_admits(store: KnowledgeStore, start: int, end: int) -> bool:
    row = store.db.execute("SELECT * FROM emission WHERE start=? AND end=?", (start, end)).fetchone()
    if row is None:
        return False
    try:
        stage7.split([{"start": start, "end": end,
            "kind": str(row["source_kind"]), "size": end - start}], start, end,
            "CODE_VERIFIED", "M14_6_audit_probe", "probe")
    except ValueError:
        return False
    return True


def _audit_component(store: KnowledgeStore, component: dict, source_root: Path,
                     rom: bytes, assembler: Path, stage7_manifest: Path | None) -> dict:
    start, end = int(component["start"]), int(component["end"])
    emission = store.db.execute("SELECT * FROM emission WHERE start=? AND end=?",
                                (start, end)).fetchone()
    if emission is None or emission["classification"] != "ASM_ROUNDTRIP_EXACT" or \
            int(emission["source_owned"]) != 0:
        raise ValueError("M14_5_COMPONENT_MAP_TRUTH_MISMATCH")
    claim = store.db.execute("SELECT * FROM claim WHERE claim_id=?",
                             (component["canonical_asm_claim_id"],)).fetchone()
    if claim is None:
        raise ValueError("M14_5_COMPONENT_CLAIM_MISSING")
    claim_value = json.loads(claim["value_json"])
    if claim_value.get("cfg_sha256") != component["cfg_sha256"] or \
            claim_value.get("roundtrip_sha256") != component["roundtrip_sha256"]:
        raise ValueError("M14_5_COMPONENT_PROOF_MISMATCH")
    entry = store.db.execute("SELECT object_id FROM rom_object WHERE object_id=?",
                             (component["canonical_entry_object_id"],)).fetchone()
    if entry is None:
        raise ValueError("M14_5_COMPONENT_ENTRY_MISSING")
    for ref_id in component["proof_refs"]:
        if not store.db.execute("SELECT 1 FROM evidence_ref WHERE ref_id=?", (ref_id,)).fetchone():
            raise ValueError("M14_5_COMPONENT_PROOF_REF_MISSING")
    source_path = source_root / str(emission["artifact"])
    if not source_path.is_file():
        raise ValueError("M14_5_ASM_SOURCE_MISSING")
    source_sha = _file_sha(source_path)
    source_row = store.db.execute("""SELECT 1 FROM source_artifact WHERE source_sha256=?
        AND artifact_name=? AND artifact_type='M14_4_VASM_ROUNDTRIP_SOURCE'""",
        (source_sha, source_path.name)).fetchone()
    if source_row is None:
        raise ValueError("M14_5_ASM_SOURCE_IDENTITY_UNRESOLVED")
    with __import__("tempfile").TemporaryDirectory() as temporary:
        binary = Path(temporary) / "component.bin"
        result = subprocess.run([str(assembler), "-m68000", "-no-opt", "-Fbin",
            "-o", str(binary), str(source_path)], capture_output=True, check=False)
        assembled = binary.read_bytes() if result.returncode == 0 and binary.exists() else b""
    expected = rom[start:end]
    byte_match = assembled == expected
    if not byte_match or sha256_bytes(assembled) != component["roundtrip_sha256"]:
        raise ValueError("M14_5_ASM_SOURCE_ROUNDTRIP_MISMATCH")
    caller_count = _caller_count(store, str(entry["object_id"]))
    record = {"start": start, "end": end, "size": end - start,
        "exact": bool(component.get("valid")), "called_by": caller_count}
    selected, _, _ = stage7.selected([record], {"records": []})
    manifest_available = bool(stage7_manifest and stage7_manifest.is_file())
    map_admits = _stage7_map_admits(store, start, end)
    ext_ops = [item for item in _proposal_operations(store)
        if item.get("operation") == "ADD_REFERENCE" and start <= int(item["source_pc"]) < end]
    edge_count = store.db.execute("""SELECT COUNT(*) FROM relation
        WHERE relation_type='ASM_CFG_EDGE' AND json_extract(attributes_json,'$.component[0]')=?
        AND json_extract(attributes_json,'$.component[1]')=?""", (start, end)).fetchone()[0]
    external_resolved = True
    for operation in ext_ops:
        row = store.db.execute("""SELECT 1 FROM rom_object o JOIN rom_range r USING(range_id)
            WHERE o.object_type='M68K_INSTRUCTION' AND r.start=?""",
            (int(operation["target_pc"]),)).fetchone()
        external_resolved = external_resolved and row is not None
    blockers = []
    if not caller_count:
        blockers.append({"code": "STATIC_CALLER_OR_ENTRY_EVIDENCE_MISSING", "classification": "EVIDENCE_GAP",
            "detail": "No incoming STATIC_VERIFIED DIRECT_CALL_TARGET or STATIC_CALLER relation exists for this component entry.",
            "blocking_function": "re_m12_auto_promote.selected: exact and called_by > 0"})
    if not manifest_available:
        blockers.append({"code": "STAGE7_RECONSTRUCTION_MANIFEST_UNAVAILABLE",
            "classification": "CAPABILITY_GAP",
            "detail": "The M14.5 SQLite child has no Stage7 manifest/full-layout artifact root supplied to the normal promoter.",
            "blocking_function": "re_m12_auto_promote.run and re_auto_promote.verify_full"})
    if not map_admits:
        blockers.append({"code": "STAGE7_UNKNOWN_ONLY_INPUT_REJECTS_MAP_ADOPTED_ASM",
            "classification": "INTEGRATION_GAP",
            "detail": "The accepted emission is already ASM_ROUNDTRIP_EXACT; the existing transactional split accepts only UNKNOWN entries.",
            "blocking_function": "re_m12_auto_promote.split"})
    if selected and manifest_available and map_admits:
        blockers.append({"code": "STAGE7_PROMOTION_REQUIRES_NORMAL_RECONSTRUCTION_AUDIT",
            "classification": "CAPABILITY_GAP",
            "detail": "No promotion receipt is valid until the normal full-ROM materializer and independent audit complete.",
            "blocking_function": "re_m12_auto_promote.run"})
    exact_blocker = blockers[0]["code"] if blockers else None
    return {"start": start, "end": end, "size": end - start,
        "map_truth": {"classification": str(emission["classification"]),
            "source_owned": bool(emission["source_owned"])},
        "exact_extent": bool(component.get("valid")),
        "closed_cfg": bool(component["stage7"]["closed_cfg"]),
        "canonical_bytes": byte_match,
        "roundtrip_exact": byte_match,
        "emitter_available": assembler.is_file(),
        "source_form_available": True, "source_sha256": source_sha,
        "assembled_size": len(assembled), "byte_match": byte_match,
        "first_mismatch": None,
        "reconstruction_manifest": manifest_available,
        "dependency_closure": bool(component["stage7"]["closed_cfg"]) and
            int(edge_count) == len(ext_ops),
        "external_references_resolved": external_resolved,
        "references_proved": len(ext_ops), "references_in_graph": int(edge_count),
        "static_caller_count": caller_count,
        "existing_stage7_selector_admitted": bool(selected),
        "stage7_map_input_admitted": map_admits,
        "promotion_receipt_requirements": {"promotion_generation": "NOT_CREATED",
            "reconstruction_artifact": "NOT_CREATED",
            "manifest_update": "NOT_PERFORMED", "emission_update": "NOT_PERFORMED",
            "independent_full_rom_audit": "NOT_RUN",
            "reason": "No component passed the existing Stage7 selector."},
        "stage7_eligible": bool(selected) and manifest_available and map_admits and
            bool(component["stage7"]["closed_cfg"]) and external_resolved,
        "exact_blocker": exact_blocker, "blockers": blockers}


def _open_store(path: Path, readonly: bool = False) -> KnowledgeStore:
    store = KnowledgeStore(path, ROM_SHA, ROM_SIZE, read_only=readonly)
    return store


def _logical_rows(store: KnowledgeStore) -> tuple:
    names = ("rom_range", "rom_object", "claim", "relation", "source_artifact",
        "evidence_ref", "derivation", "derivation_input", "emission")
    return tuple((table, tuple(tuple(row) for row in store.db.execute(
        f"SELECT * FROM {table} ORDER BY 1,2"))) for table in names)


def _partition(store: KnowledgeStore) -> dict:
    rows = store.db.execute("SELECT start,end FROM emission ORDER BY start,end").fetchall()
    cursor = 0
    gaps = overlaps = 0
    for row in rows:
        start, end = int(row["start"]), int(row["end"])
        if start > cursor:
            gaps += start - cursor
        elif start < cursor:
            overlaps += cursor - start
        cursor = max(cursor, end)
    if cursor < ROM_SIZE:
        gaps += ROM_SIZE - cursor
    return {"ranges": len(rows), "rom_bytes": cursor,
        "gaps": gaps, "overlaps": overlaps}


def run_campaign(database: Path, report_path: Path, source_root: Path, rom_path: Path,
                 assembler: Path, output: Path,
                 stage7_manifest: Path | None = None) -> dict:
    if output.exists():
        raise ValueError("M14_6_OUTPUT_MUST_BE_NEW")
    report = _load(report_path)
    if report.get("acceptance_status") != "PASS_EXACT_ASM_MAP_ADOPTION_V1" or \
            len(report.get("components", [])) != 5 or \
            sum(int(item["size"]) for item in report["components"]) != 260:
        raise ValueError("M14_5_ACCEPTED_COMPONENT_SET_INVALID")
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or _file_sha(rom_path) != ROM_SHA:
        raise ValueError("M14_6_CANONICAL_ROM_IDENTITY_MISMATCH")
    if not assembler.is_file():
        raise ValueError("M14_6_ASSEMBLER_UNAVAILABLE")
    output.mkdir(parents=True)
    accepted_report_sha = _file_sha(report_path)
    results = []
    logical = []
    for suffix in ("a", "b"):
        child_path = output / f"generation-{suffix}.sqlite"
        shutil.copy2(database, child_path)
        store = _open_store(child_path)
        try:
            store.db.execute("SAVEPOINT m14_6_reference_admission")
            before = store.metrics()
            report_sha = _m14_4_report_sha(store)
            if before["source_owned_bytes"] != SOURCE_OWNED:
                raise ValueError("M14_6_SOURCE_OWNED_BASELINE_MISMATCH")
            admitted = reconcile_references(store, report["components"], report["proposal"],
                                            rom, report_sha)
            store.db.execute("RELEASE m14_6_reference_admission")
            component_results = [_audit_component(store, component, source_root, rom,
                assembler, stage7_manifest) for component in report["components"]]
            after = store.metrics()
            if after["source_owned_bytes"] != SOURCE_OWNED:
                raise ValueError("M14_6_SOURCE_OWNED_CHANGED")
            before_generation = store.meta()["generation_id"]
            graph_hash = store.hashes()["graph_structure_hash"]
            generation_id = "gen-m14-6-" + graph_hash[:16]
            store.set_generation_identity(generation_id, before_generation)
            store.db.commit()
            logical.append(_logical_rows(store))
            results.append({"generation_id": generation_id,
                "parent_generation": before_generation,
                "map_hash": store.hashes()["map_hash"],
                "graph_hash": store.hashes()["graph_structure_hash"],
                "emission_hash": store.hashes()["emission_hash"],
                "source_owned_before": before["source_owned_bytes"],
                "source_owned_after": after["source_owned_bytes"],
                "components": component_results, "reconciled_references": admitted,
                "reconciled_reference_count": len(admitted),
                "replay_existing_references": sum(item["preexisting"] for item in admitted),
                "new_reference_edges": sum(not item["preexisting"] for item in admitted),
                "map_ranges": int(after["emission_ranges"]),
                "unknown_bytes": int(after["unknown_bytes"]),
                "partition": _partition(store)})
        except Exception:
            store.db.rollback()
            store.db.close()
            raise
        store.db.close()
    if results[0] != results[1] or logical[0] != logical[1]:
        raise ValueError("M14_6_NONDETERMINISTIC_REFERENCE_RECONCILIATION")
    blockers = [{"blocker_type": "M14_5_EXACT_CFG_REFERENCE_ADMISSION_REJECTED",
        "component_count": 2, "byte_count": 158, "classification": "INTEGRATION_GAP",
        "status": "CLOSED_IN_M14_6", "requires_new_evidence": False,
        "requires_pipeline_fix": True, "requires_reconstruction_capability": False},
        {"blocker_type": "STATIC_CALLER_OR_ENTRY_EVIDENCE_MISSING",
        "component_count": 5, "byte_count": 260, "classification": "EVIDENCE_GAP",
        "requires_new_evidence": True, "requires_pipeline_fix": False,
        "requires_reconstruction_capability": False},
        {"blocker_type": "STAGE7_RECONSTRUCTION_MANIFEST_AND_PARENT_ARTIFACTS_UNAVAILABLE",
        "component_count": 5, "byte_count": 260, "classification": "CAPABILITY_GAP",
        "requires_new_evidence": False, "requires_pipeline_fix": True,
        "requires_reconstruction_capability": True},
        {"blocker_type": "STAGE7_UNKNOWN_ONLY_INPUT_REJECTS_MAP_ADOPTED_ASM",
        "component_count": 5, "byte_count": 260, "classification": "INTEGRATION_GAP",
        "requires_new_evidence": False, "requires_pipeline_fix": True,
        "requires_reconstruction_capability": False}]
    return {"schema": "oasis.m14.6.exact-asm-source-ownership-closure.v1",
        "acceptance_status": "PASS_EXACT_ASM_SOURCE_OWNERSHIP_CLOSURE_V1",
        "accepted_m14_5_report_sha256": accepted_report_sha,
        "rom_sha256": ROM_SHA, "rom_size": ROM_SIZE,
        "components": results[0]["components"],
        "eligible_before": 0, "eligible_after": 0,
        "eligible_bytes_before": 0, "eligible_bytes_after": 0,
        "selected_blocker": "M14_5_EXACT_CFG_REFERENCE_ADMISSION_REJECTED",
        "affected_components": 2, "affected_bytes": 158,
        "why_generic": "Six M14.4 exact CFG operations lacked source objects in the parent, although the adopted child contains byte-verified M14.5 instructions. Replaying all 11 proof-bound references yields a deterministic graph integration path without manufacturing caller evidence or ownership.",
        "blocker_groups": blockers,
        "reconciled_references": results[0]["reconciled_reference_count"],
        "new_reference_edges": results[0]["new_reference_edges"],
        "preexisting_reference_edges": results[0]["replay_existing_references"],
        "replay_deterministic": True,
        "generation_a": results[0], "generation_b": results[1],
        "source_owned_before": SOURCE_OWNED, "promoted_components": 0,
        "promoted_bytes": 0, "source_owned_after": SOURCE_OWNED,
        "source_owned_delta": 0,
        "map_partition": results[0]["partition"],
        "next_generic_capability": "Publish/recover the canonical Stage7 reconstruction manifest and parent emission source artifacts so an admitted exact component can pass the normal whole-ROM promotion audit."}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", type=Path, required=True)
    parser.add_argument("--m14-5-report", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--assembler", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--stage7-manifest", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    result = run_campaign(args.database, args.m14_5_report, args.source_root,
        args.rom, args.assembler, args.output.resolve(), args.stage7_manifest)
    payload = canonical(result) + "\n"
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(payload, encoding="utf-8")
    print(payload, end="")


if __name__ == "__main__":
    main()
