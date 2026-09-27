"""Summarize exact evidence gaps in an S3 sprite-piece catalog."""

from __future__ import annotations

import argparse
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Mapping


SCHEMA = "oasis.m68k.sprite-evidence-gap-report.v1"


def _counts(entries: list[Mapping[str, Any]]) -> dict[str, int]:
    counts = Counter(entry.get("classification") for entry in entries)
    return {"PROVEN": counts["PROVEN"], "INCOMPLETE": counts["INCOMPLETE"],
            "OBSERVED_LINKAGE_ONLY": counts["OBSERVED_LINKAGE_ONLY"],
            "CONFLICT": counts["CONFLICT"]}


def _distribution(entries: list[Mapping[str, Any]], key: str) -> list[dict[str, Any]]:
    grouped: dict[Any, Counter[str]] = defaultdict(Counter)
    for entry in entries:
        value = entry.get(key)
        grouped[value][str(entry.get("classification"))] += 1
    result = []
    for value in sorted(grouped, key=lambda item: str(item)):
        item: dict[str, Any] = {key: value, "total": sum(grouped[value].values())}
        item.update(_counts_from_counter(grouped[value]))
        result.append(item)
    return result


def _counts_from_counter(counter: Counter[str]) -> dict[str, int]:
    return {"PROVEN": counter["PROVEN"], "INCOMPLETE": counter["INCOMPLETE"],
            "OBSERVED_LINKAGE_ONLY": counter["OBSERVED_LINKAGE_ONLY"],
            "CONFLICT": counter["CONFLICT"]}


def _frame_distribution(entries: list[Mapping[str, Any]]) -> list[dict[str, Any]]:
    grouped: dict[Any, list[Mapping[str, Any]]] = defaultdict(list)
    for entry in entries:
        grouped[entry.get("frame")].append(entry)
    return [{"frame": frame, "total": len(group), **_counts(group)}
            for frame, group in sorted(grouped.items(), key=lambda item: str(item[0]))]


def _reason_data(entries: list[Mapping[str, Any]]) -> tuple[Counter[str], Counter[str]]:
    reasons: Counter[str] = Counter()
    combinations: Counter[str] = Counter()
    for entry in entries:
        codes = sorted(entry.get("gap_reasons", []))
        if not codes and entry.get("classification") != "PROVEN":
            codes = ["OTHER_EXACT_REASON"]
        reasons.update(codes)
        if codes:
            combinations["+".join(codes)] += 1
    return reasons, combinations


def _capture_cost(entries: list[Mapping[str, Any]], old_path: Path | None,
                  new_path: Path | None) -> dict[str, Any]:
    new_size = new_path.stat().st_size if new_path and new_path.exists() else None
    old_size = old_path.stat().st_size if old_path and old_path.exists() else None
    vram_bytes = sum(sum(len(tile.get("raw_bytes", [])) for tile in entry.get("tiles", []))
                      for entry in entries)
    cram_bytes = sum(len(entry.get("cram", {}).get("raw_bytes", []))
                     for entry in entries if entry.get("cram"))
    sat_bytes = sum(len(entry.get("vram_sat", {}).get("raw_bytes", []))
                    for entry in entries if entry.get("vram_sat"))
    return {"old_capture_bytes": old_size, "new_capture_bytes": new_size,
            "delta_bytes": new_size - old_size if old_size is not None and new_size is not None else None,
            "vram_bytes_captured": vram_bytes, "cram_bytes_captured": cram_bytes,
            "sat_evidence_bytes": sat_bytes}


def build_report(catalog: Mapping[str, Any], old_catalog: Mapping[str, Any] | None = None,
                 old_capture: Path | None = None, new_capture: Path | None = None) -> dict[str, Any]:
    entries = list(catalog.get("entries", []))
    reasons, combinations = _reason_data(entries)
    dimensions = []
    for entry in entries:
        attributes = entry.get("decoded_attributes") or {}
        dimensions.append(f"{attributes.get('width_cells', 'unknown')}x{attributes.get('height_cells', 'unknown')}")
    tile_counts = [{"tile_count": len(entry.get("tiles", [])), "classification": entry.get("classification")}
                   for entry in entries]
    tile_distribution: Counter[str] = Counter(
        f"{item['tile_count']}:{item['classification']}" for item in tile_counts)
    return {
        "schema": SCHEMA, "catalog_schema": catalog.get("schema"),
        "run_id": catalog.get("run_id"),
        "capture_sha256": catalog.get("capture_sha256"),
        "total_observations": len(entries),
        "proven_count": catalog.get("proven_count", 0),
        "incomplete_count": catalog.get("incomplete_count", 0),
        "observed_linkage_only_count": catalog.get("observed_linkage_only_count", 0),
        "conflict_count": catalog.get("conflict_count", 0),
        "before_counts": _counts(list((old_catalog or {}).get("entries", []))) if old_catalog else None,
        "after_counts": _counts(entries),
        "direct_publication_count": catalog.get("direct_publication_count", 0),
        "persisted_publication_count": catalog.get("persisted_publication_count", 0),
        "unproven_publication_count": catalog.get("unproven_publication_count", 0),
        "ordered_causality_count": catalog.get("ordered_causality_count", 0),
        "unproven_causality_count": catalog.get("unproven_causality_count", 0),
        "persistence_relations": dict(sorted(Counter(
            (entry.get("persistence") or {}).get("relation", "NONE")
            for entry in entries).items())),
        "reason_counts": dict(sorted(reasons.items())),
        "reason_combinations": dict(sorted(combinations.items())),
        "primary_blockers": [{"reason": reason, "count": count}
                             for reason, count in reasons.most_common()],
        "per_frame_distribution": _frame_distribution(entries),
        "per_sat_entry_distribution": _distribution(entries, "sat_entry"),
        "dimensions_distribution": dict(sorted(Counter(dimensions).items())),
        "tile_count_distribution": dict(sorted(tile_distribution.items())),
        "palette_line_distribution": _distribution(
            [{"palette_line": (entry.get("decoded_attributes") or {}).get("palette_line"),
              "classification": entry.get("classification")} for entry in entries], "palette_line"),
        "capture_cost": _capture_cost(entries, old_capture, new_capture),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--old-catalog", type=Path)
    parser.add_argument("--old-capture", type=Path)
    parser.add_argument("--new-capture", type=Path)
    args = parser.parse_args()
    catalog_raw = args.catalog.read_bytes()
    old_catalog = json.loads(args.old_catalog.read_text(encoding="utf-8")) if args.old_catalog else None
    report = build_report(json.loads(catalog_raw), old_catalog, args.old_capture, args.new_capture)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
