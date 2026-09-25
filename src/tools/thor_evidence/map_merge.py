"""Offline deterministic merge of a saved AUTO67 session map."""

from __future__ import annotations

import hashlib
import os
import shutil
import sqlite3
from pathlib import Path
from typing import Any

try:
    from .cartographer import Cartographer
    from .runtime_occurrence_merge import merge_occurrences
except ImportError:
    from cartographer import Cartographer
    from runtime_occurrence_merge import merge_occurrences


def _file_hash(path: Path) -> str | None:
    if not path.exists():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _rom_sha(path: Path) -> str:
    db = sqlite3.connect(path)
    try:
        row = db.execute("SELECT value FROM map_meta WHERE key='rom_sha256'").fetchone()
    finally:
        db.close()
    if not row or not row[0]:
        raise ValueError("session map is missing rom_sha256")
    return str(row[0])


def merge_session_map(global_path: Path, session_path: Path,
                      reject_conflicts: bool = False) -> dict[str, Any]:
    """Merge a closed session graph into a temporary global copy atomically."""
    global_path = Path(global_path).resolve()
    session_path = Path(session_path).resolve()
    if not session_path.exists():
        raise FileNotFoundError(session_path)
    session_rom = _rom_sha(session_path)
    session_graph = Cartographer(session_path, session_rom)
    temp_path = global_path.with_name(global_path.stem + ".merge.tmp.sqlite")
    global_before_hash = _file_hash(global_path)
    global_before_graph = None
    conflicts_before = 0
    target = None
    try:
        if global_path.exists():
            global_rom = _rom_sha(global_path)
            if global_rom != session_rom:
                raise ValueError("global/session rom_sha256 mismatch")
        session_hash = session_graph.graph_hash()
        bundle = session_graph.export_bundle()
        if temp_path.exists():
            temp_path.unlink()
        if global_path.exists():
            shutil.copy2(global_path, temp_path)
            target = Cartographer(temp_path, session_rom)
            global_before_graph = target.graph_hash()
            conflicts_before = int(target.db.execute(
                "SELECT COUNT(*) FROM map_conflict").fetchone()[0])
        else:
            target = Cartographer(temp_path, session_rom)
            global_before_graph = target.graph_hash()
        delta = target.merge(bundle, "session-map:" + session_hash, session_rom)
        occurrence_delta = merge_occurrences(session_graph.db, target.db)
        metrics = target.metrics()
        if metrics["graph_hash"] != delta.graph_hash:
            raise ValueError("global graph hash validation failed")
        if reject_conflicts and int(metrics["conflicts"]) > conflicts_before:
            raise ValueError("STOP_ARCHIVIST_MERGE_CONFLICT")
        target.close()
        target = None
        with temp_path.open("r+b") as stream:
            os.fsync(stream.fileno())
        os.replace(temp_path, global_path)
        return {"status": "PASS", "session_graph_hash": session_hash,
                "global_graph_hash_before": global_before_graph,
                "global_graph_hash_after": metrics["graph_hash"],
                "global_merge_delta": delta.as_dict(),
                "runtime_occurrence_delta": occurrence_delta,
                "global_before_file_hash": global_before_hash,
                "global_after_file_hash": _file_hash(global_path),
                "import_ref": "session-map:" + session_hash}
    except Exception:
        if target is not None:
            target.close()
        if temp_path.exists():
            temp_path.unlink()
        raise
    finally:
        session_graph.close()


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--global", dest="global_path", type=Path, required=True)
    parser.add_argument("--session", type=Path, required=True)
    args = parser.parse_args()
    print(merge_session_map(args.global_path, args.session))
