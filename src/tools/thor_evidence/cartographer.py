"""Persistent, deterministic provenance graph for the M12 MAP-1 checkpoint."""

from __future__ import annotations

import hashlib
import json
import sqlite3
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


STATUSES = {"OBSERVED", "PROVEN", "UNRESOLVED", "CONFLICT"}
_RANK = {"UNRESOLVED": 0, "OBSERVED": 1, "PROVEN": 2, "CONFLICT": 3}


def canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def digest(value: Any) -> str:
    return hashlib.sha256(canonical(value).encode("utf-8")).hexdigest()


@dataclass
class MapDelta:
    new_nodes: int = 0
    new_edges: int = 0
    promoted_nodes: int = 0
    promoted_edges: int = 0
    new_frontiers: int = 0
    resolved_frontiers: int = 0
    new_conflicts: int = 0
    component_joins: int = 0
    import_ref: str = ""
    graph_hash: str = ""

    def as_dict(self) -> dict[str, Any]:
        return self.__dict__.copy()


class Cartographer:
    """Small SQLite graph with content-addressed identities and idempotent merge."""

    def __init__(self, path: str | Path, rom_sha256: str, source_owned_bytes: int = 0):
        self.path = Path(path)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.db = sqlite3.connect(self.path)
        self.db.row_factory = sqlite3.Row
        self.db.executescript(
            """CREATE TABLE IF NOT EXISTS map_meta(key TEXT PRIMARY KEY, value TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS map_node(
              node_id TEXT PRIMARY KEY, kind TEXT NOT NULL, node_key TEXT NOT NULL,
              scope TEXT NOT NULL, status TEXT NOT NULL, body TEXT NOT NULL,
              lineage TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS map_edge(
              edge_id TEXT PRIMARY KEY, source_id TEXT NOT NULL, target_id TEXT NOT NULL,
              relation TEXT NOT NULL, scope TEXT NOT NULL, status TEXT NOT NULL,
              body TEXT NOT NULL, lineage TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS map_frontier(
              frontier_id TEXT PRIMARY KEY, anchor TEXT NOT NULL, role TEXT NOT NULL,
              reason TEXT NOT NULL, target TEXT NOT NULL, status TEXT NOT NULL,
              body TEXT NOT NULL, lineage TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS map_conflict(
              conflict_id TEXT PRIMARY KEY, object_type TEXT NOT NULL, object_id TEXT NOT NULL,
              claims TEXT NOT NULL, lineage TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS map_import(
              import_ref TEXT PRIMARY KEY, source_sha256 TEXT NOT NULL, body TEXT NOT NULL);
            """
        )
        self._put_meta("schema", "m12.map1.v1")
        self._put_meta("rom_sha256", rom_sha256)
        self._put_meta("source_owned_bytes", str(source_owned_bytes))
        self.db.commit()

    def _put_meta(self, key: str, value: str) -> None:
        self.db.execute("INSERT OR IGNORE INTO map_meta VALUES (?, ?)", (key, value))

    @staticmethod
    def _status(old: str, new: str) -> str:
        if old == new:
            return old
        if "CONFLICT" in (old, new):
            return "CONFLICT"
        return new if _RANK[new] > _RANK[old] else old

    @staticmethod
    def _union(old: str, values: list[Any]) -> str:
        merged = json.loads(old) if old else []
        for value in values:
            if value not in merged:
                merged.append(value)
        return canonical(sorted(merged, key=canonical))

    def _node_id(self, node: dict[str, Any]) -> str:
        return digest({"kind": node["kind"], "key": node["key"], "scope": node.get("scope", "global")})

    def _edge_id(self, edge: dict[str, Any]) -> str:
        return digest({"source": edge["source"], "target": edge["target"],
                       "relation": edge["relation"], "scope": edge.get("scope", "global")})

    def _frontier_id(self, item: dict[str, Any]) -> str:
        return digest({"anchor": item["anchor"], "role": item["role"],
                       "reason": item["reason"], "target": item.get("target", "")})

    def _record_conflict(self, object_type: str, object_id: str, claims: list[Any], lineage: list[Any]) -> bool:
        conflict_id = digest({"type": object_type, "id": object_id, "claims": claims})
        before = self.db.execute("SELECT 1 FROM map_conflict WHERE conflict_id=?", (conflict_id,)).fetchone()
        self.db.execute(
            "INSERT OR IGNORE INTO map_conflict VALUES (?, ?, ?, ?, ?)",
            (conflict_id, object_type, object_id, canonical(claims), canonical(sorted(lineage, key=canonical))),
        )
        return before is None

    @staticmethod
    def _compatible(old_body: dict[str, Any], new_body: dict[str, Any]) -> bool:
        if old_body.get("kind") != new_body.get("kind") or old_body.get("key") != new_body.get("key"):
            return False
        old_attrs, new_attrs = old_body.get("attributes", {}), new_body.get("attributes", {})
        return all(key not in new_attrs or new_attrs[key] == value for key, value in old_attrs.items()) or all(
            key not in old_attrs or old_attrs[key] == value for key, value in new_attrs.items())

    def _merge_nodes(self, bundle: dict[str, Any], delta: MapDelta) -> None:
        for item in bundle.get("nodes", []):
            if item["status"] not in STATUSES:
                raise ValueError("invalid node status")
            node_id = self._node_id(item)
            body = {"kind": item["kind"], "key": item["key"], "scope": item.get("scope", "global"),
                    "attributes": item.get("attributes", {})}
            lineages = item.get("lineage", [])
            row = self.db.execute("SELECT * FROM map_node WHERE node_id=?", (node_id,)).fetchone()
            if row is None:
                self.db.execute("INSERT INTO map_node VALUES (?, ?, ?, ?, ?, ?, ?)",
                                (node_id, item["kind"], item["key"], body["scope"], item["status"],
                                 canonical(body), canonical(sorted(lineages, key=canonical))))
                delta.new_nodes += 1
                continue
            old_body = json.loads(row["body"])
            if old_body != body and not self._compatible(old_body, body):
                if self._record_conflict("node", node_id, [old_body, body], lineages):
                    delta.new_conflicts += 1
                self.db.execute("UPDATE map_node SET status='CONFLICT' WHERE node_id=?", (node_id,))
            elif old_body != body:
                attrs = dict(old_body.get("attributes", {}))
                attrs.update(body.get("attributes", {}))
                old_body["attributes"] = attrs
                self.db.execute("UPDATE map_node SET body=? WHERE node_id=?", (canonical(old_body), node_id))
            new_status = self._status(row["status"], item["status"])
            if new_status != row["status"] and new_status == "PROVEN":
                delta.promoted_nodes += 1
            self.db.execute("UPDATE map_node SET status=?, lineage=? WHERE node_id=?",
                            (new_status, self._union(row["lineage"], lineages), node_id))

    def _merge_edges(self, bundle: dict[str, Any], delta: MapDelta) -> None:
        for item in bundle.get("edges", []):
            if item["status"] not in STATUSES:
                raise ValueError("invalid edge status")
            edge_id = self._edge_id(item)
            body = {"source": item["source"], "target": item["target"], "relation": item["relation"],
                    "scope": item.get("scope", "global"), "rule": item.get("rule", ""),
                    "assumptions": item.get("assumptions", [])}
            lineages = item.get("lineage", [])
            row = self.db.execute("SELECT * FROM map_edge WHERE edge_id=?", (edge_id,)).fetchone()
            if row is None:
                self.db.execute("INSERT INTO map_edge VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                                (edge_id, item["source"], item["target"], item["relation"], body["scope"],
                                 item["status"], canonical(body), canonical(sorted(lineages, key=canonical))))
                delta.new_edges += 1
                continue
            old_body = json.loads(row["body"])
            if old_body != body:
                if self._record_conflict("edge", edge_id, [old_body, body], lineages):
                    delta.new_conflicts += 1
                self.db.execute("UPDATE map_edge SET status='CONFLICT' WHERE edge_id=?", (edge_id,))
            new_status = self._status(row["status"], item["status"])
            if new_status != row["status"] and new_status == "PROVEN":
                delta.promoted_edges += 1
            self.db.execute("UPDATE map_edge SET status=?, lineage=? WHERE edge_id=?",
                            (new_status, self._union(row["lineage"], lineages), edge_id))

    def _merge_frontiers(self, bundle: dict[str, Any], delta: MapDelta) -> None:
        for item in bundle.get("frontiers", []):
            fid = self._frontier_id(item)
            body = {"anchor": item["anchor"], "role": item["role"], "reason": item["reason"],
                    "target": item.get("target", ""), "evidence": item.get("evidence", [])}
            row = self.db.execute("SELECT * FROM map_frontier WHERE frontier_id=?", (fid,)).fetchone()
            if row is None:
                self.db.execute("INSERT INTO map_frontier VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                                (fid, item["anchor"], item["role"], item["reason"], item.get("target", ""),
                                 item.get("status", "OPEN"), canonical(body),
                                 canonical(sorted(item.get("lineage", []), key=canonical))))
                delta.new_frontiers += 1
            else:
                self.db.execute("UPDATE map_frontier SET lineage=? WHERE frontier_id=?",
                                (self._union(row["lineage"], item.get("lineage", [])), fid))
        for item in bundle.get("resolves_frontiers", []):
            fid = self._frontier_id(item)
            row = self.db.execute("SELECT status FROM map_frontier WHERE frontier_id=?", (fid,)).fetchone()
            if row and row["status"] != "RESOLVED":
                self.db.execute("UPDATE map_frontier SET status='RESOLVED' WHERE frontier_id=?", (fid,))
                delta.resolved_frontiers += 1

    def _components(self) -> int:
        rows = self.db.execute("SELECT source_id,target_id FROM map_edge WHERE status='PROVEN'").fetchall()
        parents: dict[str, str] = {}
        def find(item: str) -> str:
            parents.setdefault(item, item)
            while parents[item] != item:
                parents[item] = parents[parents[item]]
                item = parents[item]
            return item
        def union(left: str, right: str) -> None:
            a, b = find(left), find(right)
            if a != b: parents[b] = a
        for row in rows: union(row[0], row[1])
        return len({find(key) for key in parents})

    def graph_hash(self) -> str:
        nodes = [dict(row) for row in self.db.execute("SELECT * FROM map_node ORDER BY node_id")]
        edges = [dict(row) for row in self.db.execute("SELECT * FROM map_edge ORDER BY edge_id")]
        frontiers = [dict(row) for row in self.db.execute("SELECT * FROM map_frontier ORDER BY frontier_id")]
        conflicts = [dict(row) for row in self.db.execute("SELECT * FROM map_conflict ORDER BY conflict_id")]
        return digest({"nodes": nodes, "edges": edges, "frontiers": frontiers, "conflicts": conflicts})

    def metrics(self) -> dict[str, int | str]:
        count = lambda table, where="": self.db.execute(f"SELECT COUNT(*) FROM {table} {where}").fetchone()[0]
        proven_ranges = self.db.execute("SELECT node_key FROM map_node WHERE kind='ROM_RANGE' AND status='PROVEN'").fetchall()
        intervals = sorted((int(row[0].split(":")[0], 0), int(row[0].split(":")[1], 0)) for row in proven_ranges if ":" in row[0])
        covered = sum(max(0, end - start) for start, end in intervals)
        return {"nodes": count("map_node"), "proven_nodes": count("map_node", "WHERE status='PROVEN'"),
                "observed_nodes": count("map_node", "WHERE status='OBSERVED'"), "edges": count("map_edge"),
                "proven_edges": count("map_edge", "WHERE status='PROVEN'"), "open_frontiers": count("map_frontier", "WHERE status='OPEN'"),
                "resolved_frontiers": count("map_frontier", "WHERE status='RESOLVED'"), "conflicts": count("map_conflict"),
                "components": self._components(), "proven_rom_ranges": len(intervals), "proven_rom_bytes": covered,
                "source_owned_bytes": int(self.db.execute("SELECT value FROM map_meta WHERE key='source_owned_bytes'").fetchone()[0]),
                "graph_hash": self.graph_hash()}

    def merge(self, bundle: dict[str, Any], import_ref: str, source_sha256: str = "") -> MapDelta:
        existing = self.db.execute("SELECT 1 FROM map_import WHERE import_ref=?", (import_ref,)).fetchone()
        delta = MapDelta(import_ref=import_ref)
        before = self._components()
        self.db.execute("BEGIN")
        try:
            self._merge_nodes(bundle, delta)
            self._merge_edges(bundle, delta)
            self._merge_frontiers(bundle, delta)
            if not existing:
                self.db.execute("INSERT INTO map_import VALUES (?, ?, ?)", (import_ref, source_sha256, canonical({"import_ref": import_ref})))
            after = self._components()
            delta.component_joins = max(0, before - after) if not existing else 0
            delta.graph_hash = self.graph_hash()
            self.db.commit()
        except Exception:
            self.db.rollback()
            raise
        return delta

    def close(self) -> None:
        self.db.close()
