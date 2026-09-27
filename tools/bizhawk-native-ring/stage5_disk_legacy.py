"""Legacy disk-backed Stage 5 kept for shadow and regression comparison."""
from __future__ import annotations

import os
import json
from pathlib import Path
import shutil
import uuid
from typing import Any

def run_stage5_disk(receipt_path: Path, receipt: dict[str, Any], output: Path, rom_path: Path,
            master_generation: Path, decoder: Path, progress: Any = None) -> dict[str, Any]:
    from live_forward_complete_pipeline import (
        IMPORTER_VERSION, ROM_SHA, ROM_SIZE, STOP_IDENTITY_CONFLICT, STOP_OWNERSHIP_CHANGED,
        _compact_session, _copy_db, _json, _ownership_snapshot, _rename_with_retry,
        _sha, _validate_rolling_master, resolve_master_canonical_view,
    )
    from live_forward_archivist import archive_session
    from rom_knowledge_live_import import import_archivist_session
    from cartographer import Cartographer
    rolling = _validate_rolling_master(receipt_path, receipt, master_generation)
    session, session_info = _compact_session(receipt, output, rom_path, decoder, progress)
    session_sha = _sha(session)
    view = resolve_master_canonical_view(Path(__file__).parents[2])
    if view is None:
        raise ValueError(f"{STOP_MAP_FACT_MISSING}:accepted canonical master/knowledge pair is unavailable")
    scratch_root = output / "canonical-master-v2"
    if (scratch_root / "current.json").is_file():
        base_pointer = _json(scratch_root / "current.json")
        base_generation = (scratch_root / base_pointer["generation_dir"]).resolve()
        base_master, base_knowledge = base_generation / "master.sqlite", base_generation / "knowledge.sqlite"
    else:
        base_generation = view.materialize(scratch_root, rom_path)
        base_pointer = _json(scratch_root / "current.json")
        base_master, base_knowledge = base_generation / "master.sqlite", base_generation / "knowledge.sqlite"
    base_snapshot = _ownership_snapshot(base_knowledge)
    generation_root = scratch_root
    current = generation_root / "current.json"
    if base_pointer.get("last_session_source_sha256") == session_sha:
        generation = (generation_root / base_pointer["generation_dir"]).resolve()
        if generation.relative_to((generation_root / "generations").resolve()) and \
                _sha(generation / "master.sqlite") == base_pointer.get("master_sha256") and \
                _sha(generation / "knowledge.sqlite") == base_pointer.get("knowledge_sha256"):
            return {"state": "NO_DELTA", "replay_status": "PASS_IDEMPOTENT_NOOP",
                    "session_path": str(session), "session_sha256": session_sha,
                    "generation_dir": str(generation), "rolling_master": rolling,
                    "before": base_snapshot, "after": base_snapshot}
    staging = generation_root / "generations" / (".staging-" + uuid.uuid4().hex)
    staging.mkdir(parents=True, exist_ok=False)
    try:
        # The accepted Archivist master is a 1+ GiB immutable proof artifact.
        # Use a tiny MAP-1 merge target for the session-chain receipt and carry
        # the accepted master into the generation by hard link.
        merge_master, staged_knowledge = staging / "merge-master.sqlite", staging / "knowledge.sqlite"
        empty_master = Cartographer(merge_master, ROM_SHA)
        empty_master.close()
        _copy_db(base_knowledge, staged_knowledge)
        merged = archive_session(merge_master, session, ROM_SHA)
        rom = rom_path.read_bytes()
        imported = import_archivist_session(session, merge_master, staged_knowledge,
                                            rom, ROM_SHA, merged["merge_receipt"])
        replay = import_archivist_session(session, merge_master, staged_knowledge,
                                          rom, ROM_SHA, merged["merge_receipt"])
        if replay.get("status") != "PASS_IDEMPOTENT_NOOP":
            raise ValueError("STOP_POSTRUN_MAP_DOUBLE_IMPORT")
        after = _ownership_snapshot(staged_knowledge)
        for key in ("source_owned_bytes", "emission_bytes"):
            if after[key] != base_snapshot[key]:
                raise ValueError(f"{STOP_OWNERSHIP_CHANGED}:{key}")
        if after["hashes"]["emission_hash"] != base_snapshot["hashes"]["emission_hash"]:
            raise ValueError(f"{STOP_OWNERSHIP_CHANGED}:emission_hash")
        try:
            os.link(base_master, staging / "master.sqlite")
        except OSError:
            _copy_db(base_master, staging / "master.sqlite")
        (staging / "merge-master.sqlite").unlink()
        generation = generation_root / "generations" / ("gen-" + session_sha[:16])
        if generation.exists():
            raise ValueError(f"{STOP_IDENTITY_CONFLICT}:generation already exists")
        metadata = {"schema": "oasis.m12.postrun-canonical-generation.v1",
                    "generation_id": generation.name,
                    "parent_generation_id": base_pointer.get("generation_dir"),
                    "rom_sha256": ROM_SHA, "rom_size": ROM_SIZE,
                    "source_run_id": int(receipt["runtime"]["run_id"]),
                    "session_source_sha256": session_sha,
                    "rolling_master": rolling, "importer_version": IMPORTER_VERSION,
                    "session": session_info, "before": base_snapshot, "after": after,
                    "import": imported, "replay": replay, "audit": "PASS"}
        (staging / "generation-metadata.json").write_text(
            json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        _rename_with_retry(staging, generation)
        pointer = {"schema": "oasis.m12.postrun-canonical.current.v2",
                   "generation_dir": str(generation.relative_to(generation_root)),
                   "master_sha256": _sha(generation / "master.sqlite"),
                   "knowledge_sha256": _sha(generation / "knowledge.sqlite"),
                   "metadata_sha256": _sha(generation / "generation-metadata.json"),
                   "last_session_source_sha256": session_sha,
                   "rom_sha256": ROM_SHA, "status": "PASS_POSTRUN_CANONICAL_MAP_REFRESH_V1"}
        temporary = current.with_suffix(".tmp-" + uuid.uuid4().hex)
        temporary.write_text(json.dumps(pointer, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        os.replace(temporary, current)
        return {"state": "PASS", "status": "PASS_POSTRUN_CANONICAL_MAP_REFRESH_V1",
                "session_path": str(session), "session_sha256": session_sha,
                "generation_dir": str(generation), "pointer": pointer,
                "rolling_master": rolling, "before": base_snapshot, "after": after,
                "import": imported, "replay": replay, "source_owned_delta": 0,
                "emission_unchanged": True}
    except Exception:
        shutil.rmtree(staging, ignore_errors=True)
        raise
