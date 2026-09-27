"""Read-only evidence overlay loader for canonical ROM knowledge generations."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import sqlite3
from typing import Any

from rom_coverage_trace import RangeTraceIndex

STATUS_NAMES = ("STATIC_VERIFIED", "DERIVED_EXACT", "OBSERVED_RUNTIME",
                "HYPOTHESIS", "CONFLICT", "OTHER_CLAIM")
STATUS_COLORS = {
    "NONE": None,
    "STATIC_VERIFIED": "#f0c75e",
    "DERIVED_EXACT": "#c792ea",
    "OBSERVED_RUNTIME": "#59a8ff",
    "HYPOTHESIS": "#8492a6",
    "CONFLICT": "#ff5c5c",
    "OTHER_CLAIM": "#718096",
    "MIXED": "#f4f4f4",
}
POINTER_SCHEMA = "oasis.m12.archivist-canonical-knowledge.current.v1"
MASTER_POINTER_SCHEMA = "oasis.m12.master-v2-startup.current.v1"
MASTER_POINTER_REL = Path("build/thor-evidence/master-v2/current.json")
M14_POINTER_REL = Path(
    "build/thor-evidence/archivist-knowledge-pipeline-2g-canonical-verified-20260918/current.json")


def resolve_knowledge_pointer(project_root: str | Path,
                              explicit: str | Path | None = None) -> Path | None:
    """Prefer the promoted head; use M14 only when no MASTER pointer exists."""
    if explicit is not None:
        return Path(explicit)
    root = Path(project_root)
    master = root / MASTER_POINTER_REL
    if master.is_file():
        return master
    fallback = root / M14_POINTER_REL
    return fallback if fallback.is_file() else None


@dataclass(frozen=True)
class EvidenceFact:
    start: int
    end: int
    object_id: str
    object_type: str
    label: str
    claim_type: str | None
    status: str | None
    value: str | None


@dataclass(frozen=True)
class RangeEvidence:
    counts: dict[str, int]
    state: str
    facts: tuple[str, ...]
    trace: tuple[str, ...] = ()


@dataclass
class CanonicalEvidenceIndex:
    rom_sha256: str
    rom_size: int
    generation_id: str
    pointer_path: Path
    status_bytes: dict[str, bytearray]
    facts: tuple[EvidenceFact, ...]
    authority: str = "M14 ARCHIVIST FALLBACK"
    trace_index: RangeTraceIndex | None = None

    @classmethod
    def load(cls, pointer_path: str | Path, expected_rom_sha256: str,
             expected_rom_size: int) -> "CanonicalEvidenceIndex":
        pointer_file = Path(pointer_path).resolve(strict=True)
        pointer = json.loads(pointer_file.read_text(encoding="utf-8"))
        if not isinstance(pointer, dict):
            raise ValueError("Canonical knowledge pointer must be an object")
        if pointer.get("schema") == MASTER_POINTER_SCHEMA:
            return cls._load_master_v2(pointer_file, pointer,
                                       expected_rom_sha256, expected_rom_size)
        if pointer.get("schema") != POINTER_SCHEMA:
            raise ValueError("Unsupported canonical knowledge pointer schema")
        generation_id = pointer.get("generation_id")
        generation_rel = pointer.get("generation_dir")
        expected_db_hash = pointer.get("knowledge_sha256")
        if (not isinstance(generation_id, str) or not generation_id or
                not isinstance(generation_rel, str) or not generation_rel or
                not isinstance(expected_db_hash, str) or len(expected_db_hash) != 64):
            raise ValueError("Canonical knowledge pointer identity is incomplete")

        root = pointer_file.parent.resolve(strict=True)
        generation = (root / generation_rel).resolve(strict=True)
        if os.path.commonpath((str(root), str(generation))) != str(root):
            raise ValueError("Canonical generation escapes the pointer directory")
        database = generation / "knowledge.sqlite"
        if not database.is_file() or _sha256_file(database) != expected_db_hash:
            raise ValueError("Canonical knowledge database hash does not match pointer")

        uri = database.as_uri() + "?mode=ro"
        connection = sqlite3.connect(uri, uri=True)
        try:
            connection.row_factory = sqlite3.Row
            connection.execute("PRAGMA query_only=ON")
            metadata = dict(connection.execute("SELECT key,value FROM map_meta"))
            actual_rom_sha = metadata.get("rom_sha256")
            if actual_rom_sha != expected_rom_sha256:
                raise ValueError("Canonical knowledge map ROM SHA-256 mismatch")
            if not expected_rom_sha256 or expected_rom_size <= 0:
                raise ValueError("Property checkpoint ROM identity is incomplete")

            status_bytes = {name: bytearray(expected_rom_size) for name in STATUS_NAMES}
            facts: list[EvidenceFact] = []
            rows = connection.execute("""SELECT r.start,r.end,o.object_id,o.object_type,
                o.attributes_json,c.claim_type,c.value_json,c.status
                FROM rom_range r JOIN rom_object o USING(range_id)
                LEFT JOIN claim c ON c.object_id=o.object_id
                WHERE r.rom_sha256=? AND r.start<? AND r.end>0
                ORDER BY r.start,r.end,o.object_id,c.status,c.claim_type""",
                (expected_rom_sha256, expected_rom_size))
            for row in rows:
                start, end = int(row["start"]), min(int(row["end"]), expected_rom_size)
                if start < 0 or end <= start:
                    raise ValueError("Canonical object range is outside the ROM")
                object_type = str(row["object_type"])
                object_id = str(row["object_id"])
                label = _object_label(object_type, row["attributes_json"])
                status = str(row["status"]) if row["status"] is not None else None
                claim_type = str(row["claim_type"]) if row["claim_type"] is not None else None
                value = _compact_value(row["value_json"]) if row["value_json"] is not None else None
                if status in status_bytes:
                    status_bytes[status][start:end] = b"\x01" * (end - start)
                facts.append(EvidenceFact(start, end, object_id, object_type,
                                          label, claim_type, status, value))

            for row in connection.execute("SELECT start,end FROM conflict WHERE start<? AND end>0",
                                          (expected_rom_size,)):
                start, end = int(row["start"]), min(int(row["end"]), expected_rom_size)
                if start < 0 or end <= start:
                    raise ValueError("Canonical conflict range is outside the ROM")
                status_bytes["CONFLICT"][start:end] = b"\x01" * (end - start)
            trace_index = RangeTraceIndex.from_sqlite(
                connection, expected_rom_sha256, expected_rom_size)
        finally:
            connection.close()

        return cls(expected_rom_sha256, expected_rom_size, generation_id,
                   pointer_file, status_bytes, tuple(facts),
                   "M14 ARCHIVIST FALLBACK", trace_index)

    @classmethod
    def _load_master_v2(cls, pointer_file: Path, pointer: dict[str, Any],
                        expected_rom_sha256: str,
                        expected_rom_size: int) -> "CanonicalEvidenceIndex":
        from master_canonical_view import MasterCanonicalView
        from master_v2_shadow import decode_master_v2

        required = ("generation_id", "path", "sha256", "bytes", "logical_sha256",
                    "rom_sha256", "rom_size", "master_schema", "sections")
        if any(key not in pointer for key in required):
            raise ValueError("MASTER V2 startup pointer identity is incomplete")
        generation_id = pointer["generation_id"]
        relative_path = pointer["path"]
        if not isinstance(generation_id, str) or not generation_id:
            raise ValueError("MASTER V2 generation ID is invalid")
        if not isinstance(relative_path, str) or not relative_path:
            raise ValueError("MASTER V2 master path is invalid")
        if Path(relative_path).is_absolute():
            raise ValueError("MASTER V2 master path must be relative")
        if (pointer["rom_sha256"] != expected_rom_sha256 or
                int(pointer["rom_size"]) != expected_rom_size):
            raise ValueError("MASTER V2 ROM identity mismatch")
        for key in ("sha256", "logical_sha256"):
            value = pointer[key]
            if (not isinstance(value, str) or len(value) != 64 or
                    any(char not in "0123456789abcdef" for char in value.lower())):
                raise ValueError(f"MASTER V2 pointer {key} is invalid")

        root = pointer_file.parent.resolve(strict=True)
        master_path = (root / relative_path).resolve(strict=True)
        if os.path.commonpath((str(root), str(master_path))) != str(root):
            raise ValueError("MASTER V2 master path escapes pointer directory")
        if not master_path.is_file():
            raise ValueError("MASTER V2 master file is missing")
        if (master_path.stat().st_size != int(pointer["bytes"]) or
                _sha256_file(master_path) != pointer["sha256"]):
            raise ValueError("MASTER V2 master file hash or size mismatch")

        decoded = decode_master_v2(master_path, materialize=False)
        if (decoded["schema"] != pointer["master_schema"] or
                decoded["overall_sha256"] != pointer["logical_sha256"]):
            raise ValueError("MASTER V2 logical container hash mismatch")
        actual_sections = {item["name"]: item["sha256"]
                           for item in decoded["sections"]}
        if not isinstance(pointer["sections"], dict) or pointer["sections"] != actual_sections:
            raise ValueError("MASTER V2 section hashes mismatch")

        view = MasterCanonicalView(master_path)
        if (view.generation_id != generation_id or
                view.rom_identity != expected_rom_sha256):
            raise ValueError("MASTER V2 canonical generation identity mismatch")
        trace_index = RangeTraceIndex(
            (row for row in view.rows("rom_range")
             if row["rom_sha256"] == expected_rom_sha256),
            view.objects, view.rows("claim"),
            view.rows("relation"), view.rows("evidence_ref"),
            view.rows("source_artifact"), view.rows("emission"),
            view.rows("derivation"), view.rows("derivation_input"))

        status_bytes = {name: bytearray(expected_rom_size) for name in STATUS_NAMES}
        ranges: dict[str, tuple[int, int]] = {}
        for row in view.rows("rom_range"):
            if row["rom_sha256"] != expected_rom_sha256:
                continue
            start, end = int(row["start"]), min(int(row["end"]), expected_rom_size)
            if start < 0 or end <= start:
                raise ValueError("MASTER V2 range is outside the ROM")
            ranges[str(row["range_id"])] = (start, end)

        claims: dict[str, list[dict[str, Any]]] = {}
        for claim in view.rows("claim"):
            claims.setdefault(str(claim["object_id"]), []).append(claim)
        facts: list[EvidenceFact] = []
        for obj in view.objects:
            bounds = ranges.get(str(obj["range_id"]))
            if bounds is None:
                continue
            start, end = bounds
            object_type = str(obj["object_type"])
            object_id = str(obj["object_id"])
            label = _object_label(object_type, obj["attributes_json"])
            object_claims = claims.get(object_id) or [None]
            for claim in object_claims:
                status = str(claim["status"]) if claim is not None else None
                claim_type = str(claim["claim_type"]) if claim is not None else None
                value = (_compact_value(claim["value_json"])
                         if claim is not None else None)
                bucket = status if status in status_bytes else (
                    "OTHER_CLAIM" if status is not None else None)
                if bucket:
                    status_bytes[bucket][start:end] = b"\x01" * (end - start)
                facts.append(EvidenceFact(start, end, object_id, object_type,
                                          label, claim_type, status, value))

        for row in view.rows("conflict"):
            start, end = int(row["start"]), min(int(row["end"]), expected_rom_size)
            if start < 0 or end <= start:
                raise ValueError("MASTER V2 conflict range is outside the ROM")
            status_bytes["CONFLICT"][start:end] = b"\x01" * (end - start)
        facts.sort(key=lambda item: (item.start, item.end, item.object_id,
                                     item.status or "", item.claim_type or ""))
        return cls(expected_rom_sha256, expected_rom_size, generation_id,
                   pointer_file, status_bytes, tuple(facts), "MASTER V2 STARTUP",
                   trace_index)

    def summarize(self, start: int, end: int, fact_limit: int = 6,
                  include_trace: bool = True) -> RangeEvidence:
        if start < 0 or end <= start or end > self.rom_size:
            raise ValueError("Evidence query range is outside the ROM")
        counts = {name: sum(values[start:end]) for name, values in self.status_bytes.items()}
        present = [name for name in STATUS_NAMES if counts[name]]
        state = "CONFLICT" if counts["CONFLICT"] else (
            present[0] if len(present) == 1 else "MIXED" if present else "NONE")
        seen: set[str] = set()
        lines: list[str] = []
        for fact in self.facts:
            if fact.start >= end:
                break
            if fact.end <= start:
                continue
            line = _fact_label(fact)
            if line not in seen:
                seen.add(line)
                lines.append(line)
                if len(lines) >= fact_limit:
                    break
        trace = (self.trace_index.describe(start, end)
                 if include_trace and self.trace_index else ())
        return RangeEvidence(counts, state, tuple(lines), trace)

    def state_for_range(self, start: int, end: int) -> str:
        """Return the map outline state without scanning object/claim facts."""
        if start < 0 or end <= start or end > self.rom_size:
            raise ValueError("Evidence query range is outside the ROM")
        present = [name for name in STATUS_NAMES
                   if self.status_bytes[name].count(1, start, end)]
        return "CONFLICT" if "CONFLICT" in present else (
            present[0] if len(present) == 1 else "MIXED" if present else "NONE")


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _object_label(object_type: str, attributes_json: str) -> str:
    try:
        attributes: Any = json.loads(attributes_json)
    except (TypeError, json.JSONDecodeError):
        return object_type
    if isinstance(attributes, dict):
        for key in ("name", "label", "symbol", "title", "resource_name"):
            value = attributes.get(key)
            if isinstance(value, str) and value.strip():
                return f"{object_type}: {value.strip()}"
    return object_type


def _compact_value(value_json: str) -> str:
    try:
        value = json.loads(value_json)
    except (TypeError, json.JSONDecodeError):
        return value_json[:160]
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))[:160]


def _fact_label(fact: EvidenceFact) -> str:
    identity = f"{fact.label} [{fact.start:#08x}–{fact.end - 1:#08x}]"
    if fact.claim_type is None:
        return identity + " (no claim)"
    return f"{identity} · {fact.status or 'UNSPECIFIED'} · {fact.claim_type}: {fact.value or ''}"
