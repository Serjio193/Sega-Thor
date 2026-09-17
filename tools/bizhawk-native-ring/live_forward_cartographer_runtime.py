#!/usr/bin/env python3
"""Run two independent 16-Worker FLOW_V1 sessions through RAM MAP-1 and Archivist."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sqlite3
import subprocess
import sys

EVIDENCE_DIR = Path(__file__).parents[2] / "src" / "tools" / "thor_evidence"
sys.path.insert(0, str(EVIDENCE_DIR))

from live_forward_scaling_runtime import resolve_runtime_paths, run_one
from cartographer import Cartographer
from live_forward_cartographer import LiveForwardCartographer


WORKER_COUNT = 16
CYCLES_PER_WORKER = 100
WORKER_DEPTH = 20
WORKER_MEMORY = 64 * 1024


def _instrumentation_identity(install: Path, script: Path) -> str:
    inputs = {"profile": "FLOW_V1", "worker": "M12-AUTO67-LIVE-FORWARD-WORKER-1B",
              "wbx_sha256": hashlib.sha256((install / "dll" / "gpgx.wbx").read_bytes()).hexdigest(),
              "lua_sha256": hashlib.sha256(script.read_bytes()).hexdigest(),
              "trace_contract_sha256": hashlib.sha256(
                  Path(__file__).with_name("live_forward_trace.h").read_bytes()).hexdigest()}
    encoded = json.dumps(inputs, sort_keys=True, separators=(",", ":")).encode("ascii")
    return hashlib.sha256(encoded).hexdigest()


def _graph_ids(path: Path) -> tuple[set[str], set[str], str]:
    uri = path.resolve().as_uri() + "?mode=ro"
    db = sqlite3.connect(uri, uri=True)
    try:
        nodes = {row[0] for row in db.execute("SELECT node_id FROM map_node")}
        edges = {row[0] for row in db.execute("SELECT edge_id FROM map_edge")}
        metadata = {row[0]: row[1] for row in db.execute(
            "SELECT key, value FROM map_meta")}
    finally:
        db.close()
    graph = Cartographer(path, metadata["rom_sha256"])
    try:
        return nodes, edges, graph.graph_hash()
    finally:
        graph.close()


def _archive(args: argparse.Namespace, session_path: Path, rom_sha: str) -> tuple[dict, str, str]:
    archivist = Path(__file__).parents[2] / "src" / "tools" / "thor_evidence" / \
        "live_forward_archivist.py"
    command = [sys.executable, str(archivist), "--master", str(args.master),
               "--session", str(session_path), "--rom-sha256", rom_sha]
    completed = subprocess.run(command, cwd=Path(__file__).parents[2],
                               capture_output=True, text=True, check=False)
    if completed.returncode:
        raise RuntimeError("Archivist failed: " + completed.stderr[-4000:] + completed.stdout[-4000:])
    result = json.loads(completed.stdout)
    if result.get("status") != "PASS" or completed.stderr:
        raise RuntimeError("Archivist returned a warning or non-PASS result")
    return result, completed.stdout, completed.stderr


def _run_session(args: argparse.Namespace, label: str, rom_sha: str,
                 instrumentation_identity: str) -> tuple[dict, dict, Path]:
    session = LiveForwardCartographer(rom_sha, instrumentation_identity)
    try:
        result = run_one(args, label, WORKER_COUNT, WORKER_DEPTH,
                         lambda segment, rows, data: session.admit(segment, rows, data))
        if result.get("outcome") != "PASS" or \
                result.get("audited_segments") != WORKER_COUNT * CYCLES_PER_WORKER:
            raise RuntimeError(f"{label}: Worker run did not pass the 16 x 100 segment gate")
        metrics = session.metrics()
        if metrics["segments_admitted"] != result["audited_segments"] or \
                metrics["segments_rejected"] != 0 or metrics["source_owned_bytes"] != 0:
            raise RuntimeError(f"{label}: Cartographer admission metrics do not reconcile")
        session_id = session.session_id
        session_path = args.output_dir / "sessions" / f"session-{session_id}.sqlite"
        saved = session.save_closed(session_path, WORKER_COUNT * CYCLES_PER_WORKER)
        if saved["graph_hash"] != session.metrics()["graph_hash"]:
            raise RuntimeError(f"{label}: RAM and saved graph hashes differ")
        return result, saved, session_path
    finally:
        session.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--master", type=Path, required=True)
    parser.add_argument("--native-budget-bytes", type=int, default=256 * 1024 * 1024)
    parser.add_argument("--process-budget-bytes", type=int, default=512 * 1024 * 1024)
    parser.add_argument("--core-reserve-bytes", type=int, default=128 * 1024 * 1024)
    parser.add_argument("--system-reserve-bytes", type=int, default=4 * 1024 * 1024 * 1024)
    parser.add_argument("--max-frames", type=int, default=1800)
    parser.add_argument("--timeout", type=int, default=1800)
    args = parser.parse_args()
    args.memory_bytes, args.rounds = WORKER_MEMORY, CYCLES_PER_WORKER
    resolve_runtime_paths(args)
    args.master = args.master.resolve()
    if args.output_dir.exists():
        parser.error(f"campaign output already exists; preserve it and choose a new path: {args.output_dir}")
    if not (args.install / "EmuHawk.exe").is_file() or not args.rom.is_file() or \
            not args.script.is_file() or not (args.install / "dll" / "gpgx.wbx").is_file():
        parser.error("install, ROM, Lua script or GPGX WBX does not exist")
    if args.master.exists():
        parser.error(f"first-run proof requires a missing test master: {args.master}")
    args.output_dir.mkdir(parents=True)
    args.master.parent.mkdir(parents=True, exist_ok=True)
    rom_sha = hashlib.sha256(args.rom.read_bytes()).hexdigest()
    instrumentation_identity = _instrumentation_identity(args.install, args.script)
    report: dict[str, object] = {"checkpoint": "M12-AUTO67-LIVE-FORWARD-CARTOGRAPHER-2A",
        "status": "IN_PROGRESS", "rom_sha256": rom_sha,
        "instrumentation_identity": instrumentation_identity,
        "worker_count": WORKER_COUNT, "cycles_per_worker": CYCLES_PER_WORKER,
        "depth": WORKER_DEPTH, "memory_bytes_each": WORKER_MEMORY, "sessions": []}
    first_run, first_saved, first_path = _run_session(
        args, "session-1", rom_sha, instrumentation_identity)
    first_archive, _, first_stderr = _archive(args, first_path, rom_sha)
    if first_archive["mode"] != "SEED" or first_stderr:
        raise RuntimeError("first missing-master Archivist seed was not silent and successful")
    first_nodes, first_edges, first_hash = _graph_ids(args.master)
    if first_hash != first_saved["graph_hash"]:
        raise RuntimeError("first Archivist master differs from saved session graph")
    report["sessions"].append({"runtime": first_run, "saved": first_saved,
                               "archivist": first_archive})

    old_nodes, old_edges = set(first_nodes), set(first_edges)
    second_run, second_saved, second_path = _run_session(
        args, "session-2", rom_sha, instrumentation_identity)
    if first_run["run_id"] == second_run["run_id"]:
        raise RuntimeError("independent EmuHawk sessions reused a run_id")
    second_archive, _, _ = _archive(args, second_path, rom_sha)
    after_nodes, after_edges, second_hash = _graph_ids(args.master)
    if not old_nodes <= after_nodes or not old_edges <= after_edges:
        raise RuntimeError("second Archivist merge lost existing master nodes or edges")
    if second_archive["mode"] != "MERGE":
        raise RuntimeError("second session did not merge into the first master")
    before_replay_hash = second_hash
    replay_archive, _, _ = _archive(args, second_path, rom_sha)
    _, _, after_replay_hash = _graph_ids(args.master)
    if after_replay_hash != before_replay_hash:
        raise RuntimeError("STOP_ARCHIVIST_NON_IDEMPOTENT")
    report["sessions"].append({"runtime": second_run, "saved": second_saved,
        "archivist": second_archive, "idempotent_replay": replay_archive})
    report.update({"status": "PASS_LIVE_FORWARD_RAM_CARTOGRAPHER_ARCHIVIST",
        "master_path": str(args.master), "master_graph_hash": after_replay_hash,
        "first_master_nodes": len(old_nodes), "first_master_edges": len(old_edges),
        "final_master_nodes": len(after_nodes), "final_master_edges": len(after_edges),
        "old_facts_preserved": True, "source_owned_delta": 0,
        "session_files_retained": [str(first_path), str(second_path)]})
    receipt = args.output_dir / "live-forward-cartographer-2a-receipt.json"
    receipt.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"status": report["status"], "receipt": str(receipt),
                      "master": str(args.master)}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
