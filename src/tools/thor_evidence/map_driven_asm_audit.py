"""Independently audit a 2F executed-ASM promotion manifest and full rebuild."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sqlite3
import subprocess
import sys
import tempfile
from pathlib import Path, PurePosixPath
from typing import Any

TOOL_ROOT = str(Path(__file__).resolve().parents[1])
if TOOL_ROOT not in sys.path:
    sys.path.insert(0, TOOL_ROOT)

from thor_evidence.rom_knowledge_map import TABLE_COLUMNS, canonical, sha256_bytes
import re_full_split_run as full_split

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 0x300000
PROMOTION_SOURCE = "M12_MAP_DRIVEN_EXECUTED_ASM_CLOSURE_2F"
DATA_TYPES = {"ROM_DATA", "GRAPHICS_STREAM", "AUDIO_DATA", "POINTER_TABLE", "Z80_PROGRAM"}
DATA_CLASSES = {"DATA_REGION_SUPPORTED", "DATA_STRUCTURE_SUPPORTED", "LOCAL_ROM_DERIVED_ASSET",
                "POINTER_TABLE"}


def _sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _file_sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _owned(entry: dict[str, Any]) -> bool:
    return entry.get("kind") not in {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"} and \
        str(entry.get("confidence", "")).upper() not in {"PROBABLE", "CANDIDATE", "UNVERIFIED"}


def _mask(entries: list[dict[str, Any]]) -> bytearray:
    result = bytearray(ROM_SIZE)
    cursor = 0
    for entry in sorted(entries, key=lambda value: int(value["start"])):
        start, end = int(entry["start"]), int(entry["end"])
        if start != cursor or end <= start or end > ROM_SIZE:
            raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")
        if _owned(entry):
            result[start:end] = b"\1" * (end - start)
        cursor = end
    if cursor != ROM_SIZE:
        raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")
    return result


def _map_hashes(db: sqlite3.Connection) -> dict[str, str]:
    def rows(table: str) -> list[tuple[Any, ...]]:
        columns = TABLE_COLUMNS[table]
        return [tuple(row) for row in db.execute(
            f"SELECT {','.join(columns)} FROM {table} ORDER BY {','.join(columns)}")]
    structure = {table: rows(table) for table in
                 ("rom_range", "rom_object", "claim", "relation", "conflict")}
    evidence = {table: rows(table) for table in ("source_artifact", "evidence_ref")}
    structure_hash = sha256_bytes(canonical(structure).encode())
    evidence_hash = sha256_bytes(canonical(evidence).encode())
    emission_hash = sha256_bytes(canonical(rows("emission")).encode())
    return {"structure_hash": structure_hash, "evidence_index_hash": evidence_hash,
            "emission_hash": emission_hash,
            "map_hash": sha256_bytes((structure_hash + evidence_hash + emission_hash).encode())}


def _emission_metrics(entries: list[dict[str, Any]]) -> dict[str, int]:
    values = {"ASM": 0, "DATA": 0, "ASSET": 0, "INCBIN": 0}
    for entry in entries:
        kind = str(entry.get("emitted_artifact_type", ""))
        if kind == "asm":
            category = "ASM"
        elif kind == "blob":
            category = "INCBIN"
        elif kind == "rom_asset":
            category = "ASSET" if entry.get("kind") == "LOCAL_ROM_DERIVED_ASSET" else "DATA"
        else:
            raise ValueError("STOP_EMISSION_TYPE_UNSUPPORTED")
        values[category] += int(entry["end"]) - int(entry["start"])
    return values


def _run(command: list[str | Path]) -> subprocess.CompletedProcess[str]:
    return subprocess.run([str(value) for value in command], text=True,
                          capture_output=True, check=False)


def _audit_control(decoded: dict[str, Any], start: int, end: int,
                   baseline_entries: list[dict[str, Any]]) -> None:
    code_ranges = [(int(entry["start"]), int(entry["end"])) for entry in baseline_entries
                  if entry.get("kind") == "CODE_VERIFIED" and _owned(entry)]
    instructions = decoded["instructions"]
    addresses = {int(row["address"]) for row in instructions}
    for index, row in enumerate(instructions):
        flow = str(row.get("flow", "unsupported"))
        if not row.get("supported") or flow in {"unsupported", "indirect_call", "indirect_jump"}:
            raise ValueError("STOP_UNSUPPORTED_OR_INDIRECT_CONTROL_FLOW")
        target = row.get("branch_target")
        if flow in {"direct_branch", "direct_call", "direct_jump"}:
            if target is None:
                raise ValueError("STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED")
            target = int(target)
            if not (start <= target < end or any(a <= target < b for a, b in code_ranges)):
                raise ValueError("STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED")
        if flow == "return":
            continue
        fallthrough = flow == "none" or flow == "direct_call" or \
            (flow == "direct_branch" and str(row.get("operation", "")).lower() != "bra")
        if fallthrough:
            next_address = int(row["address"]) + 2 * len(row.get("raw_words", []))
            if not (start <= next_address < end or
                    any(a <= next_address < b for a, b in code_ranges)):
                raise ValueError("STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED")
            if start <= next_address < end and next_address not in addresses:
                raise ValueError("STOP_BOUNDARY_UNPROVEN")
        if index + 1 < len(instructions):
            next_row = instructions[index + 1]
            actual_end = int(row["address"]) + 2 * len(row.get("raw_words", []))
            if actual_end != int(next_row["address"]):
                raise ValueError("STOP_INSTRUCTION_BOUNDARY_GAP")


def audit(args: argparse.Namespace) -> dict[str, Any]:
    rom_path, base_path, final_path = map(Path, (args.rom, args.baseline_manifest,
                                                 args.final_manifest))
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or _sha(rom) != ROM_SHA:
        raise ValueError("STOP_CANONICAL_ROM_IDENTITY_MISMATCH")
    baseline = json.loads(base_path.read_text(encoding="utf-8"))
    final = json.loads(final_path.read_text(encoding="utf-8"))
    before_entries, after_entries = baseline["entries"], final["entries"]
    before_mask, after_mask = _mask(before_entries), _mask(after_entries)
    if int(baseline.get("metrics", {}).get("SOURCE_OWNED_BYTES", -1)) != 1475600:
        raise ValueError("STOP_BASELINE_OWNERSHIP_MISMATCH")
    if final.get("rom_sha256") != ROM_SHA or final.get("full_match") is not True or \
            int(final.get("metrics", {}).get("CONFLICT_BYTES", 0)) != 0:
        raise ValueError("STOP_FINAL_MANIFEST_IDENTITY_OR_CONFLICT")

    promotions = [entry for entry in after_entries if entry.get("source") == PROMOTION_SOURCE]
    if not promotions:
        raise ValueError("STOP_NO_PROMOTED_EXECUTED_ASM")
    added = [index for index, (before, after) in enumerate(zip(before_mask, after_mask))
             if after and not before]
    removed = [index for index, (before, after) in enumerate(zip(before_mask, after_mask))
               if before and not after]
    promoted_ranges = sorted((int(entry["start"]), int(entry["end"])) for entry in promotions)
    expected_added = {offset for start, end in promoted_ranges for offset in range(start, end)}
    if removed or set(added) != expected_added:
        raise ValueError("STOP_SOURCE_OWNED_DELTA_MISMATCH")
    if sum(after_mask) != 1475600 + len(expected_added) or \
            int(final.get("metrics", {}).get("SOURCE_OWNED_BYTES", -1)) != sum(after_mask):
        raise ValueError("STOP_SOURCE_OWNED_DELTA_MISMATCH")

    map_path, receipt_path = Path(args.map), Path(args.map_receipt)
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    map_uri = "file:" + map_path.resolve().as_posix() + "?mode=ro&immutable=1"
    db = sqlite3.connect(map_uri, uri=True)
    db.row_factory = sqlite3.Row
    try:
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise ValueError("STOP_BASELINE_KNOWLEDGE_INTEGRITY")
        hashes = _map_hashes(db)
        if hashes != receipt.get("knowledge_after", {}).get("hashes") or \
                hashes["map_hash"] != "80828f5c178b5e7373e6530c11f98c02578aff43ab4661eb7c15b1724ff23d1f":
            raise ValueError("STOP_BASELINE_KNOWLEDGE_HASH_MISMATCH")
        if int(db.execute("SELECT COUNT(*) FROM conflict").fetchone()[0]) != 0 or \
                int(db.execute("SELECT SUM(end-start) FROM emission WHERE source_owned=1").fetchone()[0]) != 1475600:
            raise ValueError("STOP_BASELINE_KNOWLEDGE_METRICS_MISMATCH")

        interval_audits = []
        with tempfile.TemporaryDirectory(prefix="thor-2f-audit-") as temp_name:
            temp = Path(temp_name)
            for entry in promotions:
                start, end = int(entry["start"]), int(entry["end"])
                if entry.get("kind") != "CODE_VERIFIED" or entry.get("emitted_artifact_type") != "asm":
                    raise ValueError("STOP_PROMOTION_NOT_ASM")
                object_rows = list(db.execute("""SELECT o.object_id,o.object_type,o.attributes_json,
                    r.start,r.end FROM rom_object o JOIN rom_range r USING(range_id)
                    JOIN claim c USING(object_id) WHERE o.object_type='M68K_INSTRUCTION'
                    AND c.claim_type='EXECUTED_FROM_ROM' AND r.start>=? AND r.end<=?
                    ORDER BY r.start,r.end,o.object_id""", (start, end)))
                if not object_rows:
                    raise ValueError("STOP_PROMOTION_HAS_NO_RUNTIME_INSTRUCTION")
                cursor = start
                for row in object_rows:
                    row_start, row_end = int(row["start"]), int(row["end"])
                    attrs = json.loads(row["attributes_json"])
                    if row_start != cursor or attrs.get("bytes_sha256") != _sha(rom[row_start:row_end]):
                        raise ValueError("STOP_MAP_INSTRUCTION_BOUNDARY_OR_BYTES")
                    cursor = row_end
                if cursor != end:
                    raise ValueError("STOP_MAP_INSTRUCTION_BOUNDARY_OR_BYTES")
                edge_rows = list(db.execute("""SELECT sr.start,tr.start FROM relation a
                    JOIN rom_object so ON so.object_id=a.source_object_id
                    JOIN rom_range sr ON sr.range_id=so.range_id
                    JOIN rom_object t ON t.object_id=a.target_object_id
                    JOIN rom_range tr ON tr.range_id=t.range_id
                    WHERE a.relation_type='EXECUTED_NEXT' AND a.status='OBSERVED_RUNTIME'
                    AND sr.start>=? AND sr.end<=? AND tr.start>=? AND tr.end<=?
                    AND sr.end=tr.start ORDER BY sr.start,tr.start""", (start, end, start, end)))
                actual_pairs = [(int(row[0]), int(row[1])) for row in edge_rows]
                expected_pairs = [(int(object_rows[index]["start"]), int(object_rows[index + 1]["start"]))
                                  for index in range(len(object_rows) - 1)]
                actual_edges = len(actual_pairs)
                if actual_pairs != expected_pairs:
                    raise ValueError("STOP_RUNTIME_LINEAGE_EDGE_MISMATCH")
                data_rows = list(db.execute("""SELECT o.object_type,r.start,r.end,c.value_json
                    FROM rom_object o JOIN rom_range r USING(range_id)
                    LEFT JOIN claim c ON c.object_id=o.object_id AND c.claim_type='SOURCE_CLASS'
                    WHERE r.start<? AND r.end>?""", (end, start)))
                for row in data_rows:
                    classification = json.loads(row["value_json"]).get("classification", "") \
                        if row["value_json"] else ""
                    if row["object_type"] in DATA_TYPES or classification in DATA_CLASSES:
                        raise ValueError("STOP_MIXED_CODE_DATA_BOUNDARY")

                asm_path = final_path.parent / Path(*PurePosixPath(str(entry["artifact"])).parts)
                source_path = Path(args.source_dir) / f"sub_{start:06X}.asm"
                if not asm_path.is_file() or not source_path.is_file() or \
                        asm_path.read_bytes() != source_path.read_bytes():
                    raise ValueError("STOP_PROMOTED_ASM_ARTIFACT_MISMATCH")
                decode_asm, decode_json = temp / f"{start:06X}.asm", temp / f"{start:06X}.json"
                decoded = _run([Path(args.range_tool), rom_path, hex(start), hex(end),
                                decode_asm, decode_json])
                if decoded.returncode:
                    raise ValueError("STOP_INDEPENDENT_DECODER_FAILED:" + decoded.stderr[-200:])
                decoded_json = json.loads(decode_json.read_text(encoding="utf-8"))
                decoded_rows = decoded_json.get("instructions", [])
                if [int(row["address"]) for row in decoded_rows] != [int(row["start"]) for row in object_rows] or \
                        len(decoded_rows) != len(object_rows):
                    raise ValueError("STOP_INDEPENDENT_DECODER_MAP_MISMATCH")
                _audit_control(decoded_json, start, end, before_entries)
                binary = temp / f"{start:06X}.bin"
                assembled = _run([Path(args.assembler), "-m68000", "-no-opt", "-Fbin", "-o", binary, asm_path])
                if assembled.returncode or not binary.is_file() or binary.read_bytes() != rom[start:end]:
                    raise ValueError("STOP_INDEPENDENT_VASM_ROUNDTRIP_MISMATCH")
                interval_audits.append({"start": start, "end": end, "bytes": end - start,
                    "instruction_count": len(object_rows), "observed_edge_count": actual_edges,
                    "asm_sha256": _file_sha(asm_path), "rom_slice_sha256": _sha(rom[start:end]),
                    "status": "PASS_INDEPENDENT_INTERVAL_AUDIT"})
    finally:
        db.close()

    # Recreate the accepted full-layout link in a fresh directory; never read producer output ROM.
    with tempfile.TemporaryDirectory(prefix="thor-2f-full-rom-") as temp_name:
        temp = Path(temp_name)
        for entry in after_entries:
            artifact = PurePosixPath(str(entry.get("artifact", "")))
            path = (final_path.parent / Path(*artifact.parts)).resolve()
            try:
                path.relative_to(final_path.parent.resolve())
            except ValueError as exc:
                raise ValueError("STOP_EMISSION_ARTIFACT_PATH_INVALID") from exc
            if not path.is_file():
                raise ValueError("STOP_EMISSION_ARTIFACT_MISSING")
            destination = temp / Path(*artifact.parts)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, destination)
        full_split.write_layout(temp, after_entries)
        output_rom = temp / "independent-rebuilt.rom"
        linked = _run([Path(args.assembler), "-m68000", "-no-opt", "-Fbin", "-o",
                       output_rom, temp / "full_layout.asm"])
        if linked.returncode or not output_rom.is_file():
            raise ValueError("STOP_INDEPENDENT_FULL_ROM_LINK_FAILED:" + linked.stderr[-500:])
        rebuilt = output_rom.read_bytes()
    if rebuilt != rom or _sha(rebuilt) != ROM_SHA:
        raise ValueError("STOP_FULL_ROM_REBUILD_MISMATCH")

    before_emission = _emission_metrics(before_entries)
    after_emission = _emission_metrics(after_entries)
    if before_emission != {"ASM": 56134, "DATA": 233676, "ASSET": 1085110, "INCBIN": 1770808}:
        raise ValueError("STOP_BASELINE_EMISSION_METRICS_MISMATCH")
    if after_emission["ASM"] != before_emission["ASM"] + len(expected_added) or \
            after_emission["INCBIN"] != before_emission["INCBIN"] - len(expected_added) or \
            any(after_emission[key] != before_emission[key] for key in ("DATA", "ASSET")):
        raise ValueError("STOP_EMISSION_DELTA_MISMATCH")
    return {"schema": "oasis.m12.map-driven-executed-asm-closure.2f.audit.v1",
        "status": "PASS_INDEPENDENT_MAP_DRIVEN_ASM_CLOSURE_AUDIT",
        "rom_sha256": ROM_SHA, "baseline_map_hash": hashes["map_hash"],
        "baseline_ownership_bytes": 1475600, "after_ownership_bytes": sum(after_mask),
        "ownership_delta": len(expected_added), "baseline_emission_bytes": before_emission,
        "after_emission_bytes": after_emission, "promoted_intervals": interval_audits,
        "promoted_bytes": len(expected_added), "preserved_prior_owned_bytes": not removed,
        "full_rom_exact": True, "full_rom_sha256": _sha(rebuilt),
        "map_conflicts": 0, "partition_bytes": ROM_SIZE}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "map", "map-receipt", "baseline-manifest", "final-manifest",
                 "range-tool", "assembler", "source-dir", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    result = audit(args)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, sort_keys=True, separators=(",", ":")) + "\n",
                           encoding="utf-8", newline="\n")
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
