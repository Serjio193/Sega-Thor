"""Read-only forensic reconciliation for ambiguous historical V2 runs."""
from __future__ import annotations

from collections import Counter
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import sqlite3
from typing import Any

from live_forward_scaling_audit import RECORD
from master_v2_contribution_boundary import canonical_bytes, contribution_hash
from master_v2_shadow import read_master_v2_section

EXACT = "ALREADY_COMMITTED_EXACT"
PARTIAL = "PARTIALLY_COMMITTED"
NOT_COMMITTED = "NOT_COMMITTED_EXACT"
AMBIGUOUS = "STILL_AMBIGUOUS"
RELATIONS = ("EXECUTED_NEXT", "OBSERVED_NEXT_PC")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _entry_path(entry: dict[str, Any], key: str) -> Path | None:
    value = entry.get("source_evidence", {}).get(key)
    return Path(value) if value else None


def _flow_key(pc: int, opcode: int) -> str:
    return f"{pc:06X}:{opcode:04X}"


def _edge_key(source: int, relation: str, target: int) -> str:
    return f"{source:06X}:{relation}:{target:06X}"


def scan_flow(entry: dict[str, Any]) -> dict[str, Any]:
    """Stream sealed FLOW slices; never materialize a session database or raw file."""
    index_path = _entry_path(entry, "index_path")
    raw_path = _entry_path(entry, "raw_path")
    if not index_path or not raw_path or not index_path.is_file() or not raw_path.is_file():
        raise ValueError("FLOW_EVIDENCE_UNAVAILABLE")
    instructions: Counter[str] = Counter()
    relations: Counter[str] = Counter()
    segments = records = terminal = 0
    raw_digest = hashlib.sha256()
    with index_path.open("r", encoding="utf-8") as index, raw_path.open("rb") as raw:
        for line in index:
            item = json.loads(line)
            segment = item["segment"]
            if int(segment["run_id"]) != int(entry["run_id"]):
                raise ValueError("FLOW_RUN_ID_MISMATCH")
            offset, length = int(item["raw_offset"]), int(item["raw_length"])
            raw.seek(offset)
            data = raw.read(length)
            if len(data) != length or hashlib.sha256(data).hexdigest() != item.get("raw_sha256"):
                raise ValueError("FLOW_SEGMENT_HASH_MISMATCH")
            raw_digest.update(data)
            rows = list(RECORD.iter_unpack(data))
            if len(rows) != int(segment["record_count"]) or length != len(rows) * RECORD.size:
                raise ValueError("FLOW_RECORD_COUNT_MISMATCH")
            executed = [row for row in rows if row[6] & 1]
            for row in executed:
                instructions[_flow_key(int(row[3]), int(row[5]))] += 1
            for left, right in zip(executed, executed[1:]):
                relations[_edge_key(int(left[3]), "EXECUTED_NEXT", int(right[3]))] += 1
            if executed:
                relations[_edge_key(int(executed[-1][3]), "OBSERVED_NEXT_PC",
                                    int(executed[-1][4]))] += 1
                terminal += 1
            segments += 1
            records += len(rows)
    value = {"run_id": int(entry["run_id"]), "segments": segments, "records": records,
             "instruction_occurrences": dict(sorted(instructions.items())),
             "relation_occurrences": dict(sorted(relations.items())),
             "terminal_occurrences": {key: value for key, value in relations.items()
                                       if ":OBSERVED_NEXT_PC:" in key},
             "raw_stream_sha256": raw_digest.hexdigest(), "raw_sha256": entry["source_evidence"].get("raw_sha256"),
             "source_owned_delta": 0, "asm": {"promoted_bytes": 0, "status": "NO_DELTA"}}
    value["instruction_occurrence_contribution"] = sum(instructions.values())
    value["relation_contribution"] = sum(relations.values())
    value["terminal_contribution"] = terminal
    value["contribution_hash"] = contribution_hash(value)
    return value


@dataclass(frozen=True)
class Snapshot:
    name: str
    path: Path
    runs: frozenset[int]
    instructions: dict[str, int]
    relations: dict[str, int]


def _read_counts(path: Path) -> Snapshot:
    db = sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)
    try:
        run_ids = frozenset(int(row[0]) for row in db.execute("SELECT run_id FROM run"))
        instructions = {_flow_key(int(pc), int(opcode)): int(count)
                        for pc, opcode, count in db.execute("SELECT pc,opcode,occurrences FROM instruction")}
        relations = {_edge_key(int(source), str(relation), int(target)): int(count)
                     for source, relation, target, count in db.execute(
                         "SELECT source_pc,relation,target_pc,occurrences FROM edge")}
    finally:
        db.close()
    return Snapshot(path.parent.name, path, run_ids, instructions, relations)


def discover_snapshots(rolling_root: Path) -> list[Snapshot]:
    return [_read_counts(path) for path in sorted((rolling_root / "generations").glob("*/master.sqlite"))]


def _delta(parent: dict[str, int], child: dict[str, int]) -> dict[str, int]:
    keys = set(parent) | set(child)
    return {key: int(child.get(key, 0)) - int(parent.get(key, 0)) for key in sorted(keys)
            if int(child.get(key, 0)) != int(parent.get(key, 0))}


def exact_transition(snapshots: list[Snapshot], run_id: int) -> tuple[Snapshot, Snapshot] | None:
    for parent in snapshots:
        for child in snapshots:
            if parent is child or child.runs != parent.runs | {run_id}:
                continue
            return parent, child
    return None


def session_probe(path: Path | None, run_id: int) -> dict[str, Any]:
    if not path or not path.is_file():
        return {"path": str(path) if path else None, "present": False}
    result: dict[str, Any] = {"path": str(path), "present": True, "bytes": path.stat().st_size}
    db = sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)
    try:
        result["journal_mode"] = db.execute("PRAGMA journal_mode").fetchone()[0]
        result["tables"] = [row[0] for row in db.execute(
            "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")]
        result["meta"] = dict(db.execute("SELECT key,value FROM map_meta ORDER BY key").fetchall())
        result["map_import_rows"] = int(db.execute("SELECT count(*) FROM map_import").fetchone()[0])
        result["run_id_match"] = result["meta"].get("live_forward_run_id") == str(run_id)
        result["state"] = result["meta"].get("live_forward_session_state")
    finally:
        db.close()
    return result


def reconcile(ledger_path: Path, backlog_path: Path, rolling_root: Path,
              master_path: Path, session_run_id: int = 1789839425) -> dict[str, Any]:
    ledger = json.loads(ledger_path.read_text(encoding="utf-8"))
    backlog = {int(item["run_id"]): item for item in
               json.loads(backlog_path.read_text(encoding="utf-8"))["runs"]}
    entries = [item for item in ledger["runs"] if item.get("classification") == "AMBIGUOUS"]
    snapshots = discover_snapshots(rolling_root)
    before = json.loads(read_master_v2_section(master_path, "rolling_master"))
    master_meta = json.loads(read_master_v2_section(master_path, "meta"))
    before_hashes = {name: sha256_file(master_path) for name in ("master",)}
    results: list[dict[str, Any]] = []
    for entry in entries:
        run_id = int(entry["run_id"])
        transition = exact_transition(snapshots, run_id)
        backlog_entry = backlog.get(run_id, {})
        item: dict[str, Any] = {"run_id": run_id,
                                "rom": master_meta.get("rom"),
                                "segments": int(entry.get("segments", 0)),
                                "records": int(entry.get("records", 0)),
                                "bulk_bytes": int(backlog_entry.get("bulk_bytes_remaining", 0) or 0),
                                "campaign_path": entry.get("campaign_path"), "proof_sources": [],
                                "historical_parent": None, "historical_child": None,
                                "classification": AMBIGUOUS, "safe_to_apply": False,
                                "safe_to_verify": False, "safe_to_delete": False,
                                "stage_absorption_state": "UNKNOWN", "contribution": None,
                                "source_hashes": entry.get("source_evidence", {}),
                                "stage_evidence": {key: backlog_entry.get(key) for key in
                                                   ("stage5", "stage6", "stage7", "stage8", "stage9")},
                                "instruction_identities": [], "relation_identities": [],
                                "terminal_contribution": 0, "provenance_delta": {},
                                "canonical_delta": {}, "ownership_delta": {},
                                "emission_delta": {}, "asm_delta": {"promoted_bytes": 0},
                                "reason": "lineage-only evidence; no durable parent/child delta"}
        if transition:
            parent, child = transition
            item["historical_parent"], item["historical_child"] = parent.name, child.name
            item["proof_sources"] = [str(parent.path), str(child.path)]
            expected_i = _delta(parent.instructions, child.instructions)
            expected_r = _delta(parent.relations, child.relations)
            try:
                contribution = scan_flow(entry)
                if contribution["instruction_occurrences"] == expected_i and \
                        contribution["relation_occurrences"] == expected_r and \
                        contribution["segments"] == int(entry["segments"]) and contribution["records"] == int(entry["records"]):
                    item.update(classification=EXACT, safe_to_verify=True,
                                contribution=contribution, stage_absorption_state="HISTORICAL_COMMIT_EXACT",
                                reason="durable parent+contribution+child equality across instruction and relation counters")
                else:
                    item.update(classification=PARTIAL, contribution=contribution,
                                stage_absorption_state="PARTIAL_HISTORICAL_DELTA",
                                reason="durable snapshots differ, but streamed contribution does not exactly equal all deltas")
            except (OSError, ValueError) as error:
                snapshot_delta = {"instruction_occurrences": expected_i,
                                  "relation_occurrences": expected_r,
                                  "source_evidence_available": False}
                snapshot_delta["contribution_hash"] = contribution_hash(snapshot_delta)
                item.update(classification=PARTIAL, contribution=snapshot_delta,
                            stage_absorption_state="PARTIAL_HISTORICAL_DELTA",
                            reason=f"durable snapshot pair found but independent FLOW replay failed: {error}")
            if item["contribution"]:
                item["instruction_identities"] = sorted(item["contribution"].get(
                    "instruction_occurrences", {}).keys())
                item["relation_identities"] = sorted(item["contribution"].get(
                    "relation_occurrences", {}).keys())
                item["terminal_contribution"] = int(item["contribution"].get(
                    "terminal_contribution", 0))
        if run_id == session_run_id:
            session_path = _entry_path(entry, "session_path")
            item["special_session_probe"] = session_probe(session_path, run_id)
            item["special_session_probe"]["disposition"] = "KEEP_STILL_AMBIGUOUS"
            item["reason"] += "; large session DB is closed/read-only and Stage 5 durable acceptance is absent"
        results.append(item)
    after = json.loads(read_master_v2_section(master_path, "rolling_master"))
    if canonical_bytes(before) != canonical_bytes(after) or sha256_file(master_path) != before_hashes["master"]:
        raise RuntimeError("STOP_FORENSIC_MASTER_MUTATION")
    counts = Counter(item["classification"] for item in results)
    bytes_by_class: dict[str, int] = {}
    for item in results:
        bytes_by_class[item["classification"]] = bytes_by_class.get(item["classification"], 0) + int(item["bulk_bytes"])
    return {"schema": "oasis.m12.v2.ambiguous-forensic-reconciliation.v1",
            "status": "PASS_V2_AMBIGUOUS_FORENSIC_RECONCILIATION_V1",
            "gameplay_started": False, "master_mutated": False, "legacy_reads": 0,
            "source_owned_delta": 0, "asm_delta": 0, "before_master_sha256": before_hashes["master"],
            "after_master_sha256": before_hashes["master"], "classification_counts": dict(counts),
            "bulk_bytes_by_class": bytes_by_class, "runs": results,
            "summary": {"ambiguous_input": len(entries), "deletions": 0,
                         "raw_bytes_deleted": 0, "large_session_run_id": session_run_id,
                         "bulk_bytes_total": sum(bytes_by_class.values())}}


__all__ = ["AMBIGUOUS", "EXACT", "NOT_COMMITTED", "PARTIAL", "Snapshot", "discover_snapshots",
           "exact_transition", "reconcile", "scan_flow", "session_probe"]


def main() -> int:
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--ledger", type=Path, required=True)
    parser.add_argument("--backlog", type=Path, required=True)
    parser.add_argument("--rolling-root", type=Path, required=True)
    parser.add_argument("--master", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--session-run-id", type=int, default=1789839425)
    args = parser.parse_args()
    report = reconcile(args.ledger, args.backlog, args.rolling_root, args.master,
                       args.session_run_id)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    temp = args.output.with_suffix(args.output.suffix + ".tmp")
    temp.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    temp.replace(args.output)
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
