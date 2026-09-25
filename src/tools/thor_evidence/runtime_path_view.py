"""Streaming derived paths over retained MAP-1 or canonical occurrence evidence.

This is a read-only window view, never a second authority or a closed CFG.
Overlapping windows are separate capture witnesses, not extra native events.
"""

from __future__ import annotations

import json
import sqlite3
from itertools import groupby
from typing import Iterator

try:
    from .cartographer import digest
    from .runtime_occurrence_merge import present
except ImportError:
    from cartographer import digest
    from runtime_occurrence_merge import present


def iter_runtime_paths(db: sqlite3.Connection, prefix: tuple[int, ...] = (),
                       max_window_events: int = 65536) -> Iterator[dict]:
    """Yield complete retained window paths whose initial PCs match prefix.

    Memory is bounded by one capture window. SQLite performs the ordered join;
    callers can paginate/limit the iterator without materializing every path.
    Missing terminal execution is explicit; next_pc is only an address fact.
    """
    if max_window_events < 1:
        raise ValueError("max_window_events must be positive")
    if present(db):
        source = "SELECT event_json AS event FROM live_forward_runtime_occurrence"
    elif db.execute("SELECT 1 FROM sqlite_master WHERE name='evidence_ref'").fetchone():
        source = ("SELECT locator_json AS event FROM evidence_ref "
                  "WHERE subject_type='RUNTIME_OCCURRENCE' "
                  "AND fact_kind LIKE 'RUNTIME_OCCURRENCE:%'")
    else:
        raise ValueError("STOP_RUNTIME_PATH_EXACT_LINEAGE_UNAVAILABLE")
    sql = f"""WITH events AS ({source})
        SELECT json_extract(event,'$.run_id'),json_extract(event,'$.epoch'),
               json_extract(event,'$.cpu_id'),w.value,event
        FROM events,json_each(event,'$.windows') AS w
        WHERE json_extract(event,'$.event_kind') IN
              ('INSTRUCTION','EXCEPTION_EVENT','CPU_STOP_EVENT')
        ORDER BY 1,2,3,4,json_extract(event,'$.native_sequence')"""
    for key, rows in groupby(db.execute(sql), key=lambda row: tuple(row[:4])):
        events = []
        for row in rows:
            if len(events) >= max_window_events:
                raise ValueError("STOP_RUNTIME_PATH_WINDOW_LIMIT")
            events.append(json.loads(row[4]))
        pcs = tuple(event["pc"] for event in events)
        if pcs[:len(prefix)] != prefix:
            continue
        structure = [{k: event.get(k) for k in
            ("event_kind", "cpu_id", "address_space", "pc", "value", "address", "flags")}
            for event in events]
        yield {"structural_path_id": digest(structure), "pcs": list(pcs),
            "structure": structure, "run_id": key[0], "epoch": key[1],
            "cpu": key[2], "window": json.loads(key[3]),
            "occurrence_ids": [event["occurrence_id"] for event in events],
            "native_sequences": [event["native_sequence"] for event in events],
            "terminal_next_pc": events[-1]["address"],
            "terminal_target_execution_proven": False,
            "coverage": "RETAINED_CAPTURE_WINDOW_ONLY"}
