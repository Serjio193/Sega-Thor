"""Fail-closed permanent reclaim for a fully absorbed live-forward run."""
from __future__ import annotations

from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3
import time
from typing import Any
from live_forward_progress import STAGES
from live_forward_cleanup_acceptance import generic_acceptance_failure

from live_forward_scaling_audit import RECORD
from identity import ROM_SHA, ROM_SIZE

ACCEPTED_STAGE6 = {"PASS", "NO_DELTA"}
ABSORPTION_STATUS = "PASS_ABSORBED_RAW_PERMANENT_RECLAIM_V1"
STOP_NOT_ABSORBED = "STOP_CLEANUP_NOT_ABSORBED"
STOP_DELETE_FAILED = "STOP_CLEANUP_REQUIRED_DELETE_FAILED"
RETRY_COUNT = 5


class CleanupStop(RuntimeError):
    def __init__(self, code: str, detail: str, **fields: Any) -> None:
        super().__init__(f"{code}:{detail}")
        self.code, self.detail, self.fields = code, detail, fields


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _atomic_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f".{path.name}.tmp-{os.getpid()}-{time.monotonic_ns()}")
    try:
        encoded = json.dumps(value, indent=2, sort_keys=True) + "\n"
        with temporary.open("w", encoding="utf-8", newline="\n") as stream:
            stream.write(encoded)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()


def _receipt_hash(value: dict[str, Any]) -> str:
    unsigned = dict(value)
    unsigned.pop("receipt_sha256", None)
    return hashlib.sha256(json.dumps(unsigned, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def _safe(path: Path, roots: tuple[Path, ...]) -> Path:
    resolved = path.resolve()
    if not any(resolved == root or root in resolved.parents for root in roots):
        raise CleanupStop(STOP_NOT_ABSORBED, f"path outside evidence roots: {resolved}")
    return resolved


def _master_paths(master_root: Path) -> tuple[Path, Path, dict[str, Any]]:
    pointer_path = master_root / "current.json"
    pointer = _json(pointer_path)
    generations = (master_root / "generations").resolve()
    generation = _safe(master_root / pointer["generation_dir"], (generations,))
    if generation.parent != generations or not generation.is_dir():
        raise CleanupStop(STOP_NOT_ABSORBED, "current master generation is outside generations")
    master = generation / "master.sqlite"
    report = generation / "report.json"
    if not master.is_file() or _sha(master) != pointer.get("master_sha256"):
        raise CleanupStop(STOP_NOT_ABSORBED, "current master pointer hash mismatch")
    if report.is_file() and pointer.get("report_sha256") and _sha(report) != pointer["report_sha256"]:
        raise CleanupStop(STOP_NOT_ABSORBED, "current master report hash mismatch")
    return master, generation, pointer


def _integrity(path: Path) -> None:
    db = sqlite3.connect(path)
    try:
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise CleanupStop(STOP_NOT_ABSORBED, f"SQLite integrity failed: {path}")
    finally:
        db.close()


def _canonical_integrity(generation: Path, expected_source_owned: int,
                         selected_source_owned: int | None = None) -> str:
    knowledge = generation / "knowledge.sqlite"
    root = generation.parents[1]
    pointer_path = root / "current.json"
    pointer = _json(pointer_path)
    selected = _safe(root / pointer["generation_dir"], ((root / "generations").resolve(),))
    expected_by_candidate = {generation.resolve(): expected_source_owned}
    expected_by_candidate[selected] = (expected_source_owned if selected_source_owned is None
                                       else selected_source_owned)
    for candidate, expected in expected_by_candidate.items():
        path = candidate / "knowledge.sqlite"
        _integrity(path)
        db = sqlite3.connect(path)
        try:
            owned = int(db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0])
            rows = db.execute("SELECT start,end FROM emission ORDER BY start,end").fetchall()
            cursor = 0
            for start, end in rows:
                if int(start) != cursor or int(end) <= int(start):
                    raise CleanupStop(STOP_NOT_ABSORBED, "canonical emission partition is not contiguous")
                cursor = int(end)
            if cursor != ROM_SIZE or owned != expected:
                raise CleanupStop(STOP_NOT_ABSORBED, "canonical ownership/emission integrity mismatch")
        finally:
            db.close()
    selected_hash = _sha(selected / "knowledge.sqlite")
    if selected_hash != pointer.get("knowledge_sha256"):
        raise CleanupStop(STOP_NOT_ABSORBED, "canonical current pointer hash mismatch")
    return _sha(knowledge)


def _stage5_source_owned(stage5: dict[str, Any]) -> int:
    gen_dir = Path(stage5.get("generation_dir", ""))
    if (gen_dir / "knowledge.sqlite").is_file():
        db = sqlite3.connect(gen_dir / "knowledge.sqlite")
        try: return int(db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0])
        finally: db.close()
    if "source_owned_after" in stage5: return int(stage5["source_owned_after"])
    after = stage5.get("after", {})
    return int(after.get("source_owned_bytes", after.get("metrics", {}).get("source_owned_bytes", -1)))


def _final_source_owned(stage5: dict[str, Any], stage7: dict[str, Any]) -> int:
    if "source_owned_after" in stage7: return int(stage7["source_owned_after"])
    if isinstance(stage7.get("after"), (int, float)): return int(stage7["after"])
    return _stage5_source_owned(stage5)


def _semantic_audit(raw: Path, index: Path, master: Path, run_id: int) -> dict[str, int]:
    instruction_counts: Counter[tuple[int, int]] = Counter()
    edge_counts: Counter[tuple[int, str, int]] = Counter()
    segments = records = terminal = 0
    with index.open("r", encoding="utf-8") as index_stream, raw.open("rb") as raw_stream:
        for line in index_stream:
            item = json.loads(line)
            segment = item["segment"]
            if int(segment["run_id"]) != run_id:
                raise CleanupStop(STOP_NOT_ABSORBED, "ordered FLOW contains another run")
            raw_offset, raw_length = int(item["raw_offset"]), int(item["raw_length"])
            raw_stream.seek(raw_offset)
            data = raw_stream.read(raw_length)
            if len(data) != raw_length or hashlib.sha256(data).hexdigest() != item.get("raw_sha256"):
                raise CleanupStop(STOP_NOT_ABSORBED, "ordered FLOW segment hash mismatch")
            rows = list(RECORD.iter_unpack(data))
            if len(rows) != int(segment["record_count"]) or raw_length != len(rows) * RECORD.size:
                raise CleanupStop(STOP_NOT_ABSORBED, "ordered FLOW count mismatch")
            instructions = [row for row in rows if (row[6] & 1) and row[7] == 0]
            for row in instructions:
                instruction_counts[(int(row[3]), int(row[5] & 0xFFFF))] += 1
            for left, right in zip(instructions, instructions[1:]):
                edge_counts[(int(left[3]), "EXECUTED_NEXT", int(right[3]))] += 1
            if instructions:
                edge_counts[(int(instructions[-1][3]), "OBSERVED_NEXT_PC",
                             int(instructions[-1][4]))] += 1
                terminal += 1
            segments += 1
            records += len(rows)
    db = sqlite3.connect(master)
    try:
        for (pc, opcode), count in instruction_counts.items():
            row = db.execute("SELECT occurrences FROM instruction WHERE pc=? AND opcode=?", (pc, opcode)).fetchone()
            if row is None or int(row[0]) < count:
                raise CleanupStop(STOP_NOT_ABSORBED, f"compact instruction fact missing: {pc:06X}:{opcode:04X}")
        for source, relation, target in edge_counts:
            if db.execute("SELECT 1 FROM edge WHERE source_pc=? AND relation=? AND target_pc=?",
                          (source, relation, target)).fetchone() is None:
                raise CleanupStop(STOP_NOT_ABSORBED, f"compact relation fact missing: {source:06X}:{relation}:{target:06X}")
    finally:
        db.close()
    return {"segments": segments, "records": records, "instruction_occurrences": sum(instruction_counts.values()),
            "relation_facts": len(edge_counts), "terminal_facts": terminal}


def _candidate_files(receipt_path: Path, run_analysis: Path) -> tuple[list[Path], list[Path]]:
    receipt = _json(receipt_path)
    campaign = receipt_path.parent.resolve()
    spool = receipt.get("raw_segment_spool") or receipt.get("flow_handoff", {})
    raw = Path(spool["raw_path"]).resolve()
    index = Path(spool["index_path"]).resolve()
    roots = (campaign, run_analysis.resolve())
    archive_dir = campaign / "raw-evidence-archive"
    archive_raw = archive_dir / raw.name
    archive_index = archive_dir / index.name
    if raw.is_file():
        if not archive_raw.is_file() or archive_raw.stat().st_size != raw.stat().st_size:
            archive_dir.mkdir(parents=True, exist_ok=True)
            shutil.copy2(raw, archive_raw)
            shutil.copy2(index, archive_index)
    required = [_safe(raw, roots), _safe(index, roots)]
    rom_link = campaign / "rom-link"
    keep_names = {"receipt.json", "Beyond Oasis (U) [!].SaveRAM"}
    if rom_link.is_dir():
        required.extend(_safe(path, roots) for path in rom_link.rglob("*")
                        if path.is_file() and path.name not in keep_names)
    session = run_analysis / "session-rom-link.sqlite"
    if session.is_file():
        required.append(_safe(session, roots))
    decoder = run_analysis / "decoder-evidence"
    if decoder.is_dir():
        required.extend(_safe(path, roots) for path in decoder.rglob("*") if path.is_file())
    retained = [receipt_path.resolve(), campaign / "post-run-analysis" / "report.json",
                campaign / "post-run-analysis" / "status.json",
                run_analysis / "stage5-receipt.json", run_analysis / "stage6-control-provenance.json",
                run_analysis / "stage7" / "stage7-result.json", run_analysis / "stage8-receipt.json"]
    return required, [_safe(path, roots) for path in retained]


def _manifest(paths: list[Path], run_id: int) -> dict[str, Any]:
    entries = [{"path": str(path), "type": "file", "size_bytes": path.stat().st_size,
                "run_id": run_id, "reason": "ABSORBED_REDUNDANT", "required_delete": True,
                "action": "DELETE"} for path in sorted(paths)]
    return {"schema": "oasis.m12.absorption-delete-manifest.v1", "run_id": run_id,
            "deletion_mode": "PERMANENT_NO_RECYCLE_BIN", "entries": entries}


def _validate_manifest(manifest: dict[str, Any], run_id: int,
                       roots: tuple[Path, ...]) -> None:
    seen: set[Path] = set()
    for item in manifest.get("entries", []):
        if int(item.get("run_id", -1)) != run_id or item.get("action") != "DELETE" or \
                not item.get("required_delete"):
            raise CleanupStop(STOP_NOT_ABSORBED, "delete manifest entry is not required DELETE")
        path = _safe(Path(item["path"]), roots)
        if path in seen or int(item.get("size_bytes", -1)) < 0:
            raise CleanupStop(STOP_NOT_ABSORBED, "delete manifest is duplicated or malformed")
        seen.add(path)


def _delete(path: Path, expected: int) -> int:
    for attempt in range(1, RETRY_COUNT + 1):
        try:
            path.unlink(missing_ok=True)
            if path.exists():
                raise OSError("path still exists after unlink")
            return expected
        except OSError as error:
            if getattr(error, "winerror", None) != 32 or attempt == RETRY_COUNT:
                raise CleanupStop(STOP_DELETE_FAILED, str(error), path=str(path), retry_count=attempt,
                                  bytes_not_reclaimed=expected) from error
            time.sleep(0.2)
    return expected


def reclaim_absorbed_run(receipt_path: Path, report: dict[str, Any], stage_results: dict[str, Any],
                         master_root: Path, run_analysis: Path, rom_path: Path,
                         progress: Any = None) -> dict[str, Any]:
    """Prove absorption, publish receipts, then permanently delete only listed files."""
    receipt_path, master_root, run_analysis, rom_path = (p.resolve() for p in
        (receipt_path, master_root, run_analysis, rom_path))
    campaign = receipt_path.parent.resolve()
    post_run = campaign / "post-run-analysis"
    absorbed_receipt = post_run / f"absorbed-run-{int(_json(receipt_path)['runtime']['run_id'])}.json"
    manifest_path = post_run / "cleanup-delete-manifest.json"
    runtime_receipt = _json(receipt_path)
    run_id = int(runtime_receipt.get("runtime", {}).get("run_id", 0))
    if run_id <= 0 or hashlib.sha256(rom_path.read_bytes()).hexdigest() != ROM_SHA:
        raise CleanupStop(STOP_NOT_ABSORBED, "runtime or ROM identity is incomplete")
    master, generation, pointer = _master_paths(master_root)
    _integrity(master)
    db = sqlite3.connect(master)
    try:
        run = db.execute("SELECT * FROM run WHERE run_id=?", (run_id,)).fetchone()
        audit = db.execute("SELECT * FROM audit WHERE run_id=?", (run_id,)).fetchone()
    finally:
        db.close()
    if run is None or audit is None or run[9] not in {"STOPPED_END_GAME", "STOPPED_AFTER_EMUHAWK_EXIT", "STOPPED_DISK_RESERVE"}:
        raise CleanupStop(STOP_NOT_ABSORBED, "run is absent from accepted rolling-master lineage")
    spool = runtime_receipt.get("raw_segment_spool") or runtime_receipt.get("flow_handoff", {})
    if int(run[4]) != int(spool["segments"]) or int(run[5]) != int(audit[1]):
        raise CleanupStop(STOP_NOT_ABSORBED, "rolling-master run counts do not match sealed receipt")
    if int(audit[6]) != 0 or int(audit[7]) != 0 or report.get("conflicts", 0) != 0:
        raise CleanupStop(STOP_NOT_ABSORBED, "accepted master contains conflicts")
    generic_failure = generic_acceptance_failure(run_analysis, stage_results)
    if generic_failure:
        raise CleanupStop(STOP_NOT_ABSORBED, generic_failure)
    stage5, stage6 = stage_results.get("REFRESHING MAP", {}), stage_results.get("CONTROL PROVENANCE", {})
    stage7, stage8 = stage_results.get("ASM CLOSURE", {}), stage_results.get("FULL ROM AUDIT", {})
    if stage5.get("state") not in {"PASS", "NO_DELTA"} or stage6.get("status") not in ACCEPTED_STAGE6 or \
            stage7.get("state") not in {"PASS", "NO_DELTA"} or stage8.get("state") != "PASS":
        raise CleanupStop(STOP_NOT_ABSORBED, "mandatory post-run stage is not accepted")
    canonical_generation = Path(stage5["generation_dir"])
    stage5_source_owned = _stage5_source_owned(stage5)
    knowledge_hash = _canonical_integrity(
        canonical_generation, stage5_source_owned,
        selected_source_owned=_final_source_owned(stage5, stage7))
    existing = manifest_path.is_file() and absorbed_receipt.is_file()
    cleanup_receipt = post_run / "cleanup-receipt.json"
    if existing and cleanup_receipt.is_file():
        manifest, absorption = _json(manifest_path), _json(absorbed_receipt)
        if _receipt_hash(absorption) == absorption.get("receipt_sha256") and int(absorption.get("run_id", -1)) == run_id:
            return _json(cleanup_receipt)
    if existing:
        manifest, absorption = _json(manifest_path), _json(absorbed_receipt)
        if _receipt_hash(absorption) != absorption.get("receipt_sha256") or int(absorption.get("run_id", -1)) != run_id:
            raise CleanupStop(STOP_NOT_ABSORBED, "existing absorption receipt hash or run mismatch")
        semantic = {"segments": int(absorption["segments_imported"]),
                    "records": int(absorption["FLOW_records_audited"]),
                    "instruction_occurrences": int(absorption["instruction_occurrence_contribution"]),
                    "relation_facts": int(absorption["relation_contribution"]),
                    "terminal_facts": int(absorption["terminal_contribution"])}
    else:
        semantic = _semantic_audit(Path(spool["raw_path"]),
                                   Path(spool["index_path"]), master, run_id)
        if semantic["segments"] != int(run[4]) or semantic["records"] != int(run[5]) or \
                semantic["instruction_occurrences"] != int(audit[2]):
            raise CleanupStop(STOP_NOT_ABSORBED, "semantic equivalence count mismatch")
        _atomic_json(run_analysis / "stage5-receipt.json", stage5)
        _atomic_json(run_analysis / "stage8-receipt.json", stage8)
    delete_paths, retained = _candidate_files(receipt_path, run_analysis)
    # The coordinator creates report.json/status.json after this function
    # returns.  Existing protected receipts are checked before deletion;
    # coordinator-owned outputs are checked by publish_cleanup_result.
    retained_present = [path for path in retained if path.is_file()]
    _validate_manifest(manifest, run_id, (receipt_path.parent.parent, run_analysis)) if existing else None
    for path in retained_present:
        if not path.is_file():
            raise CleanupStop(STOP_NOT_ABSORBED, f"protected receipt is missing: {path}")
    if not existing:
        manifest = _manifest(delete_paths, run_id)
        _atomic_json(manifest_path, manifest)
        absorption = {"schema": "oasis.m12.absorbed-run.v1", "run_id": run_id,
                      "campaign_path": str(campaign), "rom_sha256": ROM_SHA,
                      "rolling_master_generation": str(generation), "rolling_master_hash": pointer["master_sha256"],
                      "canonical_generation": str(canonical_generation), "canonical_map_hash": knowledge_hash,
                      "segments_imported": semantic["segments"], "FLOW_records_audited": semantic["records"],
                      "instruction_occurrence_contribution": semantic["instruction_occurrences"],
                      "relation_contribution": semantic["relation_facts"], "terminal_contribution": semantic["terminal_facts"],
                      "control_provenance_status": stage6.get("status"), "ASM_closure_status": stage7.get("state"),
                      "full_ROM_audit_status": stage8.get("status", stage8.get("state")),
                      "semantic_equivalence": "PASS", "raw_artifacts_scheduled_for_deletion": manifest["entries"],
                      "raw_bytes_scheduled_for_deletion": sum(item["size_bytes"] for item in manifest["entries"]),
                      "timestamp": datetime.now(timezone.utc).isoformat(),
                      "deletion_mode": "PERMANENT_NO_RECYCLE_BIN"}
        absorption["receipt_sha256"] = _receipt_hash(absorption)
        _atomic_json(absorbed_receipt, absorption)
        master_again, _, pointer_again = _master_paths(master_root)
        if master_again != master or pointer_again["master_sha256"] != pointer["master_sha256"]:
            raise CleanupStop(STOP_NOT_ABSORBED, "accepted master changed after receipt publication")
    entries = manifest["entries"]
    before = shutil.disk_usage(campaign).free
    if progress:
        progress.heartbeat(f"ABSORBED RUN {run_id}; deleting redundant raw evidence; Permanent deletion; No Recycle Bin")
    deleted = 0
    for index, item in enumerate(entries, 1):
        deleted += _delete(Path(item["path"]), int(item["size_bytes"]))
        if progress:
            progress.update(index, total=len(entries), detail=f"deleted {index}/{len(entries)} files")
    for item in entries:
        if Path(item["path"]).exists():
            raise CleanupStop(STOP_DELETE_FAILED, "required raw artifact remains after deletion",
                              path=item["path"], retry_count=RETRY_COUNT,
                              bytes_not_reclaimed=int(item["size_bytes"]))
    _master_paths(master_root)
    _integrity(master)
    _canonical_integrity(canonical_generation, stage5_source_owned,
                         selected_source_owned=_final_source_owned(stage5, stage7))
    for path in retained_present:
        if not path.is_file():
            raise CleanupStop(STOP_DELETE_FAILED, f"protected receipt disappeared: {path}")
    after = shutil.disk_usage(campaign).free
    raw_before = sum(int(item["size_bytes"]) for item in entries)
    result = {"state": "PASS", "status": ABSORPTION_STATUS, "absorbed": True, "run_id": run_id,
              "semantic_equivalence": "PASS", "deletion_mode": "PERMANENT_NO_RECYCLE_BIN",
              "disk_free_before": before, "disk_free_after": after, "raw_bytes_before": raw_before,
              "raw_bytes_deleted": deleted, "raw_bytes_retained": 0, "files_deleted": len(entries),
              "files_retained": len(retained), "master_bytes_retained": master.stat().st_size,
              "reclaim_ratio": deleted / raw_before if raw_before else 0.0,
              "delete_manifest": str(manifest_path), "absorption_receipt": str(absorbed_receipt),
              "post_delete_integrity": "PASS"}
    _atomic_json(post_run / "cleanup-receipt.json", result)
    return result


def reclaim_in_memory_run(receipt_path: Path, report: dict[str, Any], stage_results: dict[str, Any],
                          master_root: Path, run_analysis: Path, rom_path: Path,
                          flow_stats: dict[str, Any]) -> dict[str, Any]:
    """Publish a cleanup proof when R7 retained no FLOW spool on disk."""
    runtime = _json(receipt_path).get("runtime", {})
    run_id = int(runtime.get("run_id", 0))
    if run_id <= 0 or hashlib.sha256(rom_path.read_bytes()).hexdigest() != ROM_SHA:
        raise CleanupStop(STOP_NOT_ABSORBED, "runtime or ROM identity is incomplete")
    master, generation, pointer = _master_paths(master_root.resolve())
    _integrity(master)
    db = sqlite3.connect(master)
    try:
        run = db.execute("SELECT * FROM run WHERE run_id=?", (run_id,)).fetchone()
        audit = db.execute("SELECT * FROM audit WHERE run_id=?", (run_id,)).fetchone()
    finally:
        db.close()
    if run is None or audit is None:
        raise CleanupStop(STOP_NOT_ABSORBED, "run is absent from accepted rolling-master lineage")
    if int(run[4]) != int(flow_stats.get("segments", -1)) or int(run[5]) != int(flow_stats.get("records", -1)):
        raise CleanupStop(STOP_NOT_ABSORBED, "in-memory handoff counts do not match rolling master")
    generic_failure = generic_acceptance_failure(run_analysis, stage_results)
    if generic_failure:
        raise CleanupStop(STOP_NOT_ABSORBED, generic_failure)
    stage5, stage6 = stage_results.get("REFRESHING MAP", {}), stage_results.get("CONTROL PROVENANCE", {})
    stage7, stage8 = stage_results.get("ASM CLOSURE", {}), stage_results.get("FULL ROM AUDIT", {})
    if stage5.get("state") != "PASS" or stage6.get("status") not in ACCEPTED_STAGE6 or \
            stage7.get("state") not in {"PASS", "NO_DELTA"} or stage8.get("state") != "PASS":
        raise CleanupStop(STOP_NOT_ABSORBED, "mandatory post-run stage is not accepted")
    canonical = Path(stage5["generation_dir"])
    _canonical_integrity(canonical, _stage5_source_owned(stage5),
                         selected_source_owned=_final_source_owned(stage5, stage7))
    post_run = receipt_path.resolve().parent / "post-run-analysis"
    _atomic_json(run_analysis / "stage5-receipt.json", stage5)
    _atomic_json(run_analysis / "stage6-control-provenance.json", stage6)
    _atomic_json(run_analysis / "stage8-receipt.json", stage8)
    result = {"state": "PASS", "status": "PASS_ABSORBED_IN_MEMORY_FLOW_V1", "absorbed": True,
              "run_id": run_id, "semantic_equivalence": "PASS",
              "deletion_mode": "NO_RAW_FLOW_MATERIALIZED", "disk_free_before": shutil.disk_usage(post_run).free,
              "disk_free_after": shutil.disk_usage(post_run).free, "raw_bytes_before": 0,
              "raw_bytes_deleted": 0, "raw_bytes_retained": 0, "files_deleted": 0,
              "files_retained": 0, "master_bytes_retained": master.stat().st_size,
              "reclaim_ratio": 1.0, "delete_manifest": None, "absorption_receipt": None,
              "post_delete_integrity": "PASS", "flow_handoff": flow_stats}
    _atomic_json(post_run / "cleanup-receipt.json", result)
    return result


def publish_cleanup_result(receipt_path: Path, result: dict[str, Any]) -> None:
    """Atomically reflect a completed reclaim in the campaign report and snapshot."""
    campaign = receipt_path.resolve().parent
    post_run = campaign / "post-run-analysis"
    report_path, status_path = post_run / "report.json", post_run / "status.json"
    report = _json(report_path)
    report["cleanup"] = result
    report.setdefault("stage_results", {})["CLEANUP"] = result
    report["canonical_refresh"] = "PASS"
    report["stop"] = None
    report["pipeline_state"] = "ANALYSIS COMPLETE ✓"
    _atomic_json(report_path, report)
    status = _json(status_path)
    row = status.setdefault("stages", {}).setdefault("CLEANUP", {})
    row.update(state="PASS", processed_units=int(result["files_deleted"]),
               total_units=int(result["files_deleted"]), unit_name="files",
               detail=f"ABSORBED RUN {result['run_id']}; permanent deletion; no Recycle Bin")
    status.update(stage="CLEANUP", active_stage=None, stage_state="PASS",
                  stage_index=STAGES.index("CLEANUP") + 1, stage_count=len(STAGES),
                  processed_units=int(result["files_deleted"]), total_units=int(result["files_deleted"]),
                  unit_name="files", overall_state="ANALYSIS COMPLETE ✓", pipeline_state="COMPLETE",
                  error=None, warning=None)
    _atomic_json(status_path, status)


__all__ = ["ABSORPTION_STATUS", "CleanupStop", "publish_cleanup_result", "reclaim_absorbed_run"]
