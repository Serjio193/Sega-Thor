"""Validated fallback materialization of canonical ROM knowledge map."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sqlite3
import sys
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
SOURCE_TOOLS = REPO_ROOT / "src" / "tools"
EVIDENCE_TOOLS = REPO_ROOT / "src" / "tools" / "thor_evidence"
for source_path in (SOURCE_TOOLS, EVIDENCE_TOOLS):
    if str(source_path) not in sys.path:
        sys.path.insert(0, str(source_path))

from cartographer import Cartographer
from identity import ROM_SHA, ROM_SIZE
from rom_knowledge_map import KnowledgeStore, TABLE_COLUMNS, canonical as _canonical

CANONICAL_SCHEMA = "oasis.m14.canonical-rom-knowledge.v2"
LEGACY_CANONICAL_SCHEMA = "oasis.m12.canonical-rom-knowledge.v1"
CANONICAL_STATUS = "PASS_CANONICAL_ROM_KNOWLEDGE_MAP_V1"
CANONICAL_SOURCE_OWNED = 1_487_672


def _sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _sha_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def validate_canonical_knowledge_map_2d(data: dict[str, Any], rom_bytes: bytes) -> None:
    """Validate all 4 mandatory criteria before permitting canonical fallback."""
    if len(rom_bytes) != ROM_SIZE or _sha(rom_bytes) != ROM_SHA:
        raise ValueError("STOP_POSTRUN_MAP_ROM_MISMATCH")
    map_rom_sha = data.get("rom_sha256") or data.get("rom", {}).get("sha256")
    if map_rom_sha != ROM_SHA or ROM_SHA != "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263":
        raise ValueError("STOP_POSTRUN_MAP_ROM_MISMATCH")
    if data.get("schema") not in {CANONICAL_SCHEMA, LEGACY_CANONICAL_SCHEMA}:
        raise ValueError("STOP_POSTRUN_MAP_FACT_MISSING:canonical map schema mismatch")
    if data.get("status") != CANONICAL_STATUS:
        raise ValueError("STOP_POSTRUN_MAP_FACT_MISSING:canonical map status mismatch")
    emissions = data.get("emission", [])
    source_owned_sum = sum(int(r["end"]) - int(r["start"]) for r in emissions
                           if int(r.get("source_owned", 0)) == 1)
    if source_owned_sum != CANONICAL_SOURCE_OWNED:
        raise ValueError(f"STOP_POSTRUN_MAP_OWNERSHIP_CHANGED:expected {CANONICAL_SOURCE_OWNED}, got {source_owned_sum}")


def _authoritative_knowledge_db(repo_root: Path,
                                expected_emission: list[tuple[Any, ...]]) -> Path | None:
    """Resolve the validated canonical pipeline pointer and match its partition."""
    root = repo_root / "build" / "thor-evidence" / "archivist-knowledge-pipeline-2g"
    try:
        pointer = json.loads((root / "current.json").read_text(encoding="utf-8"))
        if pointer.get("schema") != "oasis.m12.archivist-canonical-knowledge.current.v1":
            return None
        generation = (root / str(pointer["generation_dir"])).resolve()
        generation.relative_to((root / "generations").resolve())
        db_path = generation / "knowledge.sqlite"
        if not db_path.is_file() or _sha_file(db_path) != pointer.get("knowledge_sha256"):
            return None
    except (OSError, KeyError, ValueError, json.JSONDecodeError):
        return None
    db = sqlite3.connect(db_path.resolve().as_uri() + "?mode=ro", uri=True)
    try:
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            return None
        meta = {str(row[0]): str(row[1]) for row in db.execute("SELECT key,value FROM map_meta")}
        if meta.get("rom_sha256") != ROM_SHA or meta.get("rom_size") != str(ROM_SIZE):
            return None
        owned = int(db.execute(
            "SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1"
        ).fetchone()[0])
        rows = [tuple(row) for row in db.execute(
            "SELECT start,end,emission_type,classification,source_kind,source_owned,artifact_type,artifact "
            "FROM emission ORDER BY start,end")]
    except sqlite3.Error:
        return None
    finally:
        db.close()
    try:
        store = KnowledgeStore(db_path, ROM_SHA, ROM_SIZE, read_only=True)
        try:
            hashes = store.hashes()
            if hashes["map_hash"] != pointer.get("knowledge_map_hash") or \
                    (pointer.get("logical_hashes") and
                     hashes.get("emission_hash") != pointer["logical_hashes"].get("emission_hash")):
                return None
        finally:
            store.close()
    except (OSError, ValueError, sqlite3.Error):
        return None
    cursor = 0
    for start, end, *_ in rows:
        if int(start) != cursor or int(end) <= cursor:
            return None
        cursor = int(end)
    return db_path if owned == CANONICAL_SOURCE_OWNED and cursor == ROM_SIZE and \
        rows == expected_emission else None


def materialize_canonical_map_fallback(root: Path, rom_path: Path) -> Path:
    """Materialize validated canonical master/knowledge generation from verified 2D map."""
    repo_root = Path(__file__).parents[2]
    json_path = repo_root / "docs" / "reports" / "THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json"
    if not json_path.is_file():
        raise ValueError("STOP_POSTRUN_MAP_FACT_MISSING:canonical map report is missing")
    data = json.loads(json_path.read_text(encoding="utf-8"))
    rom = rom_path.read_bytes()
    validate_canonical_knowledge_map_2d(data, rom)
    emissions = data.get("emission", [])
    generation = root / "generations" / "canonical-m12"
    generation.mkdir(parents=True, exist_ok=False)
    master_path = generation / "master.sqlite"
    knowledge_path = generation / "knowledge.sqlite"
    Cartographer(master_path, ROM_SHA).close()
    expected_emission = [tuple((int(row["start"]), int(row["end"]),
        row["emission_type"], row["classification"], row["source_kind"],
        int(row["source_owned"]), row["artifact_type"], row["artifact"]))
        for row in emissions]
    authoritative_db = _authoritative_knowledge_db(repo_root, expected_emission)
    if authoritative_db is not None:
        import shutil
        shutil.copy2(authoritative_db, knowledge_path)
    else:
        store = KnowledgeStore(knowledge_path, ROM_SHA, ROM_SIZE)
        try:
            conv = {"attributes_json": "attributes", "value_json": "value",
                    "locator_json": "locator", "detail_json": "detail"}
            tmap = {"source_artifact": "evidence_artifacts", "rom_range": "ranges",
                    "rom_object": "objects", "claim": "claims", "relation": "relations",
                    "evidence_ref": "evidence_refs", "conflict": "conflicts",
                    "emission": "emission"}
            for tbl, key in tmap.items():
                if key in data:
                    cols = TABLE_COLUMNS[tbl]
                    col_list, q_list = ','.join(cols), ','.join('?' for _ in cols)
                    sql = f"INSERT OR IGNORE INTO {tbl}({col_list}) VALUES ({q_list})"
                    rows = [tuple((_canonical(r[conv.get(c, c)]) if c.endswith('_json')
                                   else r[conv.get(c, c)]) for c in cols) for r in data[key]]
                    store.db.executemany(sql, rows)
            store.db.commit()
        finally:
            store.close()
    baseline = generation / "stage7-baseline"
    (baseline / "code").mkdir(parents=True)
    (baseline / "blobs").mkdir()
    entries = []
    for idx, r in enumerate(emissions):
        start, end = int(r["start"]), int(r["end"])
        art_type = str(r["artifact_type"])
        art = f"code/sub_{start:06X}.asm" if art_type == "asm" else f"blobs/{start:06X}_{end:06X}.bin"
        dst = baseline / art
        if art_type == "asm":
            lines = ["; MASTER V2 canonical baseline"]
            for off in range(start, end, 16):
                lines.append("    dc.b " + ",".join(f"${b:02X}" for b in rom[off:min(end, off + 16)]))
            dst.write_text("\n".join(lines) + "\n", encoding="utf-8")
        else:
            dst.write_bytes(rom[start:end])
        entries.append({"start": start, "end": end, "size": end - start,
                        "kind": "CODE_VERIFIED" if art_type == "asm" else "UNKNOWN",
                        "source": "MASTER_V2_CANONICAL", "confidence": "ROM_HASH_VERIFIED",
                        "emitted_artifact_type": art_type, "artifact": art, "manifest_index": idx})
    manifest = {"schema": "oasis.full-rom-split.v1", "rom_sha256": ROM_SHA, "rom_size": ROM_SIZE,
                "entries": entries, "full_match": True,
                "metrics": {"SOURCE_OWNED_BYTES": CANONICAL_SOURCE_OWNED},
                "source": "M12_MASTER_V2_CANONICAL_BASELINE"}
    (baseline / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    meta = {"schema": "oasis.m12.postrun-canonical-generation.v1", "generation_id": "canonical-m12",
            "rom_sha256": ROM_SHA, "rom_size": ROM_SIZE, "parent_generation_id": None,
            "stage7": {"materialized": str(baseline)}}
    (generation / "generation-metadata.json").write_text(json.dumps(meta, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    pointer = {"schema": "oasis.m12.master-v2-materialized.current.v1",
               "generation_dir": "generations/canonical-m12",
               "master_sha256": _sha(master_path.read_bytes()),
               "knowledge_sha256": _sha(knowledge_path.read_bytes()),
               "rom_sha256": ROM_SHA, "source": str(json_path)}
    (root / "current.json").write_text(json.dumps(pointer, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return generation


__all__ = ["validate_canonical_knowledge_map_2d", "materialize_canonical_map_fallback",
           "CANONICAL_SOURCE_OWNED", "CANONICAL_SCHEMA", "CANONICAL_STATUS"]
