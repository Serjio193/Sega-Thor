"""Coherent Stage 7 generation and emission-manifest preflight."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

from map_driven_asm_closure import validate_partition


class Stage7PreflightError(ValueError):
    """Fail-closed error carrying the complete authority comparison."""

    def __init__(self, reason: str, diagnostic: dict[str, Any]) -> None:
        super().__init__(reason)
        self.diagnostic = diagnostic


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _categories(emissions: list[tuple[Any, ...]]) -> dict[str, int]:
    result = {"ASM": 0, "DATA": 0, "ASSET": 0, "INCBIN": 0}
    for row in emissions:
        name, size = str(row[2]), int(row[1]) - int(row[0])
        result[name] = result.get(name, 0) + size
    return result


def _manifest_authority(path: Path, generation_id: str, rom_sha: str,
                        rom_size: int) -> tuple[dict[str, Any], list[dict[str, Any]] | None]:
    authority: dict[str, Any] = {"generation_id": generation_id,
        "manifest_path": str(path), "manifest_sha256": _sha(path) if path.is_file() else None,
        "SOURCE_OWNED": "UNKNOWN", "ASM": "UNKNOWN", "DATA": "UNKNOWN",
        "ASSET": "UNKNOWN", "INCBIN": "UNKNOWN", "sum_emission_categories": "UNKNOWN"}
    if not path.is_file():
        authority["status"] = "MISSING"
        return authority, None
    data = json.loads(path.read_text(encoding="utf-8"))
    entries = sorted(data.get("entries", []), key=lambda item: int(item["start"]))
    try:
        validate_partition(entries, rom_size)
    except ValueError as error:
        authority["status"] = str(error)
        return authority, None
    metrics = data.get("metrics", {})
    authority["SOURCE_OWNED"] = metrics.get("SOURCE_OWNED_BYTES", "UNKNOWN")
    authority["rom_sha256"] = data.get("rom_sha256")
    authority["rom_size"] = data.get("rom_size")
    authority["entry_count"] = len(entries)
    authority["ranges_sha256"] = hashlib.sha256(json.dumps(
        [[int(item["start"]), int(item["end"])] for item in entries],
        separators=(",", ":")).encode()).hexdigest()
    authority["status"] = "PASS" if data.get("rom_sha256") == rom_sha else "ROM_MISMATCH"
    if data.get("rom_sha256") != rom_sha:
        return authority, None
    return authority, entries


def _parent_chain(start: Path) -> list[tuple[str, Path]]:
    root = start.parent.parent
    chain: list[tuple[str, Path]] = []
    current = start.resolve()
    seen: set[Path] = set()
    while current not in seen:
        seen.add(current)
        metadata_path = current / "generation-metadata.json"
        if not metadata_path.is_file():
            break
        metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        chain.append((str(metadata.get("generation_id", current.name)), current))
        parent = metadata.get("parent_generation_id")
        if not parent:
            break
        parent_path = Path(str(parent))
        current = (root / parent_path if parent_path.parts and
                   parent_path.parts[0] == "generations" else root / "generations" / parent_path.name).resolve()
    return chain


def resolve_manifest(canonical_generation: Path, rom_sha: str, rom_size: int,
                     emissions: list[tuple[Any, ...]], source_owned: int,
                     map_hash: str) -> tuple[Path, list[dict[str, Any]], dict[str, Any]]:
    """Resolve a manifest only through explicit generation lineage."""
    map_ranges = [[int(row[0]), int(row[1])] for row in emissions]
    canonical_authority = {"generation_id": canonical_generation.name,
        "canonical_map_hash": map_hash, "ownership_manifest_hash": None,
        "emission_manifest_hash": None, "SOURCE_OWNED": source_owned,
        **_categories(emissions)}
    canonical_authority["sum_emission_categories"] = sum(_categories(emissions).values())
    canonical_authority["rom_sha256"] = rom_sha
    canonical_authority["rom_size"] = rom_size
    authorities: list[dict[str, Any]] = [{"authority": "canonical_map", **canonical_authority}]
    mismatch: list[dict[str, Any]] = []
    for generation_id, generation in _parent_chain(canonical_generation):
        metadata = json.loads((generation / "generation-metadata.json").read_text(encoding="utf-8"))
        prior = metadata.get("stage7", {}).get("materialized")
        if not prior:
            continue
        materialized = Path(str(prior))
        manifest_path = materialized / "manifest.json"
        authority, entries = _manifest_authority(manifest_path, generation_id, rom_sha, rom_size)
        authority["authority"] = "ownership_manifest"
        authorities.append(authority)
        if entries is None:
            continue
        entry_ranges = [[int(item["start"]), int(item["end"])] for item in entries]
        if entry_ranges != map_ranges:
            mismatch.append({"generation_id": generation_id, "field": "ranges",
                             "canonical_count": len(map_ranges), "manifest_count": len(entry_ranges)})
            continue
        authority["lineage"] = "MATCHED_CANONICAL_GENERATION"
        canonical_authority["ownership_manifest_hash"] = authority["manifest_sha256"]
        canonical_authority["emission_manifest_hash"] = authority["manifest_sha256"]
        diagnostic = {"status": "PASS", "canonical_generation_id": canonical_generation.name,
            "canonical_map_hash": map_hash, "canonical_map": canonical_authority,
            "authorities": authorities, "mismatch_fields": []}
        return materialized, entries, diagnostic
    diagnostic = {"status": "STOP", "canonical_generation_id": canonical_generation.name,
        "canonical_map_hash": map_hash, "canonical_map": canonical_authority,
        "authorities": authorities, "mismatch_fields": mismatch or [{
            "field": "emission_manifest_lineage", "reason": "NO_MATCHING_GENERATION_MANIFEST"}]}
    raise Stage7PreflightError("STOP_STAGE7_EMISSION_PARTITION_MISMATCH", diagnostic)
