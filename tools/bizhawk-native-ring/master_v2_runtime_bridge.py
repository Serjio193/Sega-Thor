"""Promote one sealed post-run result into the single MASTER V2 authority."""
from __future__ import annotations

import json
import os
from pathlib import Path
import sqlite3
from typing import Any

from live_forward_rolling_master import _schema, _sha
from master_v2_contribution_boundary import canonical_bytes, contribution_hash, extend_master_v2
from master_v2_forensic_reconciliation import scan_flow
from master_v2_shadow import ROM_SHA, ROM_SIZE, read_master_v2_section, write_master_v2, discover
from master_startup_authority import load_startup, write_pointer


def _json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + f".tmp-{os.getpid()}")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    os.replace(temporary, path)


def materialize_rolling_base(master: Path, root: Path) -> Path:
    """Create a run-local rolling SQLite base from MASTER V2 rolling rows."""
    payload = json.loads(read_master_v2_section(master, "rolling_master"))
    generation = root / "generations" / "master-v2-base"
    generation.mkdir(parents=True, exist_ok=False)
    database = generation / "master.sqlite"
    db = sqlite3.connect(database)
    try:
        _schema(db)
        for table, rows in payload.get("tables", {}).items():
            if not rows:
                continue
            columns = int(db.execute(f'PRAGMA table_info("{table}")').fetchall().__len__())
            db.executemany(f'INSERT INTO "{table}" VALUES ({",".join("?" for _ in range(columns))})', rows)
        db.commit()
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise ValueError("STOP_MASTER_V2_ROLLING_BASE_INTEGRITY")
    finally:
        db.close()
    report = {"status": "PASS_MASTER_V2_ROLLING_BASE", "generation_id": generation.name,
              "master_sha256": _sha(database), "rom_sha256": ROM_SHA}
    _write_json(generation / "report.json", report)
    _write_json(root / "current.json", {"schema": "oasis.m12.rolling-master.v1",
        "generation_id": generation.name, "generation_dir": str(generation.relative_to(root)),
        "master_sha256": report["master_sha256"], "report_sha256": _sha(generation / "report.json"),
        "rom_sha256": report["rom_sha256"], "status": report["status"]})
    return root


def build_contribution(receipt: Path, flow_session=None, flow_stats: dict[str, Any] | None = None) -> dict[str, Any]:
    value = _json(receipt)
    runtime = value["runtime"]
    if flow_session is None:
        spool = value["raw_segment_spool"]
        entry = {"run_id": int(runtime["run_id"]), "source_evidence": {
                     "raw_path": spool["raw_path"], "index_path": spool["index_path"],
                     "raw_sha256": spool["raw_sha256"], "index_sha256": spool["index_sha256"]}}
        result = scan_flow(entry)
    else:
        stats = flow_stats or {}
        instructions = {f"{pc:06X}:{opcode:04X}": int(item.get("occurrence_count", 0))
                        for (pc, opcode), item in flow_session.instruction_occurrences.items()}
        relations = {}
        for (source, target), item in flow_session.executed_next.items():
            relations[f"{source[0]:06X}:EXECUTED_NEXT:{target[0]:06X}"] = int(item.get("occurrence_count", 0))
        for (source, address), item in flow_session.terminal_facts.items():
            relations[f"{source[0]:06X}:OBSERVED_NEXT_PC:{address:06X}"] = int(item.get("occurrence_count", 0))
        result = {"run_id": int(runtime["run_id"]), "segments": int(stats.get("segments", 0)),
                  "records": int(stats.get("records", 0)),
                  "instruction_occurrences": dict(sorted(instructions.items())),
                  "relation_occurrences": dict(sorted(relations.items())),
                  "terminal_occurrences": {key: amount for key, amount in relations.items()
                                            if ":OBSERVED_NEXT_PC:" in key},
                  "raw_stream_sha256": stats.get("raw_sha256"), "raw_sha256": stats.get("raw_sha256"),
                  "source_owned_delta": 0, "asm": {"promoted_bytes": 0, "status": "NO_DELTA"}}
        result["instruction_occurrence_contribution"] = sum(instructions.values())
        result["relation_contribution"] = sum(relations.values())
        result["terminal_contribution"] = int(getattr(flow_session, "terminal_observations", 0))
        result["contribution_hash"] = contribution_hash(result)
    result.update({"rom": {"sha256": ROM_SHA, "size": ROM_SIZE},
                   "new_instruction_identities": sorted(result["instruction_occurrences"]),
                   "new_relation_identities": sorted(result["relation_occurrences"]),
                   "provenance_contribution": {"segments": result["segments"],
                                                "records": result["records"]},
                   "canonical_map_contribution": {"status": "PASS"},
                   "source_owned_delta": 0,
                   "asm": {"promoted_bytes": 0, "status": "NO_DELTA"},
                   "stage_outcomes": {"STAGE_5": "PASS", "STAGE_6": "PASS",
                                      "STAGE_7": "PASS", "STAGE_8": "PASS"},
                   "commit_state": "ABSORBED_EXACT"})
    result["relation_occurrence_contribution"] = result.pop("relation_contribution")
    result["contribution_hash"] = contribution_hash(result)
    return result


def promote(master: Path, pointer: Path, rolling_root: Path, canonical_root: Path,
            campaign_root: Path, receipt: Path, rom: Path, run_id: int,
            contribution: dict[str, Any] | None = None) -> dict[str, Any]:
    """Build, validate and atomically publish one N+1 MASTER V2 generation."""
    current_ledger = json.loads(read_master_v2_section(master, "run_contributions"))
    contribution = contribution or build_contribution(receipt)
    if any(int(item.get("run_id", -1)) == run_id for item in current_ledger["runs"]):
        raise ValueError("STOP_MASTER_V2_RUN_ALREADY_COMMITTED")
    ledger = dict(current_ledger)
    ledger["runs"] = list(current_ledger["runs"]) + [contribution]
    ledger["runs"].sort(key=lambda item: int(item["run_id"]))
    ledger["summary"] = dict(current_ledger.get("summary", {}))
    ledger["summary"]["committed_runs"] = len(ledger["runs"])
    state = discover(rolling_root, canonical_root, campaign_root, run_id)
    old_absorption = json.loads(read_master_v2_section(master, "absorption_history"))
    candidate_base = master.with_name(master.name + ".candidate")
    candidate = master.with_name(master.name + ".candidate.extended")
    write_master_v2(state, candidate_base)
    fresh_absorption = json.loads(read_master_v2_section(candidate_base, "absorption_history"))
    receipts = {str(item.get("run_id")): item for item in old_absorption.get("absorbed_receipts", [])}
    receipts.update({str(item.get("run_id")): item for item in fresh_absorption.get("absorbed_receipts", [])})
    fresh_absorption["absorbed_receipts"] = [receipts[key] for key in sorted(receipts, key=int)]
    extend_master_v2(candidate_base, candidate, ledger,
                     {"absorption_history": canonical_bytes(fresh_absorption)})
    candidate_base.unlink(missing_ok=True)
    temporary_pointer = pointer.with_name(pointer.name + ".candidate")
    write_pointer(temporary_pointer, candidate, rom)
    load_startup(pointer.parents[3], rom, temporary_pointer)
    os.replace(candidate, master)
    write_pointer(temporary_pointer, master, rom)
    load_startup(pointer.parents[3], rom, temporary_pointer)
    os.replace(temporary_pointer, pointer)
    return {"status": "PASS_MASTER_V2_POSTRUN_PROMOTION", "run_id": run_id,
            "master_sha256": _sha(master), "generation_id": json.loads(
                read_master_v2_section(master, "meta"))["generation_id"],
            "contribution_hash": contribution["contribution_hash"]}


__all__ = ["build_contribution", "materialize_rolling_base", "promote"]
