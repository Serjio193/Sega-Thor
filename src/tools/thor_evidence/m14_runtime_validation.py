"""Fail-closed M14.1 runtime validation over sealed post-run evidence."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
from typing import Any

from canonical_full_rom_map import CanonicalRomMap, ROM_SHA256, ROM_SIZE


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _audit_db(path: Path) -> dict[str, int]:
    db = sqlite3.connect(path)
    try:
        rows = db.execute("SELECT start,end FROM emission ORDER BY start,end").fetchall()
        owned = int(db.execute(
            "SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1"
        ).fetchone()[0])
    finally:
        db.close()
    cursor = gaps = overlaps = 0
    for start, end in rows:
        start, end = int(start), int(end)
        if start > cursor:
            gaps += start - cursor
        elif start < cursor:
            overlaps += cursor - start
        cursor = max(cursor, end)
    gaps += max(0, ROM_SIZE - cursor)
    return {"TOTAL_BYTES": ROM_SIZE, "GAPS": gaps, "OVERLAPS": overlaps,
            "SOURCE_OWNED": owned, "RANGES": len(rows)}


def validate(report_path: Path, baseline_report: Path) -> dict[str, Any]:
    report = json.loads(report_path.read_text(encoding="utf-8"))
    stages = report["stage_results"]
    baseline = CanonicalRomMap.from_report_path(baseline_report)
    baseline_audit = baseline.audit()
    map_generation = Path(stages["ASM CLOSURE"]["generation_dir"])
    generation_audit = _audit_db(map_generation / "knowledge.sqlite")
    runtime = json.loads((report_path.parent.parent / "live-worker-interactive-receipt.json").read_text(encoding="utf-8"))
    full_audit = stages["FULL ROM AUDIT"]
    generic = stages["GENERIC RECURSIVE CLOSURE"]
    required = {
        "runtime_outcome": runtime["status"],
        "runtime_frames": int(runtime["runtime"]["total_frames"]),
        "runtime_segments": int(runtime["runtime"]["audited_segments"]),
        "pipeline_state": report["pipeline_state"],
        "map_status": stages["REFRESHING MAP"]["status"],
        "map_source_owned": generation_audit["SOURCE_OWNED"],
        "generic_status": generic["status"],
        "generic_replay": generic["deterministic_replay_status"],
        "gap_ranking": generic["gap_ranking_status"],
        "asm_status": stages["ASM CLOSURE"]["status"],
        "asm_source_owned": int(stages["ASM CLOSURE"]["source_owned_after"]),
        "rom_audit_status": full_audit["status"],
        "rom_sha256": full_audit["full_rom_sha256"],
        "cleanup_status": stages["CLEANUP"]["status"],
    }
    checks = {
        "runtime_completed": required["runtime_outcome"] == "STOPPED_FRAME_LIMIT" and required["runtime_frames"] >= 1200 and required["runtime_segments"] > 0,
        "authoritative_baseline": baseline_audit == {"TOTAL_BYTES": ROM_SIZE, "GAPS": 0, "OVERLAPS": 0, "SOURCE_OWNED": 1487672},
        "runtime_map_integrity": generation_audit["TOTAL_BYTES"] == ROM_SIZE and generation_audit["GAPS"] == 0 and generation_audit["OVERLAPS"] == 0,
        "computed_ownership": generation_audit["SOURCE_OWNED"] == 1487672 and required["asm_source_owned"] == generation_audit["SOURCE_OWNED"],
        "generic_projection": required["generic_status"] == "NO_DELTA" and required["generic_replay"] == "PASS" and required["gap_ranking"] == "PASS",
        "full_rom_identity": required["rom_audit_status"] == "PASS_POSTRUN_FULL_ROM_AUDIT_V1" and required["rom_sha256"] == ROM_SHA256,
        "cleanup_fail_closed": required["cleanup_status"] == "RAW_RETAINED",
    }
    passed = all(checks.values())
    return {"status": "PASS_M14_1_CANONICAL_FULL_ROM_MAP_RUNTIME_VALIDATION_V1" if passed else "STOP_M14_1_RUNTIME_VALIDATION",
            "checks": checks, "required": required, "baseline_report_sha256": _sha(baseline_report),
            "baseline_map_sha256": baseline.hash(), "runtime_report_sha256": _sha(report_path),
            "generation_dir": str(map_generation), "generation_audit": generation_audit}


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[3]
    runtime_root = root / "build/thor-evidence/live-worker-control/m14-1-runtime-20260924-r3/post-run-analysis"
    result = validate(runtime_root / "report.json", root / "docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json")
    output = runtime_root / "m14-1-runtime-validation-receipt.json"
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"status": result["status"], "output": str(output), "checks": result["checks"]}, indent=2))
