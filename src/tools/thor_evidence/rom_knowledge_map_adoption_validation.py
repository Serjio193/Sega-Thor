"""Independent M14.4 proposal and component validation."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import subprocess
import sys
from typing import Any

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
import rom_generic_asm_closure as m14_4
from rom_knowledge_fusion import logical_graph_hash
from rom_knowledge_map import KnowledgeStore, canonical, sha256_bytes

ROM_SHA256 = m14_4.ROM_SHA256
ROM_SIZE = m14_4.ROM_SIZE
PROPOSAL_ID = "m14-4-proposal:8073082b1d516c4bb8ac47685ba605a5694bb9565ff5f45c1b4a2016445cd6f9"
PROPOSAL_HASH = "f9b2965a000c26d47203a49eef738e0f6eb013f7904f545e4a8cb4d8c0714af9"
EXPECTED_COMPONENTS = 5
EXPECTED_OPERATIONS = 31

def _file_sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()

def _rows(store: KnowledgeStore) -> list[dict[str, Any]]:
    return [json.loads(row[0]) for row in store.db.execute(
        "SELECT operation_json FROM map_proposal_operation WHERE proposal_id=? ORDER BY ordinal",
        (PROPOSAL_ID,))]

def _tracked_identity(receipt: dict[str, Any], reproduced: dict[str, Any],
                      base: KnowledgeStore) -> dict[str, str]:
    meta, hashes = base.meta(), base.hashes()
    graph_hash = logical_graph_hash(base)
    fields = {"proposal_id": PROPOSAL_ID, "proposal_hash": PROPOSAL_HASH,
        "base_generation": str(meta["generation_id"]), "base_map_hash": hashes["map_hash"],
        "graph_hash": graph_hash, "emission_hash": hashes["emission_hash"]}
    if receipt.get("acceptance_status") != "PASS_CANONICAL_ROM_GENERIC_ASM_CLOSURE_V1" or \
            reproduced.get("deterministic_replay") != "PASS":
        raise ValueError("STOP_M14_4_RECEIPT_NOT_ACCEPTED_OR_REPLAYED")
    if receipt.get("map_proposal", {}).get("operation_count") != EXPECTED_OPERATIONS:
        raise ValueError("STOP_M14_4_TRACKED_OPERATION_COUNT")
    for field, value in fields.items():
        actual = (receipt.get("map_proposal", {}).get(field) if field.startswith("proposal_")
                  else receipt.get({"base_generation": "generation_id",
                                    "base_map_hash": "map_hash"}.get(field, field)))
        if actual != value:
            raise ValueError(f"STOP_M14_4_TRACKED_IDENTITY_MISMATCH:{field}")
    if reproduced.get("map_proposal") != receipt.get("map_proposal"):
        raise ValueError("STOP_M14_4_PROPOSAL_REPRODUCTION_MISMATCH")
    return fields

def reproduce_proposal(base_db: Path, rom: Path, decoder: Path, assembler: Path,
                       output: Path, tracked_receipt: Path) -> tuple[dict, dict, Path]:
    if output.exists():
        raise ValueError("STOP_M14_5_OUTPUT_ALREADY_EXISTS")
    output.mkdir(parents=True)
    campaign_root = output / "m14-4-reproduction"
    result = m14_4.campaign(base_db, rom, decoder, assembler, campaign_root)
    replay = m14_4.campaign(base_db, rom, decoder, assembler,
                            output / "m14-4-reproduction-replay")
    if result != replay:
        raise ValueError("STOP_M14_4_REPLAY_MISMATCH")
    result["deterministic_replay"] = "PASS"
    result["acceptance_status"] = "PASS_CANONICAL_ROM_GENERIC_ASM_CLOSURE_V1"
    receipt = json.loads(tracked_receipt.read_text(encoding="utf-8"))
    return receipt, result, campaign_root

class _StoreContext:
    def __init__(self, path: Path):
        self.path = path
        self.store: KnowledgeStore | None = None

    def __enter__(self) -> KnowledgeStore:
        self.store = KnowledgeStore(self.path, ROM_SHA256, ROM_SIZE, read_only=True)
        return self.store

    def __exit__(self, *_: Any) -> None:
        if self.store:
            self.store.db.close()

def as_store(path: Path) -> _StoreContext:
    return _StoreContext(path)

def _proof_rows(result: dict[str, Any]) -> dict[tuple[int, int], tuple[dict, dict]]:
    output = {}
    for item in result["ranges"]:
        for proof in item["exact_extent_proofs"]:
            output[(int(proof["start"]), int(proof["end"]))] = (item, proof)
    return output

def _validate_component(store: KnowledgeStore, proof: dict, analysis: dict,
                        campaign_root: Path, rom: bytes, assembler: Path,
                        validation_root: Path) -> dict[str, Any]:
    start, end = int(proof["start"]), int(proof["end"])
    window_start, window_end = int(analysis["start"]), int(analysis["end"])
    cfg_path = campaign_root / f"{window_start:06X}-{window_end:06X}" / "decode" / "cfg.json"
    cfg = json.loads(cfg_path.read_text(encoding="utf-8"))
    cfg_hash = sha256_bytes(canonical(cfg).encode("utf-8"))
    if cfg_hash != proof["cfg_sha256"] or not cfg["closed_cfg"] or cfg["blockers"]:
        raise ValueError("CFG_PROOF_MISMATCH_OR_OPEN")
    extent = next((item for item in cfg["extents"]
                   if (int(item["start"]), int(item["end"])) == (start, end)), None)
    if extent is None:
        raise ValueError("EXACT_EXTENT_NOT_IN_CFG")
    slice_path = cfg_path.parent / extent["slice"]
    slice_data = json.loads(slice_path.read_text(encoding="utf-8"))
    instructions = slice_data["instructions"]
    cursor, spans = start, []
    for instruction in instructions:
        address = int(instruction["address"])
        raw = b"".join(int(word).to_bytes(2, "big")
                       for word in instruction["raw_words"])
        if address != cursor or not raw or rom[address:address + len(raw)] != raw:
            raise ValueError("INSTRUCTION_BOUNDARY_OR_CANONICAL_BYTES_MISMATCH")
        spans.append((address, address + len(raw)))
        cursor += len(raw)
    if cursor != end or not spans:
        raise ValueError("EXACT_INSTRUCTION_EXTENT_INCOMPLETE")
    refs = [{"source_pc": int(item["address"]),
             "target_pc": int(item["branch_target"]),
             "instruction_operation": item["operation"]}
            for item in instructions if item["branch_target"] is not None]
    if refs != proof["references"]:
        raise ValueError("CONTROL_FLOW_REFERENCE_PROOF_MISMATCH")
    conflicts = store.db.execute("SELECT 1 FROM conflict WHERE start<? AND ?<end LIMIT 1",
                                 (end, start)).fetchone()
    if conflicts:
        raise ValueError("INCOMPATIBLE_MAP_CONFLICT")
    existing = store.db.execute("""SELECT o.object_type,r.start,r.end FROM rom_object o
        JOIN rom_range r USING(range_id) WHERE r.start<? AND ?<r.end""", (end, start)).fetchall()
    span_set = set(spans)
    for row in existing:
        old = (int(row["start"]), int(row["end"]))
        if row["object_type"] == "UNKNOWN":
            continue
        if row["object_type"] != "M68K_INSTRUCTION" or old not in span_set:
            raise ValueError("INCOMPATIBLE_OBJECT_OVERLAP")
    owner = store.db.execute("SELECT * FROM emission WHERE start<=? AND ?<end",
                             (start, start)).fetchone()
    if owner is None or str(owner["source_kind"]) not in {
            "UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"} or int(owner["end"]) < end:
        raise ValueError("PARENT_EMISSION_NOT_UNKNOWN_OR_NOT_COVERING")
    source = cfg_path.parent / extent["asm"]
    validation_root.mkdir(parents=True, exist_ok=True)
    binary = validation_root / f"{start:06X}-{end:06X}.bin"
    assembled = subprocess.run([str(assembler), "-m68000", "-no-opt", "-Fbin",
        "-o", str(binary), str(source)], capture_output=True, text=True, check=False)
    if assembled.returncode:
        raise ValueError("VASMM68K_ROUNDTRIP_FAILED")
    actual = binary.read_bytes()
    expected = rom[start:end]
    digest = sha256_bytes(actual)
    if actual != expected or digest != proof["roundtrip_sha256"]:
        raise ValueError("VASMM68K_ROUNDTRIP_BYTES_OR_HASH_MISMATCH")
    return {"start": start, "end": end, "size": end - start,
        "cfg_sha256": cfg_hash, "roundtrip_sha256": digest,
        "proof_refs": sorted(proof["proof_refs"]), "references": refs,
        "instruction_spans": spans, "instructions": instructions,
        "asm_source": source, "valid": True}

def validate_proposal(base_db: Path, proposal_db: Path, receipt: dict,
                      reproduced: dict, campaign_root: Path, rom: Path,
                      assembler: Path, output: Path) -> dict[str, Any]:
    rom_bytes = rom.read_bytes()
    if len(rom_bytes) != ROM_SIZE or sha256_bytes(rom_bytes) != ROM_SHA256:
        raise ValueError("STOP_M14_5_CANONICAL_ROM_IDENTITY")
    with as_store(base_db) as base, as_store(proposal_db) as proposal_store:
        identity = _tracked_identity(receipt, reproduced, base)
        proposal = proposal_store.db.execute("SELECT * FROM map_proposal WHERE proposal_id=?",
                                             (PROPOSAL_ID,)).fetchone()
        if proposal is None or proposal["status"] != "PROPOSED":
            raise ValueError("STOP_M14_4_PROPOSAL_ROW_MISSING")
        if (proposal["base_generation"], proposal["base_map_hash"], proposal["graph_hash"],
                proposal["proposal_set_hash"]) != (identity["base_generation"],
                identity["base_map_hash"], identity["graph_hash"], PROPOSAL_HASH):
            raise ValueError("STOP_M14_4_PROPOSAL_PARENT_OR_HASH_MISMATCH")
        if proposal_store.hashes()["map_hash"] != base.hashes()["map_hash"]:
            raise ValueError("STOP_M14_4_PROPOSAL_CLONE_CHANGED_PARENT_MAP")
        proposal_store.validate_map_proposal_parent(PROPOSAL_ID, identity["base_generation"])
        operations = _rows(proposal_store)
        rows = [{"ordinal": index, "operation": operation}
                for index, operation in enumerate(operations)]
        if sha256_bytes(canonical(rows).encode("utf-8")) != PROPOSAL_HASH or \
                len(operations) != EXPECTED_OPERATIONS:
            raise ValueError("STOP_M14_4_OPERATION_SET_HASH_OR_COUNT")
        for operation in operations:
            if any(operation.get(key) != identity[value] for key, value in (
                    ("base_generation", "base_generation"), ("base_map_hash", "base_map_hash"),
                    ("base_emission_hash", "emission_hash"), ("graph_hash", "graph_hash"))):
                raise ValueError("STOP_M14_4_OPERATION_PARENT_MISMATCH")
            for ref_id in operation.get("proof_refs", []):
                if not base.db.execute("SELECT 1 FROM evidence_ref WHERE ref_id=?", (ref_id,)).fetchone():
                    raise ValueError("STOP_M14_4_PROOF_REFERENCE_UNRESOLVED")
        proofs = _proof_rows(reproduced)
        components = []
        accepted_ops: set[int] = set()
        rejected_ops: dict[int, str] = {}
        by_extent: dict[tuple[int, int], list[tuple[int, dict]]] = {}
        for ordinal, operation in enumerate(operations):
            extent = (int(operation.get("start", operation.get("address", -1))),
                      int(operation.get("end", -1)))
            if operation["operation"] == "ADD_REFERENCE":
                extent = next((key for key, (_, proof) in proofs.items()
                    if {"source_pc": operation["source_pc"], "target_pc": operation["target_pc"],
                        "instruction_operation": operation["instruction_operation"]}
                    in proof["references"]), (-1, -1))
            by_extent.setdefault(extent, []).append((ordinal, operation))
        validation_root = output / "roundtrip-validation"
        for extent, (analysis, proof) in sorted(proofs.items()):
            try:
                component = _validate_component(base, proof, analysis, campaign_root,
                    rom_bytes, assembler, validation_root)
            except (OSError, ValueError, KeyError, StopIteration) as error:
                component = {"start": extent[0], "end": extent[1],
                    "size": extent[1] - extent[0], "valid": False,
                    "blocker": str(error)}
            ops = [(i, op) for values in by_extent.values() for i, op in values
                   if (op.get("start"), op.get("end")) == extent or
                   (op["operation"] == "ADD_BOUNDARY" and
                    int(op["address"]) in extent) or
                   (op["operation"] == "ADD_REFERENCE" and
                    int(op["source_pc"]) >= extent[0] and int(op["source_pc"]) < extent[1])]
            core = [entry for entry in ops if entry[1]["operation"] != "ADD_REFERENCE"]
            core_types = [op["operation"] for _, op in core]
            core_valid = (component["valid"] and core_types.count("SPLIT_RANGE") == 1 and
                core_types.count("ADD_BOUNDARY") == 2 and core_types.count("CLASSIFY_RANGE") == 1 and
                all(op.get("analysis_window") == [analysis["start"], analysis["end"]]
                    for _, op in core) and
                all(op.get("cfg_proof") == proof["cfg_sha256"] and
                    op.get("roundtrip_proof") == proof["roundtrip_sha256"]
                    for _, op in core) and
                any(op["operation"] == "SPLIT_RANGE" and (op["start"], op["end"]) == extent
                    for _, op in core) and
                any(op["operation"] == "CLASSIFY_RANGE" and
                    (op["start"], op["end"], op["emission_type"]) == (*extent, "ASM")
                    for _, op in core) and
                {int(op["address"]) for _, op in core if op["operation"] == "ADD_BOUNDARY"}
                    == set(extent))
            if core_valid:
                accepted_ops.update(index for index, _ in core)
            else:
                for index, _ in core:
                    rejected_ops[index] = component.get("blocker", "CORE_OPERATION_PROOF_MISMATCH")
            accepted_references = []
            for index, operation in ops:
                if operation["operation"] != "ADD_REFERENCE":
                    continue
                reference = {key: operation[key] for key in
                    ("source_pc", "target_pc", "instruction_operation")}
                if core_valid and reference in component["references"] and \
                        operation.get("reference_kind") == "M68K_CONTROL_FLOW":
                    source = base.db.execute("""SELECT 1 FROM rom_object o JOIN rom_range r USING(range_id)
                        WHERE o.object_type='M68K_INSTRUCTION' AND r.start=? AND r.end>?""",
                        (reference["source_pc"], reference["source_pc"])).fetchone()
                    target = base.db.execute("""SELECT 1 FROM rom_object o JOIN rom_range r USING(range_id)
                        WHERE o.object_type='M68K_INSTRUCTION' AND r.start=?""",
                        (reference["target_pc"],)).fetchone()
                    if source and target:
                        accepted_ops.add(index)
                        accepted_references.append(reference)
                        continue
                rejected_ops[index] = "CONTROL_FLOW_REFERENCE_TARGET_OR_PROOF_INVALID"
            component.update({"operation_count": len(ops),
                "adoptable": core_valid,
                "accepted_references": accepted_references,
                "stage7": _stage7_eligibility(base, component, analysis)})
            components.append(component)
        classified_components = [item for item in components if item.get("adoptable")]
        if sum(op["operation"] == "SPLIT_RANGE" for op in operations) != EXPECTED_COMPONENTS:
            raise ValueError("STOP_M14_4_EXPECTED_COMPONENT_COUNT")
        operation_audit = [{"ordinal": index, "operation": op["operation"],
            "accepted": index in accepted_ops,
            "reason": "EXACT_PROOF_VALIDATED" if index in accepted_ops else
                rejected_ops.get(index, "OPERATION_NOT_SUPPORTED")}
            for index, op in enumerate(operations)]
        return {"identity": identity, "operations": operations,
            "operation_audit": operation_audit, "accepted_operation_ordinals": sorted(accepted_ops),
            "rejected_operation_count": len(operations) - len(accepted_ops),
            "components": components, "valid_components": classified_components,
            "rom_sha256": ROM_SHA256, "rom_size": ROM_SIZE,
            "receipt_sha256": _file_sha(Path("docs/reports/THOR_M14_4_CANONICAL_ROM_GENERIC_ASM_CLOSURE.json"))}

def _stage7_eligibility(store: KnowledgeStore, component: dict,
                        analysis: dict) -> dict[str, Any]:
    start, end = component["start"], component["end"]
    entry = store.db.execute("""SELECT o.object_id FROM rom_object o JOIN rom_range r USING(range_id)
        WHERE o.object_type='M68K_INSTRUCTION' AND r.start=? ORDER BY r.end LIMIT 1""",
        (start,)).fetchone()
    static_callers = 0
    if entry:
        static_callers = int(store.db.execute("""SELECT COUNT(*) FROM relation
            WHERE target_object_id=? AND relation_type IN ('DIRECT_CALL_TARGET','STATIC_CALLER')
            AND status='STATIC_VERIFIED'""", (entry["object_id"],)).fetchone()[0])
    source_owned = int(store.db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission "
        "WHERE source_owned=1 AND start<? AND ?<end", (end, start)).fetchone()[0])
    exact = bool(component.get("valid"))
    callers_ok = static_callers > 0
    return {"exact_extent": exact, "closed_cfg": exact,
        "roundtrip_exact": exact, "emitter_available": exact,
        "reconstruction_proof": False,
        "ownership_preconditions": {"parent_not_owned": source_owned == 0,
            "static_caller_or_entry_support": callers_ok,
            "static_caller_count": static_callers}, "stage7_eligible": False,
        "exact_blocker": (None if exact and callers_ok else
            "MISSING_STATIC_CALLER_OR_ENTRY_AND_WHOLE_ROM_PROMOTION_PROOF")}
