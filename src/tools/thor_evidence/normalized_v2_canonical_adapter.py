"""Stream normalized-generic v2 primary records into staged canonical evidence."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
from typing import Any, Iterator

try:
    from .rom_knowledge_fusion_stream import _Stream, find_unique_array_value
    from .rom_knowledge_live_import import sha256_file
    from .rom_knowledge_map import KnowledgeStore, TABLE_COLUMNS, stable_id
except ImportError:
    from rom_knowledge_fusion_stream import _Stream, find_unique_array_value
    from rom_knowledge_live_import import sha256_file
    from rom_knowledge_map import KnowledgeStore, TABLE_COLUMNS, stable_id


SCHEMA = "oasis.m13.normalized-generic-corpus.v2"
METADATA = {"schema", "corpus_id", "record_count", "sealed", "raw_sha256",
            "index_sha256", "source_owned_before"}
FACT_PREFIX = "NORMALIZED_V2_RECORD:"


class NormalizedV2Reader:
    """Expose producer-defined `records[]` exactly once with source ordinals."""

    def __init__(self, path: Path):
        self.path = Path(path)
        self.metadata: dict[str, Any] = {}

    def records(self) -> Iterator[tuple[int, Any]]:
        offset = find_unique_array_value(self.path, "records")
        stream = _Stream(self.path, offset=offset)
        try:
            yield from stream.array_values()
            stream.top_level_tail(METADATA, self.metadata)
        finally:
            stream.file.close()


def _record_type(record: Any) -> str:
    return str(record.get("kind", "<missing-kind>")) if isinstance(record, dict) else "<non-object>"


def _identity(record: dict[str, Any]) -> dict[str, Any]:
    return {key: record.get(key) for key in ("run_id", "epoch", "frame",
        "stream_sequence", "instruction_sequence", "cpu_id") if key in record}


def _valid_base(record: dict[str, Any]) -> bool:
    return type(record.get("run_id")) is int and record["run_id"] > 0 and \
        type(record.get("epoch")) is int and record["epoch"] > 0 and \
        type(record.get("stream_sequence")) is int and record["stream_sequence"] >= 0


def _exact_object(db: sqlite3.Connection, record: dict[str, Any], rom: bytes) -> str | None:
    pc, opcode = record.get("pc"), record.get("opcode")
    if record.get("cpu_id") != "M68K" or record.get("opcode_verification") != "ROM_OPCODE_EXACT" or \
            type(pc) is not int or type(opcode) is not int or pc < 0 or pc + 2 > len(rom) or \
            opcode != int.from_bytes(rom[pc:pc + 2], "big"):
        return None
    for row in db.execute("""SELECT o.object_id,o.attributes_json,r.start,r.end
            FROM rom_object o JOIN rom_range r USING(range_id)
            WHERE o.object_type='M68K_INSTRUCTION' AND r.start=? ORDER BY r.end""", (pc,)):
        attrs = json.loads(str(row[1]))
        if int(row[2]) == pc and int(row[3]) >= pc + 2 and int(attrs.get("opcode", -1)) == opcode:
            return str(row[0])
    return None


def _validate(record: Any) -> tuple[bool, str | None]:
    if not isinstance(record, dict) or not _valid_base(record):
        return False, "record must be an object with positive run/epoch and stream ordinal"
    kind = record.get("kind")
    if kind in {"instruction", "bus"}:
        if any(type(record.get(key)) is not int or record[key] < 0
               for key in ("pc", "address", "opcode")):
            return False, "instruction/bus record lacks nonnegative pc/address/opcode integers"
        if record.get("cpu_id") not in {"M68K", "Z80", "NONE"}:
            return False, "instruction/bus record has unsupported cpu_id field"
        if kind == "instruction" and not isinstance(record.get("opcode_verification"), str):
            return False, "instruction record lacks opcode_verification"
        return True, None
    if kind == "frame_boundary":
        if type(record.get("frame")) is not int:
            return False, "frame_boundary lacks integer frame"
        return True, None
    return True, None


def _evidence_row(artifact_sha: str, ordinal: int, record: Any,
                  outcome: str, subject_id: str, reason: str | None) -> dict[str, Any]:
    record_type = _record_type(record)
    payload = {"artifact_sha256": artifact_sha, "record_ordinal": ordinal,
               "record_type": record_type}
    locator = {**payload, "run_id": record.get("run_id") if isinstance(record, dict) else None,
        "source_identity": _identity(record) if isinstance(record, dict) else {},
        "original_fields": record, "outcome": outcome, "reason": reason}
    return {"ref_id": stable_id("evidence", payload), "subject_type":
            "ROM_OBJECT" if outcome in {"MERGED", "ALREADY_KNOWN"} else "NORMALIZED_RECORD",
            "subject_id": subject_id, "source_sha256": artifact_sha,
            "fact_kind": FACT_PREFIX + outcome, "fact_count": 1,
            "locator_json": json.dumps(locator, sort_keys=True, separators=(",", ":"),
                                       ensure_ascii=True)}


def _receipt_seed(store: KnowledgeStore, artifact_sha: str, path: Path) -> None:
    row = {"source_sha256": artifact_sha,
        "checkpoint": "M14.7B-NORMALIZED-V2-CANONICAL-ADAPTER",
        "artifact_name": Path(path).name,
        "artifact_type": "NORMALIZED_GENERIC_CORPUS_V2"}
    existing = store.db.execute("SELECT source_sha256,checkpoint,artifact_name,artifact_type "
        "FROM source_artifact WHERE source_sha256=?", (artifact_sha,)).fetchone()
    if existing is not None and tuple(existing) != tuple(row.values()):
        raise ValueError("STOP_NORMALIZED_V2_SOURCE_IDENTITY_CONFLICT")
    store.insert_rows("source_artifact", [row])


def apply(store: KnowledgeStore, artifact: Path, expected_sha256: str,
          rom: bytes, *, limit: int | None = None,
          verify_only: bool = False) -> dict[str, Any]:
    """Account every selected record and persist source provenance in the staged map."""
    artifact = Path(artifact)
    actual_sha = sha256_file(artifact)
    if actual_sha != expected_sha256:
        raise ValueError("STOP_NORMALIZED_V2_ARTIFACT_HASH_MISMATCH")
    reader = NormalizedV2Reader(artifact)
    counts = {key: 0 for key in ("MERGED", "ALREADY_KNOWN", "UNRESOLVED", "REJECTED")}
    types: dict[str, int] = {}
    direct: dict[str, int] = {}
    unresolved: dict[str, int] = {}
    mapped_records = 0
    inserted = 0
    run_ids: set[int] = set()
    object_cache: dict[tuple[Any, ...], str | None] = {}
    pending: list[tuple[dict[str, Any], str, str, int]] = []
    evidence_columns = TABLE_COLUMNS["evidence_ref"]
    evidence_select = ("SELECT " + ",".join(evidence_columns) +
                       " FROM evidence_ref WHERE ref_id IN ({})")
    evidence_insert = ("INSERT OR IGNORE INTO evidence_ref(" + ",".join(evidence_columns) +
        ") VALUES (" + ",".join("?" for _ in evidence_columns) + ")")

    def flush() -> None:
        nonlocal inserted
        if not pending:
            return
        existing: dict[str, tuple[Any, ...]] = {}
        for start in range(0, len(pending), 500):
            ids = [row[0]["ref_id"] for row in pending[start:start + 500]]
            select = evidence_select.format(",".join("?" for _ in ids))
            for row in store.db.execute(select, ids):
                existing[str(row[0])] = tuple(row)
        new_rows: list[dict[str, Any]] = []
        for entry, semantic, record_type, ordinal in pending:
            old = existing.get(entry["ref_id"])
            if old is not None:
                if old != tuple(entry[column] for column in evidence_columns):
                    raise ValueError("STOP_NORMALIZED_V2_PROVENANCE_IDENTITY_CONFLICT")
                outcome = "ALREADY_KNOWN"
            else:
                if verify_only:
                    raise ValueError("STOP_NORMALIZED_V2_REPLAY_PROVENANCE_MISSING:" + str(ordinal))
                outcome = semantic
                new_rows.append(entry)
            counts[outcome] += 1
            if outcome == "UNRESOLVED":
                unresolved[record_type] = unresolved.get(record_type, 0) + 1
        if new_rows:
            values = [tuple(row[column] for column in evidence_columns) for row in new_rows]
            cursor = store.db.executemany(evidence_insert, values)
            if cursor.rowcount != len(new_rows):
                raise ValueError("STOP_NORMALIZED_V2_EVIDENCE_BATCH_INSERT_CONFLICT")
            inserted += len(new_rows)
        pending.clear()

    if not verify_only:
        _receipt_seed(store, actual_sha, artifact)
    for ordinal, record in reader.records():
        record_type = _record_type(record)
        types[record_type] = types.get(record_type, 0) + 1
        valid, reason = _validate(record)
        subject_id = "normalized-record:" + actual_sha + ":" + str(ordinal)
        semantic = "UNRESOLVED"
        if not valid:
            semantic = "REJECTED"
        else:
            run_ids.add(int(record["run_id"]))
            if record_type == "instruction":
                pc, opcode = record.get("pc"), record.get("opcode")
                cache_key = (record.get("cpu_id"), record.get("opcode_verification"),
                             pc, opcode) if type(pc) is int and type(opcode) is int else None
                object_id = (object_cache[cache_key] if cache_key in object_cache else
                    _exact_object(store.db, record, rom)) if cache_key is not None else \
                    _exact_object(store.db, record, rom)
                if cache_key is not None:
                    object_cache[cache_key] = object_id
                if object_id:
                    semantic = "MERGED"
                    subject_id = object_id
                    mapped_records += 1
                    direct[record_type] = direct.get(record_type, 0) + 1
                    known = store.db.execute("SELECT 1 FROM claim WHERE object_id=? "
                        "AND claim_type='EXECUTED_FROM_ROM' LIMIT 1", (object_id,)).fetchone()
                    if known:
                        semantic = "ALREADY_KNOWN"
                else:
                    reason = "no exact current canonical M68K instruction object; no new code boundary inferred"
            elif record_type == "bus":
                reason = "canonical ROM map has no runtime bus-event object type"
            elif record_type == "frame_boundary":
                reason = "normalized frame boundary lacks a canonical runtime-path endpoint"
            else:
                reason = "valid producer record kind has no canonical projection"
        entry = _evidence_row(actual_sha, ordinal, record, semantic, subject_id, reason)
        pending.append((entry, semantic, record_type, ordinal))
        if len(pending) >= 1000:
            flush()
        if limit is not None and ordinal + 1 >= limit:
            break
    flush()
    input_records = sum(counts.values())
    if input_records != sum(counts.values()):
        raise ValueError("STOP_NORMALIZED_V2_UNACCOUNTED_RECORD")
    if limit is None:
        metadata = reader.metadata
        producer_records = int(metadata.get("record_count", -1))
        if metadata.get("schema") != SCHEMA or metadata.get("sealed") is not True or \
                producer_records < input_records or len(run_ids) != 1:
            raise ValueError("STOP_NORMALIZED_V2_RECEIPT_RECONCILIATION_FAILED")
    if input_records != sum(counts.values()) or inserted + (input_records - inserted) != input_records:
        raise ValueError("STOP_NORMALIZED_V2_ACCOUNTING_INVARIANT_FAILED")
    return {"status": "PASS_NORMALIZED_V2_ACCOUNTED",
        "artifact_sha256": actual_sha, "input_records": input_records,
        "record_types_seen": types, "record_types_direct": direct,
        "record_types_adapted": {}, "record_types_unresolved": unresolved,
        "outcomes": counts, "unaccounted_records": 0,
        "provenance_rows_inserted": inserted, "mapped_rom_records": mapped_records,
        "run_ids": sorted(run_ids), "historical_receipt_records": reader.metadata.get("record_count"),
        "producer_record_count": reader.metadata.get("record_count"),
        "producer_deduplicated_records": (int(reader.metadata["record_count"]) - input_records)
            if limit is None else None,
        "producer_deduplication_rule": "closure_rows['records'].setdefault((kind,cpu_id,pc,opcode,address,opcode_verification), row)",
        "producer_metadata": {key: reader.metadata.get(key) for key in sorted(METADATA)}}


def apply_path(knowledge_path: Path, artifact: Path, expected_sha256: str,
               rom: bytes, rom_sha256: str, rom_size: int, *,
               limit: int | None = None, verify_only: bool = False) -> dict[str, Any]:
    store = KnowledgeStore(knowledge_path, rom_sha256, rom_size,
                           read_only=verify_only)
    try:
        return apply(store, artifact, expected_sha256, rom,
                     limit=limit, verify_only=verify_only)
    finally:
        store.close()
