"""Canonical full-ROM interval map and deterministic map operations.

This module is deliberately semantic-neutral: it records exact address facts,
not PLAYER/entity interpretations.  SOURCE_OWNED is calculated from intervals.
"""
from __future__ import annotations

from dataclasses import dataclass, replace
import hashlib
import json
from pathlib import Path
from typing import Any, Iterable

ROM_SIZE = 0x300000
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
SOURCE_OWNED_BEFORE = 1_487_672
CLASSES = {"ASM", "DATA_TABLE", "POINTER_TABLE", "RESOURCE", "HEADER", "PADDING", "UNKNOWN"}
TRUTHS = {"OBSERVED", "DERIVED_EXACT", "STATIC_VERIFIED", "HYPOTHESIS", "UNRESOLVED"}


def _hex(value: int) -> str:
    return f"0x{value:06X}"


def _canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _classify(entry: dict[str, Any]) -> str:
    text = " ".join(str(entry.get(key, "")).upper() for key in
                    ("emission_type", "classification", "source_kind"))
    if "HEADER" in text:
        return "HEADER"
    if "PADDING" in text or "ALIGNMENT" in text:
        return "PADDING"
    if "POINTER" in text:
        return "POINTER_TABLE"
    if any(word in text for word in ("GRAPHICS", "AUDIO", "RESOURCE", "ASSET", "COMPRESSED", "SOUND")):
        return "RESOURCE"
    if any(word in text for word in ("TABLE", "RECORD", "LOOKUP", "DESCRIPTOR", "DISPATCH", "SCRIPT", "LIST")):
        return "DATA_TABLE"
    if "ASM" in text or "CODE" in text or "Z80" in text:
        return "ASM"
    return "UNKNOWN"


def _truth(entry: dict[str, Any]) -> str:
    value = str(entry.get("truth", "")).upper()
    if value in TRUTHS:
        return value
    if int(entry.get("source_owned", 0)) == 1:
        return "STATIC_VERIFIED"
    return "DERIVED_EXACT"


@dataclass(frozen=True)
class Range:
    start: int
    end_exclusive: int
    class_name: str = "UNKNOWN"
    truth: str = "DERIVED_EXACT"
    source_owned: int = 0
    symbol: str | None = None
    section: str | None = None
    emission_source: str = "canonical_full_rom_map"
    proof: tuple[str, ...] = ()
    discovery_method: str = "import"
    start_boundary: str | None = "UNKNOWN_BOUNDARY"
    end_boundary: str | None = "UNKNOWN_BOUNDARY"

    @property
    def size(self) -> int:
        return self.end_exclusive - self.start

    def as_dict(self) -> dict[str, Any]:
        return {"start": self.start, "end_exclusive": self.end_exclusive, "size": self.size,
                "class": self.class_name, "truth": self.truth, "source_owned": self.source_owned,
                "symbol": self.symbol, "section": self.section, "emission_source": self.emission_source,
                "proof": list(self.proof), "discovery_method": self.discovery_method,
                "start_boundary": self.start_boundary, "end_boundary": self.end_boundary}


class CanonicalRomMap:
    def __init__(self, ranges: Iterable[Range], rom_size: int = ROM_SIZE,
                 rom_sha256: str = ROM_SHA256):
        self.rom_size = rom_size
        self.rom_sha256 = rom_sha256
        self.ranges = sorted(tuple(ranges), key=lambda item: item.start)
        self.audit()

    @classmethod
    def from_report(cls, report: dict[str, Any], expected_source_owned: int = SOURCE_OWNED_BEFORE):
        rom = report.get("rom", {})
        if int(rom.get("bytes", report.get("rom_size", -1))) != ROM_SIZE or \
                rom.get("sha256", report.get("rom_sha256")) != ROM_SHA256:
            raise ValueError("STOP_MAP_ROM_IDENTITY_MISMATCH")
        ranges = []
        for item in report.get("emission", []):
            start, end = int(item["start"]), int(item["end"])
            ranges.append(Range(start, end, _classify(item), _truth(item),
                                 int(item.get("source_owned", 0)),
                                 Path(str(item.get("artifact", ""))).name or None,
                                 str(item.get("source_kind", "")) or None,
                                 str(item.get("emission_type", "canonical")),
                                 (str(item.get("classification", "UNKNOWN")),), "import"))
        result = cls(ranges)
        if result.source_owned_bytes != expected_source_owned:
            raise ValueError("STOP_MAP_OWNERSHIP_ACCOUNTING_MISMATCH")
        return result

    @classmethod
    def from_report_path(cls, path: Path, expected_source_owned: int = SOURCE_OWNED_BEFORE):
        return cls.from_report(json.loads(Path(path).read_text(encoding="utf-8")), expected_source_owned)

    @property
    def source_owned_bytes(self) -> int:
        return sum(item.size for item in self.ranges if item.source_owned == 1)

    @property
    def unknown_bytes(self) -> int:
        return sum(item.size for item in self.ranges if item.class_name == "UNKNOWN")

    def audit(self) -> dict[str, Any]:
        cursor, overlaps = 0, 0
        for item in self.ranges:
            if item.class_name not in CLASSES or item.truth not in TRUTHS:
                raise ValueError("STOP_MAP_INVALID_ENUMERATION")
            if item.start < cursor:
                overlaps += 1
            if item.start != cursor or item.end_exclusive <= item.start or item.end_exclusive > self.rom_size:
                raise ValueError("STOP_MAP_COVERAGE_INVALID")
            cursor = item.end_exclusive
        if cursor != self.rom_size:
            raise ValueError("STOP_MAP_COVERAGE_INVALID")
        return {"TOTAL_BYTES": self.rom_size, "GAPS": 0, "OVERLAPS": overlaps,
                "SOURCE_OWNED": self.source_owned_bytes}

    def lookup(self, address: int) -> dict[str, Any]:
        if not 0 <= address < self.rom_size:
            raise ValueError("address outside canonical ROM")
        for item in self.ranges:
            if item.start <= address < item.end_exclusive:
                return {"address": address, "range_start": item.start,
                        "range_end": item.end_exclusive, "class": item.class_name,
                        "truth": item.truth, "source_owned": item.source_owned,
                        "symbol": item.symbol, "start_boundary": item.start_boundary,
                        "end_boundary": item.end_boundary, "proof": list(item.proof)}
        raise AssertionError("audited map failed lookup")

    def hash(self) -> str:
        return hashlib.sha256(_canonical([item.as_dict() for item in self.ranges]).encode()).hexdigest()

    def delta(self, before: "CanonicalRomMap", new_boundaries: int = 0) -> dict[str, Any]:
        def count(class_name: str) -> int:
            return sum(item.size for item in self.ranges if item.class_name == class_name)
        return {"RANGES_BEFORE": len(before.ranges), "RANGES_AFTER": len(self.ranges),
                "UNKNOWN_BYTES_BEFORE": before.unknown_bytes, "UNKNOWN_BYTES_AFTER": self.unknown_bytes,
                "ASM_BYTES_BEFORE": sum(r.size for r in before.ranges if r.class_name == "ASM"),
                "ASM_BYTES_AFTER": count("ASM"),
                "DATA_TABLE_BYTES_BEFORE": sum(r.size for r in before.ranges if r.class_name == "DATA_TABLE"),
                "DATA_TABLE_BYTES_AFTER": count("DATA_TABLE"),
                "RESOURCE_BYTES_BEFORE": sum(r.size for r in before.ranges if r.class_name == "RESOURCE"),
                "RESOURCE_BYTES_AFTER": count("RESOURCE"),
                "BOUNDARIES_BEFORE": 2 * len(before.ranges), "BOUNDARIES_AFTER": 2 * len(self.ranges),
                "NEW_BOUNDARIES": new_boundaries, "NEW_CLASSIFIED_BYTES": before.unknown_bytes - self.unknown_bytes,
                "SOURCE_OWNED_BEFORE": before.source_owned_bytes, "SOURCE_OWNED_AFTER": self.source_owned_bytes,
                "SOURCE_OWNED_DELTA": self.source_owned_bytes - before.source_owned_bytes}

    def apply(self, operations: Iterable[dict[str, Any]]) -> tuple["CanonicalRomMap", dict[str, Any]]:
        current = list(self.ranges)
        added_boundaries = 0
        for operation in operations:
            kind = operation["op"]
            if kind == "SPLIT_RANGE":
                start, split = int(operation["start"]), int(operation["split"])
                target = next(item for item in current if item.start == start and item.end_exclusive > split)
                if split <= target.start or split >= target.end_exclusive:
                    raise ValueError("STOP_MAP_INVALID_SPLIT")
                index = current.index(target)
                current[index:index + 1] = [replace(target, end_exclusive=split),
                                             replace(target, start=split)]
                added_boundaries += 2
            elif kind == "CLASSIFY_RANGE":
                start, end = int(operation["start"]), int(operation["end_exclusive"])
                target = next(item for item in current if item.start == start and item.end_exclusive == end)
                class_name = str(operation["class"])
                if class_name not in CLASSES:
                    raise ValueError("STOP_MAP_INVALID_CLASS")
                current[current.index(target)] = replace(target, class_name=class_name,
                                                          truth=str(operation.get("truth", target.truth)),
                                                          discovery_method="generic_closure")
            elif kind == "ADD_BOUNDARY":
                address, boundary = int(operation["address"]), str(operation["boundary"])
                for index, item in enumerate(current):
                    if item.start == address:
                        current[index] = replace(item, start_boundary=boundary)
                    if item.end_exclusive == address:
                        current[index] = replace(item, end_boundary=boundary)
                added_boundaries += 1
            elif kind == "PROMOTE_SOURCE_OWNED":
                start, end = int(operation["start"]), int(operation["end_exclusive"])
                target = next(item for item in current if item.start == start and item.end_exclusive == end)
                if not operation.get("exact_extent") or not operation.get("exact_class") or \
                        not operation.get("exact_bytes") or not operation.get("exact_emission") or \
                        not operation.get("byte_roundtrip"):
                    raise ValueError("STOP_MAP_PROMOTION_GATE")
                current[current.index(target)] = replace(target, source_owned=1,
                                                          truth="STATIC_VERIFIED")
            elif kind in {"ADD_SYMBOL", "ADD_REFERENCE"}:
                continue
            else:
                raise ValueError(f"STOP_MAP_UNKNOWN_OPERATION:{kind}")
            current = sorted(current, key=lambda item: item.start)
        result = CanonicalRomMap(current, self.rom_size, self.rom_sha256)
        return result, result.delta(self, added_boundaries)

    def rank_unknown(self, evidence: dict[int, dict[str, Any]] | None = None) -> list[dict[str, Any]]:
        evidence = evidence or {}
        rows = []
        for item in self.ranges:
            if item.class_name != "UNKNOWN":
                continue
            fact = evidence.get(item.start, {})
            rows.append({"RANK": 0, "ROM_START": item.start, "ROM_END": item.end_exclusive,
                         "SIZE": item.size, "CURRENT_CLASS": item.class_name, "CURRENT_TRUTH": item.truth,
                         "KNOWN_REFERENCES": fact.get("references", 0), "KNOWN_CFG_EDGES": fact.get("cfg_edges", 0),
                         "KNOWN_ROM_READS": fact.get("rom_reads", 0), "BLOCKER": fact.get("blocker", "missing exact extent/class/roundtrip"),
                         "MISSING_EVIDENCE": fact.get("missing", "exact reconstruction and byte-for-byte roundtrip"),
                         "EXPECTED_NEXT_ACTION": fact.get("next_action", "capture bounded evidence and propose map operations"),
                         "EXPECTED_BYTE_PAYOFF": fact.get("expected_bytes", "UNKNOWN")})
        rows.sort(key=lambda row: (-row["KNOWN_REFERENCES"], -row["KNOWN_ROM_READS"], row["ROM_START"]))
        for rank, row in enumerate(rows, 1):
            row["RANK"] = rank
        return rows


__all__ = ["CanonicalRomMap", "Range", "ROM_SIZE", "ROM_SHA256", "SOURCE_OWNED_BEFORE"]
