"""Stream exact native witnesses between existing MAP-1 session artifacts."""

from __future__ import annotations

import hashlib
import json
import sqlite3

try:
    from .cartographer import canonical
    from .rom_knowledge_map import runtime_occurrence_id
except ImportError:
    from cartographer import canonical
    from rom_knowledge_map import runtime_occurrence_id


TABLE = "live_forward_runtime_occurrence"


def present(db: sqlite3.Connection) -> bool:
    return db.execute("SELECT 1 FROM sqlite_master WHERE type='table' AND name=?",
                      (TABLE,)).fetchone() is not None


def occurrence_hash(db: sqlite3.Connection) -> str | None:
    """Separate evidence seal; the historical MAP-1 structural hash is stable."""
    if not present(db):
        return None
    digest = hashlib.sha256()
    for identity, encoded in db.execute(
            f"SELECT occurrence_id,event_json FROM {TABLE} ORDER BY occurrence_id"):
        digest.update(canonical([identity, json.loads(encoded)]).encode())
        digest.update(b"\n")
    return digest.hexdigest()


def merge_occurrences(source: sqlite3.Connection, target: sqlite3.Connection) -> dict:
    """Union witnesses by native identity, rejecting conflicting same-ID facts.

    No per-instruction graph objects are created. Legacy maps without the
    optional session table remain readable; absent evidence is not fabricated.
    Caller owns the enclosing temporary-file publication transaction.
    """
    seal = source.execute("SELECT value FROM map_meta WHERE "
                          "key='live_forward_occurrence_sha256'").fetchone()
    if seal and seal[0] != occurrence_hash(source):
        raise ValueError("STOP_ARCHIVIST_OCCURRENCE_HASH_MISMATCH")
    if not present(source):
        return {"available": False, "new_occurrences": 0, "source_occurrences": 0}
    target.execute(f"CREATE TABLE IF NOT EXISTS {TABLE}("
                   "occurrence_id TEXT PRIMARY KEY,event_json TEXT NOT NULL)")
    added = count = 0
    for identity, encoded in source.execute(
            f"SELECT occurrence_id,event_json FROM {TABLE} ORDER BY occurrence_id"):
        event = json.loads(encoded)
        expected = runtime_occurrence_id(capture_id=event["capture_id"],
            epoch=event["epoch"], cpu=event["cpu_id"],
            address_space=event["address_space"], native_sequence=event["native_sequence"],
            event_kind=event["event_kind"], run_id=event["run_id"],
            instruction_sequence=event["instruction_sequence"])
        if (identity != expected or event.get("occurrence_id") != identity or
                not event.get("windows") or not event.get("capture_ids")):
            raise ValueError("STOP_RUNTIME_OCCURRENCE_IDENTITY_INVALID")
        for key, table, column in (("instruction_node_id", "map_node", "node_id"),
                                   ("target_node_id", "map_node", "node_id"),
                                   ("edge_id", "map_edge", "edge_id")):
            if event.get(key) and target.execute(
                    f"SELECT 1 FROM {table} WHERE {column}=?", (event[key],)).fetchone() is None:
                raise ValueError("STOP_RUNTIME_OCCURRENCE_GRAPH_REFERENCE_MISSING")
        prior = target.execute(f"SELECT event_json FROM {TABLE} WHERE occurrence_id=?",
                               (identity,)).fetchone()
        if prior:
            old = json.loads(prior[0])
            core = lambda value: {k: v for k, v in value.items()
                                  if k not in {"windows", "capture_ids"}}
            if core(old) != core(event):
                raise ValueError("STOP_RUNTIME_OCCURRENCE_IDENTITY_CONFLICT")
            for key in ("windows", "capture_ids"):
                event[key] = sorted({canonical(v): v for v in old[key] + event[key]}.values(),
                                    key=canonical)
        else:
            added += 1
        target.execute(f"INSERT OR REPLACE INTO {TABLE} VALUES (?,?)",
                       (identity, canonical(event)))
        count += 1
    target.commit()
    return {"available": True, "new_occurrences": added, "source_occurrences": count,
            "occurrence_hash": occurrence_hash(target)}
