"""Independent transition checks for normalized-v2 evidence publication."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
from typing import Any

try:
    from .rom_knowledge_evidence_preservation import verify_database_superset
    from .rom_knowledge_map import KnowledgeStore, TABLE_COLUMNS
    from .rom_knowledge_live_import import sha256_file
    from .runtime_path_view import iter_runtime_paths
except ImportError:
    from rom_knowledge_evidence_preservation import verify_database_superset
    from rom_knowledge_map import KnowledgeStore, TABLE_COLUMNS
    from rom_knowledge_live_import import sha256_file
    from runtime_path_view import iter_runtime_paths


def _canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _paths(db: sqlite3.Connection) -> tuple[int, str, bool, bool]:
    digest, count, ordered, windows = hashlib.sha256(), 0, True, True
    for item in iter_runtime_paths(db):
        sequence, window = item["native_sequences"], item["window"]
        ordered = ordered and all(type(value) is int for value in sequence) and all(
            left < right for left, right in zip(sequence, sequence[1:]))
        windows = windows and type(window.get("worker_id")) is int and \
            window["worker_id"] >= 0 and all(type(window.get(key)) is int and window[key] > 0
            for key in ("capture_id", "generation")) and \
            len(item["occurrence_ids"]) == len(item["pcs"])
        digest.update(_canonical(item).encode("utf-8") + b"\n")
        count += 1
    return count, digest.hexdigest(), ordered, windows


def audit_transition(base_path: Path, staged_path: Path, artifact_sha256: str,
                     expected_records: int, adapter: dict[str, Any], rom_path: Path,
                     rom_sha256: str, rom_size: int) -> dict[str, Any]:
    """Prove monotonic evidence addition and unchanged canonical semantics."""
    if sha256_file(rom_path) != rom_sha256 or Path(rom_path).stat().st_size != rom_size:
        raise ValueError("STOP_NORMALIZED_V2_AUDIT_ROM_IDENTITY_FAILED")
    base = sqlite3.connect(Path(base_path).resolve().as_uri() + "?mode=ro&immutable=1", uri=True)
    final = sqlite3.connect(Path(staged_path).resolve().as_uri() + "?mode=ro&immutable=1", uri=True)
    try:
        base.row_factory = final.row_factory = sqlite3.Row
        for db in (base, final):
            if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok" or \
                    db.execute("PRAGMA foreign_key_check").fetchone() is not None:
                raise ValueError("STOP_NORMALIZED_V2_AUDIT_SQLITE_FAILED")
        meta = dict(final.execute("SELECT key,value FROM map_meta"))
        if meta.get("schema") != "oasis.m14.canonical-rom-knowledge.v2" or \
                meta.get("rom_sha256") != rom_sha256 or meta.get("rom_size") != str(rom_size):
            raise ValueError("STOP_NORMALIZED_V2_AUDIT_ROM_IDENTITY_FAILED")
        base_store = KnowledgeStore(base_path, rom_sha256, rom_size, read_only=True)
        final_store = KnowledgeStore(staged_path, rom_sha256, rom_size, read_only=True)
        try:
            before_metrics, after_metrics = base_store.metrics(), final_store.metrics()
            before_hashes, after_hashes = base_store.hashes(), final_store.hashes()
        finally:
            base_store.close()
            final_store.close()
        if before_metrics["source_owned_bytes"] != after_metrics["source_owned_bytes"] or \
                before_hashes["emission_hash"] != after_hashes["emission_hash"]:
            raise ValueError("STOP_RUNTIME_SOURCE_OWNED_MUTATION")
        for table, columns in TABLE_COLUMNS.items():
            if not verify_database_superset(base, final, table, tuple(columns)):
                raise ValueError("STOP_NORMALIZED_V2_AUDIT_PRIOR_EVIDENCE_LOSS:" + table)
        before_paths = _paths(base)
        after_paths = _paths(final)
        if before_paths != after_paths:
            raise ValueError("STOP_NORMALIZED_V2_AUDIT_RUNTIME_PATH_CHANGE")
        source_count, first, last, distinct, runs = final.execute("""SELECT COUNT(*),
            MIN(json_extract(locator_json,'$.record_ordinal')),
            MAX(json_extract(locator_json,'$.record_ordinal')),
            COUNT(DISTINCT json_extract(locator_json,'$.record_ordinal')),
            COUNT(DISTINCT json_extract(locator_json,'$.run_id')) FROM evidence_ref
            WHERE source_sha256=? AND fact_kind LIKE 'NORMALIZED_V2_RECORD:%'""",
            (artifact_sha256,)).fetchone()
        mapped, bad_links = final.execute("""SELECT COUNT(*),SUM(CASE WHEN o.object_id IS NULL
            OR o.object_type!='M68K_INSTRUCTION' THEN 1 ELSE 0 END) FROM evidence_ref e
            LEFT JOIN rom_object o ON o.object_id=e.subject_id WHERE e.source_sha256=?
            AND e.fact_kind IN ('NORMALIZED_V2_RECORD:MERGED',
                                'NORMALIZED_V2_RECORD:ALREADY_KNOWN')""",
            (artifact_sha256,)).fetchone()
        outcomes = {str(row[0]).rsplit(":", 1)[1]: int(row[1]) for row in final.execute(
            "SELECT fact_kind,COUNT(*) FROM evidence_ref WHERE source_sha256=? "
            "AND fact_kind LIKE 'NORMALIZED_V2_RECORD:%' GROUP BY fact_kind", (artifact_sha256,))}
        if int(source_count) != expected_records or first != 0 or last != expected_records - 1 or \
                int(distinct) != expected_records or int(runs) != 1 or int(bad_links or 0) != 0 or \
                int(mapped) != int(adapter.get("mapped_rom_records", -1)) or \
                sum(int(value) for value in outcomes.values()) != expected_records or \
                int(adapter.get("input_records", -1)) != expected_records or \
                int(adapter.get("unaccounted_records", -1)) != 0:
            raise ValueError("STOP_NORMALIZED_V2_AUDIT_ACCOUNTING_OR_PROVENANCE_FAILED")
        return {"status": "PASS_NORMALIZED_V2_CANONICAL_AUDIT_V1",
            "artifact_sha256": artifact_sha256, "input_records": expected_records,
            "outcomes": outcomes, "provenance_rows": int(source_count),
            "mapped_rom_records": int(mapped), "path_count": before_paths[0],
            "path_sha256_before": before_paths[1], "path_sha256_after": after_paths[1],
            "source_owned_before": before_metrics["source_owned_bytes"],
            "source_owned_after": after_metrics["source_owned_bytes"],
            "source_owned_delta": after_metrics["source_owned_bytes"] -
                before_metrics["source_owned_bytes"],
            "hashes_before": before_hashes, "hashes_after": after_hashes,
            "sqlite_integrity": "PASS", "foreign_keys": "PASS",
            "record_ordering": "PRESERVED_AS_SOURCE_PROVENANCE",
            "runtime_paths_unchanged": True}
    finally:
        base.close()
        final.close()
