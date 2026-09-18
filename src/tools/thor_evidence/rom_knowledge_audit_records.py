"""Independent deterministic identity checks for knowledge graph records."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from typing import Any


OBJECT_TYPES = {"ROM_RANGE", "M68K_INSTRUCTION", "M68K_FUNCTION", "ROM_DATA",
                "POINTER_TABLE", "TABLE_ENTRY", "GRAPHICS_STREAM", "AUDIO_DATA",
                "Z80_PROGRAM", "UNKNOWN"}
STATUSES = {"OBSERVED_RUNTIME", "DERIVED_EXACT", "STATIC_VERIFIED",
            "MANUAL_VERIFIED", "HYPOTHESIS", "CONFLICT"}
RELATIONS = {"EXECUTED_NEXT", "OBSERVED_NEXT_PC", "EXECUTED_FROM_ROM",
             "CONTAINS", "POINTS_TO", "CALLS", "READS", "WRITES"}
UNKNOWN_KINDS = {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"}
UNCONFIRMED = {"PROBABLE", "CANDIDATE", "UNVERIFIED"}
GRAPHICS = {"GRAPHICS_COMPRESSED_STREAM", "CONSUMER_BACKED_GRAPHICS_STREAM",
    "TABLE_SELECTED_GRAPHICS_STREAM", "DIRECT_GRAPHICS_STREAM", "GRAPHICS_DIRECT_LOADER_STREAM",
    "DIRECT_MENU_GRAPHICS_STREAM", "RUNTIME_CORRELATED_GRAPHICS_STREAM", "EXACT_3820_GRAPHICS_STREAM"}
POINTERS = {"ABSOLUTE_STATE_DISPATCH_POINTER_TABLE", "STATIC_DISPATCH_POINTER_TABLE",
    "STATIC_MULTI_DISPATCH_POINTER_TABLE", "SCREEN_GROUP_POINTER_TABLE", "GROUP_POINTER_TABLE_32X32",
    "COMPRESSED_RESOURCE_POINTER_TABLE", "CCB0_RELATIVE_TARGET_TABLE_256X16",
    "SIGNED_RELATIVE_EVENT_DISPATCH_TABLE"}


def _canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _id(prefix: str, value: Any) -> str:
    digest = hashlib.sha256(_canonical(value).encode("utf-8")).hexdigest()
    return f"{prefix}:{digest}"


def _object_type(entry: dict[str, Any]) -> str:
    kind = str(entry.get("kind", "UNKNOWN"))
    label = str(entry.get("classification", kind))
    if kind in UNKNOWN_KINDS:
        return "UNKNOWN"
    if label in GRAPHICS:
        return "GRAPHICS_STREAM"
    if label == "SOUND_DATA_CONTAINER_CONFIRMED":
        return "AUDIO_DATA"
    if label == "Z80_ASM_SOURCE_OWNED":
        return "Z80_PROGRAM"
    if label in POINTERS:
        return "POINTER_TABLE"
    if kind in {"STRUCTURED_DATA_CONFIRMED", "DATA_KNOWN", "PADDING_ALIGNMENT_CONFIRMED", "HEADER_VECTOR_ASM"}:
        return "ROM_DATA"
    return "ROM_RANGE"


def _owned(entry: dict[str, Any]) -> bool:
    return entry.get("kind") not in UNKNOWN_KINDS and \
        str(entry.get("confidence", "")).upper() not in UNCONFIRMED


def audit_record_ids(db: sqlite3.Connection, entries: list[dict[str, Any]],
                     object_by_id: dict[str, tuple[str, int, int, dict[str, Any]]],
                     source_owned_mask: bytearray, rom_sha: str) -> tuple[dict[str, Any], set[str], set[str]]:
    """Verify canonical IDs, allowed statuses, manifest claims and evidence subjects."""
    if any(item[0] not in OBJECT_TYPES for item in object_by_id.values()):
        raise ValueError("STOP_OBJECT_TYPE_INVALID")
    claim_rows = list(db.execute("SELECT claim_id,object_id,claim_type,value_json,status FROM claim"))
    claim_by_id: dict[str, tuple[str, str, str]] = {}
    for row in claim_rows:
        claim_id, oid, claim_type, value_text, status = map(str, row)
        value = json.loads(value_text)
        payload = {"object_id": oid, "claim_type": claim_type, "value": value, "status": status}
        if oid not in object_by_id or status not in STATUSES or claim_id != _id("claim", payload):
            raise ValueError("STOP_CLAIM_IDENTITY_INVALID")
        claim_by_id[claim_id] = (oid, claim_type, value_text)

    relation_ids: set[str] = set()
    relation_rows = db.execute("""SELECT relation_id,relation_type,source_object_id,
        target_object_id,target_address,status FROM relation""")
    for row in relation_rows:
        relation_id, relation_type, source_oid, target_oid, target_address, status = row
        payload = {"relation_type": str(relation_type), "source_object_id": str(source_oid),
                   "target_object_id": str(target_oid) if target_oid is not None else None,
                   "target_address": int(target_address) if target_address is not None else None,
                   "status": str(status)}
        if relation_type not in RELATIONS or status not in STATUSES or \
                source_oid not in object_by_id or (target_oid is not None and target_oid not in object_by_id) or \
                relation_id != _id("relation", payload) or relation_id in relation_ids:
            raise ValueError("STOP_RELATION_IDENTITY_INVALID")
        relation_ids.add(str(relation_id))

    for entry in entries:
        start, end = int(entry["start"]), int(entry["end"])
        object_type = _object_type(entry)
        oid = _id("object", {"rom_sha256": rom_sha, "start": start,
                             "end": end, "object_type": object_type})
        if oid not in object_by_id:
            raise ValueError("STOP_SOURCE_OWNED_IMPORT_MISMATCH")
        owned = _owned(entry)
        expected_class = {"source_kind": str(entry.get("kind", "UNKNOWN")),
                          "classification": str(entry.get("classification", entry.get("kind", "UNKNOWN"))),
                          "confidence": str(entry.get("confidence", "UNKNOWN"))}
        found = {str(r[0]): (str(r[1]), str(r[2])) for r in db.execute(
            "SELECT claim_type,value_json,status FROM claim WHERE object_id=? AND claim_type IN ('SOURCE_CLASS','SOURCE_OWNED')",
            (oid,))}
        expected_status = "STATIC_VERIFIED" if owned else "DERIVED_EXACT"
        if found.get("SOURCE_CLASS") != (_canonical(expected_class), expected_status) or \
                found.get("SOURCE_OWNED") != (_canonical(owned), expected_status):
            raise ValueError("STOP_SOURCE_OWNED_CLAIM_MISMATCH")
        if sum(source_owned_mask[start:end]) != (end - start if owned else 0):
            raise ValueError("STOP_SOURCE_OWNED_CLAIM_MISMATCH")

    evidence_ids: set[str] = set()
    evidence_rows = db.execute("""SELECT ref_id,subject_type,subject_id,source_sha256,
        fact_kind,fact_count,locator_json FROM evidence_ref""")
    for row in evidence_rows:
        ref_id, subject_type, subject_id, source_hash, fact_kind, fact_count, locator_text = row
        payload = {"subject_type": str(subject_type), "subject_id": str(subject_id),
                   "source_sha256": str(source_hash), "fact_kind": str(fact_kind),
                   "fact_count": int(fact_count), "locator": json.loads(str(locator_text))}
        subject_exists = (subject_type == "CLAIM" and subject_id in claim_by_id) or \
                         (subject_type == "RELATION" and subject_id in relation_ids)
        if not subject_exists or len(str(source_hash)) != 64 or int(fact_count) < 0 or \
                ref_id != _id("evidence", payload) or ref_id in evidence_ids:
            raise ValueError("STOP_EVIDENCE_REFERENCE_INVALID")
        evidence_ids.add(str(ref_id))
    return claim_by_id, relation_ids, evidence_ids
