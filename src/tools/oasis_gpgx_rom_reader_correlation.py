#!/usr/bin/env python3
"""Deterministic correlation of GPGX analysis ROM reads with reader PCs."""

from __future__ import annotations

import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

CANONICAL_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3145728
READER_CLASSES = (
    "ASM_ROUNDTRIP_EXACT", "CODE_STATIC_SUPPORTED", "CODE_EXECUTED_AT_ADDRESS",
    "RUNTIME_EXECUTED_UNKNOWN", "UNKNOWN",
)


def number(value: object) -> int:
    if isinstance(value, str):
        return int(value, 16) if value.lower().startswith("0x") else int(value)
    return int(value)


def hex_address(value: int) -> str:
    return f"0x{value:06X}"


def load(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bitmap_count(bitmap: bytes) -> int:
    return sum(byte.bit_count() for byte in bitmap)


def bitmap_set(bitmap: bytes, address: int) -> bool:
    return bool(bitmap[address >> 3] & (1 << (address & 7)))


def source_classifications(path: Path) -> dict[int, str]:
    document = load(path)
    result: dict[int, str] = {}
    for item in document.get("instructions", []):
        address = number(item["address"])
        value = item.get("classification", "UNKNOWN")
        if address in result and result[address] != value:
            raise ValueError(f"conflicting classification at {hex_address(address)}")
        result[address] = value
    if not result:
        raise ValueError("classification artifact has no instructions")
    return result


def normalize_classification(source: str, executed: bool) -> str:
    if source == "CODE_EXECUTED":
        return "CODE_EXECUTED_AT_ADDRESS"
    if source in {"ASM_ROUNDTRIP_EXACT", "CODE_STATIC_SUPPORTED"}:
        return source
    if executed and source in {"UNKNOWN", "RUNTIME_EXECUTED_UNKNOWN"}:
        return "RUNTIME_EXECUTED_UNKNOWN"
    return "UNKNOWN"


def known_code_ranges(classifications: dict[int, str]) -> list[dict]:
    items = sorted(
        (address, normalize_classification(value, True))
        for address, value in classifications.items()
        if value not in {"UNKNOWN", "RUNTIME_EXECUTED_UNKNOWN"}
    )
    result: list[dict] = []
    for address, classification in items:
        if (result and result[-1]["classification"] == classification and
                address == result[-1]["end"] + 2):
            result[-1]["end"] = address
        else:
            result.append({"start": address, "end": address,
                           "classification": classification})
    return result


def nearest_code_range(ranges: list[dict], address: int) -> dict | None:
    if not ranges:
        return None
    def key(item: dict) -> tuple[int, int]:
        distance = (0 if item["start"] <= address <= item["end"] else
                    item["start"] - address if address < item["start"] else
                    address - item["end"])
        return distance, item["start"]
    item = min(ranges, key=key)
    distance = key(item)[0]
    return {"start": hex_address(item["start"]), "end": hex_address(item["end"]),
            "classification": item["classification"], "distance": distance}


def read_regions(path: Path, bitmap: bytes) -> list[dict]:
    source = load(path)
    if not isinstance(source, list):
        raise ValueError("analysis ranges must be a JSON array")
    unique: dict[tuple[int, int], dict] = {}
    for item in source:
        start, end = number(item["start"]), number(item["end"])
        reader = number(item["first_reader_pc"])
        width = int(item["access_width"])
        observed_bytes = int(item.get("bytes_observed", -1))
        if not (0 <= start <= end < ROM_SIZE):
            raise ValueError("analysis region is outside canonical ROM")
        if reader & 1 or not 0 <= reader < ROM_SIZE:
            raise ValueError(f"invalid first reader PC {hex_address(reader)}")
        if width not in {1, 2, 4}:
            raise ValueError(f"invalid access width at {hex_address(start)}")
        if observed_bytes != end - start + 1:
            raise ValueError(f"analysis region byte count mismatch at {hex_address(start)}")
        if any(not bitmap_set(bitmap, address) for address in range(start, end + 1)):
            raise ValueError(f"analysis range is not represented in bitmap: {hex_address(start)}")
        value = {"start": start, "end": end, "observed_bytes": end - start + 1,
                 "first_reader_pc": reader, "first_access_width": width,
                 "range_reader_executed": bool(item.get("reader_executed", False))}
        key = (start, end)
        if key in unique and unique[key] != value:
            raise ValueError(f"conflicting duplicate region {hex_address(start)}")
        unique[key] = value
    regions = sorted(unique.values(), key=lambda item: (item["start"], item["end"]))
    if sum(item["observed_bytes"] for item in regions) != bitmap_count(bitmap):
        raise ValueError("analysis ranges do not exactly cover analysis bitmap")
    return regions


def validate_provenance(rom: Path, analysis_meta: Path, bitmap: Path,
                        evidence: dict) -> dict:
    rom_hash = sha256(rom)
    if rom_hash != CANONICAL_SHA256 or rom_hash != evidence.get("canonical_rom_sha256"):
        raise ValueError("canonical ROM SHA-256 does not match project/evidence identity")
    if rom.stat().st_size != ROM_SIZE:
        raise ValueError("canonical ROM size mismatch")
    if len(bitmap.read_bytes()) != ROM_SIZE // 8:
        raise ValueError("analysis bitmap size mismatch")
    meta = load(analysis_meta)
    if meta.get("rom_size") != ROM_SIZE:
        raise ValueError("analysis metadata ROM size mismatch")
    if meta.get("gpgx_sha") != "27426f00aa68f9f358c86919e8a40985326fa05b":
        raise ValueError("analysis metadata GPGX SHA mismatch")
    actual_bitmap_hash = sha256(bitmap)
    if actual_bitmap_hash != meta.get("analysis_rom_read_bitmap_sha256"):
        raise ValueError("analysis bitmap hash mismatch in metadata")
    if evidence.get("source") != "GPGX_MANUAL_REALTIME":
        raise ValueError("runtime evidence source is not GPGX_MANUAL_REALTIME")
    return {"status": "PASS", "canonical_rom_sha256": rom_hash,
            "rom_size": ROM_SIZE, "analysis_bitmap_sha256": actual_bitmap_hash,
            "runtime_evidence_sha256": sha256(Path(evidence["_path"])),
            "analysis_meta_sha256": sha256(analysis_meta),
            "gpgx_sha": meta["gpgx_sha"]}


def reader_group(regions: list[dict], evidence_addresses: set[int],
                 classifications: dict[int, str], code_ranges: list[dict]) -> list[dict]:
    groups: dict[int, dict] = {}
    for region in regions:
        reader = region["first_reader_pc"]
        executed = reader in evidence_addresses
        source = classifications.get(reader, "UNKNOWN")
        normalized = normalize_classification(source, executed)
        group = groups.setdefault(reader, {"reader_pc": reader, "regions": 0,
                                            "total_unique_rom_bytes": 0,
                                            "min_rom_address": region["start"],
                                            "max_rom_address": region["end"],
                                            "classes": set(), "executed": executed})
        group["regions"] += 1
        group["total_unique_rom_bytes"] += region["observed_bytes"]
        group["min_rom_address"] = min(group["min_rom_address"], region["start"])
        group["max_rom_address"] = max(group["max_rom_address"], region["end"])
        group["classes"].add(normalized)
    result = []
    for group in groups.values():
        if len(group["classes"]) != 1:
            classification = "CONTEXT_CONFLICT"
        else:
            classification = next(iter(group["classes"]))
        result.append({"reader_pc": hex_address(group["reader_pc"]),
                       "region_count": group["regions"],
                       "total_unique_rom_bytes": group["total_unique_rom_bytes"],
                       "min_rom_address": hex_address(group["min_rom_address"]),
                       "max_rom_address": hex_address(group["max_rom_address"]),
                       "rom_span": (hex_address(group["min_rom_address"]) + "-" +
                                    hex_address(group["max_rom_address"])),
                       "classification": classification,
                       "executed": group["executed"],
                       "nearest_known_code_range": nearest_code_range(
                           code_ranges, group["reader_pc"])})
    return sorted(result, key=lambda item: (-item["total_unique_rom_bytes"],
                                             -item["region_count"],
                                             number(item["reader_pc"])))


def build(args: argparse.Namespace) -> dict:
    evidence = load(args.runtime_evidence)
    evidence["_path"] = str(args.runtime_evidence)
    bitmap = args.analysis_bitmap.read_bytes()
    provenance = validate_provenance(args.rom, args.analysis_meta, args.analysis_bitmap, evidence)
    regions = read_regions(args.analysis_ranges, bitmap)
    classifications = source_classifications(args.classification)
    executed_addresses = {number(item) for item in evidence["executed_addresses"]}
    if any(address & 1 or not 0 <= address < ROM_SIZE for address in executed_addresses):
        raise ValueError("runtime evidence contains invalid PC")
    code_ranges = known_code_ranges(classifications)
    groups = reader_group(regions, executed_addresses, classifications, code_ranges)
    enriched = []
    for region in regions:
        reader = region["first_reader_pc"]
        executed = reader in executed_addresses
        source_class = classifications.get(reader, "UNKNOWN")
        nearest = nearest_code_range(code_ranges, reader)
        enriched.append({"region_start": hex_address(region["start"]),
                         "region_end": hex_address(region["end"]),
                         "observed_bytes": region["observed_bytes"],
                         "first_reader_pc": hex_address(reader),
                         "first_access_width": region["first_access_width"],
                         "reader_executed": executed,
                         "reader_classification": normalize_classification(source_class, executed),
                         "source_classification": source_class,
                         "containing_or_nearest_known_code_range": nearest,
                         "canonical_rom_provenance": "PASS"})
    enriched.sort(key=lambda item: (number(item["region_start"]), number(item["region_end"])))
    breakdown = {label: {"readers": 0, "regions": 0, "bytes": 0} for label in READER_CLASSES}
    for group in groups:
        label = group["classification"]
        if label not in breakdown:
            continue
        breakdown[label]["readers"] += 1
        breakdown[label]["regions"] += group["region_count"]
        breakdown[label]["bytes"] += group["total_unique_rom_bytes"]
    checksum = [item for item in enriched if item["first_reader_pc"] == "0x000380"]
    static = [item for item in groups if item["executed"] and
              item["classification"] in {"CODE_STATIC_SUPPORTED", "ASM_ROUNDTRIP_EXACT"}]
    unknown = [item for item in groups if item["executed"] and
               item["classification"] in {"RUNTIME_EXECUTED_UNKNOWN", "UNKNOWN"}]
    return {"schema": "oasis.gpgx.rom.reader.correlation.v1",
            "decision": ("GPGX_ROM_READER_CORRELATION_HIGH_VALUE" if regions and
                         not checksum else "GPGX_ROM_READER_CORRELATION_NEEDS_FIXUPS"),
            "provenance": provenance,
            "analysis_capture": {"analysis_ranges_sha256": sha256(args.analysis_ranges),
                                  "analysis_bitmap_bytes": bitmap_count(bitmap),
                                  "analysis_region_count": len(regions),
                                  "analysis_meta": load(args.analysis_meta)},
            "execution_evidence": {"executed_pc_count": len(executed_addresses),
                                    "classification_artifact_sha256": sha256(args.classification)},
            "reader_association_model": "FIRST_READER_ONLY",
            "regions": enriched,
            "reader_groups": groups,
            "classification_breakdown": breakdown,
            "top_readers_by_bytes": groups[:20],
            "top_readers_by_regions": sorted(groups, key=lambda item: (
                -item["region_count"], -item["total_unique_rom_bytes"], number(item["reader_pc"])))[:20],
            "analysis_readers_static_corroborated": static,
            "analysis_readers_runtime_unknown": unknown[:20],
            "checksum_contamination": {"reader_pc": "0x000380", "region_count": len(checksum),
                                        "bytes": sum(item["observed_bytes"] for item in checksum),
                                        "status": "ABSENT" if not checksum else
                                        "PRESENT_FIRST_READER_REQUIRES_REVIEW"},
            "limitations": ["FIRST_READER_ONLY", "no destination provenance",
                            "correlation != semantic identification"]}


def render(result: dict, output_hash: str) -> str:
    lines = ["# GPGX Analysis ROM-Read to Reader-PC Correlation", "",
             f"Decision: `{result['decision']}`", "",
             f"Analysis bytes: **{result['analysis_capture']['analysis_bitmap_bytes']}**",
             f"Analysis regions: **{result['analysis_capture']['analysis_region_count']}**",
             f"Unique reader PCs: **{len(result['reader_groups'])}**",
             f"Artifact SHA-256: `{output_hash}`", "",
             "## Provenance", "", f"- Canonical ROM: `{result['provenance']['canonical_rom_sha256']}` (PASS)",
             f"- GPGX SHA: `{result['provenance']['gpgx_sha']}`",
             f"- Runtime evidence SHA-256: `{result['provenance']['runtime_evidence_sha256']}`",
             f"- Analysis metadata SHA-256: `{result['provenance']['analysis_meta_sha256']}`", "",
             "## Classification breakdown", "", "| Classification | Readers | Regions | Bytes |", "|---|---:|---:|---:|"]
    for label in READER_CLASSES:
        item = result["classification_breakdown"][label]
        lines.append(f"| {label} | {item['readers']} | {item['regions']} | {item['bytes']} |")
    lines += ["", "## Top readers by unique ROM bytes", "", "| Reader PC | Classification | Regions | Bytes | ROM span | Executed |", "|---|---|---:|---:|---|---|"]
    for item in result["top_readers_by_bytes"]:
        lines.append(f"| {item['reader_pc']} | {item['classification']} | {item['region_count']} | {item['total_unique_rom_bytes']} | {item['rom_span']} | {'yes' if item['executed'] else 'no'} |")
    lines += ["", "## Top readers by region count", "", "| Reader PC | Classification | Regions | Bytes | ROM span |", "|---|---|---:|---:|---|"]
    for item in result["top_readers_by_regions"]:
        lines.append(f"| {item['reader_pc']} | {item['classification']} | {item['region_count']} | {item['total_unique_rom_bytes']} | {item['rom_span']} |")
    lines += ["", "## ANALYSIS_READERS_STATIC_CORROBORATED", ""]
    for item in result["analysis_readers_static_corroborated"]:
        lines.append(f"- `{item['reader_pc']}` {item['classification']}: {item['total_unique_rom_bytes']} bytes in {item['region_count']} regions")
    lines += ["", "## ANALYSIS_READERS_RUNTIME_UNKNOWN", ""]
    for item in result["analysis_readers_runtime_unknown"]:
        lines.append(f"- `{item['reader_pc']}` {item['classification']}: {item['total_unique_rom_bytes']} bytes in {item['region_count']} regions")
    checksum = result["checksum_contamination"]
    lines += ["", "## Checksum contamination", "", f"Reader `0x000380`: **{checksum['status']}**, regions={checksum['region_count']}, bytes={checksum['bytes']}", "", "## Limitations", ""]
    lines.extend(f"- {item}" for item in result["limitations"])
    lines += ["", "Correlation is evidence linkage only; it does not identify semantic roles or function boundaries.", ""]
    return "\n".join(lines)


def self_test() -> None:
    assert normalize_classification("CODE_EXECUTED", True) == "CODE_EXECUTED_AT_ADDRESS"
    assert normalize_classification("UNKNOWN", True) == "RUNTIME_EXECUTED_UNKNOWN"
    assert normalize_classification("UNKNOWN", False) == "UNKNOWN"
    ranges = known_code_ranges({0x100: "CODE_STATIC_SUPPORTED", 0x102: "CODE_STATIC_SUPPORTED"})
    assert nearest_code_range(ranges, 0x102)["classification"] == "CODE_STATIC_SUPPORTED"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--analysis-ranges", type=Path)
    parser.add_argument("--analysis-bitmap", type=Path)
    parser.add_argument("--analysis-meta", type=Path)
    parser.add_argument("--runtime-evidence", type=Path)
    parser.add_argument("--classification", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("GPGX reader correlation self-test: PASS")
        return 0
    required = (args.rom, args.analysis_ranges, args.analysis_bitmap, args.analysis_meta,
                args.runtime_evidence, args.classification, args.output, args.report)
    if any(item is None for item in required):
        parser.error("all input/output paths are required unless --self-test is used")
    try:
        result = build(args)
        encoded = json.dumps(result, indent=2, sort_keys=True) + "\n"
        args.output.write_text(encoded, encoding="utf-8")
        artifact_hash = sha256(args.output)
        args.report.write_text(render(result, artifact_hash), encoding="utf-8")
        print(f"correlated {len(result['regions'])} regions across {len(result['reader_groups'])} reader PCs")
        print(f"decision={result['decision']}")
        return 0
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        print(f"error: {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
