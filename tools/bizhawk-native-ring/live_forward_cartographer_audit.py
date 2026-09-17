#!/usr/bin/env python3
"""Independently audit all Worker segments and lineage in a 2A campaign receipt."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
from typing import Any


LINEAGE_KEYS = {"run_id", "epoch", "worker_id", "capture_id", "generation",
                "segment_sha256", "records_sha256", "entry_stream_sequence",
                "exit_stream_sequence", "entry_instruction_sequence",
                "exit_instruction_sequence", "profile", "session_id"}


def _readonly(path: Path) -> sqlite3.Connection:
    if not path.is_file():
        raise FileNotFoundError(path)
    return sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)


def _segments(runtime: dict[str, Any], worker_count: int,
              cycles: int) -> tuple[set[tuple], dict[str, int]]:
    path = Path(runtime["segment_audit_jsonl"])
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != runtime["segment_audit_sha256"]:
        raise ValueError("STOP_CARTOGRAPHER_SEGMENT_AUDIT_HASH_MISMATCH")
    rows = [json.loads(line) for line in raw.splitlines() if line]
    if len(rows) != worker_count * cycles or len(rows) != runtime["audited_segments"]:
        raise ValueError("STOP_CARTOGRAPHER_SEGMENT_AUDIT_COUNT_MISMATCH")
    workers: dict[int, list[dict[str, Any]]] = {worker: [] for worker in range(worker_count)}
    identities: set[tuple] = set()
    last_exit_by_worker: dict[int, int] = {}
    last_entry = 0
    for row in rows:
        worker, cycle, run_id = row["worker_id"], row["cycle"], row["run_id"]
        if row["valid"] is not True or run_id != runtime["run_id"] or \
                worker not in workers or not 1 <= cycle <= cycles:
            raise ValueError("STOP_CARTOGRAPHER_INVALID_SEGMENT_AUDIT")
        expected_capture = run_id * 1_000_000 + (cycle - 1) * worker_count + worker + 1
        if row["capture_id"] != expected_capture or row["generation"] != cycle or row["epoch"] <= 0:
            raise ValueError("STOP_CARTOGRAPHER_CAPTURE_IDENTITY_MISMATCH")
        if row["exit_stream_sequence"] - row["entry_stream_sequence"] != row["record_count"] or \
                row["entry_stream_sequence"] <= last_entry or \
                row["entry_stream_sequence"] < last_exit_by_worker.get(worker, 0):
            raise ValueError("STOP_CARTOGRAPHER_EXECUTION_BOUNDS_MISMATCH")
        last_entry = row["entry_stream_sequence"]
        last_exit_by_worker[worker] = row["exit_stream_sequence"]
        identity = (run_id, row["epoch"], worker, row["capture_id"],
                    row["generation"], row["segment_sha256"])
        if identity in identities:
            raise ValueError("STOP_CARTOGRAPHER_DUPLICATE_SEGMENT_IDENTITY")
        identities.add(identity)
        workers[worker].append(row)
    for worker, worker_rows in workers.items():
        if len(worker_rows) != cycles or {row["cycle"] for row in worker_rows} != set(range(1, cycles + 1)):
            raise ValueError(f"STOP_CARTOGRAPHER_WORKER_CYCLE_GAP:{worker}")
        if len({row["capture_id"] for row in worker_rows}) != cycles or \
                len({row["generation"] for row in worker_rows}) != cycles:
            raise ValueError(f"STOP_CARTOGRAPHER_WORKER_IDENTITY_REUSE:{worker}")
    if any(worker["capture_count"] != cycles for worker in runtime["workers"]):
        raise ValueError("STOP_CARTOGRAPHER_WORKER_SUMMARY_MISMATCH")
    return identities, {str(worker): len(worker_rows) for worker, worker_rows in workers.items()}


def _session(path: Path, saved: dict[str, Any], runtime: dict[str, Any],
             campaign: dict[str, Any], segment_ids: set[tuple]) -> dict[str, Any]:
    db = _readonly(path)
    try:
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise ValueError("STOP_SESSION_MAP_SQLITE_INTEGRITY")
        metadata = dict(db.execute("SELECT key, value FROM map_meta"))
        if metadata.get("schema") != "m12.map1.v1" or \
                metadata.get("live_forward_session_schema") != "oasis.m12.live-forward-session.v1" or \
                metadata.get("live_forward_session_state") != "CLOSED" or \
                metadata.get("rom_sha256") != campaign["rom_sha256"] or \
                metadata.get("live_forward_run_id") != str(runtime["run_id"]) or \
                metadata.get("live_forward_session_id") != saved["session_id"] or \
                metadata.get("live_forward_graph_sha256") != saved["graph_hash"]:
            raise ValueError("STOP_SESSION_MAP_IDENTITY_MISMATCH")
        tables = {row[0] for row in db.execute(
            "SELECT name FROM sqlite_master WHERE type='table'")}
        if "live_forward_pending_lineage" in tables:
            raise ValueError("STOP_SESSION_MAP_PENDING_LINEAGE_LEAK")
        imports = db.execute("SELECT COUNT(*) FROM map_import").fetchone()[0]
        if imports != saved["segments_admitted"] or imports != len(segment_ids):
            raise ValueError("STOP_SESSION_MAP_IMPORT_COUNT_MISMATCH")
        if int(metadata["source_owned_bytes"]) != 0 or saved["source_owned_bytes"] != 0:
            raise ValueError("STOP_SESSION_MAP_SOURCE_OWNED_CHANGED")
        if db.execute("SELECT COUNT(*) FROM map_conflict").fetchone()[0] or \
                db.execute("SELECT COUNT(*) FROM map_node WHERE status!='OBSERVED'").fetchone()[0] or \
                db.execute("SELECT COUNT(*) FROM map_edge WHERE status!='OBSERVED'").fetchone()[0]:
            raise ValueError("STOP_SESSION_MAP_FALSE_PROOF_STATUS")
        node_ids = {row[0] for row in db.execute("SELECT node_id FROM map_node")}
        edge_ids = {row[0] for row in db.execute("SELECT edge_id FROM map_edge")}
        seen_segments: set[tuple] = set()
        lineage_count = 0
        for object_type, encoded in db.execute(
                "SELECT 'node', lineage FROM map_node UNION ALL "
                "SELECT 'edge', lineage FROM map_edge"):
            for lineage in json.loads(encoded):
                if not LINEAGE_KEYS <= lineage.keys() or lineage["profile"] != "FLOW_V1" or \
                        lineage["run_id"] != runtime["run_id"] or \
                        lineage["session_id"] != saved["session_id"]:
                    raise ValueError("STOP_SESSION_MAP_LINEAGE_IDENTITY_INCOMPLETE")
                if object_type == "edge" and (not lineage.get("control_flow_outcomes") or
                        not set(lineage["control_flow_outcomes"]) <= {
                            "sequential", "control_flow", "branch_taken", "branch_not_taken",
                            "call", "return", "exception_interrupt", "cpu_stop", "exception",
                            "asynchronous_exception"}):
                    raise ValueError("STOP_SESSION_MAP_CONTROL_FLOW_LINEAGE_INVALID")
                seen_segments.add((lineage["run_id"], lineage["epoch"], lineage["worker_id"],
                    lineage["capture_id"], lineage["generation"], lineage["segment_sha256"]))
                lineage_count += 1
        if segment_ids != seen_segments:
            raise ValueError("STOP_SESSION_MAP_SEGMENT_LINEAGE_SET_MISMATCH")
        if db.execute("SELECT COUNT(*) FROM map_edge").fetchone()[0] != saved["edges"] or \
                db.execute("SELECT COUNT(*) FROM map_edge WHERE relation='EXECUTED_NEXT'").fetchone()[0] != saved["observed_edges"]:
            raise ValueError("STOP_SESSION_MAP_EXECUTION_EDGE_COUNT_MISMATCH")
        if db.execute("SELECT COUNT(*) FROM map_frontier").fetchone()[0]:
            raise ValueError("STOP_SESSION_MAP_UNEXPECTED_FRONTIER")
        return {"segments": len(segment_ids), "lineage_segments": len(seen_segments),
                "lineage_rows": lineage_count, "nodes": len(node_ids), "edges": len(edge_ids),
                "node_ids": node_ids, "edge_ids": edge_ids,
                "graph_hash": metadata["live_forward_graph_sha256"],
                "sqlite_bytes": path.stat().st_size}
    finally:
        db.close()


def audit_campaign(receipt_path: Path) -> dict[str, Any]:
    receipt_path = Path(receipt_path).resolve()
    campaign = json.loads(receipt_path.read_text(encoding="utf-8"))
    if campaign.get("status") != "PASS_LIVE_FORWARD_RAM_CARTOGRAPHER_ARCHIVIST":
        raise ValueError("campaign did not reach the 2A PASS status")
    worker_count, cycles = int(campaign["worker_count"]), int(campaign["cycles_per_worker"])
    if worker_count != 16 or cycles != 100:
        raise ValueError("2A audit requires exactly 16 Workers x 100 cycles")
    if len(campaign["sessions"]) != 2 or len(campaign["session_files_retained"]) != 2:
        raise ValueError("campaign does not contain two retained sessions")
    run_ids: set[int] = set()
    session_facts = []
    session_ids: list[set[tuple]] = []
    for item, session_name in zip(campaign["sessions"], campaign["session_files_retained"]):
        runtime, saved = item["runtime"], item["saved"]
        identities, worker_counts = _segments(runtime, worker_count, cycles)
        if runtime["run_id"] in run_ids:
            raise ValueError("independent runs reused run_id")
        run_ids.add(runtime["run_id"])
        facts = _session(Path(session_name), saved, runtime, campaign, identities)
        session_ids.append(identities)
        session_facts.append({key: value for key, value in facts.items()
                              if key not in ("node_ids", "edge_ids")})
        session_facts[-1]["worker_cycles"] = worker_counts
    if campaign["sessions"][0]["archivist"].get("mode") != "SEED" or \
            campaign["sessions"][0]["archivist"].get("warning") is not None or \
            campaign["sessions"][1]["archivist"].get("mode") != "MERGE":
        raise ValueError("Archivist did not prove silent seed followed by merge")
    if campaign["sessions"][1]["idempotent_replay"].get(
            "master_graph_hash_after") != campaign["master_graph_hash"]:
        raise ValueError("STOP_ARCHIVIST_NON_IDEMPOTENT")
    master_path = Path(campaign["master_path"]).resolve()
    db = _readonly(master_path)
    try:
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise ValueError("STOP_ARCHIVIST_MASTER_CORRUPTED")
        metadata = dict(db.execute("SELECT key, value FROM map_meta"))
        if metadata.get("schema") != "m12.map1.v1" or \
                metadata.get("rom_sha256") != campaign["rom_sha256"] or \
                metadata.get("source_owned_bytes") != "0":
            raise ValueError("STOP_ARCHIVIST_MASTER_IDENTITY_MISMATCH")
        imports = db.execute("SELECT COUNT(*) FROM map_import").fetchone()[0]
        if imports != 2:
            raise ValueError("STOP_ARCHIVIST_REPLAY_DUPLICATED_IMPORT")
        master_nodes = {row[0] for row in db.execute("SELECT node_id FROM map_node")}
        master_edges = {row[0] for row in db.execute("SELECT edge_id FROM map_edge")}
        if not session_facts or not session_ids:
            raise ValueError("campaign session facts are missing")
        first = _session(Path(campaign["session_files_retained"][0]),
                         campaign["sessions"][0]["saved"],
                         campaign["sessions"][0]["runtime"], campaign, session_ids[0])
        second = _session(Path(campaign["session_files_retained"][1]),
                          campaign["sessions"][1]["saved"],
                          campaign["sessions"][1]["runtime"], campaign, session_ids[1])
        if not first["node_ids"] <= master_nodes or not first["edge_ids"] <= master_edges or \
                not second["node_ids"] <= master_nodes or not second["edge_ids"] <= master_edges:
            raise ValueError("STOP_ARCHIVIST_MERGE_LOSS")
        runs = set()
        master_lineage_rows = 0
        for object_type, encoded in db.execute(
                "SELECT 'node', lineage FROM map_node UNION ALL "
                "SELECT 'edge', lineage FROM map_edge"):
            for lineage in json.loads(encoded):
                if lineage.get("profile") == "FLOW_V1":
                    if object_type == "edge" and not lineage.get("control_flow_outcomes"):
                        raise ValueError("master execution edge lost control-flow lineage")
                    runs.add(lineage["run_id"])
                    master_lineage_rows += 1
        if runs != run_ids:
            raise ValueError("master does not contain both independent session lineages")
    finally:
        db.close()
    return {"status": "PASS_INDEPENDENT_AUDIT", "checkpoint": campaign["checkpoint"],
            "rom_sha256": campaign["rom_sha256"], "segments_audited": worker_count * cycles * 2,
            "runs": session_facts, "master_graph_hash": campaign["master_graph_hash"],
            "master": {"nodes": len(master_nodes), "edges": len(master_edges),
                       "lineage_rows": master_lineage_rows, "imports": imports,
                       "runs": sorted(run_ids)}, "source_owned_delta": 0,
            "old_facts_preserved": True}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = audit_campaign(args.receipt)
    output = args.output or args.receipt.resolve().with_name("cartographer-2a-independent-audit.json")
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": result["status"], "segments_audited": result["segments_audited"],
                      "receipt": str(output.resolve())}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
