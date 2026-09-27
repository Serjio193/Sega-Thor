"""Overlay proven runtime properties on canonical ranges without changing ownership."""

from __future__ import annotations

from dataclasses import dataclass
import csv
from pathlib import Path
from typing import Iterable

from canonical_full_rom_map import CanonicalRomMap, Range as CanonicalRange

SCHEMA_ID = "thor.rom-properties.v1"
CONTRACT_SHA256 = "c88eb4dcc273b55681bc1d0fc04e483b4d200d292b054488f7c87348b2c07842"

@dataclass(frozen=True)
class PropertyRange:
    start: int
    end_exclusive: int
    property_mask: int


def read_range_export(path: Path) -> tuple[dict[str, str], list[PropertyRange]]:
    """Read the deterministic TSV export and validate its exact full coverage."""
    metadata: dict[str, str] = {}
    ranges: list[PropertyRange] = []
    with Path(path).open("r", encoding="ascii", newline="") as stream:
        rows = csv.reader(stream, delimiter="\t")
        header_seen = False
        for row in rows:
            if not row:
                continue
            if row[0].startswith("# "):
                key, separator, value = row[0][2:].partition("=")
                if not separator or key in metadata:
                    raise ValueError("STOP_RUNTIME_RANGE_METADATA_INVALID")
                metadata[key] = value
                continue
            if row == ["start", "end_exclusive", "property_mask"]:
                if header_seen:
                    raise ValueError("STOP_RUNTIME_RANGE_HEADER_DUPLICATE")
                header_seen = True
                continue
            if not header_seen or len(row) != 3:
                raise ValueError("STOP_RUNTIME_RANGE_ROW_INVALID")
            try:
                ranges.append(PropertyRange(int(row[0], 16), int(row[1], 16), int(row[2], 16)))
            except ValueError as error:
                raise ValueError("STOP_RUNTIME_RANGE_ROW_INVALID") from error
    required = {"schema", "rom_sha256", "rom_size", "classifier_schema", "contract_sha256",
                "core_build_id", "run_id", "generation", "capabilities", "validation_state"}
    if not header_seen or metadata.keys() != required or metadata["schema"] != "thor.rom-property-ranges.v1":
        raise ValueError("STOP_RUNTIME_RANGE_METADATA_INVALID")
    try:
        rom_size = int(metadata["rom_size"])
        if rom_size <= 0:
            raise ValueError
    except ValueError as error:
        raise ValueError("STOP_RUNTIME_RANGE_ROM_SIZE_INVALID") from error
    if (len(metadata["rom_sha256"]) != 64 or any(c not in "0123456789abcdef" for c in metadata["rom_sha256"]) or
            len(metadata["contract_sha256"]) != 64 or any(c not in "0123456789abcdef" for c in metadata["contract_sha256"]) or
            metadata["classifier_schema"] != SCHEMA_ID or not metadata["core_build_id"] or
            not metadata["run_id"] or metadata["validation_state"] not in {"VALIDATED", "PARTIAL"}):
        raise ValueError("STOP_RUNTIME_RANGE_IDENTITY_INVALID")
    cursor = 0
    for item in ranges:
        if item.start != cursor or item.end_exclusive <= item.start or item.end_exclusive > rom_size:
            raise ValueError("STOP_RUNTIME_PROPERTY_COVERAGE_INVALID")
        if item.property_mask & ~0x1FF:
            raise ValueError("STOP_RUNTIME_PROPERTY_MASK_INVALID")
        cursor = item.end_exclusive
    if cursor != rom_size:
        raise ValueError("STOP_RUNTIME_PROPERTY_COVERAGE_INVALID")
    return metadata, ranges


@dataclass(frozen=True)
class CombinedRange:
    start: int
    end_exclusive: int
    canonical_class: str
    canonical_truth: str
    source_owned: int
    runtime_properties: int


def combine(canonical: CanonicalRomMap, rom_sha256: str, rom_size: int,
            runtime_ranges: Iterable[PropertyRange], *, identity: dict[str, str]) -> list[CombinedRange]:
    """Split at the union of boundaries; preserve canonical ownership verbatim."""
    if canonical.rom_sha256 != rom_sha256 or canonical.rom_size != rom_size:
        raise ValueError("STOP_RUNTIME_CANONICAL_ROM_IDENTITY_MISMATCH")
    if (identity.get("rom_sha256") != rom_sha256 or identity.get("rom_size") != str(rom_size) or
            identity.get("classifier_schema") != SCHEMA_ID or
            identity.get("contract_sha256") != CONTRACT_SHA256 or
            not identity.get("core_build_id") or not identity.get("run_id") or
            identity.get("validation_state") not in {"VALIDATED", "PARTIAL"}):
        raise ValueError("STOP_RUNTIME_PROPERTY_IDENTITY_INVALID")
    properties = list(runtime_ranges)
    cursor = 0
    for item in properties:
        if item.start != cursor or item.end_exclusive <= item.start or item.end_exclusive > rom_size:
            raise ValueError("STOP_RUNTIME_PROPERTY_COVERAGE_INVALID")
        if item.property_mask < 0 or item.property_mask & ~0x1FF:
            raise ValueError("STOP_RUNTIME_PROPERTY_MASK_INVALID")
        cursor = item.end_exclusive
    if cursor != rom_size:
        raise ValueError("STOP_RUNTIME_PROPERTY_COVERAGE_INVALID")

    result: list[CombinedRange] = []
    canonical_index = runtime_index = 0
    position = 0
    while position < rom_size:
        base: CanonicalRange = canonical.ranges[canonical_index]
        runtime = properties[runtime_index]
        end = min(base.end_exclusive, runtime.end_exclusive)
        result.append(CombinedRange(position, end, base.class_name, base.truth,
                                   base.source_owned, runtime.property_mask))
        position = end
        if position == base.end_exclusive:
            canonical_index += 1
        if position == runtime.end_exclusive:
            runtime_index += 1
    if sum((item.end_exclusive - item.start) * item.source_owned for item in result) != canonical.source_owned_bytes:
        raise AssertionError("runtime overlay changed canonical ownership accounting")
    return result
