"""Atomically archive a closed MAP-1 session and refresh canonical knowledge."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3
import sys
import time
import uuid
from typing import Any

_TOOLS = str(Path(__file__).resolve().parents[1])
if _TOOLS not in sys.path:
    sys.path.insert(0, _TOOLS)

try:
    from .identity import ROM_SHA, ROM_SIZE
    from .live_forward_archivist import archive_session
    from .rom_knowledge_live_import import (KnowledgeImportStop, import_archivist_session,
                                            sha256_file)
    from .rom_knowledge_map import KnowledgeStore
    from .rom_knowledge_pipeline_audit import audit_pipeline
except ImportError:
    from identity import ROM_SHA, ROM_SIZE
    from live_forward_archivist import archive_session
    from rom_knowledge_live_import import KnowledgeImportStop, import_archivist_session, sha256_file
    from rom_knowledge_map import KnowledgeStore
    from rom_knowledge_pipeline_audit import audit_pipeline


PASS = "PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1"
POINTER_SCHEMA = "oasis.m12.archivist-canonical-knowledge.current.v1"


def _canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _hash_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def _hash_json(value: Any) -> str:
    return _hash_bytes(_canonical(value).encode("utf-8"))


def _write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp-" + uuid.uuid4().hex)
    temporary.write_text(json.dumps(value, sort_keys=True, indent=2) + "\n",
                         encoding="utf-8", newline="\n")
    with temporary.open("rb+") as stream:
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def _record_failure(root: Path, code: str, detail: str,
                    unmapped: list[str] | None = None) -> None:
    facts = [{"status": "UNMAPPED_FACT_TYPE", "type": item} for item in (unmapped or [])]
    _write_json(Path(root) / "last_failure.json", {"status": code,
        "detail": detail, "unmapped_facts": facts})


def _copy_database(source: Path, target: Path) -> None:
    source, target = Path(source), Path(target)
    target.parent.mkdir(parents=True, exist_ok=True)
    if not source.exists():
        return
    read = sqlite3.connect(source.resolve().as_uri() + "?mode=ro", uri=True)
    write = sqlite3.connect(target)
    try:
        read.backup(write)
        write.commit()
    except Exception:
        write.rollback()
        raise
    finally:
        read.close()
        write.close()


def _metrics(path: Path, rom_sha: str, rom_size: int) -> tuple[dict[str, int], dict[str, str], dict[str, Any]]:
    store = KnowledgeStore(path, rom_sha, rom_size)
    try:
        return store.counts(), store.hashes(), store.metrics()
    finally:
        store.close()


def _current_inputs(root: Path) -> tuple[Path | None, Path | None, dict[str, Any] | None]:
    pointer = root / "current.json"
    if not pointer.is_file():
        return None, None, None
    try:
        data = json.loads(pointer.read_text(encoding="utf-8"))
        if data.get("schema") != POINTER_SCHEMA:
            raise ValueError
        generation = (root / str(data["generation_dir"])).resolve()
        generation.relative_to((root / "generations").resolve())
        master, knowledge = generation / "master.sqlite", generation / "knowledge.sqlite"
        if not master.is_file() or not knowledge.is_file() or \
                sha256_file(master) != data.get("master_sha256") or \
                sha256_file(knowledge) != data.get("knowledge_sha256"):
            raise ValueError
        return master, knowledge, data
    except (KeyError, ValueError, json.JSONDecodeError) as exc:
        raise ValueError("STOP_KNOWLEDGE_AUDIT_FAILED:invalid current generation pointer") from exc


def _verify_base_receipt(knowledge_path: Path, receipt_path: Path | None,
                         rom_sha: str, rom_size: int) -> dict[str, Any]:
    counts, hashes, metrics = _metrics(knowledge_path, rom_sha, rom_size)
    if receipt_path is None:
        return {"counts": counts, "hashes": hashes, "metrics": metrics}
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    if receipt.get("status") != "PASS_CANONICAL_ROM_KNOWLEDGE_MAP_V1" or \
            receipt.get("rom", {}).get("sha256") != rom_sha or \
            receipt.get("hashes") != hashes or receipt.get("metrics") != metrics:
        raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH:2D receipt mismatch")
    return {"counts": counts, "hashes": hashes, "metrics": metrics,
            "receipt_sha256": sha256_file(receipt_path)}


def _markdown(receipt: dict[str, Any]) -> str:
    before, after = receipt["knowledge_before"], receipt["knowledge_after"]
    merge, audit = receipt["archivist_merge"], receipt["independent_audit"]
    imp = receipt["import"]
    campaign = receipt.get("campaign")
    lines = [
        "# M12 Archivist → Canonical Knowledge Pipeline 2G", "",
        f"Status: `{receipt['status']}`.", "",
        f"Canonical ROM SHA-256: `{receipt['rom']['sha256']}`.",
        f"Archivist merge: `{merge['merge_mode']}`; session graph `{merge['session_graph_hash']}`;",
        f"master graph `{merge['master_graph_hash_before'] or 'NONE'}` → `{merge['master_graph_hash_after']}`.",
        f"MAP-1 nodes: {merge['master_node_count_before']:,} → {merge['master_node_count_after']:,}; "
        f"edges: {merge['master_edge_count_before']:,} → {merge['master_edge_count_after']:,}; "
        f"conflicts: {merge['conflict_count']:,}.",
        f"Session artifact SHA-256: `{merge['source_artifact_sha256']}`; receipt `{merge['receipt_sha256']}`.", "",
        "| Measure | Before | After | Delta |", "| --- | ---: | ---: | ---: |",
    ]
    for key in ("rom_object", "claim", "relation", "evidence_ref"):
        lines.append(f"| {key} | {before['counts'][key]:,} | {after['counts'][key]:,} | "
                     f"{after['counts'][key] - before['counts'][key]:+,} |")
    lines.extend(["", f"Runtime import added {imp['objects_added']:,} objects, "
        f"{imp['claims_added']:,} claims, {imp['relations_added']:,} relations and "
        f"{imp['evidence_refs_added']:,} evidence references. Duplicate import added zero rows and "
        "preserved all hashes. New overlapping runtime occurrences attach evidence to stable objects/relations.", "",
    ])
    if campaign:
        lines.extend([f"Canonical runtime proof: `{campaign['status']}`, "
            f"{campaign['worker_count']} Workers × {campaign['cycles_per_worker']} cycles, "
            f"{campaign['audited_segments']:,}/{campaign['required_segments']:,} segments independently "
            f"audited; {campaign['unique_capture_ids']:,} unique captures, "
            f"{campaign['instruction_occurrences']:,} instruction occurrences, "
            f"{campaign['terminal_next_pc_facts']:,} terminal address facts; "
            f"runtime SOURCE_OWNED delta {campaign['source_owned_delta']:+,}.", ""])
    lines.extend([
        f"Executed instruction objects: {before['metrics']['executed_instruction_objects']:,} → "
        f"{after['metrics']['executed_instruction_objects']:,}; runtime occurrence references: "
        f"{before['metrics']['runtime_occurrences_referenced']:,} → "
        f"{after['metrics']['runtime_occurrences_referenced']:,}.",
        f"Canonical relations: `EXECUTED_NEXT` "
        f"{before['metrics']['relations_by_type'].get('EXECUTED_NEXT', 0):,} → "
        f"{after['metrics']['relations_by_type'].get('EXECUTED_NEXT', 0):,}; `OBSERVED_NEXT_PC` "
        f"{before['metrics']['relations_by_type'].get('OBSERVED_NEXT_PC', 0):,} → "
        f"{after['metrics']['relations_by_type'].get('OBSERVED_NEXT_PC', 0):,}. "
        f"2E pointer/offset/table relations: {imp['canonical_control_relations']}.",
        f"Exception endpoint edges excluded from instruction adjacency: {imp['exception_flow_edges_excluded']:,} "
        f"edges / {imp['exception_flow_occurrences_excluded']:,} occurrences.", "",
        f"`SOURCE_OWNED`: {audit['source_owned_bytes']:,} bytes; delta `{audit['source_owned_delta']}`. "
        f"Emission bytes by type are unchanged: `{audit['emission_bytes_by_type']}`.",
        f"Hashes before → after: structure `{before['hashes']['structure_hash']}` → "
        f"`{after['hashes']['structure_hash']}`; evidence `{before['hashes']['evidence_index_hash']}` → "
        f"`{after['hashes']['evidence_index_hash']}`; emission `{before['hashes']['emission_hash']}` → "
        f"`{after['hashes']['emission_hash']}`; combined `{before['hashes']['map_hash']}` → "
        f"`{after['hashes']['map_hash']}`.",
        f"Independent audit: `{audit['status']}`. Post-run processing only; CPU runtime overhead from 2G is 0. "
        f"Archive/import/audit: {receipt['performance']['archive_seconds']:.3f}/"
        f"{receipt['performance']['import_seconds']:.3f}/{receipt['performance']['audit_seconds']:.3f} s; "
        f"knowledge DB {receipt['performance']['knowledge_db_bytes']:,} bytes.", "",
        "SQLite generations and runtime/session artifacts remain under ignored `build/`; "
        "this report and its compact JSON receipt contain no raw FLOW or lineage arrays.", ""])
    return "\n".join(lines)


def archive_and_refresh_knowledge(session_path: Path, bootstrap_master: Path,
                                  bootstrap_knowledge: Path, rom_path: Path,
                                  output_root: Path, base_receipt_path: Path | None = None,
                                  campaign_receipt_path: Path | None = None,
                                  expected_source_owned: int = 1_475_600,
                                  expected_rom_sha256: str = ROM_SHA,
                                  expected_rom_size: int = ROM_SIZE,
                                  report_path: Path | None = None,
                                  receipt_path: Path | None = None) -> dict[str, Any]:
    """Build, audit, then atomically select one paired master/knowledge generation."""
    session_path, rom_path, output_root = map(Path, (session_path, rom_path, output_root))
    rom = rom_path.read_bytes()
    if len(rom) != expected_rom_size or _hash_bytes(rom) != expected_rom_sha256:
        raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH")
    current_master, current_knowledge, current_pointer = _current_inputs(output_root)
    if current_master and current_knowledge:
        base_master, base_knowledge = current_master, current_knowledge
        current_metrics = _metrics(base_knowledge, expected_rom_sha256, expected_rom_size)
        base_snapshot = {"counts": current_metrics[0], "hashes": current_metrics[1],
                         "metrics": current_metrics[2]}
    else:
        base_master = Path(bootstrap_master)
        base_knowledge = Path(bootstrap_knowledge)
        if not base_knowledge.is_file():
            raise FileNotFoundError("STOP_KNOWLEDGE_IMPORT_EVIDENCE_MISSING:2D knowledge DB missing")
        if base_master.exists():
            base_snapshot_master = sha256_file(base_master)
        else:
            base_snapshot_master = None
        base_snapshot = _verify_base_receipt(base_knowledge, base_receipt_path,
                                             expected_rom_sha256, expected_rom_size)
    session_sha = sha256_file(session_path)
    if current_pointer and current_pointer.get("last_session_source_sha256") == session_sha:
        prior_receipt = output_root / current_pointer["generation_dir"] / "receipt.json"
        if prior_receipt.is_file():
            value = json.loads(prior_receipt.read_text(encoding="utf-8"))
            if value.get("status") == PASS:
                value["replay_status"] = "PASS_IDEMPOTENT_NOOP"
                value["import"] = {**value["import"], "objects_added": 0,
                    "claims_added": 0, "relations_added": 0, "evidence_refs_added": 0}
                return value
    generation_root = output_root / "generations"
    generation_root.mkdir(parents=True, exist_ok=True)
    staging = generation_root / (".staging-" + uuid.uuid4().hex)
    staging.mkdir()
    staged_master, staged_knowledge = staging / "master.sqlite", staging / "knowledge.sqlite"
    _copy_database(base_master, staged_master)
    _copy_database(base_knowledge, staged_knowledge)
    campaign_summary = None
    if campaign_receipt_path:
        campaign = json.loads(Path(campaign_receipt_path).read_text(encoding="utf-8"))
        runtime, projection = campaign.get("runtime", {}), campaign.get("rom_projection", {})
        campaign_audit = campaign.get("independent_audit", {})
        worker_count = int(runtime.get("configured_count", 0))
        cycles = int(runtime.get("required_cycles_per_worker", 0))
        required_segments = worker_count * cycles
        workers = runtime.get("workers", [])
        worker_ids = {int(worker.get("worker_id", -1)) for worker in workers}
        lifecycle_valid = len(workers) == worker_count and worker_ids == set(range(worker_count)) and all(
            int(worker.get("capture_count", 0)) == cycles and
            worker.get("lifecycle_transition_counts") == [cycles, cycles, cycles, cycles]
            for worker in workers)
        segment_counts = campaign_audit.get("worker_segment_counts", {})
        generation_counts = campaign_audit.get("worker_generation_counts", {})
        workers_valid = set(segment_counts) == {str(i) for i in range(worker_count)} and \
            set(generation_counts) == {str(i) for i in range(worker_count)} and \
            all(int(value) == cycles for value in segment_counts.values()) and \
            all(int(value) == cycles for value in generation_counts.values())
        final_metrics = runtime.get("final_metrics", {})
        if campaign.get("checkpoint") != "M12-ROM-RANGE-LINKAGE-2B" or \
                campaign.get("status") != "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE" or \
                Path(campaign.get("session_path", "")).resolve() != session_path.resolve() or \
                runtime.get("outcome") != "PASS" or worker_count <= 0 or cycles <= 0 or \
                not lifecycle_valid or not workers_valid or \
                runtime.get("audited_segments") != required_segments or \
                runtime.get("required_completed_segments") != required_segments or \
                int(final_metrics.get("captures_dropped", -1)) != 0 or \
                int(final_metrics.get("captures_invalid", -1)) != 0 or \
                int(final_metrics.get("identity_collisions", -1)) != 0 or \
                campaign.get("saved_session", {}).get("segments_admitted") != required_segments or \
                campaign.get("saved_session", {}).get("segments_rejected") != 0 or \
                campaign_audit.get("status") != "PASS_INDEPENDENT_ROM_RANGE_AUDIT" or \
                campaign_audit.get("segments_audited") != required_segments or \
                campaign_audit.get("unique_capture_ids") != required_segments or \
                campaign_audit.get("workers_represented") != list(range(worker_count)) or \
                campaign_audit.get("audited_range_occurrences") != projection.get("instruction_occurrences") or \
                campaign_audit.get("audited_terminal_next_pc_facts") != \
                    campaign_audit.get("terminal_next_pc_facts") or \
                campaign_audit.get("identity_conflicts") != 0 or \
                campaign_audit.get("opcode_mismatches") != 0 or \
                campaign_audit.get("unresolved_instruction_occurrences") != 0 or \
                campaign_audit.get("rom_unresolved_occurrences") != 0 or \
                campaign_audit.get("decode_unsupported_occurrences") != 0 or \
                projection.get("status") != "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE" or \
                projection.get("rom_sha256") != expected_rom_sha256 or \
                campaign.get("source_owned_delta") != 0:
            raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH:campaign receipt mismatch")
        campaign_summary = {"status": campaign_audit["status"], "worker_count": worker_count,
            "cycles_per_worker": cycles, "required_segments": required_segments,
            "audited_segments": runtime["audited_segments"],
            "unique_capture_ids": campaign_audit["unique_capture_ids"],
            "instruction_occurrences": campaign_audit["audited_range_occurrences"],
            "unique_instruction_ranges": campaign_audit.get("unique_instruction_ranges", 0),
            "terminal_next_pc_facts": campaign_audit["audited_terminal_next_pc_facts"],
            "source_owned_delta": campaign["source_owned_delta"]}
    archive_started = time.perf_counter()
    try:
        archived = archive_session(staged_master, session_path, expected_rom_sha256)
    except ValueError as exc:
        text = str(exc)
        if "ROM" in text:
            raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_ROM_MISMATCH") from exc
        if "CONFLICT" in text:
            raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH:MAP1_CONFLICT") from exc
        if "OWNERSHIP" in text or "SOURCE_OWNED" in text:
            raise ValueError("STOP_RUNTIME_SOURCE_OWNED_MUTATION") from exc
        raise ValueError("STOP_ARCHIVIST_TO_KNOWLEDGE_GRAPH_MISMATCH:" + text) from exc
    archive_seconds = time.perf_counter() - archive_started
    merge_receipt = archived["merge_receipt"]
    import_started = time.perf_counter()
    try:
        import_report = import_archivist_session(session_path, staged_master,
            staged_knowledge, rom, expected_rom_sha256, merge_receipt)
        counts_first, hashes_first, metrics_first = _metrics(
            staged_knowledge, expected_rom_sha256, expected_rom_size)
        replay = import_archivist_session(session_path, staged_master,
            staged_knowledge, rom, expected_rom_sha256, merge_receipt)
        counts_replay, hashes_replay, _ = _metrics(
            staged_knowledge, expected_rom_sha256, expected_rom_size)
        if replay["objects_added"] or replay["claims_added"] or replay["relations_added"] or \
                replay["evidence_refs_added"] or counts_first != counts_replay or hashes_first != hashes_replay:
            raise ValueError("STOP_KNOWLEDGE_IMPORT_NONIDEMPOTENT")
        import_report["reimport"] = {"status": "PASS_IDEMPOTENT_NOOP",
            "objects_added": replay["objects_added"], "claims_added": replay["claims_added"],
            "relations_added": replay["relations_added"],
            "evidence_refs_added": replay["evidence_refs_added"],
            "hashes_unchanged": hashes_first == hashes_replay}
    except KnowledgeImportStop as exc:
        _record_failure(output_root, exc.code, str(exc),
                        list(exc.report.get("unmapped_fact_types", [])))
        raise
    except Exception as exc:
        if "EMISSION" in str(exc):
            raise ValueError("STOP_RUNTIME_EMISSION_MUTATION") from exc
        raise
    import_seconds = time.perf_counter() - import_started
    audit_started = time.perf_counter()
    try:
        audit = audit_pipeline(session_path, staged_master, base_knowledge,
            staged_knowledge, rom_path, expected_rom_sha256, merge_receipt, import_report,
            expected_source_owned)
    except Exception as exc:
        _record_failure(output_root, str(exc).split(":", 1)[0], str(exc))
        if str(exc).startswith("STOP_"):
            raise
        raise ValueError("STOP_KNOWLEDGE_AUDIT_FAILED:" + str(exc)) from exc
    audit_seconds = time.perf_counter() - audit_started
    after_counts, after_hashes, after_metrics = _metrics(
        staged_knowledge, expected_rom_sha256, expected_rom_size)
    if base_snapshot["metrics"]["source_owned_bytes"] != expected_source_owned or \
            after_metrics["source_owned_bytes"] != expected_source_owned:
        raise ValueError("STOP_RUNTIME_SOURCE_OWNED_MUTATION")
    if base_snapshot["hashes"]["emission_hash"] != after_hashes["emission_hash"]:
        raise ValueError("STOP_RUNTIME_EMISSION_MUTATION")
    performance = {"archive_seconds": archive_seconds, "import_seconds": import_seconds,
        "audit_seconds": audit_seconds, "knowledge_db_bytes": staged_knowledge.stat().st_size,
        "cpu_runtime_overhead": 0, "processing_phase": "post-run"}
    receipt = {"schema": "oasis.m12.archivist-canonical-knowledge.2g.v1",
        "checkpoint": "M12-ARCHIVIST-CANONICAL-KNOWLEDGE-PIPELINE-2G", "status": PASS,
        "rom": {"sha256": expected_rom_sha256, "bytes": expected_rom_size},
        "archivist_merge": merge_receipt,
        "master_before": {"graph_hash": merge_receipt["master_graph_hash_before"],
            "nodes": merge_receipt["master_node_count_before"],
            "edges": merge_receipt["master_edge_count_before"]},
        "master_after": {"graph_hash": merge_receipt["master_graph_hash_after"],
            "nodes": merge_receipt["master_node_count_after"],
            "edges": merge_receipt["master_edge_count_after"]},
        "knowledge_before": base_snapshot,
        "knowledge_after": {"counts": after_counts, "hashes": after_hashes,
                             "metrics": after_metrics},
        "import": import_report, "independent_audit": audit, "campaign": campaign_summary,
        "source_owned": {"before": base_snapshot["metrics"]["source_owned_bytes"],
                         "after": after_metrics["source_owned_bytes"], "delta": 0},
        "emission_unchanged": True, "performance": performance,
        "campaign_receipt_sha256": sha256_file(campaign_receipt_path) if campaign_receipt_path else None}
    generation_id = "gen-" + merge_receipt["source_artifact_sha256"][:16] + "-" + uuid.uuid4().hex[:8]
    receipt["generation_id"] = generation_id
    _write_json(staging / "merge-receipt.json", merge_receipt)
    _write_json(staging / "import-report.json", import_report)
    _write_json(staging / "audit.json", audit)
    _write_json(staging / "receipt.json", receipt)
    final_generation = generation_root / generation_id
    os.replace(staging, final_generation)
    pointer = {"schema": POINTER_SCHEMA, "generation_id": generation_id,
        "generation_dir": "generations/" + generation_id,
        "master_sha256": sha256_file(final_generation / "master.sqlite"),
        "knowledge_sha256": sha256_file(final_generation / "knowledge.sqlite"),
        "last_session_source_sha256": merge_receipt["source_artifact_sha256"],
        "merge_receipt_sha256": merge_receipt["receipt_sha256"],
        "knowledge_map_hash": after_hashes["map_hash"]}
    _write_json(output_root / "current.json", pointer)
    receipt["generation_dir"] = str(final_generation)
    if receipt_path:
        _write_json(Path(receipt_path), receipt)
    if report_path:
        Path(report_path).parent.mkdir(parents=True, exist_ok=True)
        Path(report_path).write_text(_markdown(receipt), encoding="utf-8", newline="\n")
    return receipt


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--session", type=Path, required=True,
                        help="closed exact-ROM Cartographer MAP-1 session")
    parser.add_argument("--master", type=Path, required=True,
                        help="accepted Archivist master used to seed the first generation")
    parser.add_argument("--knowledge-db", type=Path, required=True,
                        help="accepted 2D knowledge DB used to seed the first generation")
    parser.add_argument("--base-receipt", type=Path,
                        default=Path("docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json"))
    parser.add_argument("--campaign-receipt", type=Path,
                        help="optional exact ROM-link campaign receipt to reconcile")
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path,
                        default=Path("build/thor-evidence/archivist-knowledge-pipeline-2g"))
    parser.add_argument("--report", type=Path,
                        default=Path("docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.md"))
    parser.add_argument("--receipt", type=Path,
                        default=Path("docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.json"))
    args = parser.parse_args()
    result = archive_and_refresh_knowledge(args.session, args.master, args.knowledge_db,
        args.rom, args.output_dir, args.base_receipt, args.campaign_receipt,
        report_path=args.report, receipt_path=args.receipt)
    print(json.dumps({"status": result["status"], "generation_id": result["generation_id"],
        "merge_receipt_sha256": result["archivist_merge"]["receipt_sha256"],
        "hashes": result["knowledge_after"]["hashes"],
        "report": str(args.report.resolve()), "receipt": str(args.receipt.resolve())},
        sort_keys=True, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
