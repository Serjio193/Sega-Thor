"""Fail-closed compaction for committed canonical knowledge generations."""

from __future__ import annotations

import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import sys
from typing import Any, Iterator

_TOOLS = str(Path(__file__).resolve().parents[1])
if _TOOLS not in sys.path:
    sys.path.insert(0, _TOOLS)

try:
    from .rom_knowledge_live_import import sha256_file
    from .rom_knowledge_map import KnowledgeStore
except ImportError:
    from rom_knowledge_live_import import sha256_file
    from rom_knowledge_map import KnowledgeStore

POINTER_SCHEMA = "oasis.m12.archivist-canonical-knowledge.current.v1"
RECEIPT_STATUS = "PASS_ARCHIVIST_CANONICAL_KNOWLEDGE_PIPELINE_V1"
EVIDENCE_RECEIPT_STATUS = "PASS_NORMALIZED_V2_CANONICAL_PUBLISH_V1"
LINEAGE_SCHEMA = "oasis.m14.canonical-generation-lineage.v1"
DB_NAMES = ("master.sqlite", "knowledge.sqlite")


@contextmanager
def transaction_lock(root: Path, *, blocking: bool = False) -> Iterator[bool]:
    """Serialize publishers and GC without adding a dependency."""
    root = Path(root)
    root.mkdir(parents=True, exist_ok=True)
    path = root / ".generation-transaction.lock"
    stream = path.open("a+b")
    stream.seek(0, os.SEEK_END)
    if stream.tell() == 0:
        stream.write(b"\0")
        stream.flush()
    acquired = False
    try:
        if os.name == "nt":
            import msvcrt
            stream.seek(0)
            try:
                msvcrt.locking(stream.fileno(), msvcrt.LK_LOCK if blocking else msvcrt.LK_NBLCK, 1)
                acquired = True
            except OSError:
                acquired = False
        else:
            import fcntl
            try:
                fcntl.flock(stream.fileno(), fcntl.LOCK_EX |
                            (0 if blocking else fcntl.LOCK_NB))
                acquired = True
            except BlockingIOError:
                acquired = False
        yield acquired
    finally:
        if acquired:
            if os.name == "nt":
                import msvcrt
                stream.seek(0)
                msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
            else:
                import fcntl
                fcntl.flock(stream.fileno(), fcntl.LOCK_UN)
        stream.close()


def _json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"not a JSON object: {path.name}")
    return value


def _regular_file(directory: Path, name: str) -> Path:
    path = directory / name
    if path.is_symlink() or not path.is_file():
        raise ValueError(f"expected regular in-root file: {path.name}")
    return path


def _canonical_hash(value: dict[str, Any]) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"),
                         ensure_ascii=True).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def _lineage_for(directory: Path, receipt: dict[str, Any], parent_id: str | None,
                 receipt_refs: list[str]) -> dict[str, Any]:
    value = {"schema": LINEAGE_SCHEMA, "generation_id": directory.name,
        "parent_generation_id": parent_id,
        "map_hash_before": receipt.get("knowledge_before", {}).get("hashes", {}).get("map_hash"),
        "map_hash_after": receipt.get("knowledge_after", {}).get("hashes", {}).get("map_hash"),
        "logical_hashes_before": receipt.get("knowledge_before", {}).get("hashes", {}),
        "logical_hashes_after": receipt.get("knowledge_after", {}).get("hashes", {}),
        "source_artifact_sha256": receipt.get("archivist_merge", {}).get("source_artifact_sha256"),
        "session_id": receipt.get("archivist_merge", {}).get("session_id"),
        "merge_receipt_sha256": receipt.get("archivist_merge", {}).get("receipt_sha256"),
        "experiment_receipt_refs": receipt_refs,
        "status": "SEALED_FROM_COMMITTED_GENERATION_RECEIPT"}
    value["lineage_sha256"] = _canonical_hash(value)
    return value


def _read_generation(directory: Path) -> tuple[dict[str, Any], dict[str, Any], str | None]:
    if directory.is_symlink() or not directory.is_dir():
        raise ValueError("generation path is not a regular directory")
    for name in DB_NAMES:
        if (directory / name).exists() and (directory / name).is_symlink():
            raise ValueError(f"database candidate is a symlink: {name}")
    receipt = _json(_regular_file(directory, "receipt.json"))
    merge = _json(_regular_file(directory, "merge-receipt.json"))
    audit = _json(_regular_file(directory, "audit.json"))
    imported = _json(_regular_file(directory, "import-report.json"))
    if receipt.get("status") not in {RECEIPT_STATUS, EVIDENCE_RECEIPT_STATUS} or \
            receipt.get("generation_id") != directory.name:
        raise ValueError("generation receipt status/id mismatch")
    if receipt.get("archivist_merge") != merge:
        raise ValueError("merge receipt differs from sealed generation receipt")
    if audit.get("status") not in {"PASS_INDEPENDENT_ARCHIVIST_CANONICAL_AUDIT_V1",
                                    "PASS_NORMALIZED_V2_CANONICAL_AUDIT_V1"}:
        raise ValueError("independent audit is not PASS")
    if imported.get("status") != "PASS_IMPORTED":
        raise ValueError("import report is not PASS")
    expected_hashes = receipt.get("knowledge_after", {}).get("hashes")
    if not isinstance(expected_hashes, dict) or audit.get("hashes_after") != expected_hashes:
        raise ValueError("audit and receipt logical hashes differ")
    if imported.get("session_graph_hash") != merge.get("session_graph_hash"):
        raise ValueError("import and merge session graph hashes differ")
    owned = receipt.get("source_owned", {})
    before_owned = receipt.get("knowledge_before", {}).get("metrics", {}).get("source_owned_bytes")
    after_owned = receipt.get("knowledge_after", {}).get("metrics", {}).get("source_owned_bytes")
    if owned.get("before") != before_owned or owned.get("after") != after_owned or \
            owned.get("delta") != 0 or before_owned != after_owned:
        raise ValueError("SOURCE_OWNED contract mismatch in generation receipt")
    lineage_path = directory / "lineage.json"
    if lineage_path.is_symlink():
        raise ValueError("lineage receipt is a symlink")
    lineage: dict[str, Any] | None = None
    if lineage_path.is_file():
        lineage = _json(lineage_path)
        seal = lineage.get("lineage_sha256")
        body = {key: value for key, value in lineage.items() if key != "lineage_sha256"}
        if lineage.get("schema") != LINEAGE_SCHEMA or seal != _canonical_hash(body):
            raise ValueError("lineage receipt seal mismatch")
        if lineage.get("generation_id") != directory.name:
            raise ValueError("lineage generation id mismatch")
        parent_id = lineage.get("parent_generation_id")
        if lineage.get("map_hash_after") != expected_hashes.get("map_hash"):
            raise ValueError("lineage after hash differs from generation receipt")
    else:
        db = directory / "knowledge.sqlite"
        if not db.is_file():
            raise ValueError("missing lineage receipt and knowledge database")
        con = sqlite3.connect(db.resolve().as_uri() + "?mode=ro&immutable=1", uri=True)
        try:
            meta = dict(con.execute("SELECT key,value FROM map_meta"))
        finally:
            con.close()
        if meta.get("generation_id") != directory.name:
            raise ValueError("knowledge database generation id mismatch")
        parent_id = meta.get("parent_generation_id") or None
    return receipt, merge, parent_id


def _self_check_current(root: Path, pointer: dict[str, Any]) -> tuple[dict[str, Any], Path]:
    generations_root = root / "generations"
    if generations_root.is_symlink() or not generations_root.is_dir():
        raise ValueError("generations root is not a regular directory")
    rel = Path(str(pointer.get("generation_dir", "")))
    if rel.is_absolute() or len(rel.parts) != 2 or rel.parts[0] != "generations":
        raise ValueError("current pointer path is not a direct generations child")
    directory = (root / rel).resolve()
    directory.relative_to((root / "generations").resolve())
    if directory.name != pointer.get("generation_id"):
        raise ValueError("pointer generation id/path mismatch")
    master, knowledge = directory / "master.sqlite", directory / "knowledge.sqlite"
    if not master.is_file() or not knowledge.is_file():
        raise ValueError("current generation is missing a database")
    if sha256_file(master) != pointer.get("master_sha256") or \
            sha256_file(knowledge) != pointer.get("knowledge_sha256"):
        raise ValueError("current database file hash mismatch")
    receipt = _json(directory / "receipt.json")
    rom = receipt.get("rom", {})
    rom_sha, rom_size = rom.get("sha256"), rom.get("bytes")
    if not isinstance(rom_sha, str) or not isinstance(rom_size, int):
        raise ValueError("current generation receipt has no ROM identity")
    store = KnowledgeStore(knowledge, rom_sha, rom_size, read_only=True)
    try:
        hashes, metrics = store.hashes(), store.metrics()
        meta = store.meta()
        if hashes.get("map_hash") != pointer.get("knowledge_map_hash"):
            raise ValueError("current logical map hash mismatch")
        if meta.get("generation_id") != pointer.get("generation_id") or \
                (meta.get("parent_generation_id") or None) != (pointer.get("parent_generation_id") or None):
            raise ValueError("current database lineage metadata mismatch")
        if pointer.get("logical_hashes") != hashes:
            raise ValueError("current logical hashes mismatch")
        source_owned = receipt.get("source_owned", {})
        if source_owned.get("after") != metrics.get("source_owned_bytes") or \
                source_owned.get("delta") != 0 or source_owned.get("before") != source_owned.get("after"):
            raise ValueError("current SOURCE_OWNED contract mismatch")
    finally:
        store.close()
    for path in (master, knowledge):
        con = sqlite3.connect(path.resolve().as_uri() + "?mode=ro", uri=True)
        try:
            if con.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
                raise ValueError(f"current SQLite integrity_check failed: {path.name}")
            if con.execute("PRAGMA foreign_key_check").fetchone() is not None:
                raise ValueError(f"current SQLite foreign_key_check failed: {path.name}")
        finally:
            con.close()
    return {"status": "PASS", "map_hash": hashes["map_hash"],
            "logical_hashes": hashes, "metrics": metrics}, directory


def _experiment_receipts(repo_root: Path) -> list[tuple[str, str]]:
    receipt_root = repo_root / "docs/reports/m14-7b-closure-receipts"
    rows = []
    if receipt_root.is_dir():
        for path in receipt_root.rglob("*.json"):
            try:
                text = path.read_text(encoding="utf-8")
            except OSError:
                continue
            rows.append((path.relative_to(repo_root).as_posix(), text))
    return rows


def _matching_receipts(merge: dict[str, Any], receipts: list[tuple[str, str]]) -> list[str]:
    tokens = [value for value in (merge.get("session_id"),
        merge.get("source_artifact_sha256"), merge.get("receipt_sha256"))
        if isinstance(value, str) and value]
    return sorted(path for path, text in receipts if any(token in text for token in tokens))


def build_plan(root: Path, *, lock_available: bool = True) -> dict[str, Any]:
    root = Path(root).resolve()
    result: dict[str, Any] = {"schema": "oasis.m14.canonical-generation-gc-plan.v1",
        "root": str(root), "status": "PASS", "self_check": None,
        "current_generation_id": None, "current_map_hash": None,
        "full_generation_count_before": 0, "full_generation_bytes_before": 0,
        "full_generation_count_after": 0, "full_generation_bytes_after": 0,
        "reclaim_bytes": 0, "generations": [], "blocked": []}
    if not lock_available:
        result["status"] = "BLOCKED_ACTIVE_TRANSACTION"
        result["blocked"].append("generation transaction lock is held")
        return result
    try:
        pointer = _json(_regular_file(root, "current.json"))
        if pointer.get("schema") != POINTER_SCHEMA:
            raise ValueError("current pointer schema mismatch")
        current_check, current_dir = _self_check_current(root, pointer)
        result["self_check"] = current_check
        result["current_generation_id"] = current_dir.name
        result["current_map_hash"] = current_check["map_hash"]
        generations = root / "generations"
        if generations.is_symlink() or not generations.is_dir():
            raise ValueError("generations root is not a regular directory")
        repo_root = Path(__file__).resolve().parents[3]
        experiment_receipts = _experiment_receipts(repo_root)
        by_id: dict[str, dict[str, Any]] = {}
        for directory in sorted(generations.iterdir()):
            if directory.name.startswith(".staging-"):
                result["generations"].append({"generation_id": directory.name,
                    "status": "BLOCKED_UNSEALED_STAGING",
                    "reason_code": "STAGING_HAS_NO_SEALED_COMMIT_RECEIPT",
                    "path": str(directory),
                    "bytes": sum(p.stat().st_size for p in directory.iterdir() if p.is_file())})
                continue
            if not directory.name.startswith("gen-"):
                result["generations"].append({"generation_id": directory.name,
                    "status": "BLOCKED_UNRECOGNIZED_ENTRY",
                    "reason_code": "ENTRY_NAME_NOT_A_GENERATION",
                    "path": str(directory)})
                continue
            receipt, merge, parent_id = _read_generation(directory)
            db_files = [directory / name for name in DB_NAMES if (directory / name).is_file()]
            size = sum(path.stat().st_size for path in db_files)
            row = {"generation_id": directory.name, "parent_generation_id": parent_id,
                "path": str(directory), "bytes": size, "database_files": [p.name for p in db_files],
                "map_hash": receipt["knowledge_after"]["hashes"]["map_hash"],
                "created_by_session": merge.get("session_id"),
                "source_artifact_sha256": merge.get("source_artifact_sha256"),
                "receipt_sha256": merge.get("receipt_sha256"),
                "receipt_refs": [str((directory / name).relative_to(root)) for name in
                    ("receipt.json", "merge-receipt.json", "import-report.json", "audit.json")
                    if (directory / name).is_file()] +
                    _matching_receipts(merge, experiment_receipts),
                "lineage_receipt_present": (directory / "lineage.json").is_file(),
                "self_check_status": "PASS_CURRENT" if directory == current_dir else "PASS_RECEIPT_AND_LINEAGE",
                "query_access_after_compaction": "CURRENT_CANONICAL_MAP",
                "rollback_requirement": "REPLAY_REQUIRED_FROM_CAPTURE_OR_BOOTSTRAP",
                "safe_removal_candidate": False,
                "status": "CURRENT" if directory == current_dir else "PENDING_COMPACTION"}
            by_id[directory.name] = row
            result["generations"].append(row)
        chain: list[str] = []
        seen: set[str] = set()
        generation_id = current_dir.name
        while generation_id:
            if generation_id in seen:
                raise ValueError("generation lineage contains a cycle")
            seen.add(generation_id)
            row = by_id.get(generation_id)
            if row is None:
                raise ValueError(f"missing committed generation in current lineage: {generation_id}")
            chain.append(generation_id)
            generation_id = row["parent_generation_id"]
            if generation_id and generation_id not in by_id:
                # The first published generation may be seeded from an external 2D DB.
                result["external_bootstrap_generation_id"] = generation_id
                generation_id = None
        for child_id, parent_id in zip(chain, chain[1:]):
            child = by_id[child_id]
            parent = by_id[parent_id]
            child_receipt = _json(Path(child["path"]) / "receipt.json")
            parent_receipt = _json(Path(parent["path"]) / "receipt.json")
            before = child_receipt.get("knowledge_before", {}).get("hashes", {})
            after = parent_receipt.get("knowledge_after", {}).get("hashes", {})
            if before != after:
                result["blocked"].append(f"logical hash transition mismatch: {parent_id} -> {child_id}")
        for row in result["generations"]:
            if row["generation_id"] in seen and row["generation_id"] != current_dir.name:
                row["status"] = "DISPOSABLE_VERIFIED" if row["database_files"] else "COMPACTED_RECEIPT_ONLY"
                row["safe_removal_candidate"] = bool(row["database_files"])
                result["reclaim_bytes"] += row["bytes"]
            elif row["status"] == "PENDING_COMPACTION":
                row["status"] = "BLOCKED_NOT_IN_CURRENT_LINEAGE"
                row["reason_code"] = "COMMITTED_GENERATION_NOT_IN_CURRENT_ANCESTRY"
                result["blocked"].append(f"orphan committed generation: {row['generation_id']}")
        full = [r for r in result["generations"] if r.get("database_files") == list(DB_NAMES)]
        result["full_generation_count_before"] = len(full)
        result["full_generation_bytes_before"] = sum(r["bytes"] for r in full)
        result["full_generation_count_after"] = 1
        result["full_generation_bytes_after"] = by_id[current_dir.name]["bytes"]
        result["chain_count"] = len(chain)
        if result["blocked"]:
            result["status"] = "BLOCKED_VERIFICATION"
            result["reclaim_bytes"] = 0
    except (OSError, ValueError, KeyError, sqlite3.Error, json.JSONDecodeError) as exc:
        result["status"] = "BLOCKED_VERIFICATION"
        result["blocked"].append(str(exc))
        result["reclaim_bytes"] = 0
    return result


def compact_verified(root: Path, *, lock_held: bool = False) -> dict[str, Any]:
    root = Path(root).resolve()
    if not lock_held:
        with transaction_lock(root) as acquired:
            if not acquired:
                return build_plan(root, lock_available=False)
            return compact_verified(root, lock_held=True)
    plan = build_plan(root)
    if plan["status"] != "PASS":
        return plan
    generations = root / "generations"
    # Seal logical lineage for every committed generation before deleting any database.
    for row in plan["generations"]:
        if row.get("status") not in ("CURRENT", "DISPOSABLE_VERIFIED", "COMPACTED_RECEIPT_ONLY"):
            continue
        directory = Path(row["path"])
        lineage_path = directory / "lineage.json"
        if not lineage_path.exists():
            receipt = _json(directory / "receipt.json")
            parent_id = row.get("parent_generation_id")
            lineage = _lineage_for(directory, receipt, parent_id, row.get("receipt_refs", []))
            temporary = directory / ("lineage.json.tmp-" + os.urandom(8).hex())
            temporary.write_text(json.dumps(lineage, sort_keys=True, indent=2) + "\n",
                                 encoding="utf-8", newline="\n")
            with temporary.open("rb+") as stream:
                os.fsync(stream.fileno())
            os.replace(temporary, lineage_path)
    # Re-audit all newly sealed lineage receipts and current pointer before exact unlink operations.
    plan = build_plan(root)
    if plan["status"] != "PASS":
        return plan
    removed_files = removed_bytes = 0
    removed_generations = []
    current_id = plan["current_generation_id"]
    for row in plan["generations"]:
        if row.get("status") != "DISPOSABLE_VERIFIED":
            continue
        if row["generation_id"] == current_id:
            continue
        directory = Path(row["path"])
        removed_items = []
        for name in DB_NAMES:
            exact = directory / name
            if exact.is_file():
                size = exact.stat().st_size
                exact.unlink()
                removed_files += 1
                removed_bytes += size
                removed_items.append({"path": str(exact), "bytes": size})
        if removed_items:
            removed_generations.append({"generation_id": row["generation_id"],
                "parent_generation_id": row.get("parent_generation_id"),
                "path": str(directory), "database_bytes_before": sum(x["bytes"] for x in removed_items),
                "map_hash": row.get("map_hash"), "created_by_session": row.get("created_by_session"),
                "source_artifact_sha256": row.get("source_artifact_sha256"),
                "receipt_sha256": row.get("receipt_sha256"),
                "receipt_refs": row.get("receipt_refs", []),
                "query_access_after_compaction": row.get("query_access_after_compaction"),
                "self_check_status": row.get("self_check_status"),
                "rollback_requirement": row.get("rollback_requirement"),
                "safe_removal_candidate": True, "removed_files": removed_items})
    final = build_plan(root)
    if final["status"] != "PASS" or final["full_generation_count_before"] != 1:
        final["status"] = "BLOCKED_POST_COMPACTION_CHECK"
        final["blocked"].append("post-compaction audit did not find exactly one full generation")
    final["removed_database_files"] = removed_files
    final["removed_database_bytes"] = removed_bytes
    final["removed_generations"] = removed_generations
    final["full_generation_count_initial"] = plan["full_generation_count_before"]
    final["full_generation_bytes_initial"] = plan["full_generation_bytes_before"]
    final["full_generation_count_before"] = plan["full_generation_count_before"]
    final["full_generation_bytes_before"] = plan["full_generation_bytes_before"]
    final["reclaim_bytes"] = removed_bytes
    return final


def serialize_publish(publish: Any, *args: Any, **kwargs: Any) -> dict[str, Any]:
    root = kwargs.get("output_root", args[4] if len(args) > 4 else None)
    if root is None:
        raise TypeError("output_root is required for canonical publication")
    with transaction_lock(Path(root), blocking=True) as acquired:
        if not acquired:
            raise RuntimeError("STOP_CANONICAL_GENERATION_TRANSACTION_LOCK_FAILED")
        return publish(*args, **kwargs)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True, help="canonical-map-accepted root")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--plan", action="store_true", help="read-only exact cleanup plan")
    mode.add_argument("--verified-only", action="store_true", help="compact only verified ancestors")
    args = parser.parse_args()
    if args.plan:
        with transaction_lock(args.root) as acquired:
            result = build_plan(args.root, lock_available=acquired)
    else:
        result = compact_verified(args.root)
    print(json.dumps(result, sort_keys=True, indent=2))
    return 0 if result.get("status") == "PASS" else 2


if __name__ == "__main__":
    raise SystemExit(main())
