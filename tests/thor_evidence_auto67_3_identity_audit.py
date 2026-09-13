"""Read-only AUTO67.3 canonical chain identity audit for a real proof DB."""

from __future__ import annotations

import copy
import hashlib
import json
import os
import sqlite3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_DB = ROOT / "build/thor-evidence/auto67-3/persistent-chain-proof-v3.sqlite"
import sys

sys.path.insert(0, str(ROOT / "src/tools"))
from thor_evidence.auto67_persistence import canonical_chain, chain_descriptor


CAUSAL_AUDIT_FIELDS = {
    "pc": "instruction PC",
    "caller_pc": "caller/predecessor PC",
    "predecessor": "predecessor",
    "reader_pc": "reader PC",
    "writer_pc": "writer PC",
    "address": "observed address (address space not inferred)",
    "source_address": "source address",
    "destination_address": "destination address",
    "pointer_target": "pointer target",
    "rom_target": "ROM address/target",
    "ram_target": "RAM address/target",
    "consumer": "consumer",
    "producer": "producer",
    "selector": "selector",
    "index": "selector/index",
    "terminal_root": "terminal/root state",
    "unresolved_frontier": "unresolved frontier",
}


def _change(value):
    if isinstance(value, str):
        if value.lower().startswith("0x"):
            width = max(1, len(value) - 2)
            return f"0x{int(value, 16) + 1:0{width}X}"
        return value + "-identity-audit"
    if isinstance(value, bool):
        return not value
    if isinstance(value, int):
        return value + 1
    if isinstance(value, list):
        return value + [{"identity_audit_delta": True}]
    if isinstance(value, dict):
        changed = copy.deepcopy(value)
        changed["identity_audit_delta"] = True
        return changed
    return {"identity_audit_delta": True}


def _hash(canonical):
    return hashlib.sha256(canonical.encode("utf-8")).hexdigest()


def run(database: Path) -> dict:
    connection = sqlite3.connect(database)
    rows = connection.execute(
        "SELECT chain_hash, canonical_payload, times_observed, last_status "
        "FROM live_chain ORDER BY chain_hash"
    ).fetchall()
    if not rows:
        raise AssertionError("live_chain is empty")

    positive_base = {
        "kind": "RAM_WRITE",
        "pc": "0x00100",
        "address": "0x00200",
        "chain_steps": [{"pc": "0x00100", "address": "0x00200"}],
        "frame": 10,
        "epoch": 1,
        "worker_id": 1,
        "lease_id": "L1",
        "session_id": "S1",
        "wall_clock": "2026-09-14T10:00:00Z",
    }
    positive_variant = dict(positive_base)
    positive_variant.update({
        "frame": 900,
        "epoch": 9,
        "worker_id": 15,
        "lease_id": "L999",
        "session_id": "S999",
        "wall_clock": "2099-01-01T00:00:00Z",
    })
    first = chain_descriptor(positive_base, "BOUNDED_UNRESOLVED", 10, 1, "L1", "I1")
    replay = chain_descriptor(positive_variant, "PROVEN", 900, 15, "L999", "I999")
    positive = {
        "canonical_equal": first["canonical_payload"] == replay["canonical_payload"],
        "hash_equal": first["chain_hash"] == replay["chain_hash"],
        "provenance_differs": first["provenance"] != replay["provenance"],
        "hash": first["chain_hash"],
    }
    assert positive["canonical_equal"] and positive["hash_equal"]

    representative = max(rows, key=lambda row: len(json.loads(row[1]).get("observed", {})))
    stored = json.loads(representative[1])
    base_event = dict(stored.get("observed", {}))
    for name in ("chain_steps", "causal_facts", "unresolved_frontier", "terminal_root"):
        if name in stored:
            base_event[name] = stored[name]
    negative = []
    for field, label in CAUSAL_AUDIT_FIELDS.items():
        if field not in base_event:
            continue
        changed = dict(base_event)
        changed[field] = _change(changed[field])
        original = canonical_chain(base_event)
        altered = canonical_chain(changed)
        item = {
            "field": field,
            "label": label,
            "canonical_differs": original != altered,
            "hash_differs": _hash(original) != _hash(altered),
        }
        assert item["canonical_differs"] and item["hash_differs"]
        negative.append(item)

    sizes = [len(row[1].encode("utf-8")) for row in rows]
    duplicate_groups = connection.execute(
        "SELECT COUNT(*) FROM (SELECT chain_hash FROM live_chain "
        "GROUP BY chain_hash HAVING COUNT(*) > 1)"
    ).fetchone()[0]
    table_sql = connection.execute(
        "SELECT sql FROM sqlite_master WHERE type='table' AND name='live_chain'"
    ).fetchone()[0]
    pk_hash = connection.execute(
        "SELECT COUNT(*) FROM pragma_table_info('live_chain') "
        "WHERE name='chain_hash' AND pk=1"
    ).fetchone()[0]
    assert duplicate_groups == 0 and pk_hash == 1
    connection.close()

    sizes.sort()
    return {
        "database": str(database.resolve()),
        "positive_identity": positive,
        "negative_identity": {
            "representative_hash": representative[0],
            "stored_observed_fields": sorted(base_event),
            "applicable_fields": negative,
            "skipped_unavailable_fields": [
                label for field, label in CAUSAL_AUDIT_FIELDS.items() if field not in base_event
            ],
        },
        "database_inspection": {
            "live_chain_records": len(rows),
            "unique_hashes": len({row[0] for row in rows}),
            "duplicate_hash_groups": duplicate_groups,
            "body_bytes_min": sizes[0],
            "body_bytes_median": sizes[len(sizes) // 2],
            "body_bytes_max": sizes[-1],
            "unresolved": sum(row[3] != "PROVEN" for row in rows),
            "rooted": sum(row[3] == "PROVEN" for row in rows),
            "top_repeated_observations": [
                {"chain_hash": row[0], "times_observed": row[2]}
                for row in sorted(rows, key=lambda row: (-row[2], row[0]))[:10]
            ],
            "primary_key_on_chain_hash": bool(pk_hash),
            "table_sql": table_sql,
        },
    }


if __name__ == "__main__":
    path = Path(os.environ.get("AUTO67_CHAIN_DB", DEFAULT_DB))
    print(json.dumps(run(path), ensure_ascii=False, indent=2))
