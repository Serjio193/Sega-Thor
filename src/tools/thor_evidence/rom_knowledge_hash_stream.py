"""Canonical JSON hashes over ordered SQLite rows without materializing tables."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from typing import Mapping, Sequence


def canonical_hash(value: object) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"),
                         ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def grouped_rows_hash(db: sqlite3.Connection, tables: Sequence[str],
                      columns: Mapping[str, Sequence[str]]) -> str:
    """Match canonical({table: [ordered tuples]}) using bounded row memory."""
    digest = hashlib.sha256()
    digest.update(b"{")
    first_table = True
    for table in sorted(tables):
        if not first_table:
            digest.update(b",")
        first_table = False
        digest.update(json.dumps(table, ensure_ascii=True).encode("utf-8"))
        digest.update(b":[")
        fields = tuple(columns[table])
        order = ",".join(fields)
        query = f"SELECT {order} FROM {table} ORDER BY {order}"
        first_row = True
        for row in db.execute(query):
            if not first_row:
                digest.update(b",")
            first_row = False
            digest.update(json.dumps(tuple(row), sort_keys=True, separators=(",", ":"),
                                     ensure_ascii=True).encode("utf-8"))
        digest.update(b"]")
    digest.update(b"}")
    return digest.hexdigest()


def ordered_rows_hash(db: sqlite3.Connection, table: str,
                      columns: Mapping[str, Sequence[str]]) -> str:
    """Match canonical([ordered tuples]) with bounded row memory."""
    digest = hashlib.sha256()
    digest.update(b"[")
    fields = tuple(columns[table])
    order = ",".join(fields)
    first_row = True
    for row in db.execute(f"SELECT {order} FROM {table} ORDER BY {order}"):
        if not first_row:
            digest.update(b",")
        first_row = False
        digest.update(json.dumps(tuple(row), sort_keys=True, separators=(",", ":"),
                                 ensure_ascii=True).encode("utf-8"))
    digest.update(b"]")
    return digest.hexdigest()
