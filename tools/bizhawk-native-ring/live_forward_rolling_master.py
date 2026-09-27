"""Compact sealed FLOW_V1 evidence into an atomic rolling master."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import os
import sqlite3
import struct
import sys
import time
import uuid

from live_forward_scaling_audit import RECORD, FLAG_INSTRUCTION
from live_forward_progress import ProgressPublisher, mark_failed
from live_forward_complete_pipeline import run_remaining


ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
MASTER_SCHEMA = "oasis.m12.rolling-master.v1"
PASS = "PASS_END_GAME_ROLLING_MASTER_COMPACT_V1"


class _Progressless:
    """Keep the same stage calls when a caller does not request a status file."""

    rows: dict = {}

    def start(self, *args, **kwargs):
        del args, kwargs

    def update(self, *args, **kwargs):
        del args, kwargs

    def finish(self, *args, **kwargs):
        del args, kwargs

    def heartbeat(self, *args, **kwargs):
        del args, kwargs

    def complete(self, *args, **kwargs):
        del args, kwargs


def _sha(path: Path, heartbeat=None) -> str:
    digest = hashlib.sha256()
    next_heartbeat = time.monotonic()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
            if heartbeat and time.monotonic() >= next_heartbeat:
                heartbeat(f"hashing {path.name}")
                next_heartbeat = time.monotonic() + 0.25
    return digest.hexdigest()


def _json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp-" + uuid.uuid4().hex)
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n",
                         encoding="utf-8", newline="\n")
    os.replace(temporary, path)


def _schema(db: sqlite3.Connection) -> None:
    db.executescript("""
    CREATE TABLE IF NOT EXISTS meta(key TEXT PRIMARY KEY, value TEXT NOT NULL);
    CREATE TABLE IF NOT EXISTS instruction(
      pc INTEGER NOT NULL, opcode INTEGER NOT NULL, bytes_sha256 TEXT NOT NULL,
      range_end INTEGER, range_status TEXT NOT NULL, occurrences INTEGER NOT NULL,
      witness_runs INTEGER NOT NULL, first_run INTEGER NOT NULL, last_run INTEGER NOT NULL,
      PRIMARY KEY(pc, opcode));
    CREATE TABLE IF NOT EXISTS edge(
      source_pc INTEGER NOT NULL, relation TEXT NOT NULL, target_pc INTEGER NOT NULL,
      occurrences INTEGER NOT NULL, witness_runs INTEGER NOT NULL,
      first_run INTEGER NOT NULL, last_run INTEGER NOT NULL,
      PRIMARY KEY(source_pc, relation, target_pc));
    CREATE TABLE IF NOT EXISTS run(
      run_id INTEGER PRIMARY KEY, receipt_sha256 TEXT NOT NULL, raw_sha256 TEXT NOT NULL,
      index_sha256 TEXT NOT NULL, segments INTEGER NOT NULL, raw_records INTEGER NOT NULL,
      unique_pcs INTEGER NOT NULL, unique_edges INTEGER NOT NULL, terminal_facts INTEGER NOT NULL,
      status TEXT NOT NULL);
    CREATE TABLE IF NOT EXISTS audit(
      run_id INTEGER PRIMARY KEY, raw_records_read INTEGER NOT NULL,
      accepted_runtime_occurrences INTEGER NOT NULL, unique_instruction_ranges INTEGER NOT NULL,
      unique_control_edges INTEGER NOT NULL, terminal_facts INTEGER NOT NULL,
      rejected_facts INTEGER NOT NULL, conflicts INTEGER NOT NULL, occurrence_totals INTEGER NOT NULL,
      witness_totals INTEGER NOT NULL);
    """)


def _meta(db: sqlite3.Connection, key: str, value: str) -> None:
    row = db.execute("SELECT value FROM meta WHERE key=?", (key,)).fetchone()
    if row is not None and row[0] != value:
        raise ValueError(f"STOP_ROLLING_MASTER_ROM_MISMATCH:{key}")
    db.execute("INSERT OR IGNORE INTO meta(key,value) VALUES (?,?)", (key, value))


def _seed_from_knowledge(db: sqlite3.Connection, path: Path, run_id: int) -> None:
    if not path.is_file():
        return
    source = sqlite3.connect(path)
    try:
        rows = source.execute("""SELECT r.start,r.end,o.attributes_json,
          COALESCE((SELECT SUM(e.fact_count) FROM evidence_ref e JOIN claim c ON c.claim_id=e.subject_id
                    WHERE c.object_id=o.object_id AND e.fact_kind='RUNTIME_INSTRUCTION_OCCURRENCE'),0)
          FROM rom_object o JOIN rom_range r ON r.range_id=o.range_id
          WHERE o.object_type='M68K_INSTRUCTION'""")
        for pc, end, attrs_text, occurrences in rows:
            attrs = json.loads(attrs_text)
            opcode = int(attrs.get("opcode", 0))
            digest = str(attrs.get("bytes_sha256", ""))
            db.execute("""INSERT OR IGNORE INTO instruction
              (pc,opcode,bytes_sha256,range_end,range_status,occurrences,witness_runs,first_run,last_run)
              VALUES (?,?,?,?,?,?,?,?,?)""", (pc, opcode, digest, end, "CANONICAL_KNOWLEDGE",
              int(occurrences), 1, 0, run_id))
        edges = source.execute("""SELECT rel.relation_type,sr.start,tr.start,rel.target_address,
          COALESCE((SELECT SUM(e.fact_count) FROM evidence_ref e WHERE e.subject_type='RELATION'
                    AND e.subject_id=rel.relation_id),0)
          FROM relation rel JOIN rom_object so ON so.object_id=rel.source_object_id
          JOIN rom_range sr ON sr.range_id=so.range_id
          LEFT JOIN rom_object tobj ON tobj.object_id=rel.target_object_id
          LEFT JOIN rom_range tr ON tr.range_id=tobj.range_id
          WHERE rel.relation_type IN ('EXECUTED_NEXT','OBSERVED_NEXT_PC')""")
        for relation, source_pc, target_pc, target_address, occurrences in edges:
            target = target_pc if relation == "EXECUTED_NEXT" else target_address
            if target is None:
                continue
            db.execute("""INSERT OR IGNORE INTO edge
              (source_pc,relation,target_pc,occurrences,witness_runs,first_run,last_run)
              VALUES (?,?,?,?,?,?,?)""", (source_pc, relation, target, int(occurrences),
              1, 0, run_id))
    finally:
        source.close()


def _upsert_instruction(db: sqlite3.Connection, pc: int, opcode: int, rom: bytes,
                        run_id: int, count: int, new_keys: set[tuple[int, int]]) -> None:
    key = (pc, opcode)
    digest = hashlib.sha256(rom[pc:pc + 2]).hexdigest()
    row = db.execute("SELECT occurrences,witness_runs,first_run FROM instruction WHERE pc=? AND opcode=?",
                     key).fetchone()
    if row is None:
        db.execute("""INSERT INTO instruction VALUES (?,?,?,?,?,?,?,?,?)""",
                   (pc, opcode, digest, None, "OBSERVED_PC_ONLY", count, 1, run_id, run_id))
        new_keys.add(key)
    else:
        witness = int(row[1]) + (0 if int(row[2]) == run_id else 1)
        db.execute("UPDATE instruction SET occurrences=?,witness_runs=?,last_run=? WHERE pc=? AND opcode=?",
                   (int(row[0]) + count, witness, run_id, pc, opcode))


def _upsert_edge(db: sqlite3.Connection, source: int, relation: str, target: int,
                 run_id: int, count: int, new_keys: set[tuple[int, str, int]]) -> None:
    key = (source, relation, target)
    row = db.execute("SELECT occurrences,witness_runs,first_run FROM edge WHERE source_pc=? AND relation=? AND target_pc=?",
                     key).fetchone()
    if row is None:
        db.execute("INSERT INTO edge VALUES (?,?,?,?,?,?,?)", (source, relation, target,
                   count, 1, run_id, run_id))
        new_keys.add(key)
    else:
        witness = int(row[1]) + (0 if int(row[2]) == run_id else 1)
        db.execute("UPDATE edge SET occurrences=?,witness_runs=?,last_run=? WHERE source_pc=? AND relation=? AND target_pc=?",
                   (int(row[0]) + count, witness, run_id, source, relation, target))


def compact(receipt_path: Path, master_root: Path, rom_path: Path,
            bootstrap_knowledge: Path | None = None, report_path: Path | None = None,
            status_path: Path | None = None, decoder: Path | None = None,
            progress=None, flow_session=None, flow_stats=None, stage6_result=None,
            range_tool: Path | None = None) -> dict:
    progress = progress or (ProgressPublisher(status_path) if status_path else _Progressless())
    if progress:
        progress.start("FINALIZING RUN", total=1, unit="receipt", detail="validating sealed run")
    receipt = _json(receipt_path)
    runtime = receipt.get("runtime", {})
    spool = receipt.get("raw_segment_spool") or receipt.get("flow_handoff", {})
    flow_stats = flow_stats or {}
    checkpoint = receipt.get("checkpoint")
    if checkpoint not in {"M12-LIVE-WORKER-INTERACTIVE", "M12-ROM-RANGE-LINKAGE-2B"}:
        raise ValueError("STOP_RUN_AUDIT_FAILED:unexpected checkpoint")
    allowed_outcomes = {"STOPPED_END_GAME", "STOPPED_AFTER_EMUHAWK_EXIT",
                        "STOPPED_DISK_RESERVE", "STOPPED_FRAME_LIMIT"}
    if checkpoint == "M12-ROM-RANGE-LINKAGE-2B":
        allowed_outcomes.add("PASS")
    if runtime.get("outcome") not in allowed_outcomes:
        raise ValueError("STOP_RUN_UNSEALED")
    if receipt.get("source_owned_delta", 0) != 0:
        raise ValueError("STOP_ROLLING_MASTER_CONFLICT:SOURCE_OWNED")
    if progress:
        progress.update(1, detail="sealed runtime receipt accepted")
        progress.finish(detail="sealed runtime receipt accepted")
    rom = rom_path.read_bytes()
    if len(rom) != 3_145_728 or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("STOP_ROLLING_MASTER_ROM_MISMATCH")
    in_memory = flow_session is not None
    input_raw_sha = (flow_stats.get("raw_sha256") if in_memory else spool.get("raw_sha256"))
    input_index_sha = (flow_stats.get("logical_sha256") if in_memory else spool.get("index_sha256"))
    raw = Path(spool.get("raw_path", ""))
    index = Path(spool.get("index_path", ""))
    if not in_memory and (not raw.is_file() or not index.is_file()):
        archive = receipt_path.parent / "raw-evidence-archive"
        if (archive / raw.name).is_file() and (archive / index.name).is_file():
            raw, index = archive / raw.name, archive / index.name
        else:
            raise ValueError("STOP_RUN_UNSEALED:spool_missing")
    if progress:
        total_records = int(flow_stats.get("records", 0)) if in_memory else raw.stat().st_size // RECORD.size
        progress.start("AUDITING FLOW", total=total_records, unit="FLOW records",
                       detail=("verifying in-memory FLOW handoff" if in_memory
                               else "verifying raw/index hashes"))
    beat = progress.heartbeat if progress else None
    if in_memory:
        if flow_stats.get("disk_reads") != 0 or flow_stats.get("disk_writes") != 0:
            raise ValueError("STOP_FLOW_IN_MEMORY_HANDOFF_DISK_IO")
    elif _sha(raw, beat) != spool.get("raw_sha256") or _sha(index, beat) != spool.get("index_sha256"):
        raise ValueError("STOP_RUN_AUDIT_FAILED:spool_hash")
    run_id = int(runtime.get("run_id", 0))
    if run_id <= 0:
        raise ValueError("STOP_RUN_AUDIT_FAILED:run_id")
    master_root = master_root.resolve()
    generations = master_root / "generations"
    generations.mkdir(parents=True, exist_ok=True)
    pointer = master_root / "current.json"
    previous = _json(pointer) if pointer.is_file() else None
    base_db = None
    if previous:
        base_db = master_root / previous["generation_dir"] / "master.sqlite"
        if not base_db.is_file() or _sha(base_db) != previous.get("master_sha256"):
            raise ValueError("STOP_ROLLING_MASTER_AUDIT_FAILED:current_pointer")
        report_file = master_root / previous["generation_dir"] / "report.json"
        if report_file.is_file():
            prev_report = _json(report_file)
            if prev_report.get("run_id") == run_id and prev_report.get("master_sha256") == previous.get("master_sha256"):
                final_generation = master_root / previous["generation_dir"]
                if progress:
                    for stage, row in prev_report.get("stages", {}).items():
                        if row.get("state") == "PASS":
                            progress.rows[stage] = dict(row)
                    progress.finish("PASS", detail="resumed from preserved rolling generation")
                report = prev_report
                if progress:
                    remaining = run_remaining(
                        receipt_path, report, final_generation,
                        master_root / "post-run-analysis", rom_path, progress, decoder,
                        flow_session=flow_session, flow_stats=flow_stats, stage6_result=stage6_result,
                        range_tool=range_tool)
                    report.update(remaining)
                    report["stage_results"] = remaining.get("stages", {})
                    report["stages"] = progress.rows
                    report["canonical_refresh"] = (
                        "STOPPED_FAIL_CLOSED" if remaining.get("stop") else "PASS")
                    _write_json(final_generation / "report.json", report)
                    previous["report_sha256"] = _sha(final_generation / "report.json")
                    _write_json(pointer, previous)
                    report["current_pointer_sha256"] = _sha(pointer)
                if report_path:
                    _write_json(report_path, report)
                if progress:
                    progress.complete(report=report)
                if report.get("pipeline_state") == "ANALYSIS COMPLETE ✓" and report_path:
                    from live_forward_postrun_receipts import emit_postrun_receipts
                    st_path = getattr(progress, "path", None) or getattr(progress, "status_path", None) or status_path
                    shas = emit_postrun_receipts(report_path.parent, receipt_path, report, st_path)
                    report["postrun_receipt_hashes"] = shas
                    _write_json(report_path, report)
                return report
    generation_id = "master-" + uuid.uuid4().hex[:16]
    staging = generations / (".staging-" + uuid.uuid4().hex)
    staging.mkdir()
    staged = staging / "master.sqlite"
    db = sqlite3.connect(staged)
    try:
        _schema(db)
        if base_db:
            if progress:
                progress.heartbeat("copying previous rolling generation")
            source = sqlite3.connect(base_db)
            source.backup(db)
            source.close()
        _schema(db)
        _meta(db, "schema", MASTER_SCHEMA)
        _meta(db, "rom_sha256", ROM_SHA)
        _meta(db, "rom_size", str(len(rom)))
        if not base_db and bootstrap_knowledge:
            if progress:
                progress.heartbeat("seeding accepted knowledge rows")
            _seed_from_knowledge(db, bootstrap_knowledge, run_id)
        segment_count = raw_records = terminal = rejected = 0
        pc_counts: dict[tuple[int, int], int] = {}
        edge_counts: dict[tuple[int, str, int], int] = {}
        if in_memory:
            pc_counts = {tuple(key): int(value.get("occurrence_count", 0))
                         for key, value in flow_session.instruction_occurrences.items()}
            edge_counts = {(int(source[0]), "EXECUTED_NEXT", int(target[0])):
                           int(value.get("occurrence_count", 0))
                           for (source, target), value in flow_session.executed_next.items()}
            segment_count = int(flow_stats.get("segments", flow_session.segments_processed))
            raw_records = int(flow_stats.get("records", flow_session.records_processed))
            terminal = int(getattr(flow_session, "terminal_observations",
                                   flow_stats.get("terminal_facts", segment_count)))
            if progress:
                progress.update(raw_records, processed_bytes=int(flow_stats.get("raw_bytes", 0)),
                                detail=f"validated {raw_records:,} FLOW records in memory")
        else:
            stream = index.open("r", encoding="utf-8")
            binary = raw.open("rb")
            try:
                for line in stream:
                    item = json.loads(line)
                    segment = item["segment"]
                    count = int(segment["record_count"])
                    binary.seek(int(item["raw_offset"]))
                    data = binary.read(int(item["raw_length"]))
                    if len(data) != count * RECORD.size or hashlib.sha256(data).hexdigest() != item["raw_sha256"]:
                        raise ValueError("STOP_RUN_AUDIT_FAILED:segment_bytes")
                    rows = list(RECORD.iter_unpack(data))
                    if len(rows) != count or any(right[0] != left[0] + 1 for left, right in zip(rows, rows[1:])):
                        raise ValueError("STOP_RUN_AUDIT_FAILED:segment_order")
                    instructions = [row for row in rows if (row[6] & FLAG_INSTRUCTION) and row[7] == 0]
                    for row in instructions:
                        pc, opcode = int(row[3]), int(row[5] & 0xFFFF)
                        if pc < 0 or pc + 2 > len(rom) or int.from_bytes(rom[pc:pc + 2], "big") != opcode:
                            raise ValueError("STOP_ROLLING_MASTER_ROM_MISMATCH:opcode")
                        pc_counts[(pc, opcode)] = pc_counts.get((pc, opcode), 0) + 1
                    for left, right in zip(instructions, instructions[1:]):
                        key = (int(left[3]), "EXECUTED_NEXT", int(right[3]))
                        edge_counts[key] = edge_counts.get(key, 0) + 1
                    if instructions:
                        key = (int(instructions[-1][3]), "OBSERVED_NEXT_PC",
                               int(instructions[-1][4]))
                        edge_counts[key] = edge_counts.get(key, 0) + 1
                        terminal += 1
                    segment_count += 1
                    raw_records += count
                    if progress:
                        progress.update(raw_records, processed_bytes=int(item["raw_offset"]) + len(data),
                                        detail=f"segments sealed {segment_count}")
            finally:
                stream.close()
                binary.close()
        if progress:
            progress.finish(detail=f"audited {raw_records:,} FLOW records")
            progress.start("MERGING MASTER", total=1, unit="generation",
                           detail="preparing rolling generation")
            progress.update(1, detail="rolling generation prepared")
            progress.finish(detail="rolling generation prepared")
            progress.start("COMPACTING EVIDENCE", total=len(pc_counts) + len(edge_counts),
                           unit="facts", detail="compacting instruction facts and edges")
        new_instructions: set[tuple[int, int]] = set()
        new_edges: set[tuple[int, str, int]] = set()
        merged_rows = 0
        for (pc, opcode), count in pc_counts.items():
            _upsert_instruction(db, pc, opcode, rom, run_id, count, new_instructions)
            merged_rows += 1
            if progress:
                progress.update(merged_rows, detail="instruction facts merged")
        for (source, relation, target), count in edge_counts.items():
            _upsert_edge(db, source, relation, target, run_id, count, new_edges)
            merged_rows += 1
            if progress:
                progress.update(merged_rows, detail="control edges merged")
        db.execute("INSERT INTO run VALUES (?,?,?,?,?,?,?,?,?,?)", (run_id, _sha(receipt_path),
            input_raw_sha, input_index_sha, segment_count, raw_records,
            len(pc_counts), len(edge_counts), terminal, str(runtime.get("outcome"))))
        db.execute("INSERT INTO audit VALUES (?,?,?,?,?,?,?,?,?,?)", (run_id, raw_records,
            sum(pc_counts.values()), len(pc_counts), len(edge_counts), terminal, rejected, 0,
            sum(pc_counts.values()), len({run_id})))
        db.commit()
        integrity = db.execute("PRAGMA integrity_check").fetchone()[0]
        if integrity != "ok":
            raise ValueError("STOP_ROLLING_MASTER_AUDIT_FAILED:integrity")
        if progress:
            progress.finish(detail="source rows compacted into generation")
    finally:
        db.close()
    final_generation = generations / generation_id
    os.replace(staging, final_generation)
    master_sha = _sha(final_generation / "master.sqlite")
    report = {"status": PASS, "schema": MASTER_SCHEMA, "generation_id": generation_id,
              "rom_sha256": ROM_SHA, "run_id": run_id, "input_receipt_sha256": _sha(receipt_path),
              "input_raw_sha256": input_raw_sha, "input_index_sha256": input_index_sha,
              "raw_records_read": raw_records, "accepted_runtime_occurrences": sum(pc_counts.values()),
              "unique_instruction_ranges": len(pc_counts), "unique_control_edges": len(edge_counts),
              "terminal_facts": terminal, "rejected_facts": rejected, "conflicts": 0,
              "occurrence_totals": sum(pc_counts.values()), "witness_totals": 1,
              "new_instructions": len(new_instructions), "new_edges": len(new_edges),
              "master_sha256": master_sha, "master_bytes": (final_generation / "master.sqlite").stat().st_size,
              "cleanup": {"status": ("IN_MEMORY_FLOW_NO_RAW_ARTIFACT"
                                       if in_memory else "RAW_RETAINED_REQUIRED_BY_2I_ROLLBACK_POLICY"),
                          "deleted": [], "retained": ([] if in_memory else [str(raw), str(index)])},
              "flow_handoff": ({**flow_stats, "mode": "IN_MEMORY_STREAM",
                                "segments": segment_count, "records": raw_records}
                               if in_memory else
                               {"mode": "DISK_SPOOL", "disk_reads": 2, "disk_writes": 2,
                                "segments": segment_count, "records": raw_records,
                                "raw_bytes": raw.stat().st_size, "fallback": "DISABLED"}),
              "source_owned_delta": 0, "canonical_refresh": "PENDING_POSTRUN_PIPELINE",
              "pipeline_state": "ANALYSIS RUNNING…",
              "stages": progress.rows if progress else {},
              "heartbeat_frequency_hz": 4.0 if progress else None,
              "ui_refresh_frequency_hz": 4.0,
              "indeterminate_stages": ["CONTROL PROVENANCE", "ASM CLOSURE",
                                       "FULL ROM AUDIT", "CLEANUP"]}
    _write_json(final_generation / "report.json", report)
    pointer_value = {"schema": MASTER_SCHEMA, "generation_id": generation_id,
                     "generation_dir": str(final_generation.relative_to(master_root)),
                     "master_sha256": master_sha, "report_sha256": _sha(final_generation / "report.json"),
                     "rom_sha256": ROM_SHA, "status": PASS}
    _write_json(pointer, pointer_value)
    report["current_pointer"] = str(pointer)
    report["current_pointer_sha256"] = _sha(pointer)
    if progress:
        remaining = run_remaining(
            receipt_path, report, final_generation,
            master_root / "post-run-analysis", rom_path, progress, decoder,
            flow_session=flow_session, flow_stats=flow_stats, stage6_result=stage6_result,
            range_tool=range_tool)
        report.update(remaining)
        report["stage_results"] = remaining.get("stages", {})
        report["stages"] = progress.rows
        report["canonical_refresh"] = (
            "STOPPED_FAIL_CLOSED" if remaining.get("stop") else "PASS")
        _write_json(final_generation / "report.json", report)
        pointer_value["report_sha256"] = _sha(final_generation / "report.json")
        _write_json(pointer, pointer_value)
        report["current_pointer_sha256"] = _sha(pointer)
    if report_path:
        _write_json(report_path, report)
    if progress:
        progress.complete(report=report)
    if report.get("pipeline_state") in {"ANALYSIS COMPLETE ✓",
                                         "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"} and report_path:
        try:
            from live_forward_postrun_receipts import emit_postrun_receipts
            st_path = getattr(progress, "path", None) or getattr(progress, "status_path", None) or status_path
            shas = emit_postrun_receipts(report_path.parent, receipt_path, report, st_path)
            report["postrun_receipt_hashes"] = shas
            _write_json(report_path, report)
        except Exception:
            pass
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--master-root", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--bootstrap-knowledge", type=Path)
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--status", type=Path)
    parser.add_argument("--decoder", type=Path)
    parser.add_argument("--range-tool", type=Path,
                        help="Stage 7 range decoder (oasis_re_assemble_range)")
    args = parser.parse_args()
    if args.decoder is None:
        dec = Path(__file__).parents[2] / "build" / "oasis_re_rom_range_decode.exe"
        if dec.is_file():
            args.decoder = dec
    if args.range_tool is None:
        rng = Path(__file__).parents[2] / "build" / "oasis_re_assemble_range.exe"
        if rng.is_file():
            args.range_tool = rng
    try:
        print(json.dumps(compact(args.receipt, args.master_root, args.rom,
                                 args.bootstrap_knowledge, args.report, args.status,
                                 args.decoder, range_tool=args.range_tool), indent=2))
        return 0
    except Exception as error:
        if args.status:
            mark_failed(args.status, str(error), 2)
        failure = {"status": str(error).split(":", 1)[0], "error": str(error)}
        _write_json(args.report, failure)
        print(json.dumps(failure, indent=2))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
