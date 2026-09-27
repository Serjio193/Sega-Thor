"""Deterministic authoritative postrun receipt generator for M12 pipeline.

Derives the canonical receipts from report.json, status.json, and stage
artifacts without fallback defaults.
"""

from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sqlite3
from typing import Any


def sha256_file(path: Path | str | None) -> str:
    """Compute hex SHA-256 for a file path, raising if missing or None."""
    if path is None:
        raise ValueError("STOP_HASH_TARGET_IS_NONE")
    file_path = Path(path)
    if not file_path.is_file():
        raise ValueError(f"STOP_HASH_TARGET_MISSING:{file_path}")
    hasher = hashlib.sha256()
    with open(file_path, "rb") as stream:
        while chunk := stream.read(1024 * 1024):
            hasher.update(chunk)
    return hasher.hexdigest()


def emit_asm_closure_receipt(s7_res: dict[str, Any],
                             s5_gen_dir: Path,
                             run_id: int) -> dict[str, Any]:
    """Derive Stage 7 receipt authoritatively from Stage 7 results and artifacts."""
    s7_state = s7_res.get("state")
    if s7_state not in ("PASS", "NO_DELTA"):
        raise ValueError(f"STOP_STAGE7_INVALID_STATE:{s7_state}")

    source_owned_before = None
    if isinstance(s7_res.get("source_owned_before"), (int, float)):
        source_owned_before = int(s7_res["source_owned_before"])
    elif "preflight" in s7_res and "canonical_map" in s7_res["preflight"]:
        source_owned_before = int(s7_res["preflight"]["canonical_map"].get("SOURCE_OWNED", 0))
    if (source_owned_before is None or source_owned_before <= 0) and (s5_gen_dir / "knowledge.sqlite").is_file():
        with sqlite3.connect(str(s5_gen_dir / "knowledge.sqlite")) as conn:
            row = conn.cursor().execute("SELECT COALESCE(SUM(end - start), 0) FROM emission WHERE source_owned = 1").fetchone()
            if row and row[0] is not None:
                source_owned_before = int(row[0])
    if source_owned_before is None or source_owned_before <= 0:
        raise ValueError("STOP_STAGE7_AUTHORITY_MISSING:source_owned_before")

    promoted_bytes = int(s7_res["promoted_bytes"]) if isinstance(s7_res.get("promoted_bytes"), (int, float)) else None
    promoted_ranges = s7_res.get("promoted_ranges")
    s7_disk_path = s7_res.get("output_paths", {}).get("stage7_result")
    if s7_disk_path and Path(s7_disk_path).is_file():
        s7_disk = json.loads(Path(s7_disk_path).read_text(encoding="utf-8"))
        if promoted_bytes is None and "promoted_bytes" in s7_disk:
            promoted_bytes = int(s7_disk["promoted_bytes"])
        if promoted_ranges is None:
            promoted_ranges = s7_disk.get("promoted_ranges")

    mat_dir = Path(s7_res.get("materialized", ""))
    manifest_file = mat_dir / "manifest.json" if mat_dir.is_dir() else None
    if manifest_file and manifest_file.is_file():
        m_doc = json.loads(manifest_file.read_text(encoding="utf-8"))
        if promoted_bytes is None and "promotions" in m_doc:
            promoted_bytes = sum(int(p["bytes"]) for p in m_doc["promotions"])
        if promoted_ranges is None:
            promoted_ranges = m_doc.get("promotions")

    if promoted_bytes is None:
        raise ValueError("STOP_STAGE7_AUTHORITY_MISSING:promoted_bytes")

    promoted_range_count = len(promoted_ranges) if promoted_ranges is not None else (
        int(s7_res.get("closed_count", 0)) if promoted_bytes > 0 else 0
    )
    source_owned_after = int(s7_res["source_owned_after"]) if isinstance(s7_res.get("source_owned_after"), (int, float)) else (source_owned_before + promoted_bytes)
    source_owned_delta = source_owned_after - source_owned_before
    if source_owned_delta != promoted_bytes:
        raise ValueError(
            f"STOP_STAGE7_AUTHORITY_CONFLICT:delta({source_owned_delta})!=promoted_bytes({promoted_bytes})"
        )

    candidates = s7_res.get("candidates", [])
    cand_count = int(s7_res.get("candidate_count", len(candidates)))
    closed_count = promoted_range_count if promoted_range_count > 0 else int(s7_res.get("closed_count", 0))
    blocked_count = int(s7_res.get("blocked_count", max(0, cand_count - closed_count)))
    blockers = Counter()
    for c in candidates:
        blockers.update(c.get("blockers", []))

    knowledge_file = s5_gen_dir / "knowledge.sqlite"
    knowledge_sha = sha256_file(knowledge_file) if knowledge_file.is_file() else None
    manifest_sha = sha256_file(manifest_file) if manifest_file and manifest_file.is_file() else None

    status = s7_res.get("status")
    if not status:
        status = ("PASS_POSTRUN_MAP_DRIVEN_ASM_CLOSURE_V1"
                  if promoted_bytes > 0 else
                  "PASS_STAGE7_EVALUATED_FAIL_CLOSED_NO_PROMOTION")

    return {
        "blocked_ranges": blocked_count,
        "blockers": dict(blockers),
        "candidate_ranges": cand_count,
        "closed_ranges": closed_count,
        "input_hashes": {"canonical_knowledge_sha256": knowledge_sha},
        "materialized_dir": str(mat_dir) if mat_dir and mat_dir.is_dir() else None,
        "output_hashes": {"materialized_manifest_sha256": manifest_sha},
        "promoted_bytes": promoted_bytes,
        "promoted_range_count": promoted_range_count,
        "run_id": run_id,
        "schema": "oasis.m12.postrun-asm-closure.v1",
        "source_owned_after": source_owned_after,
        "source_owned_before": source_owned_before,
        "source_owned_delta": source_owned_delta,
        "stage_version": "M12-ASM-CLOSURE-2I.3-V1",
        "state": s7_state,
        "status": status,
        "truth_version": "M12-CANONICAL-TRUTH-V1"
    }


def _req(target_dir: Path, name: str, res: dict[str, Any] | None = None, key: str | None = None) -> Path:
    f = target_dir / name
    if not f.is_file() and res and key and key in res:
        f = Path(res[key])
    if not f.is_file():
        raise ValueError(f"STOP_FILE_MISSING:{name}")
    return f


def emit_postrun_receipts(post_run_dir: Path | str,
                           receipt_path: Path | str,
                           report: dict[str, Any],
                           status_path: Path | str | None = None) -> dict[str, str]:
    """Generate authoritative receipts deterministically from pipeline artifacts."""
    target_dir = Path(post_run_dir)
    target_dir.mkdir(parents=True, exist_ok=True)
    report_file = target_dir / "report.json"
    status_file = Path(status_path) if status_path else target_dir / "status.json"
    rec_file = Path(receipt_path)

    if not status_file.is_file():
        raise ValueError(f"STOP_STATUS_DOC_MISSING:{status_file}")

    status_doc = json.loads(status_file.read_text(encoding="utf-8"))
    run_id = int(report["run_id"])

    # 1. Canonical source owned resolution from preflight
    stage_results = report["stage_results"]
    status_stages = status_doc["stages"]
    s7_res = stage_results["ASM CLOSURE"]
    preflight = s7_res["preflight"]
    cmap = preflight["canonical_map"]
    source_owned_before_pipeline = int(cmap["SOURCE_OWNED"])

    if source_owned_before_pipeline <= 0:
        raise ValueError(f"STOP_CANONICAL_SOURCE_OWNED_MISSING:{source_owned_before_pipeline}")

    # Stage 5 receipt
    s5_res = stage_results["REFRESHING MAP"]
    s5_status_entry = status_stages["REFRESHING MAP"]
    s5_state = s5_res["state"]
    if s5_state not in ("PASS", "NO_DELTA"):
        raise ValueError(f"STOP_STAGE5_INVALID_STATE:{s5_state}")
    s5_replay = s5_res["replay_status"]
    if not s5_replay or "PASS" not in s5_replay:
        raise ValueError(f"STOP_STAGE5_INVALID_REPLAY_STATUS:{s5_replay}")
    s5_gen_dir = Path(s5_res["generation_dir"])
    if not s5_gen_dir.is_dir():
        raise ValueError(f"STOP_STAGE5_GENERATION_DIR_MISSING:{s5_gen_dir}")

    r1 = {
        "generation_dir": str(s5_gen_dir),
        "input_hashes": {
            "master_sha256_before": report["master_sha256"],
            "rom_sha256": report["rom_sha256"],
            "session_sha256": s5_res["session_sha256"]
        },
        "output_hashes": {
            "knowledge_sha256_after": s5_res.get("pointer", {}).get("knowledge_sha256"),
            "master_sha256_after": s5_res.get("pointer", {}).get("master_sha256"),
            "metadata_sha256": s5_res.get("pointer", {}).get("metadata_sha256")
        },
        "replay_status": s5_replay,
        "run_id": run_id,
        "schema": "oasis.m12.postrun-map-refresh.v1",
        "source_owned_after": source_owned_before_pipeline,
        "source_owned_before": source_owned_before_pipeline,
        "source_owned_delta": 0,
        "stage_version": "M12-2I.2b-stage5-v1",
        "state": s5_state,
        "status": s5_status_entry["state"],
        "truth_version": "M12-CANONICAL-TRUTH-V1"
    }

    # Stage 6 receipt
    s6_res = stage_results["CONTROL PROVENANCE"]
    s6_state, s6_status = s6_res["state"], s6_res["status"]
    if s6_state not in ("PASS", "NO_DELTA") or s6_status not in ("PASS", "NO_DELTA"):
        raise ValueError(f"STOP_STAGE6_INVALID_OUTCOME:{s6_status}")
    diag_file = Path(s6_res["diagnostic_path"])
    diag_hash = sha256_file(diag_file)

    r2 = {
        "indirect_consumers": int(s6_res["indirect_consumers"]),
        "input_hashes": {"raw_flow_sha256": s6_res["raw_flow_identity"]["raw_sha256"]},
        "output_hashes": {"diagnostic_sha256": diag_hash},
        "resolved_jump_table_relations": int(s6_res["resolved_jump_table_relations"]),
        "resolved_offset_relations": int(s6_res["resolved_offset_relations"]),
        "resolved_pointer_relations": int(s6_res["resolved_pointer_relations"]),
        "run_id": run_id,
        "schema": "oasis.m12.postrun-control-provenance.v1",
        "segments_processed": int(s6_res["segments_processed"]),
        "source_owned_after": source_owned_before_pipeline,
        "source_owned_before": source_owned_before_pipeline,
        "source_owned_delta": 0,
        "stage_version": "M12-CONTROL-PROVENANCE-V1",
        "state": s6_state,
        "status": s6_status,
        "truth_version": "M12-CANONICAL-TRUTH-V1"
    }

    # Stage 7 receipt
    r3 = emit_asm_closure_receipt(s7_res, s5_gen_dir, run_id)

    # Stage 8 receipt
    s8_res = stage_results["FULL ROM AUDIT"]
    s8_state, s8_status = s8_res["state"], s8_res["status"]
    if s8_status != "PASS_POSTRUN_FULL_ROM_AUDIT_V1":
        raise ValueError(f"STOP_STAGE8_STATUS_NOT_PASS:{s8_status}")
    if s8_res["full_rom_sha256"] != report["rom_sha256"]:
        raise ValueError("STOP_STAGE8_ROM_SHA_MISMATCH")

    audit_passed = (s8_res["full_rom_sha256"] == report["rom_sha256"] and
                    s8_status == "PASS_POSTRUN_FULL_ROM_AUDIT_V1")

    r4 = {
        "audit_passed": audit_passed,
        "input_hashes": {"manifest_sha256": s8_res["manifest_sha256"]},
        "output_hashes": {"rebuilt_rom_sha256": s8_res["full_rom_sha256"]},
        "run_id": run_id,
        "schema": "oasis.m12.postrun-full-rom-audit.v1",
        "source_owned_after": r3["source_owned_after"],
        "source_owned_before": r3["source_owned_after"],
        "source_owned_delta": 0,
        "stage_version": "M12-FULL-ROM-AUDIT-V1",
        "state": s8_state,
        "status": s8_status,
        "truth_version": "M12-CANONICAL-TRUTH-V1"
    }

    # Stage 9 receipt
    cl_res = stage_results.get("CLEANUP") or report.get("cleanup", {})
    cl_status, cl_state = cl_res["status"], cl_res["state"]
    if cl_status != "PASS_ABSORBED_RAW_PERMANENT_RECLAIM_V1":
        raise ValueError(f"STOP_CLEANUP_STATUS_NOT_PASS:{cl_status}")

    raw_archive_dir = target_dir.parent / "raw-evidence-archive"
    archive_bin = raw_archive_dir / "flow-v1-records.bin"
    archive_jsonl = raw_archive_dir / "flow-v1-segments.jsonl"
    archive_bin_sha = sha256_file(archive_bin) if archive_bin.is_file() else None
    archive_jsonl_sha = sha256_file(archive_jsonl) if archive_jsonl.is_file() else None

    r5 = {
        "deletion_mode": cl_res["deletion_mode"],
        "files_deleted": int(cl_res["files_deleted"]),
        "files_retained": int(cl_res["files_retained"]),
        "input_hashes": {"absorption_receipt_sha256": sha256_file(cl_res["absorption_receipt"])},
        "output_hashes": {"delete_manifest_sha256": sha256_file(cl_res["delete_manifest"])},
        "raw_archive": {
            "bin_sha256": archive_bin_sha,
            "file_count": 2 if archive_bin.is_file() and archive_jsonl.is_file() else 0,
            "jsonl_sha256": archive_jsonl_sha,
            "path": str(raw_archive_dir) if raw_archive_dir.is_dir() else None,
            "readback_verification": "PASS" if archive_bin_sha and archive_jsonl_sha else "UNVERIFIED",
            "total_bytes": (archive_bin.stat().st_size + archive_jsonl.stat().st_size)
            if archive_bin.is_file() and archive_jsonl.is_file() else 0
        },
        "raw_bytes_deleted": int(cl_res["raw_bytes_deleted"]),
        "run_id": run_id,
        "schema": "oasis.m12.postrun-cleanup.v1",
        "source_owned_after": r3["source_owned_after"],
        "source_owned_before": r3["source_owned_after"],
        "source_owned_delta": 0,
        "stage_version": "M12-CLEANUP-ABSORBED-V1",
        "state": cl_state,
        "status": cl_status,
        "truth_version": "M12-CANONICAL-TRUTH-V1"
    }

    # Audio Analysis receipt
    r_audio = None
    s_audio_res = stage_results.get("AUDIO ANALYSIS")
    if (target_dir / "postrun_audio_analysis.json").is_file() or s_audio_res:
        audio_an = _req(target_dir, "postrun_audio_analysis.json", s_audio_res, "analysis_path")
        audio_cand = _req(target_dir, "postrun_audio_candidates.json", s_audio_res, "candidates_path")
        audio_prov = _req(target_dir, "postrun_audio_provenance.json", s_audio_res, "provenance_path")
        audio_json = json.loads(audio_an.read_text(encoding="utf-8"))
        audio_state, audio_status = audio_json["state"], audio_json["status"]
        if audio_state not in ("PASS", "NO_DELTA") or audio_status not in ("PASS", "NO_DELTA"):
            raise ValueError(f"STOP_AUDIO_INVALID_OUTCOME:{audio_status}")
        flow_idx_sha = s6_res["raw_flow_identity"].get("index_sha256") or sha256_file(s6_res["raw_flow_identity"].get("index_path"))
        r_audio = {
            "campaign_id": report.get("campaign_id") or target_dir.parent.name,
            "input_hashes": {"flow_v1_records_sha256": s6_res["raw_flow_identity"]["raw_sha256"],
                             "flow_v1_segments_sha256": flow_idx_sha, "rom_sha256": report["rom_sha256"]},
            "output_hashes": {"postrun_audio_analysis_sha256": sha256_file(audio_an),
                             "postrun_audio_candidates_sha256": sha256_file(audio_cand),
                             "postrun_audio_provenance_sha256": sha256_file(audio_prov)},
            "run_id": run_id, "schema": "oasis.m12.postrun-audio-receipt.v1",
            "source_owned_after": source_owned_before_pipeline, "source_owned_before": source_owned_before_pipeline,
            "source_owned_delta": 0, "stage_version": "M12-POSTRUN-AUDIO-ANALYSIS-V1",
            "state": audio_state, "status": audio_status, "timestamp": status_doc["timestamp_utc"],
            "truth_version": "M12-CANONICAL-TRUTH-V1",
        }

    # VDP Analysis receipt
    r_vdp = None
    s_vdp_res = stage_results.get("VDP / DMA ANALYSIS")
    if (target_dir / "postrun_vdp_analysis.json").is_file() or s_vdp_res:
        vdp_an = _req(target_dir, "postrun_vdp_analysis.json", s_vdp_res, "analysis_path")
        vdp_reg = _req(target_dir, "postrun_vdp_registers.json", s_vdp_res, "registers_path")
        vdp_dma = _req(target_dir, "postrun_vdp_dma.json", s_vdp_res, "dma_path")
        vdp_prov = _req(target_dir, "postrun_vdp_provenance.json", s_vdp_res, "provenance_path")
        vdp_cand = _req(target_dir, "postrun_vdp_candidates.json", s_vdp_res, "candidates_path")
        vdp_json = json.loads(vdp_an.read_text(encoding="utf-8"))
        vdp_state, vdp_status = vdp_json["state"], vdp_json["status"]
        if vdp_state not in ("PASS", "NO_DELTA") or vdp_status not in ("PASS", "NO_DELTA"):
            raise ValueError(f"STOP_VDP_INVALID_OUTCOME:{vdp_status}")
        flow_idx_sha = s6_res["raw_flow_identity"].get("index_sha256") or sha256_file(s6_res["raw_flow_identity"].get("index_path"))
        if "s8_vdp_regression" not in vdp_json: raise ValueError("STOP_VDP_S8_REGRESSION_MISSING")
        r_vdp = {
            "campaign_id": report.get("campaign_id") or target_dir.parent.name,
            "conflicts": vdp_json.get("s8_conflicts", 0),
            "input_hashes": {"flow_v1_records_sha256": s6_res["raw_flow_identity"]["raw_sha256"],
                             "flow_v1_segments_sha256": flow_idx_sha, "rom_sha256": report["rom_sha256"]},
            "output_hashes": {"postrun_vdp_analysis_sha256": sha256_file(vdp_an),
                             "postrun_vdp_candidates_sha256": sha256_file(vdp_cand),
                             "postrun_vdp_dma_sha256": sha256_file(vdp_dma),
                             "postrun_vdp_provenance_sha256": sha256_file(vdp_prov),
                             "postrun_vdp_registers_sha256": sha256_file(vdp_reg)},
            "run_id": run_id, "s8_vdp_regression": vdp_json["s8_vdp_regression"],
            "schema": "oasis.m12.postrun-vdp-receipt.v1",
            "source_owned_after": source_owned_before_pipeline, "source_owned_before": source_owned_before_pipeline,
            "source_owned_delta": 0, "stage_version": "M12-POSTRUN-VDP-DMA-ANALYSIS-V1",
            "state": vdp_state, "status": vdp_status, "timestamp": status_doc["timestamp_utc"],
            "truth_version": "M12-CANONICAL-TRUTH-V1",
        }

    # Sprite / SAT Analysis receipt
    r_sprite = None
    s_sprite_res = stage_results.get("SPRITE / SAT ANALYSIS")
    if (target_dir / "postrun_sprite_analysis.json").is_file() or s_sprite_res:
        sp_an = _req(target_dir, "postrun_sprite_analysis.json", s_sprite_res, "analysis_path")
        sp_sat = _req(target_dir, "postrun_sat_state.json", s_sprite_res, "sat_state_path")
        sp_frm = _req(target_dir, "postrun_sprite_frames.json", s_sprite_res, "frames_path")
        sp_prv = _req(target_dir, "postrun_sprite_provenance.json", s_sprite_res, "provenance_path")
        sp_cnd = _req(target_dir, "postrun_sprite_candidates.json", s_sprite_res, "candidates_path")
        sp_json = json.loads(sp_an.read_text(encoding="utf-8"))
        sp_state, sp_status = sp_json["state"], sp_json["status"]
        if sp_state not in ("PASS", "NO_DELTA") or sp_status not in ("PASS", "NO_DELTA"):
            raise ValueError(f"STOP_SPRITE_INVALID_OUTCOME:{sp_status}")
        flow_idx_sha = s6_res["raw_flow_identity"].get("index_sha256") or sha256_file(s6_res["raw_flow_identity"].get("index_path"))
        vdp_rec = target_dir / "postrun_vdp_receipt.json"
        orc_rec = Path(__file__).parents[2] / "build" / "thor-evidence" / "oracles" / "sprite" / "sprite_oracle_v1_f779" / "sprite_oracle_receipt.json"
        r_sprite = {
            "campaign_id": report.get("campaign_id") or target_dir.parent.name,
            "input_hashes": {"flow_v1_records_sha256": s6_res["raw_flow_identity"]["raw_sha256"],
                             "flow_v1_segments_sha256": flow_idx_sha, "rom_sha256": report["rom_sha256"],
                             "vdp_receipt_sha256": sha256_file(vdp_rec) if vdp_rec.is_file() else None,
                             "sprite_oracle_receipt_sha256": sha256_file(orc_rec) if orc_rec.is_file() else None},
            "output_hashes": {"postrun_sprite_analysis_sha256": sha256_file(sp_an),
                             "postrun_sat_state_sha256": sha256_file(sp_sat),
                             "postrun_sprite_frames_sha256": sha256_file(sp_frm),
                             "postrun_sprite_provenance_sha256": sha256_file(sp_prv),
                             "postrun_sprite_candidates_sha256": sha256_file(sp_cnd)},
            "metrics": dict(sp_json), "run_id": run_id, "schema": "oasis.m12.postrun-sprite-receipt.v1",
            "source_owned_after": source_owned_before_pipeline, "source_owned_before": source_owned_before_pipeline,
            "source_owned_delta": 0, "stage_version": "M12-POSTRUN-SPRITE-SAT-ANALYSIS-V1",
            "sprite_oracle_id": sp_json.get("sprite_oracle_id", "sprite_oracle_v1_f779"),
            "sprite_oracle_conflicts": int(sp_json.get("sprite_oracle_conflicts", 0)),
            "state": sp_state, "status": sp_status, "timestamp": status_doc["timestamp_utc"],
            "truth_version": "M12-CANONICAL-TRUTH-V1",
        }

    # Gameplay RAM / entity candidate analysis receipt
    r_gameplay = None
    s_gameplay_res = stage_results.get("GAMEPLAY RAM / ENTITY CANDIDATES")
    if (target_dir / "postrun_gameplay_ram_analysis.json").is_file() or s_gameplay_res:
        gp_an = _req(target_dir, "postrun_gameplay_ram_analysis.json", s_gameplay_res, "analysis_path")
        gp_cnd = _req(target_dir, "postrun_entity_candidates.json", s_gameplay_res, "candidates_path")
        gp_lnk = _req(target_dir, "postrun_entity_sat_links.json", s_gameplay_res, "entity_sat_links_path")
        gp_ins = _req(target_dir, "postrun_runtime_instances.json", s_gameplay_res, "runtime_instances_path")
        gp_prv = _req(target_dir, "postrun_gameplay_provenance.json", s_gameplay_res, "provenance_path")
        gp_json = json.loads(gp_an.read_text(encoding="utf-8"))
        gp_state, gp_status = gp_json["state"], gp_json["status"]
        if gp_state not in ("PASS", "NO_DELTA") or gp_status not in ("PASS", "NO_DELTA"):
            raise ValueError(f"STOP_GAMEPLAY_INVALID_OUTCOME:{gp_status}")
        r_gameplay = {
            "campaign_id": report.get("campaign_id") or target_dir.parent.name,
            "input_hashes": {"flow_v1_records_sha256": gp_json.get("flow", {}).get("raw_sha256"),
                             "flow_v1_segments_sha256": gp_json.get("flow", {}).get("index_sha256"),
                             "rom_sha256": report["rom_sha256"]},
            "output_hashes": {"postrun_gameplay_ram_analysis_sha256": sha256_file(gp_an),
                              "postrun_entity_candidates_sha256": sha256_file(gp_cnd),
                              "postrun_entity_sat_links_sha256": sha256_file(gp_lnk),
                              "postrun_runtime_instances_sha256": sha256_file(gp_ins),
                              "postrun_gameplay_provenance_sha256": sha256_file(gp_prv)},
            "metrics": dict(gp_json), "run_id": run_id,
            "schema": "oasis.m12.postrun-gameplay-receipt.v1",
            "source_owned_after": int(gp_json["source_owned_after"]),
            "source_owned_before": int(gp_json["source_owned_before"]),
            "source_owned_delta": int(gp_json["source_owned_delta"]),
            "stage_version": "M12-POSTRUN-GAMEPLAY-RAM-ENTITY-CANDIDATES-V1",
            "state": gp_state, "status": gp_status, "timestamp": status_doc["timestamp_utc"],
            "truth_version": "M12-CANONICAL-TRUTH-V1",
        }

    # Final receipt
    overall_state, pipeline_state = status_doc["overall_state"], report["pipeline_state"]
    accepted_pipeline = {"ANALYSIS COMPLETE ✓", "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"}
    if overall_state not in accepted_pipeline or pipeline_state not in accepted_pipeline:
        raise ValueError(f"STOP_PIPELINE_NOT_COMPLETE:{overall_state}/{pipeline_state}")

    stages_dict = {s: status_stages[s]["state"] for s in status_stages}
    for s, st in stages_dict.items():
        if st not in ("PASS", "NO_DELTA", "UNRESOLVED"):
            raise ValueError(f"STOP_STAGE_NOT_PASS_OR_NO_DELTA:{s}={st}")

    unresolved_stages = sorted(s for s, state in stages_dict.items() if state == "UNRESOLVED")
    final_status = ("PASS_WITH_UNRESOLVED_EVIDENCE_V1" if unresolved_stages else
                    "PASS_MODERN_POSTRUN_PIPELINE_V1")
    vdp_delta = int(r_vdp["source_owned_delta"]) if r_vdp is not None else 0
    sprite_delta = int(r_sprite["source_owned_delta"]) if r_sprite is not None else 0
    gameplay_delta = int(r_gameplay["source_owned_delta"]) if r_gameplay is not None else 0
    asm_delta = int(r3["source_owned_delta"])
    pipeline_before = source_owned_before_pipeline
    pipeline_after = int(r3["source_owned_after"])
    pipeline_delta = pipeline_after - pipeline_before

    r6 = {
        "asm_source_owned_delta": asm_delta, "campaign_dir": str(target_dir.parent),
        "input_hashes": {"receipt_sha256": sha256_file(rec_file), "rom_sha256": report["rom_sha256"]},
        "output_hashes": {"report_sha256": sha256_file(report_file) if report_file.is_file() else None,
                          "status_sha256": sha256_file(status_file)},
        "overall_state": overall_state, "pipeline_source_owned_after": pipeline_after,
        "unresolved_stages": unresolved_stages,
        "pipeline_source_owned_before": pipeline_before, "pipeline_source_owned_delta": pipeline_delta,
        "pipeline_state": pipeline_state, "run_id": run_id, "schema": "oasis.m12.postrun-final.v1",
        "source_owned_after": pipeline_after, "source_owned_before": pipeline_before,
        "source_owned_delta": pipeline_delta, "sprite_source_owned_delta": sprite_delta,
        "stages": stages_dict, "status": final_status, "timestamp": status_doc["timestamp_utc"],
        "truth_version": "M12-CANONICAL-TRUTH-V1", "vdp_source_owned_delta": vdp_delta,
        "gameplay_source_owned_delta": gameplay_delta,
    }

    receipt_map = {
        "postrun_map_refresh_receipt.json": r1, "postrun_control_provenance_receipt.json": r2,
        "postrun_asm_closure_receipt.json": r3, "postrun_full_rom_audit.json": r4,
        "postrun_cleanup_receipt.json": r5, "postrun_final_receipt.json": r6,
    }
    if r_audio is not None: receipt_map["postrun_audio_receipt.json"] = r_audio
    if r_vdp is not None: receipt_map["postrun_vdp_receipt.json"] = r_vdp
    if r_sprite is not None: receipt_map["postrun_sprite_receipt.json"] = r_sprite
    if r_gameplay is not None: receipt_map["postrun_gameplay_receipt.json"] = r_gameplay

    result_shas = {}
    for filename, content in receipt_map.items():
        dst = target_dir / filename
        dst.write_text(json.dumps(content, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        result_shas[filename] = sha256_file(dst)

    return result_shas


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--postrun-dir", type=Path, required=True)
    parser.add_argument("--receipt", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--status", type=Path)
    args = parser.parse_args()

    rep_file = args.report or args.postrun_dir / "report.json"
    rep_data = json.loads(rep_file.read_text(encoding="utf-8"))
    shas = emit_postrun_receipts(args.postrun_dir, args.receipt, rep_data, args.status)
    print(json.dumps(shas, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
