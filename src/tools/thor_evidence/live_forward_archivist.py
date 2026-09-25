"""Validate and atomically seed or merge one closed live-forward session map."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
from typing import Any

try:
    from .cartographer import Cartographer
    from .live_forward_cartographer import SESSION_SCHEMA, _read_meta, _sha256
    from .map_merge import merge_session_map
    from .runtime_occurrence_merge import occurrence_hash
except ImportError:
    from cartographer import Cartographer
    from live_forward_cartographer import SESSION_SCHEMA, _read_meta, _sha256
    from map_merge import merge_session_map
    from runtime_occurrence_merge import occurrence_hash


def _inspect(path: Path) -> tuple[dict[str, str], dict[str, Any]]:
    if not path.is_file():
        raise FileNotFoundError(path)
    try:
        db = sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)
        metadata = _read_meta(db)
        required_tables = {row[0] for row in db.execute(
            "SELECT name FROM sqlite_master WHERE type='table'")}
        if not {"map_meta", "map_node", "map_edge", "map_frontier", "map_conflict", "map_import"} <= required_tables:
            raise ValueError("STOP_ARCHIVIST_SCHEMA_MISMATCH")
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise ValueError("STOP_ARCHIVIST_GRAPH_MISMATCH")
    except sqlite3.DatabaseError as exc:
        raise ValueError("STOP_ARCHIVIST_SCHEMA_MISMATCH") from exc
    finally:
        if "db" in locals():
            db.close()
    if metadata.get("schema") != "m12.map1.v1":
        raise ValueError("STOP_ARCHIVIST_SCHEMA_MISMATCH")
    rom_sha = _sha256(metadata.get("rom_sha256", ""), "ROM")
    graph = Cartographer(path, rom_sha)
    try:
        actual_graph_hash = graph.graph_hash()
        expected_occurrences = metadata.get("live_forward_occurrence_sha256")
        if expected_occurrences and occurrence_hash(graph.db) != expected_occurrences:
            raise ValueError("STOP_ARCHIVIST_OCCURRENCE_HASH_MISMATCH")
        metrics = graph.metrics()
    finally:
        graph.close()
    expected = metadata.get("live_forward_graph_sha256")
    if expected and expected != actual_graph_hash:
        raise ValueError("STOP_ARCHIVIST_GRAPH_MISMATCH")
    return metadata, {**metrics, "graph_hash": actual_graph_hash}


def _file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _merge_receipt(rom_sha256: str, session_meta: dict[str, str],
                   session_metrics: dict[str, Any], master_before: dict[str, Any] | None,
                   master_after: dict[str, Any], mode: str, source_sha256: str) -> dict[str, Any]:
    receipt = {
        "schema": "oasis.m12.archivist-merge-receipt.v1",
        "rom_sha256": rom_sha256,
        "session_id": session_meta["live_forward_session_id"],
        "session_graph_hash": session_metrics["graph_hash"],
        "master_graph_hash_before": master_before["graph_hash"] if master_before else None,
        "master_graph_hash_after": master_after["graph_hash"],
        "merge_mode": mode,
        "session_node_count": session_metrics["nodes"],
        "session_edge_count": session_metrics["edges"],
        "master_node_count_before": master_before["nodes"] if master_before else 0,
        "master_edge_count_before": master_before["edges"] if master_before else 0,
        "master_node_count_after": master_after["nodes"],
        "master_edge_count_after": master_after["edges"],
        "conflict_count": master_after["conflicts"],
        "source_artifact_sha256": source_sha256,
    }
    receipt["receipt_sha256"] = hashlib.sha256(json.dumps(
        receipt, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("utf-8")).hexdigest()
    return receipt


def archive_session(master_path: Path, session_path: Path,
                    expected_rom_sha256: str | None = None) -> dict[str, Any]:
    master_path, session_path = Path(master_path).resolve(), Path(session_path).resolve()
    session_meta, session_metrics = _inspect(session_path)
    if session_meta.get("live_forward_session_schema") != SESSION_SCHEMA or \
            session_meta.get("live_forward_session_state") != "CLOSED":
        raise ValueError("STOP_ARCHIVIST_SESSION_SCHEMA_MISMATCH")
    if master_path == session_path:
        raise ValueError("STOP_ARCHIVIST_SESSION_IS_MASTER")
    for key in ("live_forward_session_id", "live_forward_instrumentation_identity",
                "live_forward_run_id", "live_forward_created_utc", "live_forward_closed_utc",
                "live_forward_graph_sha256"):
        if not session_meta.get(key):
            raise ValueError(f"STOP_ARCHIVIST_SESSION_IDENTITY_MISSING:{key}")
    if expected_rom_sha256 and session_meta["rom_sha256"] != _sha256(
            expected_rom_sha256, "expected ROM"):
        raise ValueError("STOP_ARCHIVIST_ROM_MISMATCH")
    if session_metrics["conflicts"]:
        raise ValueError("STOP_ARCHIVIST_SESSION_CONFLICT")
    if session_metrics["proven_nodes"] or session_metrics["proven_edges"] or \
            session_metrics["source_owned_bytes"]:
        raise ValueError("STOP_ARCHIVIST_FALSE_PROVEN_OR_OWNERSHIP")
    _sha256(session_meta["live_forward_instrumentation_identity"], "instrumentation")
    if int(session_meta["live_forward_run_id"]) <= 0:
        raise ValueError("STOP_ARCHIVIST_SESSION_IDENTITY_INVALID")
    master_existed = master_path.exists()
    master_hash_before = None
    master_graph_before = None
    if master_existed:
        master_meta, master_metrics = _inspect(master_path)
        if master_meta.get("schema") != "m12.map1.v1":
            raise ValueError("STOP_ARCHIVIST_SCHEMA_MISMATCH")
        if master_meta.get("rom_sha256") != session_meta["rom_sha256"]:
            raise ValueError("STOP_ARCHIVIST_ROM_MISMATCH")
        master_hash_before = master_metrics["graph_hash"]
        master_graph_before = master_metrics
    result = merge_session_map(master_path, session_path, reject_conflicts=True)
    final_meta, final_metrics = _inspect(master_path)
    if final_meta.get("schema") != "m12.map1.v1" or \
            final_meta.get("rom_sha256") != session_meta["rom_sha256"]:
        raise ValueError("STOP_ARCHIVIST_MERGED_IDENTITY_MISMATCH")
    if not master_existed and final_metrics["graph_hash"] != session_metrics["graph_hash"]:
        raise ValueError("STOP_ARCHIVIST_SEED_GRAPH_MISMATCH")
    if master_graph_before is not None and (
            final_metrics["nodes"] < master_graph_before["nodes"] or
            final_metrics["edges"] < master_graph_before["edges"]):
        raise ValueError("STOP_ARCHIVIST_MERGE_LOSS")
    mode = "MERGE" if master_existed else "SEED"
    receipt = _merge_receipt(session_meta["rom_sha256"], session_meta,
        session_metrics, master_graph_before, final_metrics, mode, _file_sha256(session_path))
    return {"status": "PASS", "mode": mode,
            "warning": None, "session_id": session_meta["live_forward_session_id"],
            "session_graph_hash": session_metrics["graph_hash"],
            "master_graph_hash_before": master_hash_before,
            "master_graph_hash_after": final_metrics["graph_hash"],
            "master_nodes": final_metrics["nodes"], "master_edges": final_metrics["edges"],
            "global_merge": result, "merge_receipt": receipt}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--master", type=Path, required=True)
    parser.add_argument("--session", type=Path, required=True)
    parser.add_argument("--rom-sha256", required=True)
    args = parser.parse_args()
    print(json.dumps(archive_session(args.master, args.session, args.rom_sha256),
                     sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
