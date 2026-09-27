"""Reconcile M14 ownership from an accepted, exact Stage 7 generation."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from rom_knowledge_map import KnowledgeStore  # noqa: E402
from rom_knowledge_import import _markdown  # noqa: E402

ROM_SIZE = 0x300000
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
EXPECTED_OWNERSHIP = 1_487_672
PROMOTION_BYTES = 284
PROMOTION_COUNT = 16


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _owned(entry: dict[str, Any]) -> bool:
    return int(entry.get("source_owned", 0)) == 1


def _sum_owned(db_path: Path) -> int:
    db = sqlite3.connect(db_path)
    try:
        return int(db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0])
    finally:
        db.close()


def _coverage(db_path: Path) -> tuple[int, int, int]:
    db = sqlite3.connect(db_path)
    try:
        rows = db.execute("SELECT start,end FROM emission ORDER BY start,end").fetchall()
    finally:
        db.close()
    cursor, gaps, overlaps = 0, 0, 0
    for start, end in rows:
        if start != cursor:
            gaps += max(0, start - cursor)
            overlaps += max(0, cursor - start)
        cursor = max(cursor, end)
    gaps += max(0, ROM_SIZE - cursor)
    return gaps, overlaps, len(rows)


def _stage7_proof(result_path: Path) -> dict[str, Any] | None:
    data = json.loads(result_path.read_text(encoding="utf-8"))
    if data.get("status") != "PASS_POSTRUN_MAP_DRIVEN_ASM_CLOSURE_V1" or \
            data.get("source_owned_before") != 1_487_388 or \
            data.get("source_owned_after") != EXPECTED_OWNERSHIP or \
            data.get("promoted_bytes") != PROMOTION_BYTES or \
            len(data.get("promoted_ranges", [])) != PROMOTION_COUNT:
        return None
    materialized = Path(data["output_paths"]["materialized"])
    generation = Path(data["generation_dir"])
    db_path = generation / "knowledge.sqlite"
    rebuilt = materialized / "rebuilt.rom"
    manifest = materialized / "manifest.json"
    post_run = result_path.parents[4] / "post-run-analysis"
    asm_receipt = post_run / "postrun_asm_closure_receipt.json"
    audit_receipt = post_run / "postrun_full_rom_audit.json"
    if not all(path.is_file() for path in (db_path, rebuilt, manifest,
                                           asm_receipt, audit_receipt)):
        return None
    asm = json.loads(asm_receipt.read_text(encoding="utf-8"))
    audit = json.loads(audit_receipt.read_text(encoding="utf-8"))
    manifest_sha = sha256(manifest)
    if asm.get("status") != "PASS_POSTRUN_MAP_DRIVEN_ASM_CLOSURE_V1" or \
            asm.get("source_owned_after") != EXPECTED_OWNERSHIP or \
            asm.get("promoted_bytes") != PROMOTION_BYTES or \
            asm.get("promoted_range_count") != PROMOTION_COUNT or \
            audit.get("status") != "PASS_POSTRUN_FULL_ROM_AUDIT_V1" or \
            audit.get("source_owned_after") != EXPECTED_OWNERSHIP or \
            audit.get("input_hashes", {}).get("manifest_sha256") != manifest_sha:
        return None
    rom = rebuilt.read_bytes()
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        return None
    if _sum_owned(db_path) != EXPECTED_OWNERSHIP or _coverage(db_path)[:2] != (0, 0):
        return None
    ranges = {(int(row["start"]), int(row["end"])) for row in data["promoted_ranges"]}
    if sum(end - start for start, end in ranges) != PROMOTION_BYTES:
        return None
    for start, end in ranges:
        if hashlib.sha256(rom[start:end]).hexdigest() not in _roundtrip_hashes(data, start, end):
            return None
    return {"result": data, "result_path": result_path, "db": db_path,
            "manifest": manifest, "rebuilt": rebuilt, "ranges": sorted(ranges)}


def _roundtrip_hashes(data: dict[str, Any], start: int, end: int) -> set[str]:
    hashes: set[str] = set()

    def visit(value: Any) -> None:
        if isinstance(value, dict):
            if value.get("start") == start and value.get("end") == end and \
                    value.get("status") == "PASS_CLOSED_ASM_RANGE":
                roundtrip = value.get("roundtrip", {})
                if roundtrip.get("status") == "PASS_ASM_ROUNDTRIP_EXACT":
                    hashes.add(str(roundtrip.get("sha256")))
            for child in value.values():
                visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)

    visit(data)
    return hashes


def find_authoritative_stage7(root: Path = ROOT) -> dict[str, Any]:
    candidates = []
    for result_path in root.glob("build/**/stage7-result.json"):
        try:
            proof = _stage7_proof(result_path)
        except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError):
            proof = None
        if proof:
            candidates.append(proof)
    if not candidates:
        raise ValueError("STOP_M14_ACCEPTED_STAGE7_AUTHORITY_MISSING")
    candidates.sort(key=lambda item: (sha256(item["db"]), sha256(item["manifest"]),
                                      str(item["result_path"])))
    selected = candidates[0]
    selected["candidate_count"] = len(candidates)
    return selected


def _export_from_generation(db_path: Path, old_report: dict[str, Any]) -> dict[str, Any]:
    store = KnowledgeStore(db_path, ROM_SHA256, ROM_SIZE, read_only=True)
    try:
        report = store.export(old_report.get("ownership_reconciliation", {}),
                              old_report.get("independent_audit", {}),
                              old_report.get("idempotence", {}))
    finally:
        store.close()
    for key in ("carver_hypothesis_count", "runtime_import"):
        if key in old_report:
            report[key] = old_report[key]
    report["status"] = "PASS_CANONICAL_ROM_KNOWLEDGE_MAP_V1"
    report["m14_ownership_reconciliation"] = {
        "status": "PASS_CANONICAL_OWNERSHIP_BASELINE_RECONCILIATION_V1",
        "source": "accepted_stage7_generation", "source_owned_before": 1_487_388,
        "source_owned_after": EXPECTED_OWNERSHIP, "source_owned_delta": PROMOTION_BYTES,
        "promoted_range_count": PROMOTION_COUNT,
    }
    return report


def reconcile(root: Path = ROOT, report_path: Path | None = None) -> dict[str, Any]:
    report_path = report_path or root / "docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json"
    old_report = json.loads(report_path.read_text(encoding="utf-8"))
    authority = find_authoritative_stage7(root)
    report = _export_from_generation(authority["db"], old_report)
    computed = int(report["metrics"]["source_owned_bytes"])
    if computed != EXPECTED_OWNERSHIP:
        raise ValueError("STOP_M14_REGENERATED_OWNERSHIP_MISMATCH")
    report_path.write_text(json.dumps(report, sort_keys=True, separators=(",", ":"),
                                  ensure_ascii=True) + "\n", encoding="utf-8", newline="\n")
    markdown_path = report_path.with_suffix(".md")
    markdown_path.write_text(_markdown(report), encoding="utf-8", newline="\n")
    return {"status": "PASS_CANONICAL_OWNERSHIP_BASELINE_RECONCILIATION_V1",
            "report": report_path, "report_sha256": sha256(report_path),
            "markdown": markdown_path, "authority": authority,
            "source_owned": computed, "map_metrics": report["metrics"]}


if __name__ == "__main__":
    result = reconcile()
    print(json.dumps({"status": result["status"], "source_owned": result["source_owned"],
                      "report_sha256": result["report_sha256"],
                      "authority": str(result["authority"]["result_path"])}, indent=2))
