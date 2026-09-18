"""Compact SQLite model for canonical ROM ranges, claims, evidence and emission."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from pathlib import Path
from typing import Any, Iterable


SCHEMA = "oasis.m12.canonical-rom-knowledge.v1"
OBJECT_TYPES = {
    "ROM_RANGE", "M68K_INSTRUCTION", "M68K_FUNCTION", "ROM_DATA",
    "POINTER_TABLE", "TABLE_ENTRY", "GRAPHICS_STREAM", "AUDIO_DATA",
    "Z80_PROGRAM", "UNKNOWN",
}
STATUSES = {
    "OBSERVED_RUNTIME", "DERIVED_EXACT", "STATIC_VERIFIED",
    "MANUAL_VERIFIED", "HYPOTHESIS", "CONFLICT",
}


def canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def stable_id(prefix: str, value: Any) -> str:
    return f"{prefix}:{sha256_bytes(canonical(value).encode('utf-8'))}"


def range_id(rom_sha256: str, start: int, end: int) -> str:
    return stable_id("range", {"rom_sha256": rom_sha256, "start": start, "end": end})


def object_id(rom_sha256: str, start: int, end: int, object_type: str) -> str:
    if object_type not in OBJECT_TYPES or start < 0 or end <= start:
        raise ValueError("STOP_OBJECT_IDENTITY_INVALID")
    return stable_id("object", {"rom_sha256": rom_sha256, "start": start,
                                 "end": end, "object_type": object_type})


SCHEMA_SQL = """
CREATE TABLE IF NOT EXISTS map_meta(key TEXT PRIMARY KEY, value TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS rom_range(
  range_id TEXT PRIMARY KEY, rom_sha256 TEXT NOT NULL, start INTEGER NOT NULL,
  end INTEGER NOT NULL, CHECK(start >= 0 AND end > start));
CREATE TABLE IF NOT EXISTS rom_object(
  object_id TEXT PRIMARY KEY, range_id TEXT NOT NULL REFERENCES rom_range(range_id),
  object_type TEXT NOT NULL, attributes_json TEXT NOT NULL);
CREATE INDEX IF NOT EXISTS rom_object_type_idx ON rom_object(object_type);
CREATE TABLE IF NOT EXISTS claim(
  claim_id TEXT PRIMARY KEY, object_id TEXT NOT NULL REFERENCES rom_object(object_id),
  claim_type TEXT NOT NULL, value_json TEXT NOT NULL, status TEXT NOT NULL);
CREATE INDEX IF NOT EXISTS claim_type_status_idx ON claim(claim_type,status);
CREATE TABLE IF NOT EXISTS relation(
  relation_id TEXT PRIMARY KEY, relation_type TEXT NOT NULL,
  source_object_id TEXT NOT NULL REFERENCES rom_object(object_id),
  target_object_id TEXT REFERENCES rom_object(object_id), target_address INTEGER,
  status TEXT NOT NULL, attributes_json TEXT NOT NULL,
  CHECK(target_object_id IS NOT NULL OR target_address IS NOT NULL));
CREATE INDEX IF NOT EXISTS relation_type_idx ON relation(relation_type);
CREATE TABLE IF NOT EXISTS emission(
  start INTEGER NOT NULL, end INTEGER NOT NULL, emission_type TEXT NOT NULL,
  classification TEXT NOT NULL, source_kind TEXT NOT NULL, source_owned INTEGER NOT NULL,
  artifact_type TEXT NOT NULL, artifact TEXT NOT NULL,
  PRIMARY KEY(start,end), CHECK(start >= 0 AND end > start));
CREATE TABLE IF NOT EXISTS source_artifact(
  source_sha256 TEXT PRIMARY KEY, checkpoint TEXT NOT NULL,
  artifact_name TEXT NOT NULL, artifact_type TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS evidence_ref(
  ref_id TEXT PRIMARY KEY, subject_type TEXT NOT NULL, subject_id TEXT NOT NULL,
  source_sha256 TEXT NOT NULL REFERENCES source_artifact(source_sha256),
  fact_kind TEXT NOT NULL, fact_count INTEGER NOT NULL, locator_json TEXT NOT NULL,
  CHECK(fact_count >= 0));
CREATE INDEX IF NOT EXISTS evidence_subject_idx ON evidence_ref(subject_type,subject_id);
CREATE TABLE IF NOT EXISTS conflict(
  conflict_id TEXT PRIMARY KEY, start INTEGER NOT NULL, end INTEGER NOT NULL,
  conflict_type TEXT NOT NULL, detail_json TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS map_import(
  import_key TEXT PRIMARY KEY, input_hash TEXT NOT NULL);
"""


TABLE_COLUMNS = {
    "rom_range": ("range_id", "rom_sha256", "start", "end"),
    "rom_object": ("object_id", "range_id", "object_type", "attributes_json"),
    "claim": ("claim_id", "object_id", "claim_type", "value_json", "status"),
    "relation": ("relation_id", "relation_type", "source_object_id", "target_object_id",
                 "target_address", "status", "attributes_json"),
    "emission": ("start", "end", "emission_type", "classification", "source_kind",
                 "source_owned", "artifact_type", "artifact"),
    "source_artifact": ("source_sha256", "checkpoint", "artifact_name", "artifact_type"),
    "evidence_ref": ("ref_id", "subject_type", "subject_id", "source_sha256", "fact_kind",
                     "fact_count", "locator_json"),
    "conflict": ("conflict_id", "start", "end", "conflict_type", "detail_json"),
    "map_import": ("import_key", "input_hash"),
}


class KnowledgeStore:
    """Deterministic, idempotent writer and query surface for one ROM identity."""

    def __init__(self, path: str | Path, rom_sha256: str, rom_size: int):
        self.path = Path(path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.db = sqlite3.connect(self.path)
        self.db.row_factory = sqlite3.Row
        self.db.execute("PRAGMA foreign_keys=ON")
        self.db.executescript(SCHEMA_SQL)
        self._meta("schema", SCHEMA)
        self._meta("rom_sha256", rom_sha256)
        self._meta("rom_size", str(rom_size))
        self.db.commit()
        found = self.meta()
        if found.get("schema") != SCHEMA or found.get("rom_sha256") != rom_sha256 or \
                found.get("rom_size") != str(rom_size):
            raise ValueError("STOP_ROM_IDENTITY_MISMATCH")

    def _meta(self, key: str, value: str) -> None:
        old = self.db.execute("SELECT value FROM map_meta WHERE key=?", (key,)).fetchone()
        if old is not None and old[0] != value:
            raise ValueError(f"STOP_MAP_META_CONFLICT:{key}")
        self.db.execute("INSERT OR IGNORE INTO map_meta VALUES (?,?)", (key, value))

    def meta(self) -> dict[str, str]:
        return {r["key"]: r["value"] for r in self.db.execute("SELECT key,value FROM map_meta")}

    def insert_rows(self, table: str, rows: Iterable[dict[str, Any]]) -> None:
        columns = TABLE_COLUMNS[table]
        sql = f"INSERT OR IGNORE INTO {table}({','.join(columns)}) VALUES ({','.join('?' for _ in columns)})"
        select = f"SELECT {','.join(columns)} FROM {table} WHERE {columns[0]}=?"
        for row in rows:
            values = tuple(row[c] for c in columns)
            cursor = self.db.execute(sql, values)
            if cursor.rowcount == 0:
                existing = self.db.execute(select, (values[0],)).fetchone()
                if existing is None or tuple(existing) != values:
                    raise ValueError(f"STOP_OBJECT_IDENTITY_CONFLICT:{table}:{values[0]}")

    def counts(self) -> dict[str, int]:
        tables = (*TABLE_COLUMNS.keys(),)
        return {table: int(self.db.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0])
                for table in tables}

    def hashes(self) -> dict[str, str]:
        rows = lambda table: [tuple(r) for r in self.db.execute(
            f"SELECT {','.join(TABLE_COLUMNS[table])} FROM {table} ORDER BY {','.join(TABLE_COLUMNS[table])}")]
        structure = {t: rows(t) for t in ("rom_range", "rom_object", "claim", "relation", "conflict")}
        evidence = {t: rows(t) for t in ("source_artifact", "evidence_ref")}
        emission = rows("emission")
        structure_hash = sha256_bytes(canonical(structure).encode("utf-8"))
        evidence_hash = sha256_bytes(canonical(evidence).encode("utf-8"))
        emission_hash = sha256_bytes(canonical(emission).encode("utf-8"))
        return {"structure_hash": structure_hash, "evidence_index_hash": evidence_hash,
                "emission_hash": emission_hash,
                "map_hash": sha256_bytes((structure_hash + evidence_hash + emission_hash).encode())}

    @staticmethod
    def _union_bytes(intervals: list[tuple[int, int]]) -> int:
        total = 0
        cursor = -1
        for start, end in sorted(intervals):
            if end <= cursor:
                continue
            total += end - max(start, cursor)
            cursor = end
        return total

    def _object_ranges(self, object_type: str | None = None) -> list[tuple[int, int]]:
        sql = "SELECT r.start,r.end FROM rom_object o JOIN rom_range r USING(range_id)"
        args: tuple[Any, ...] = ()
        if object_type:
            sql += " WHERE o.object_type=?"
            args = (object_type,)
        return [(int(r[0]), int(r[1])) for r in self.db.execute(sql, args)]

    def coverage(self) -> dict[str, Any]:
        emission = {r[0]: int(r[1]) for r in self.db.execute(
            "SELECT emission_type,SUM(end-start) FROM emission GROUP BY emission_type ORDER BY emission_type")}
        classes = {r[0]: int(r[1]) for r in self.db.execute(
            "SELECT classification,SUM(end-start) FROM emission GROUP BY classification ORDER BY classification")}
        source_kinds = {r[0]: int(r[1]) for r in self.db.execute(
            "SELECT source_kind,SUM(end-start) FROM emission GROUP BY source_kind ORDER BY source_kind")}
        owned = int(self.db.execute("SELECT SUM(end-start) FROM emission WHERE source_owned=1").fetchone()[0] or 0)
        by_status: dict[str, list[tuple[int, int]]] = {}
        for r in self.db.execute("""SELECT c.status,rr.start,rr.end FROM claim c
          JOIN rom_object o USING(object_id) JOIN rom_range rr USING(range_id)"""):
            by_status.setdefault(str(r[0]), []).append((int(r[1]), int(r[2])))
        statuses = {key: self._union_bytes(value) for key, value in sorted(by_status.items())}
        return {"bytes_by_emission": emission, "bytes_by_classification": classes,
                "bytes_by_source_kind": source_kinds,
                "source_owned_bytes": owned, "bytes_with_claim_status": statuses}

    def metrics(self) -> dict[str, Any]:
        counts = lambda sql, args=(): int(self.db.execute(sql, args).fetchone()[0])
        objects_by_type = {str(r[0]): int(r[1]) for r in self.db.execute(
            "SELECT object_type,COUNT(*) FROM rom_object GROUP BY object_type ORDER BY object_type")}
        claim_status = {str(r[0]): int(r[1]) for r in self.db.execute(
            "SELECT status,COUNT(*) FROM claim GROUP BY status ORDER BY status")}
        relation_types = {str(r[0]): int(r[1]) for r in self.db.execute(
            "SELECT relation_type,COUNT(*) FROM relation GROUP BY relation_type ORDER BY relation_type")}
        cov = self.coverage()
        lengths = [int(r[1]) - int(r[0]) for r in self.db.execute(
            "SELECT rr.start,rr.end FROM rom_object o JOIN rom_range rr USING(range_id) "
            "WHERE o.object_type='M68K_INSTRUCTION'")]
        unobserved_code = self.query("owned-code-unseen")
        runtime_occurrences = counts(
            "SELECT COALESCE(SUM(fact_count),0) FROM evidence_ref WHERE fact_kind='RUNTIME_INSTRUCTION_OCCURRENCE'")
        return {
            "rom_bytes_total": int(self.meta()["rom_size"]),
            "emission_ranges": counts("SELECT COUNT(*) FROM emission"),
            "semantic_objects": counts("SELECT COUNT(*) FROM rom_object"),
            "objects_by_type": objects_by_type,
            "unknown_bytes": sum(cov["bytes_by_source_kind"].get(kind, 0)
                                  for kind in ("UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE")),
            "source_owned_bytes": cov["source_owned_bytes"],
            "source_owned_percent": f"{100 * cov['source_owned_bytes'] / int(self.meta()['rom_size']):.10f}",
            "executed_instruction_objects": counts("SELECT COUNT(*) FROM claim WHERE claim_type='EXECUTED_FROM_ROM'"),
            "executed_unique_rom_bytes": self._union_bytes([
                (int(r[0]), int(r[1])) for r in self.db.execute(
                    "SELECT rr.start,rr.end FROM rom_object o JOIN rom_range rr USING(range_id) "
                    "JOIN claim c USING(object_id) WHERE o.object_type='M68K_INSTRUCTION' "
                    "AND c.claim_type='EXECUTED_FROM_ROM'")]),
            "runtime_occurrences_referenced": runtime_occurrences,
            "typed_data_objects": sum(objects_by_type.get(t, 0) for t in ("ROM_DATA", "POINTER_TABLE", "TABLE_ENTRY")),
            "graphics_objects": objects_by_type.get("GRAPHICS_STREAM", 0),
            "audio_objects": objects_by_type.get("AUDIO_DATA", 0),
            "z80_objects": objects_by_type.get("Z80_PROGRAM", 0),
            "relations_by_type": relation_types,
            "claims_by_status": claim_status,
            "conflicts": counts("SELECT COUNT(*) FROM conflict"),
            "evidence_refs": counts("SELECT COUNT(*) FROM evidence_ref"),
            "emission_bytes_by_type": cov["bytes_by_emission"],
            "classification_bytes": cov["bytes_by_classification"],
            "source_kind_bytes": cov["bytes_by_source_kind"],
            "status_coverage_bytes": cov["bytes_with_claim_status"],
            "incbin_bytes": cov["bytes_by_emission"].get("INCBIN", 0),
            "asm_bytes": cov["bytes_by_emission"].get("ASM", 0),
            "data_bytes": cov["bytes_by_emission"].get("DATA", 0),
            "asset_bytes": cov["bytes_by_emission"].get("ASSET", 0),
            "hypothesis_claims": counts("SELECT COUNT(*) FROM claim WHERE status='HYPOTHESIS'"),
            "source_owned_code_ranges": counts("""SELECT COUNT(DISTINCT o.object_id) FROM rom_object o
                JOIN claim c USING(object_id) WHERE c.claim_type='SOURCE_CLASS'
                AND json_extract(c.value_json,'$.source_kind')='CODE_VERIFIED'
                AND EXISTS (SELECT 1 FROM claim oc WHERE oc.object_id=o.object_id
                  AND oc.claim_type='SOURCE_OWNED' AND json_extract(oc.value_json,'$')=1)"""),
            "source_owned_code_ranges_unobserved": len(unobserved_code),
            "source_owned_code_bytes_unobserved": sum(int(r["end"]) - int(r["start"])
                                                       for r in unobserved_code),
            "executed_not_fully_owned_objects": counts("""SELECT COUNT(*) FROM rom_object o
                JOIN claim c USING(object_id) JOIN rom_range r USING(range_id)
                WHERE o.object_type='M68K_INSTRUCTION' AND c.claim_type='EXECUTED_FROM_ROM'
                AND json_extract(o.attributes_json,'$.source_owned_bytes') < r.end-r.start"""),
            "runtime_instruction_range_bytes": sum(lengths),
        }

    def query(self, name: str, start: int | None = None, end: int | None = None,
              address: int | None = None, subject_id: str | None = None) -> list[dict[str, Any]]:
        if name == "unknown":
            rows = self.db.execute("""SELECT * FROM emission
                WHERE source_kind IN ('UNKNOWN','UNKNOWN_DATA','UNKNOWN_WITH_EVIDENCE') ORDER BY start""")
        elif name == "executed":
            rows = self.db.execute("""SELECT o.object_id,o.object_type,r.start,r.end,o.attributes_json
                FROM rom_object o JOIN rom_range r USING(range_id) JOIN claim c USING(object_id)
                WHERE c.claim_type='EXECUTED_FROM_ROM' ORDER BY r.start,r.end""")
        elif name == "executed-not-owned":
            rows = self.db.execute("""SELECT o.object_id,r.start,r.end,o.attributes_json
                FROM rom_object o JOIN rom_range r USING(range_id) JOIN claim c USING(object_id)
                WHERE o.object_type='M68K_INSTRUCTION' AND c.claim_type='EXECUTED_FROM_ROM'
                AND json_extract(o.attributes_json,'$.source_owned_bytes') < r.end-r.start
                ORDER BY r.start,r.end""")
        elif name == "owned-code-unseen":
            rows = self.db.execute("""SELECT DISTINCT o.object_id,r.start,r.end,c.value_json
                FROM rom_object o JOIN rom_range r USING(range_id) JOIN claim c USING(object_id)
                WHERE c.claim_type='SOURCE_CLASS' AND json_extract(c.value_json,'$.source_kind')='CODE_VERIFIED'
                AND EXISTS (SELECT 1 FROM claim oc WHERE oc.object_id=o.object_id
                  AND oc.claim_type='SOURCE_OWNED' AND json_extract(oc.value_json,'$')=1)
                AND NOT EXISTS (SELECT 1 FROM rom_object x JOIN rom_range xr USING(range_id)
                  JOIN claim ec USING(object_id) WHERE x.object_type='M68K_INSTRUCTION'
                  AND ec.claim_type='EXECUTED_FROM_ROM' AND xr.start<r.end AND r.start<xr.end)
                ORDER BY r.start,r.end""")
        elif name == "conflicts":
            rows = self.db.execute("SELECT * FROM conflict ORDER BY start,end,conflict_id")
        elif name == "hypotheses":
            rows = self.db.execute("""SELECT c.claim_id,c.object_id,c.value_json,r.start,r.end
                FROM claim c JOIN rom_object o USING(object_id) JOIN rom_range r USING(range_id)
                WHERE c.status='HYPOTHESIS' ORDER BY r.start,r.end,c.claim_id""")
        elif name == "emission-at" and address is not None:
            rows = self.db.execute("SELECT * FROM emission WHERE start<=? AND ?<end", (address, address))
        elif name == "objects-in-range" and start is not None and end is not None:
            rows = self.db.execute("""SELECT o.object_id,o.object_type,r.start,r.end,o.attributes_json
                FROM rom_object o JOIN rom_range r USING(range_id)
                WHERE r.start<? AND ?<r.end ORDER BY r.start,r.end,o.object_type,o.object_id""", (end, start))
        elif name == "evidence" and subject_id:
            rows = self.db.execute("SELECT * FROM evidence_ref WHERE subject_id=? ORDER BY ref_id", (subject_id,))
        elif name == "coverage":
            return [self.coverage()]
        else:
            raise ValueError("STOP_QUERY_INVALID")
        return [dict(row) for row in rows]

    def export(self, reconciliation: dict[str, Any], audit: dict[str, Any],
               idempotence: dict[str, Any]) -> dict[str, Any]:
        get = lambda table: [dict(r) for r in self.db.execute(
            f"SELECT * FROM {table} ORDER BY {','.join(TABLE_COLUMNS[table])}")]
        objects = get("rom_object")
        claims = get("claim")
        relations = get("relation")
        evidence_refs = get("evidence_ref")
        conflicts = get("conflict")
        for row in objects:
            row["attributes"] = json.loads(row.pop("attributes_json"))
        for row in claims:
            row["value"] = json.loads(row.pop("value_json"))
        for row in relations:
            row["attributes"] = json.loads(row.pop("attributes_json"))
        for row in evidence_refs:
            row["locator"] = json.loads(row.pop("locator_json"))
        for row in conflicts:
            row["detail"] = json.loads(row.pop("detail_json"))
        return {
            "schema": SCHEMA,
            "rom": {"sha256": self.meta()["rom_sha256"], "bytes": int(self.meta()["rom_size"])},
            "hashes": self.hashes(), "metrics": self.metrics(), "idempotence": idempotence,
            "ownership_reconciliation": reconciliation,
            "independent_audit": audit,
            "emission": get("emission"), "ranges": get("rom_range"),
            "objects": objects, "claims": claims,
            "relations": relations, "evidence_artifacts": get("source_artifact"),
            "evidence_refs": evidence_refs, "conflicts": conflicts,
        }

    def close(self) -> None:
        self.db.commit()
        self.db.close()
