"""Localized conflicts and fail-closed dry-run proposal filtering."""

from __future__ import annotations

from typing import Any

try:
    from .rom_knowledge_map import KnowledgeStore, canonical, stable_id
except ImportError:
    from rom_knowledge_map import KnowledgeStore, canonical, stable_id


def record_conflict(store: KnowledgeStore, start: int, end: int,
                    conflict_type: str, assertions: list[dict[str, Any]]) -> str:
    if start < 0 or end <= start or not conflict_type or len(assertions) < 2:
        raise ValueError("STOP_FUSION_CONFLICT_INPUT_INVALID")
    detail = {"assertions": sorted(assertions, key=canonical),
        "resolution": "EXACT_PROPOSAL_SUPPRESSED_FOR_OVERLAPPING_RANGE"}
    conflict_id = stable_id("fusion-conflict", {"start": start, "end": end,
        "conflict_type": conflict_type, "detail": detail})
    store.insert_rows("conflict", [{"conflict_id": conflict_id, "start": start,
        "end": end, "conflict_type": conflict_type,
        "detail_json": canonical(detail)}])
    return conflict_id


def eligible_exact_operations(store: KnowledgeStore,
                              operations: list[dict[str, Any]]) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    eligible, suppressed = [], []
    conflicts = [(int(row[0]), int(row[1])) for row in store.db.execute(
        "SELECT start,end FROM conflict ORDER BY start,end")]
    for operation in operations:
        if operation.get("truth") not in {"DERIVED_EXACT", "STATIC_VERIFIED"}:
            suppressed.append({"operation": operation, "reason": "NON_EXACT_TRUTH"})
            continue
        bounds = operation.get("range")
        if bounds is None:
            eligible.append(operation)
            continue
        start, end = int(bounds[0]), int(bounds[1])
        if any(start < conflict_end and conflict_start < end
               for conflict_start, conflict_end in conflicts):
            suppressed.append({"operation": operation, "reason": "OVERLAPS_CONFLICT"})
        else:
            eligible.append(operation)
    return eligible, suppressed
