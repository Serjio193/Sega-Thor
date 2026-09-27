"""Monotonic preservation rules for canonical runtime evidence references."""

from __future__ import annotations

import json
from typing import Any


def _json(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def locator_preserved(old_locator: dict[str, Any], new_locator: dict[str, Any]) -> bool:
    """Allow locator enrichment only when every previous provenance fact survives."""
    for key, value in old_locator.items():
        if key in {"capture_ids", "windows", "session_source_sha256s"}:
            previous = value if isinstance(value, list) else []
            current = new_locator.get(key, [])
            if not isinstance(current, list) or not {
                    _json(item) for item in previous} <= {
                    _json(item) for item in current}:
                return False
        elif key == "session_source_sha256":
            if value and value not in new_locator.get("session_source_sha256s", []):
                return False
        elif new_locator.get(key) != value:
            return False
    return True


def evidence_ref_preserved(old: tuple, new: tuple) -> bool:
    if old[:6] != new[:6]:
        return False
    try:
        return locator_preserved(json.loads(old[6]), json.loads(new[6]))
    except (TypeError, json.JSONDecodeError):
        return False


def rows_preserved(table: str, before: set[tuple], after: set[tuple]) -> bool:
    if table != "evidence_ref":
        return before <= after
    old_by_id = {row[0]: row for row in before}
    new_by_id = {row[0]: row for row in after}
    return all(key in new_by_id and evidence_ref_preserved(row, new_by_id[key])
               for key, row in old_by_id.items())


def verify_database_superset(base: Any, final: Any, table: str,
                             columns: tuple[str, ...]) -> bool:
    """Verify staged preservation with indexed lookups, bounded by one old row."""
    key_columns = {
        "rom_range": ("range_id",), "rom_object": ("object_id",),
        "claim": ("claim_id",), "relation": ("relation_id",),
        "conflict": ("conflict_id",), "source_artifact": ("source_sha256",),
        "map_import": ("import_key",),
        "evidence_ref": ("ref_id",), "emission": ("start", "end"),
        "derivation": ("derivation_id",),
        "derivation_input": ("derivation_id", "ordinal"),
        "map_proposal": ("proposal_id",),
        "map_proposal_operation": ("proposal_id", "ordinal"),
    }
    keys = key_columns[table]
    select = f"SELECT {','.join(columns)} FROM {table} WHERE " + " AND ".join(
        f"{key}=?" for key in keys)
    positions = tuple(columns.index(key) for key in keys)
    for old in base.execute(f"SELECT {','.join(columns)} FROM {table}"):
        found = final.execute(select, tuple(old[index] for index in positions)).fetchone()
        if found is None:
            return False
        if table == "evidence_ref":
            if not evidence_ref_preserved(tuple(old), tuple(found)):
                return False
        elif tuple(old) != tuple(found):
            return False
    return True
