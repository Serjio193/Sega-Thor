"""Build transactional exact ASM promotions from the accepted 2G knowledge map."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sqlite3
import subprocess
import sys
from pathlib import Path
from typing import Any

HERE = Path(__file__).resolve().parent
TOOLS = HERE.parent
ROOT = HERE.parents[2]
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from thor_evidence import map_driven_asm_closure as closure
from thor_evidence.rom_knowledge_map import TABLE_COLUMNS, canonical, sha256_bytes
import re_m12_auto_promote as split_tools

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 0x300000
MAP_HASH = "80828f5c178b5e7373e6530c11f98c02578aff43ab4661eb7c15b1724ff23d1f"
EMISSION_BYTES = {"ASM": 56134, "DATA": 233676, "ASSET": 1085110, "INCBIN": 1770808}
PROTECTED_OBJECT_TYPES = {"ROM_DATA", "GRAPHICS_STREAM", "AUDIO_DATA", "POINTER_TABLE", "Z80_PROGRAM"}
PROTECTED_CLASSIFICATIONS = {"DATA_REGION_SUPPORTED", "DATA_STRUCTURE_SUPPORTED",
                             "LOCAL_ROM_DERIVED_ASSET", "POINTER_TABLE"}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _map_hashes(db: sqlite3.Connection) -> dict[str, str]:
    rows = lambda table: [tuple(row) for row in db.execute(
        f"SELECT {','.join(TABLE_COLUMNS[table])} FROM {table} "
        f"ORDER BY {','.join(TABLE_COLUMNS[table])}")]
    structure = {table: rows(table) for table in
                 ("rom_range", "rom_object", "claim", "relation", "conflict")}
    evidence = {table: rows(table) for table in ("source_artifact", "evidence_ref")}
    structure_hash = sha256_bytes(canonical(structure).encode())
    evidence_hash = sha256_bytes(canonical(evidence).encode())
    emission_hash = sha256_bytes(canonical(rows("emission")).encode())
    return {"structure_hash": structure_hash, "evidence_index_hash": evidence_hash,
            "emission_hash": emission_hash,
            "map_hash": sha256_bytes((structure_hash + evidence_hash + emission_hash).encode())}


def read_knowledge_map(path: Path, receipt_path: Path) -> dict[str, Any]:
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    expected_hashes = receipt.get("knowledge_after", {}).get("hashes", {})
    if receipt.get("checkpoint") != "M12-ARCHIVIST-CANONICAL-KNOWLEDGE-PIPELINE-2G" or \
            receipt.get("status") != "PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1" or \
            expected_hashes.get("map_hash") != MAP_HASH or \
            receipt.get("knowledge_after", {}).get("metrics", {}).get("source_owned_bytes") != 1475600:
        raise ValueError("STOP_BASELINE_KNOWLEDGE_MISMATCH")
    uri = "file:" + path.resolve().as_posix() + "?mode=ro&immutable=1"
    db = sqlite3.connect(uri, uri=True)
    db.row_factory = sqlite3.Row
    try:
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise ValueError("STOP_BASELINE_KNOWLEDGE_MISMATCH:integrity")
        meta = {row[0]: row[1] for row in db.execute("SELECT key,value FROM map_meta")}
        if meta.get("rom_sha256") != ROM_SHA or int(meta.get("rom_size", 0)) != ROM_SIZE:
            raise ValueError("STOP_BASELINE_KNOWLEDGE_MISMATCH:ROM")
        hashes = _map_hashes(db)
        if hashes != expected_hashes:
            raise ValueError("STOP_BASELINE_KNOWLEDGE_MISMATCH:map_hash")
        emissions = {str(row[0]): int(row[1]) for row in db.execute(
            "SELECT emission_type,SUM(end-start) FROM emission GROUP BY emission_type")}
        owned = int(db.execute("SELECT SUM(end-start) FROM emission WHERE source_owned=1").fetchone()[0] or 0)
        conflicts = int(db.execute("SELECT COUNT(*) FROM conflict").fetchone()[0])
        executed_count = int(db.execute("""SELECT COUNT(DISTINCT o.object_id) FROM rom_object o
            JOIN claim c USING(object_id) WHERE o.object_type='M68K_INSTRUCTION'
            AND c.claim_type='EXECUTED_FROM_ROM'""").fetchone()[0])
        backlog = []
        for row in db.execute("""SELECT DISTINCT o.object_id,r.start,r.end,o.attributes_json
            FROM rom_object o JOIN rom_range r USING(range_id) JOIN claim c USING(object_id)
            WHERE o.object_type='M68K_INSTRUCTION' AND c.claim_type='EXECUTED_FROM_ROM'
            AND json_extract(o.attributes_json,'$.source_owned_bytes') < r.end-r.start
            ORDER BY r.start,r.end,o.object_id"""):
            backlog.append({"object_id": str(row[0]), "start": int(row[1]), "end": int(row[2]),
                            "attributes": json.loads(row[3])})
        if (owned, conflicts, executed_count, len(backlog), emissions) != (
                1475600, 0, 360, 225, EMISSION_BYTES):
            raise ValueError("STOP_BASELINE_KNOWLEDGE_MISMATCH:metrics")
        all_instructions = []
        for row in db.execute("""SELECT DISTINCT o.object_id,r.start,r.end,o.object_type,o.attributes_json
            FROM rom_object o JOIN rom_range r USING(range_id) ORDER BY r.start,r.end,o.object_id"""):
            all_instructions.append({"object_id": str(row[0]), "start": int(row[1]),
                "end": int(row[2]), "object_type": str(row[3]),
                "attributes": json.loads(row[4])})
        edges = [{"relation_id": str(row[0]), "source_object_id": str(row[1]),
                  "target_object_id": str(row[2])}
                 for row in db.execute("""SELECT relation_id,source_object_id,target_object_id
                    FROM relation WHERE relation_type='EXECUTED_NEXT' AND status='OBSERVED_RUNTIME'
                    AND target_object_id IS NOT NULL ORDER BY relation_id""")]
        owned_ranges = [(int(row[0]), int(row[1])) for row in db.execute(
            "SELECT start,end FROM emission WHERE source_owned=1 AND source_kind='CODE_VERIFIED' ORDER BY start,end")]
        emissions_rows = [tuple(row) for row in db.execute(
            "SELECT start,end,emission_type,source_owned FROM emission ORDER BY start,end")]
        protected = []
        for row in db.execute("""SELECT o.object_type,r.start,r.end,c.value_json
            FROM rom_object o JOIN rom_range r USING(range_id)
            LEFT JOIN claim c ON c.object_id=o.object_id AND c.claim_type='SOURCE_CLASS'
            ORDER BY r.start,r.end,o.object_id"""):
            classification = ""
            if row[3]:
                classification = str(json.loads(row[3]).get("classification", ""))
            if str(row[0]) in PROTECTED_OBJECT_TYPES or classification in PROTECTED_CLASSIFICATIONS:
                protected.append((int(row[1]), int(row[2])))
        return {"backlog": backlog, "all_instructions": all_instructions, "edges": edges,
                "owned_ranges": owned_ranges, "emissions": emissions_rows,
                "protected_ranges": protected, "metrics": {"source_owned": owned,
                "conflicts": conflicts, "executed_instructions": executed_count,
                "backlog": len(backlog), "emission_bytes": emissions},
                "hashes": hashes, "receipt_sha256": sha256_file(receipt_path)}
    finally:
        db.close()


def _manifest_owned(entry: dict[str, Any]) -> bool:
    return entry.get("kind") not in {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"} and \
        str(entry.get("confidence", "")).upper() not in {"PROBABLE", "CANDIDATE", "UNVERIFIED"}


def validate_base_manifest(path: Path, rom: bytes) -> tuple[dict[str, Any], list[dict[str, Any]]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    entries = sorted(data.get("entries", []), key=lambda item: int(item["start"]))
    closure.validate_partition(entries, len(rom))
    owned = sum(int(item["end"]) - int(item["start"]) for item in entries if _manifest_owned(item))
    if data.get("schema") != "oasis.full-rom-split.v1" or data.get("rom_sha256") != ROM_SHA or \
            len(rom) != ROM_SIZE or owned != 1475600 or \
            int(data.get("metrics", {}).get("SOURCE_OWNED_BYTES", -1)) != owned:
        raise ValueError("STOP_BASELINE_OWNERSHIP_MISMATCH")
    return data, entries


def _emission_blocker(snapshot: dict[str, Any], start: int, end: int) -> str | None:
    cursor = start
    for lo, hi, kind, owned in snapshot["emissions"]:
        lo, hi = int(lo), int(hi)
        if hi <= start or lo >= end:
            continue
        covered_start, covered_end = max(lo, start), min(hi, end)
        if covered_start != cursor:
            return "STOP_BOUNDARY_UNPROVEN"
        if str(kind) in {"DATA", "ASSET"}:
            return "STOP_MIXED_CODE_DATA_BOUNDARY"
        if str(kind) != "INCBIN" or int(owned):
            return "STOP_ALREADY_OWNED_NO_DELTA"
        cursor = covered_end
    return None if cursor == end else "STOP_BOUNDARY_UNPROVEN"


def _object_data_blocker(snapshot: dict[str, Any], start: int, end: int) -> str | None:
    return "STOP_MIXED_CODE_DATA_BOUNDARY" if any(
        start < hi and lo < end for lo, hi in snapshot["protected_ranges"]) else None


def _run(command: list[Path | str], cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    result = subprocess.run([str(item) for item in command], cwd=cwd,
                            text=True, capture_output=True, check=False)
    return result


def _write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, sort_keys=True, separators=(",", ":"),
                               ensure_ascii=True) + "\n", encoding="utf-8", newline="\n")


def run(args: argparse.Namespace) -> dict[str, Any]:
    rom_path, map_path, baseline_path = map(Path, (args.rom, args.map, args.baseline_manifest))
    receipt_path = Path(args.map_receipt)
    range_tool, assembler = Path(args.range_tool), Path(args.assembler)
    output, source_dir = Path(args.output).resolve(), Path(args.source_dir).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    source_root = (ROOT / "src/tools/m12_2f_asm").resolve()
    source_dir.relative_to(source_root)
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or sha256_bytes(rom) != ROM_SHA:
        raise ValueError("STOP_CANONICAL_ROM_IDENTITY_MISMATCH")
    snapshot = read_knowledge_map(map_path, receipt_path)
    base_data, base_entries = validate_base_manifest(baseline_path, rom)
    candidates = closure.merge_overlapping_candidates(
        closure.observed_chains(snapshot["backlog"], snapshot["edges"], ROM_SHA))
    if not candidates:
        raise ValueError("STOP_BASELINE_KNOWLEDGE_MISMATCH:no observed candidate chains")
    output.mkdir(parents=True)
    candidate_dir = output / "candidate-audit"
    candidate_dir.mkdir()
    backlog_by_id = {item["object_id"]: item for item in snapshot["backlog"]}
    starts = {int(item["start"]) for item in snapshot["backlog"]}
    observed_pairs = {(int(backlog_by_id[src]["start"]), int(backlog_by_id[dst]["start"]))
                      for edge in snapshot["edges"] for src, dst in
                      [(edge["source_object_id"], edge["target_object_id"])]
                      if src in backlog_by_id and dst in backlog_by_id}
    records: list[dict[str, Any]] = []
    exact_candidates: list[tuple[dict[str, Any], Path, dict[str, Any], dict[str, Any]]] = []
    for candidate in sorted(candidates, key=lambda item: (-len(item["instructions"]),
            -int(item["observed_edge_count"]), item["intervals"], item["candidate_id"])):
        start, end = candidate["intervals"][0]
        stem = f"{start:06X}-{end:06X}"
        record = {"candidate_id": candidate["candidate_id"], "start": start, "end": end,
                  "instruction_count": len(candidate["instructions"]),
                  "observed_edge_count": candidate["observed_edge_count"],
                  "executed_instruction_count": len(candidate["instructions"]),
                  "status": "STOP_BOUNDARY_UNPROVEN", "blockers": []}
        blocker = _emission_blocker(snapshot, start, end) or \
            _object_data_blocker(snapshot, start, end)
        if blocker:
            record["status"], record["blockers"] = blocker, [blocker]
            records.append(record)
            continue
        stem_dir = candidate_dir / stem
        stem_dir.mkdir()
        asm_path, decode_path = stem_dir / "candidate.asm", stem_dir / "decode.json"
        decoded = _run([range_tool, rom_path, hex(start), hex(end), asm_path, decode_path])
        if decoded.returncode:
            detail = (decoded.stdout + decoded.stderr).strip()
            record["status"] = "STOP_UNSUPPORTED_M68K_DECODE" if "UNSUPPORTED" in detail or "no exact IR" in detail else "STOP_BOUNDARY_UNPROVEN"
            record["blockers"] = [record["status"]]
            record["detail"] = detail[:300]
            records.append(record)
            continue
        decoded_json = json.loads(decode_path.read_text(encoding="utf-8"))
        proof = closure.prove_decoded_island(candidate, decoded_json, rom,
            snapshot["owned_ranges"], snapshot["protected_ranges"], starts,
            [(a, b) for a, b in observed_pairs if start <= a < end and start <= b < end])
        record["status"], record["blockers"] = proof["status"], proof["blockers"]
        record["flow_edges"] = proof["edges"]
        record["decoded_instruction_count"] = len(decoded_json["instructions"])
        if proof["status"] == "PASS_CLOSED_ASM_RANGE":
            assembler_bin = stem_dir / "candidate.bin"
            assembled = _run([assembler, "-m68000", "-no-opt", "-Fbin", "-o",
                              assembler_bin, asm_path])
            if assembled.returncode or not assembler_bin.is_file():
                record["status"] = "STOP_ASM_ROUNDTRIP_MISMATCH"
                record["blockers"] = [record["status"]]
                record["detail"] = (assembled.stdout + assembled.stderr).strip()[:300]
            else:
                roundtrip = closure.verify_roundtrip(rom[start:end], assembler_bin.read_bytes())
                record["roundtrip"] = roundtrip
                record["status"] = roundtrip["status"] if roundtrip["status"] != "PASS_ASM_ROUNDTRIP_EXACT" else "PASS_CLOSED_ASM_RANGE"
                if record["status"] == "PASS_CLOSED_ASM_RANGE":
                    exact_candidates.append((candidate, asm_path, decoded_json, proof))
        records.append(record)

    selected_starts, dependencies = closure.dependency_closed_starts([
        {"start": int(candidate["intervals"][0][0]), "edges": proof["edges"]}
        for candidate, _asm, _decoded, proof in exact_candidates])
    retained = []
    for item in exact_candidates:
        candidate = item[0]
        start = int(candidate["intervals"][0][0])
        if start in selected_starts:
            retained.append(item)
            continue
        record = next(row for row in records if row["candidate_id"] == candidate["candidate_id"])
        targets = [target for target in dependencies[start] if target not in selected_starts]
        record["status"] = "STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED"
        record["blockers"] = sorted(set(record["blockers"] + [record["status"]]))
        record["unclosed_candidate_targets"] = targets
    exact_candidates = retained

    blocker_counts: dict[str, int] = {}
    for record in records:
        for blocker in record["blockers"]:
            blocker_counts[blocker] = blocker_counts.get(blocker, 0) + 1
    if not exact_candidates:
        stopped = {"schema": "oasis.m12.map-driven-executed-asm-closure.2f.v1",
            "status": "STOP_NO_PROMOTABLE_EXECUTED_ASM_ISLAND", "rom_sha256": ROM_SHA,
            "baseline_map_hash": snapshot["hashes"]["map_hash"],
            "candidate_count": len(records), "closed_count": 0,
            "blocked_count": len(records), "blocker_counts": blocker_counts,
            "candidates": records, "promotions": [], "before": snapshot["metrics"],
            "map_hashes_before": snapshot["hashes"]}
        _write_json(output / "promotion_report.json", stopped)
        return stopped

    promotions = []
    staged_sources = output / "staged-sources"
    staged_sources.mkdir()
    for candidate, generated_asm, decoded_json, proof in exact_candidates:
        start, end = candidate["intervals"][0]
        source_path = staged_sources / f"sub_{start:06X}.asm"
        source_text = generated_asm.read_text(encoding="utf-8")
        first = "; M12 2F MAP-driven executed ASM; canonical ROM " + ROM_SHA
        lines = source_text.splitlines()
        if lines and lines[0].startswith("; Local ROM-derived code."):
            lines[0] = first
        else:
            lines.insert(0, first)
        generated_source = ("\n".join(lines) + "\n").encode("utf-8")
        if source_path.exists() and source_path.read_bytes() != generated_source:
            raise ValueError("STOP_PROMOTED_ASM_SOURCE_CONFLICT")
        source_path.write_bytes(generated_source)
        promotions.append({"candidate_id": candidate["candidate_id"], "start": start, "end": end,
            "bytes": end - start, "instruction_count": len(decoded_json["instructions"]),
            "executed_instruction_count": len(candidate["instructions"]),
            "observed_edge_count": candidate["observed_edge_count"],
            "status": "PASS_CLOSED_ASM_RANGE", "artifact": f"code/{source_path.name}",
            "source_path": (source_root / source_path.name).relative_to(ROOT).as_posix(),
            "asm_sha256": hashlib.sha256(generated_source).hexdigest(),
            "canonical_bytes_sha256": hashlib.sha256(rom[start:end]).hexdigest(),
            "proof_edges": proof["edges"]})

    entries = closure.promote_entries(base_entries, promotions)
    sources_by_start = {int(entry["start"]): split_tools.auto.resolve_artifact(
        baseline_path, str(entry["artifact"])) for entry in base_entries
        if entry.get("emitted_artifact_type") == "asm"}
    source_map: dict[int, Path] = {}
    promotion_paths = {int(item["start"]): staged_sources / Path(item["source_path"]).name
                       for item in promotions}
    for index, entry in enumerate(entries):
        if entry.get("emitted_artifact_type") != "asm":
            continue
        start = int(entry["start"])
        source_map[index] = promotion_paths[start] if start in promotion_paths else sources_by_start[start]
    materialized = output / "materialized"
    materialized_entries = split_tools.auto.materialize(materialized, entries, rom, source_map)
    if not split_tools.auto.verify_full(materialized, rom, assembler, materialized_entries)[0]:
        raise ValueError("STOP_FULL_ROM_REBUILD_MISMATCH")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    metrics = split_tools.metrics(materialized_entries, len(rom))
    if metrics["SOURCE_OWNED_BYTES"] != 1475600 + sum(item["bytes"] for item in promotions):
        raise ValueError("STOP_SOURCE_OWNED_DELTA_MISMATCH")
    manifest = split_tools.auto.manifest_for(materialized_entries, len(rom))
    manifest.update({"rom_sha256": ROM_SHA, "full_match": True,
        "metrics": metrics,
        "hashes": {"sha256": hashlib.sha256(rebuilt).hexdigest()},
        "promotion_checkpoint": "M12-AUTO67-LIVE-FORWARD-WORKER-1B-MAP-DRIVEN-EXECUTED-ASM-CLOSURE-2F",
        "promotions": promotions})
    _write_json(materialized / "manifest.json", manifest)
    after_emission: dict[str, int] = {}
    for entry in materialized_entries:
        emission_type = {
            "asm": "ASM",
            "blob": "INCBIN",
            "rom_asset": "ASSET" if entry.get("kind") == "LOCAL_ROM_DERIVED_ASSET" else "DATA",
        }.get(str(entry.get("emitted_artifact_type", "")))
        if emission_type is None:
            raise ValueError("STOP_EMISSION_TYPE_UNSUPPORTED")
        after_emission[emission_type] = after_emission.get(emission_type, 0) + int(entry["end"]) - int(entry["start"])
    report = {"schema": "oasis.m12.map-driven-executed-asm-closure.2f.v1",
        "status": "PASS_PROMOTION_TRANSACTION", "rom_sha256": ROM_SHA,
        "baseline_map_hash": snapshot["hashes"]["map_hash"],
        "baseline_map_receipt_sha256": snapshot["receipt_sha256"],
        "baseline_ownership_bytes": 1475600, "after_ownership_bytes": metrics["SOURCE_OWNED_BYTES"],
        "ownership_delta": metrics["SOURCE_OWNED_BYTES"] - 1475600,
        "baseline_emission_bytes": EMISSION_BYTES,
        "after_emission_bytes": after_emission,
        "executed_instruction_objects": 360, "executed_not_fully_owned_before": 225,
        "candidate_count": len(records), "closed_count": sum(x["status"] == "PASS_CLOSED_ASM_RANGE" for x in records),
        "blocked_count": sum(x["status"] != "PASS_CLOSED_ASM_RANGE" for x in records),
        "blocker_counts": blocker_counts,
        "promoted_islands": len(promotions), "promoted_instruction_objects": sum(x["instruction_count"] for x in promotions),
        "promoted_executed_instruction_objects": sum(x["executed_instruction_count"] for x in promotions),
        "promoted_bytes": sum(x["bytes"] for x in promotions), "promotions": promotions,
        "candidates": records, "full_rom_sha256": hashlib.sha256(rebuilt).hexdigest(),
        "full_rom_exact": rebuilt == rom, "materialized_manifest_sha256": sha256_file(materialized / "manifest.json"),
        "metric_source_owned_reconciled": metrics["SOURCE_OWNED_BYTES"]}
    _write_json(output / "promotion_report.json", report)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--map-receipt", type=Path, required=True)
    parser.add_argument("--baseline-manifest", type=Path, required=True)
    parser.add_argument("--range-tool", type=Path, required=True)
    parser.add_argument("--assembler", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-dir", type=Path, required=True)
    args = parser.parse_args()
    result = run(args)
    print(json.dumps({key: result.get(key) for key in ("status", "candidate_count",
        "closed_count", "blocked_count", "promoted_islands", "promoted_bytes",
        "ownership_delta", "full_rom_sha256")}, sort_keys=True, indent=2))
    return 0 if result["status"] == "PASS_PROMOTION_TRANSACTION" else 1


if __name__ == "__main__":
    raise SystemExit(main())
