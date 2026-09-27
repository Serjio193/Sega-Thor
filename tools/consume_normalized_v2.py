"""Publish one normalized-generic v2 corpus through the canonical transaction."""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import sqlite3
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "src/tools"), str(ROOT / "src/tools/thor_evidence")]

from experiment_lifecycle import build_receipt, cleanup_closed, write_receipt
from knowledge_generation_gc import _self_check_current, build_plan
from normalized_v2_canonical_adapter import apply_path
from rom_knowledge_live_import import sha256_file
from rom_knowledge_map import KnowledgeStore
from rom_knowledge_pipeline import publish_normalized_v2_evidence
from runtime_path_view import iter_runtime_paths


def _json(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _path_state(path: Path, rom_sha: str, rom_size: int,
                exclude_artifact_sha: str | None = None) -> dict[str, Any]:
    db = sqlite3.connect(Path(path).resolve().as_uri() + "?mode=ro&immutable=1", uri=True)
    digest, count, ordered, windows_valid = hashlib.sha256(), 0, True, True
    multiplicity: Counter[tuple[int, ...]] = Counter()
    try:
        for item in iter_runtime_paths(db):
            sequence = item["native_sequences"]
            ordered = ordered and all(type(value) is int for value in sequence) and all(
                left < right for left, right in zip(sequence, sequence[1:]))
            window = item["window"]
            windows_valid = windows_valid and type(window.get("worker_id")) is int and \
                window["worker_id"] >= 0 and all(type(window.get(key)) is int and window[key] > 0
                for key in ("capture_id", "generation"))
            windows_valid = windows_valid and len(item["occurrence_ids"]) == len(item["pcs"])
            multiplicity[tuple(item["pcs"])] += 1
            digest.update(_json(item).encode("utf-8"))
            digest.update(b"\n")
            count += 1
        store = KnowledgeStore(path, rom_sha, rom_size, read_only=True)
        try:
            metrics = store.metrics()
            hashes = store.hashes()
            meta = store.meta()
        finally:
            store.close()
        unresolved = hashlib.sha256()
        preserved_queries = [("conflict", "SELECT * FROM conflict ORDER BY conflict_id"),
            ("hypothesis", "SELECT claim_id,object_id,claim_type,value_json,status FROM claim "
                           "WHERE status='HYPOTHESIS' ORDER BY claim_id")]
        if exclude_artifact_sha:
            preserved_queries.append(("normalized_unresolved", "SELECT ref_id,subject_type,"
                "subject_id,source_sha256,fact_kind,fact_count,locator_json FROM evidence_ref "
                "WHERE fact_kind='NORMALIZED_V2_RECORD:UNRESOLVED' AND source_sha256!=? "
                "ORDER BY ref_id"))
        else:
            preserved_queries.append(("normalized_unresolved", "SELECT ref_id,subject_type,"
                "subject_id,source_sha256,fact_kind,fact_count,locator_json FROM evidence_ref "
                "WHERE fact_kind='NORMALIZED_V2_RECORD:UNRESOLVED' ORDER BY ref_id"))
        for table, query in preserved_queries:
            unresolved.update(table.encode() + b"\0")
            rows = db.execute(query, (exclude_artifact_sha,)) if "?" in query else db.execute(query)
            for row in rows:
                unresolved.update(_json(tuple(row)).encode() + b"\n")
        endpoint_errors = int(db.execute("""SELECT COUNT(*) FROM relation s
            WHERE NOT EXISTS(SELECT 1 FROM rom_object o WHERE o.object_id=s.source_object_id)
            OR (s.target_object_id IS NOT NULL AND NOT EXISTS(
                SELECT 1 FROM rom_object o WHERE o.object_id=s.target_object_id))""").fetchone()[0])
        object_errors = int(db.execute("""SELECT COUNT(*) FROM rom_object o
            LEFT JOIN rom_range r USING(range_id) WHERE r.range_id IS NULL
            OR o.object_type NOT IN ('ROM_RANGE','M68K_INSTRUCTION','M68K_FUNCTION',
              'ROM_DATA','POINTER_TABLE','TABLE_ENTRY','GRAPHICS_STREAM','AUDIO_DATA',
              'Z80_PROGRAM','UNKNOWN')""").fetchone()[0])
        return {"hashes": hashes, "metrics": metrics, "meta": meta,
            "path_count": count, "path_sha256": digest.hexdigest(),
            "path_multiset": multiplicity, "ordering_valid": ordered,
            "windows_valid": windows_valid, "endpoint_errors": endpoint_errors,
            "object_errors": object_errors, "unresolved_sha256": unresolved.hexdigest()}
    finally:
        db.close()


def _provenance_state(db_path: Path, artifact_sha: str,
                      expected: int) -> dict[str, Any]:
    db = sqlite3.connect(Path(db_path).resolve().as_uri() + "?mode=ro&immutable=1", uri=True)
    try:
        row = db.execute("""SELECT COUNT(*),MIN(json_extract(locator_json,'$.record_ordinal')),
            MAX(json_extract(locator_json,'$.record_ordinal')),
            COUNT(DISTINCT json_extract(locator_json,'$.record_ordinal')),
            COUNT(DISTINCT json_extract(locator_json,'$.run_id'))
            FROM evidence_ref WHERE source_sha256=? AND fact_kind LIKE 'NORMALIZED_V2_RECORD:%'""",
            (artifact_sha,)).fetchone()
        refs, first, last, distinct_ordinals, runs = map(int, row)
        types = {str(kind): int(count) for kind, count in db.execute(
            "SELECT fact_kind,COUNT(*) FROM evidence_ref WHERE source_sha256=? "
            "AND fact_kind LIKE 'NORMALIZED_V2_RECORD:%' GROUP BY fact_kind ORDER BY fact_kind",
            (artifact_sha,))}
        bad_links = int(db.execute("""SELECT COUNT(*) FROM evidence_ref e
            LEFT JOIN rom_object o ON o.object_id=e.subject_id
            WHERE e.source_sha256=? AND e.fact_kind IN
              ('NORMALIZED_V2_RECORD:MERGED','NORMALIZED_V2_RECORD:ALREADY_KNOWN')
              AND (o.object_id IS NULL OR o.object_type!='M68K_INSTRUCTION')""",
            (artifact_sha,)).fetchone()[0])
        return {"references": refs, "first_ordinal": first, "last_ordinal": last,
            "distinct_ordinals": distinct_ordinals, "distinct_runs": runs,
            "outcomes_by_type": types, "bad_rom_links": bad_links,
            "complete": refs == expected and first == 0 and last == expected - 1 and
                distinct_ordinals == expected and runs == 1 and bad_links == 0}
    finally:
        db.close()


def run(args: argparse.Namespace) -> dict[str, Any]:
    artifact, receipt_file, rom_path = map(Path, (args.artifact, args.historical_receipt, args.rom))
    output_root = Path(args.canonical_root)
    rom = rom_path.read_bytes()
    rom_sha, rom_size = hashlib.sha256(rom).hexdigest(), len(rom)
    if rom_sha != args.expected_rom_sha256 or rom_size != args.expected_rom_size:
        raise ValueError("STOP_NORMALIZED_V2_ROM_IDENTITY_MISMATCH")
    artifact_sha = sha256_file(artifact)
    if args.expected_artifact_sha256 and artifact_sha != args.expected_artifact_sha256:
        raise ValueError("STOP_NORMALIZED_V2_ARTIFACT_HASH_MISMATCH")
    old_receipt = json.loads(receipt_file.read_text(encoding="utf-8"))
    expected_records = int(old_receipt["corpus_records"])
    pointer_path = output_root / "current.json"
    pointer = json.loads(pointer_path.read_text(encoding="utf-8"))
    resumed = pointer.get("last_evidence_source_sha256") == artifact_sha
    free_before = shutil.disk_usage("C:\\").free
    receipt_out = ROOT / "docs/reports/THOR_M14_7B_NORMALIZED_V2_PUBLISH.json"
    report_out = ROOT / "docs/reports/THOR_M14_7B_NORMALIZED_V2_PUBLISH.md"
    if resumed:
        published = json.loads((output_root / pointer["generation_dir"] / "receipt.json")
                               .read_text(encoding="utf-8"))
        if published.get("status") != "PASS_NORMALIZED_V2_CANONICAL_PUBLISH_V1":
            raise ValueError("STOP_NORMALIZED_V2_RESUME_RECEIPT_INVALID")
        before_state = None
        audit_before = published["independent_audit"]
        before_generation = published["archivist_merge"]["parent_generation_id"]
        before_map_hash = published["knowledge_before"]["hashes"]["map_hash"]
        before_source_owned = int(published["knowledge_before"]["metrics"]["source_owned_bytes"])
        before_occurrences = int(published["knowledge_before"]["metrics"][
            "runtime_occurrences_referenced"])
    else:
        before_db = output_root / pointer["generation_dir"] / "knowledge.sqlite"
        before_state = _path_state(before_db, rom_sha, rom_size)
        if before_state["hashes"]["map_hash"] != pointer["knowledge_map_hash"]:
            raise ValueError("STOP_NORMALIZED_V2_BASE_MAP_HASH_MISMATCH")
        before_generation = pointer["generation_id"]
        before_map_hash = pointer["knowledge_map_hash"]
        before_source_owned = int(before_state["metrics"]["source_owned_bytes"])
        before_occurrences = int(before_state["metrics"]["runtime_occurrences_referenced"])
        published = publish_normalized_v2_evidence(artifact, artifact_sha, rom_path,
            output_root, pointer["knowledge_map_hash"], before_source_owned,
            expected_rom_sha256=rom_sha, expected_rom_size=rom_size,
            report_path=report_out, receipt_path=receipt_out,
            expected_producer_record_count=expected_records)
    adapted = published["import"]["supplemental_evidence"]
    if adapted["producer_record_count"] != expected_records or \
            adapted["historical_receipt_records"] != expected_records or \
            adapted["input_records"] + adapted["producer_deduplicated_records"] != expected_records:
        raise ValueError("STOP_NORMALIZED_V2_HISTORICAL_RECEIPT_RECORD_MISMATCH")
    current = json.loads(pointer_path.read_text(encoding="utf-8"))
    current_db = output_root / current["generation_dir"] / "knowledge.sqlite"
    replay = apply_path(current_db, artifact, artifact_sha, rom, rom_sha, rom_size,
                        verify_only=True)
    provenance = _provenance_state(current_db, artifact_sha, adapted["input_records"])
    after_state = _path_state(current_db, rom_sha, rom_size, artifact_sha)
    plan = build_plan(output_root)
    self_check, _ = _self_check_current(output_root, current)
    path_before = audit_before["path_sha256_before"] if resumed else before_state["path_sha256"]
    if plan.get("status") != "PASS" or self_check.get("status") != "PASS" or \
            not provenance["complete"] or after_state["path_sha256"] != path_before or \
            after_state["metrics"]["source_owned_bytes"] != before_source_owned or \
            after_state["object_errors"] or after_state["endpoint_errors"] or \
            not after_state["ordering_valid"] or not after_state["windows_valid"] or \
            (not resumed and after_state["unresolved_sha256"] != before_state["unresolved_sha256"]) or \
            (resumed and audit_before.get("status") != "PASS_NORMALIZED_V2_CANONICAL_AUDIT_V1"):
        raise ValueError("STOP_NORMALIZED_V2_CANONICAL_POSTCHECK_FAILED")
    if replay["input_records"] != adapted["input_records"] or \
            replay["outcomes"]["ALREADY_KNOWN"] != adapted["input_records"] or \
            replay["provenance_rows_inserted"] != 0 or replay["unaccounted_records"] != 0:
        raise ValueError("STOP_NORMALIZED_V2_REPLAY_FAILED")
    map_checks = {"objects_valid": after_state["object_errors"] == 0,
        "edge_endpoints_valid": after_state["endpoint_errors"] == 0,
        "occurrences_valid": after_state["metrics"]["runtime_occurrences_referenced"] == before_occurrences,
        "paths_reference_occurrences": after_state["path_sha256"] == path_before,
        "ordering_valid": after_state["ordering_valid"],
        "capture_boundaries_valid": after_state["windows_valid"],
        "loop_multiplicity_preserved": (after_state["path_sha256"] == path_before if resumed else
            after_state["path_multiset"] == before_state["path_multiset"]),
        "alternate_tails_preserved": after_state["path_sha256"] == path_before,
        "unresolved_preserved": (audit_before.get("status") == "PASS_NORMALIZED_V2_CANONICAL_AUDIT_V1"
            if resumed else after_state["unresolved_sha256"] == before_state["unresolved_sha256"]),
        "conflicts_preserved": (audit_before.get("status") == "PASS_NORMALIZED_V2_CANONICAL_AUDIT_V1"
            if resumed else after_state["unresolved_sha256"] == before_state["unresolved_sha256"]),
        "rom_identities_valid": after_state["meta"].get("rom_sha256") == rom_sha}
    if not all(map_checks.values()):
        raise ValueError("STOP_NORMALIZED_V2_LIFECYCLE_SELF_CHECK_FAILED")
    raw_root = next((parent for parent in artifact.parents if parent.name == "thor-evidence"), None)
    if raw_root is None or raw_root not in artifact.parents:
        raise ValueError("STOP_NORMALIZED_V2_AUDITED_ROOT_NOT_FOUND")
    closure = build_receipt(experiment_id=f"normalized-v2:{old_receipt['corpus_id']}:{artifact_sha[:16]}",
        raw_path=artifact, raw_root=raw_root, format_name="NORMALIZED_GENERIC_CORPUS_V2",
        producer="generic_flow_normalizer.py; live_forward_generic_closure_stage.py",
        schema_version="oasis.m13.normalized-generic-corpus.v2",
        counts={"input_events": adapted["input_records"],
            "merged_events": adapted["outcomes"]["MERGED"],
            "already_known_events": adapted["outcomes"]["ALREADY_KNOWN"],
            "unresolved_events": adapted["outcomes"]["UNRESOLVED"],
            "rejected_events": adapted["outcomes"]["REJECTED"]},
        map_before={"generation": before_generation,
                    "map_sha256": before_map_hash},
        map_after={"generation": current["generation_id"],
                   "map_sha256": current["knowledge_map_hash"]},
        result="MERGED", map_self_check={"status": "PASS", "map_sha256":
            current["knowledge_map_hash"], **map_checks},
        source_owned_before=before_source_owned,
        source_owned_after=after_state["metrics"]["source_owned_bytes"],
        new_occurrences=0, new_paths=0, new_unresolved=adapted["outcomes"]["UNRESOLVED"])
    closure["producer_reconciliation"] = {
        "historical_receipt_raw_records": expected_records,
        "producer_record_count": adapted["producer_record_count"],
        "primary_records_accounted": adapted["input_records"],
        "producer_deduplicated_records": adapted["producer_deduplicated_records"],
        "deduplication_rule": adapted["producer_deduplication_rule"],
        "status": "PASS"}
    closure_path = ROOT / "docs/reports/m14-7b-closure-receipts" / (
        "normalized-v2-" + artifact_sha[:16] + ".experiment.json")
    write_receipt(closure_path, closure)
    free_before_cleanup = shutil.disk_usage("C:\\").free
    cleanup = cleanup_closed([closure_path], raw_root, execute=True)
    free_after = shutil.disk_usage("C:\\").free
    if artifact.exists():
        raise ValueError("STOP_NORMALIZED_V2_CLEANUP_TARGET_REMAINS")
    final_pointer = json.loads(pointer_path.read_text(encoding="utf-8"))
    final_check, _ = _self_check_current(output_root, final_pointer)
    if final_pointer["knowledge_map_hash"] != current["knowledge_map_hash"] or \
            final_check.get("status") != "PASS":
        raise ValueError("STOP_NORMALIZED_V2_POST_CLEANUP_MAP_CHANGED")
    report = {"status": "PASS_NORMALIZED_V2_CAPTURE_CONSUMED",
        "artifact_sha256": artifact_sha, "artifact_bytes": cleanup["bytes"],
        "historical_receipt_records": expected_records, "adapter": adapted,
        "replay": replay, "provenance_query": provenance,
        "map_generation_before": before_generation,
        "map_generation_after": current["generation_id"],
        "map_hash_before": before_map_hash,
        "map_hash_after": final_pointer["knowledge_map_hash"],
        "source_owned_before": before_source_owned,
        "source_owned_after": after_state["metrics"]["source_owned_bytes"],
        "map_self_check": final_check, "lifecycle_checks": map_checks,
        "experiment_closed": closure["experiment_closed"],
        "normalized_deleted": not artifact.exists(), "bytes_freed_logical": cleanup["bytes"],
        "free_bytes_before": free_before, "free_bytes_before_cleanup": free_before_cleanup,
        "free_bytes_after": free_after, "free_bytes_delta": free_after - free_before_cleanup,
        "cleanup": cleanup}
    report_out.write_text("# M14.7B normalized-generic v2 consumption\n\n```json\n" +
        json.dumps(report, sort_keys=True, indent=2) + "\n```\n", encoding="utf-8")
    receipt_out.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n", encoding="utf-8")
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact", type=Path, required=True)
    parser.add_argument("--historical-receipt", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--canonical-root", type=Path, required=True)
    parser.add_argument("--expected-artifact-sha256")
    parser.add_argument("--expected-rom-sha256", required=True)
    parser.add_argument("--expected-rom-size", type=int, required=True)
    args = parser.parse_args()
    print(json.dumps(run(args), sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
