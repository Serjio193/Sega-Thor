"""Evidence-only canonical generation publication under the shared map lock."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import uuid
from typing import Any

try:
    from .knowledge_generation_gc import compact_verified, serialize_publish
    from .rom_knowledge_live_import import sha256_file
    from .rom_knowledge_map import KnowledgeStore
    from .rom_knowledge_normalized_audit import audit_transition
    from .normalized_v2_canonical_adapter import apply_path
except ImportError:
    from knowledge_generation_gc import compact_verified, serialize_publish
    from rom_knowledge_live_import import sha256_file
    from rom_knowledge_map import KnowledgeStore
    from rom_knowledge_normalized_audit import audit_transition
    from normalized_v2_canonical_adapter import apply_path


def _hash_json(value: Any) -> str:
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":"),
        ensure_ascii=True).encode("utf-8")).hexdigest()


def _publish_locked(artifact: Path, artifact_sha256: str, rom_path: Path,
                    output_root: Path, expected_parent_map_hash: str,
                    expected_source_owned: int, *, expected_rom_sha256: str = "",
                    expected_rom_size: int = 0, report_path: Path | None = None,
                    receipt_path: Path | None = None,
                    expected_producer_record_count: int | None = None) -> dict[str, Any]:
    try:
        from .rom_knowledge_pipeline import _copy_database, _current_inputs, _metrics, _write_json
    except ImportError:
        from rom_knowledge_pipeline import _copy_database, _current_inputs, _metrics, _write_json
    try:
        from .identity import ROM_SHA, ROM_SIZE
    except ImportError:
        from identity import ROM_SHA, ROM_SIZE
    artifact, rom_path, output_root = map(Path, (artifact, rom_path, output_root))
    if sha256_file(artifact) != artifact_sha256:
        raise ValueError("STOP_NORMALIZED_V2_ARTIFACT_HASH_MISMATCH")
    rom = rom_path.read_bytes()
    expected_rom_sha256 = expected_rom_sha256 or ROM_SHA
    expected_rom_size = expected_rom_size or ROM_SIZE
    if len(rom) != expected_rom_size or hashlib.sha256(rom).hexdigest() != expected_rom_sha256:
        raise ValueError("STOP_NORMALIZED_V2_ROM_IDENTITY_MISMATCH")
    current_master, current_knowledge, pointer = _current_inputs(
        output_root, expected_rom_sha256, expected_rom_size)
    if not pointer or pointer.get("knowledge_map_hash") != expected_parent_map_hash:
        raise ValueError("STOP_CANONICAL_PARENT_CHANGED_BEFORE_NORMALIZED_V2_PUBLISH")
    base_counts, base_hashes, base_metrics = _metrics(
        current_knowledge, expected_rom_sha256, expected_rom_size)
    if base_metrics["source_owned_bytes"] != expected_source_owned:
        raise ValueError("STOP_RUNTIME_SOURCE_OWNED_MUTATION")
    staging_root = output_root / "generations"
    staging_root.mkdir(parents=True, exist_ok=True)
    staging = staging_root / (".staging-" + uuid.uuid4().hex)
    staging.mkdir()
    staged_master, staged_knowledge = staging / "master.sqlite", staging / "knowledge.sqlite"
    committed = False
    try:
        _copy_database(current_master, staged_master)
        _copy_database(current_knowledge, staged_knowledge)
        parent_generation = str(pointer["generation_id"])
        generation_id = "gen-" + artifact_sha256[:16] + "-" + uuid.uuid4().hex[:8]
        store = KnowledgeStore(staged_knowledge, expected_rom_sha256, expected_rom_size)
        try:
            store.set_generation_identity(generation_id, parent_generation)
        finally:
            store.close()
        adapter = apply_path(staged_knowledge, artifact, artifact_sha256, rom,
                             expected_rom_sha256, expected_rom_size)
        if int(adapter["input_records"]) + int(adapter["producer_deduplicated_records"]) != \
                int(adapter["producer_record_count"]):
            raise ValueError("STOP_NORMALIZED_V2_HISTORICAL_RECEIPT_RECORD_MISMATCH")
        if expected_producer_record_count is not None and \
                int(adapter["producer_record_count"]) != expected_producer_record_count:
            raise ValueError("STOP_NORMALIZED_V2_HISTORICAL_RECEIPT_RECORD_MISMATCH")
        audit = audit_transition(current_knowledge, staged_knowledge, artifact_sha256,
            int(adapter["input_records"]), adapter, rom_path,
            expected_rom_sha256, expected_rom_size)
        if audit.get("status") != "PASS_NORMALIZED_V2_CANONICAL_AUDIT_V1" or \
                audit["source_owned_delta"] != 0:
            raise ValueError("STOP_NORMALIZED_V2_CANONICAL_AUDIT_FAILED")
        after_counts, after_hashes, after_metrics = _metrics(
            staged_knowledge, expected_rom_sha256, expected_rom_size)
        if after_metrics["source_owned_bytes"] != expected_source_owned or \
                after_hashes["emission_hash"] != base_hashes["emission_hash"] or \
                after_hashes["map_hash"] == base_hashes["map_hash"]:
            raise ValueError("STOP_NORMALIZED_V2_CANONICAL_DELTA_INVALID")
        merge = {"schema": "oasis.m14.normalized-v2-evidence-transaction.v1",
            "transaction_type": "NORMALIZED_V2_EVIDENCE_ONLY",
            "session_id": "normalized-v2-" + artifact_sha256[:24],
            "source_artifact_sha256": artifact_sha256, "rom_sha256": expected_rom_sha256,
            "parent_generation_id": parent_generation,
            "parent_map_hash": base_hashes["map_hash"],
            "input_records": adapter["input_records"], "outcomes": adapter["outcomes"]}
        merge["receipt_sha256"] = _hash_json(merge)
        imported = {"status": "PASS_IMPORTED", "session_graph_hash": None,
            "source_artifact_sha256": artifact_sha256,
            "supplemental_evidence": adapter}
        receipt = {"schema": "oasis.m14.normalized-v2-canonical-publish.v1",
            "checkpoint": "M14.7B-NORMALIZED-V2-CANONICAL-PUBLISH",
            "status": "PASS_NORMALIZED_V2_CANONICAL_PUBLISH_V1",
            "rom": {"sha256": expected_rom_sha256, "bytes": expected_rom_size},
            "generation_id": generation_id, "archivist_merge": merge,
            "knowledge_before": {"counts": base_counts, "hashes": base_hashes,
                                 "metrics": base_metrics},
            "knowledge_after": {"counts": after_counts, "hashes": after_hashes,
                                "metrics": after_metrics},
            "import": imported, "independent_audit": audit,
            "source_owned": {"before": base_metrics["source_owned_bytes"],
                "after": after_metrics["source_owned_bytes"], "delta": 0},
            "emission_unchanged": True,
            "performance": {"processing_phase": "post-run",
                "knowledge_db_bytes": staged_knowledge.stat().st_size}}
        _write_json(staging / "merge-receipt.json", merge)
        _write_json(staging / "import-report.json", imported)
        _write_json(staging / "audit.json", audit)
        _write_json(staging / "receipt.json", receipt)
        final_generation = staging_root / generation_id
        os.replace(staging, final_generation)
        committed = True
        pointer_after = {"schema": pointer["schema"], "generation_id": generation_id,
            "generation_dir": "generations/" + generation_id,
            "parent_generation_id": parent_generation,
            "master_sha256": sha256_file(final_generation / "master.sqlite"),
            "knowledge_sha256": sha256_file(final_generation / "knowledge.sqlite"),
            "last_session_source_sha256": pointer.get("last_session_source_sha256"),
            "merge_receipt_sha256": merge["receipt_sha256"],
            "knowledge_map_hash": after_hashes["map_hash"], "logical_hashes": after_hashes,
            "last_evidence_source_sha256": artifact_sha256}
        _write_json(output_root / "current.json", pointer_after)
        receipt["generation_dir"] = str(final_generation)
        if receipt_path:
            _write_json(Path(receipt_path), receipt)
        if report_path:
            Path(report_path).parent.mkdir(parents=True, exist_ok=True)
            Path(report_path).write_text(json.dumps(receipt, sort_keys=True, indent=2) + "\n",
                                         encoding="utf-8", newline="\n")
        compaction = compact_verified(output_root, lock_held=True)
        if compaction.get("status") != "PASS":
            raise RuntimeError("STOP_CANONICAL_GENERATION_COMPACTION_FAILED:" +
                               "; ".join(compaction.get("blocked", [])))
        receipt["generation_compaction"] = {key: compaction.get(key) for key in (
            "removed_database_files", "removed_database_bytes", "full_generation_count_before",
            "full_generation_bytes_before", "full_generation_count_after",
            "full_generation_bytes_after", "current_map_hash")}
        return receipt
    except Exception:
        if not committed and staging.exists():
            shutil.rmtree(staging)
        raise


def publish_normalized_v2(*args: Any, **kwargs: Any) -> dict[str, Any]:
    try:
        from .knowledge_generation_gc import serialize_publish
    except ImportError:
        from knowledge_generation_gc import serialize_publish
    return serialize_publish(_publish_locked, *args, **kwargs)
