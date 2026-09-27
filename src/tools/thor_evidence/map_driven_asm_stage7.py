"""Current-generation MAP-driven executed ASM closure and Stage 8 audit."""

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3
import tempfile
import time
from typing import Any, Callable

from map_driven_asm_closure import (dependency_closed_starts,
    merge_overlapping_candidates, observed_chains, ownership_delta,
    promote_entries, prove_decoded_island, verify_roundtrip)
from map_driven_stage7_preflight import Stage7PreflightError, resolve_manifest
from re_full_split_run import write_layout
from rom_knowledge_map import TABLE_COLUMNS, canonical, sha256_bytes, stable_id
from stage7_subprocess import (DEFAULT_TIMEOUT_SECONDS, STAGE8_TIMEOUT_CODE,
    STAGE7_TIMEOUT_CODE, run as run_subprocess)
from stage7_decode import (decode_candidates, summarize_decode_metrics,
                           validate_range_decoder)

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 0x300000
PROMOTION_SOURCE = "M12_MAP_DRIVEN_EXECUTED_ASM_CLOSURE_2I.3"
STOP_STAGE8_SUBPROCESS_TIMEOUT = STAGE8_TIMEOUT_CODE
PASS_STATUS = "PASS_POSTRUN_MAP_DRIVEN_ASM_CLOSURE_V1"
PASS_AUDIT = "PASS_POSTRUN_FULL_ROM_AUDIT_V1"
PROTECTED_TYPES = {"ROM_DATA", "GRAPHICS_STREAM", "AUDIO_DATA", "POINTER_TABLE", "Z80_PROGRAM"}
PROTECTED_CLASSES = {"DATA_REGION_SUPPORTED", "DATA_STRUCTURE_SUPPORTED",
                     "LOCAL_ROM_DERIVED_ASSET", "POINTER_TABLE"}


def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()
def _write_json_atomic(path: Path, value: dict[str, Any]) -> None:
    """Persist a terminal Stage 7 receipt without exposing a partial JSON."""
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f".{path.name}.tmp-{os.getpid()}-{time.monotonic_ns()}")
    try:
        temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n",
                             encoding="utf-8")
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()
def _run(command: list[Path | str], cwd: Path | None = None,
         heartbeat: Callable[[], None] | None = None,
         label: str = "stage7 subprocess",
         timeout_seconds: float = DEFAULT_TIMEOUT_SECONDS,
         timeout_code: str = STAGE7_TIMEOUT_CODE) -> subprocess.CompletedProcess[str]:
    return run_subprocess(command, cwd=cwd, heartbeat=heartbeat,
                          timeout_seconds=timeout_seconds, label=label,
                          timeout_code=timeout_code)

def _heartbeat(progress: Any, *, candidate: int | None = None,
               total: int | None = None, start: int | None = None,
               end: int | None = None, phase: str) -> Callable[[], None] | None:
    if progress is None:
        return None
    started = time.monotonic()
    if candidate is None:
        prefix = f"Phase: {phase}"
    else:
        prefix = f"Candidate {candidate}/{total} 0x{start:06X}-0x{end:06X}; Phase: {phase}"

    def beat() -> None:
        progress.heartbeat(f"{prefix}; child elapsed: {time.monotonic() - started:.0f}s")

    return beat

def _map_hashes(db: sqlite3.Connection) -> dict[str, str]:
    def rows(table: str) -> list[tuple[Any, ...]]:
        cols = TABLE_COLUMNS[table]
        return [tuple(row) for row in db.execute(
            f"SELECT {','.join(cols)} FROM {table} ORDER BY {','.join(cols)}")]
    structure = {table: rows(table) for table in ("rom_range", "rom_object", "claim",
                                                   "relation", "conflict")}
    evidence = {table: rows(table) for table in ("source_artifact", "evidence_ref")}
    structure_hash = sha256_bytes(canonical(structure).encode())
    evidence_hash = sha256_bytes(canonical(evidence).encode())
    emission_hash = sha256_bytes(canonical(rows("emission")).encode())
    return {"structure_hash": structure_hash, "evidence_index_hash": evidence_hash,
            "emission_hash": emission_hash,
            "map_hash": sha256_bytes((structure_hash + evidence_hash + emission_hash).encode())}

def _snapshot(path: Path) -> dict[str, Any]:
    db = sqlite3.connect(f"file:{path.resolve().as_posix()}?mode=ro&immutable=1", uri=True)
    db.row_factory = sqlite3.Row
    try:
        if db.execute("PRAGMA integrity_check").fetchone()[0] != "ok":
            raise ValueError("STOP_STAGE7_KNOWLEDGE_INTEGRITY")
        instructions, backlog = [], []
        query = """SELECT DISTINCT o.object_id,r.start,r.end,o.object_type,o.attributes_json
            FROM rom_object o JOIN rom_range r USING(range_id) JOIN claim c USING(object_id)
            WHERE o.object_type='M68K_INSTRUCTION' AND c.claim_type='EXECUTED_FROM_ROM'
            ORDER BY r.start,r.end,o.object_id"""
        for row in db.execute(query):
            item = {"object_id": str(row[0]), "start": int(row[1]), "end": int(row[2]),
                    "object_type": str(row[3]), "attributes": json.loads(row[4])}
            instructions.append(item)
            if int(item["attributes"].get("source_owned_bytes", 0)) < item["end"] - item["start"]:
                backlog.append(item)
        edges = [{"source_object_id": str(row[0]), "target_object_id": str(row[1])}
                 for row in db.execute("""SELECT source_object_id,target_object_id FROM relation
                     WHERE relation_type='EXECUTED_NEXT' AND status='OBSERVED_RUNTIME'
                     AND target_object_id IS NOT NULL ORDER BY relation_id""")]
        owned = [(int(row[0]), int(row[1])) for row in db.execute(
            "SELECT start,end FROM emission WHERE source_owned=1 AND source_kind='CODE_VERIFIED'")]
        emissions = [tuple(row) for row in db.execute(
            "SELECT start,end,emission_type,classification,source_kind,source_owned,artifact_type,artifact "
            "FROM emission ORDER BY start,end")]
        protected = []
        for row in db.execute("""SELECT o.object_type,r.start,r.end,c.value_json
            FROM rom_object o JOIN rom_range r USING(range_id)
            LEFT JOIN claim c ON c.object_id=o.object_id AND c.claim_type='SOURCE_CLASS'"""):
            value = json.loads(row[3]).get("classification", "") if row[3] else ""
            if str(row[0]) in PROTECTED_TYPES or value in PROTECTED_CLASSES:
                protected.append((int(row[1]), int(row[2])))
        owned_bytes = int(db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0])
        return {"instructions": instructions, "backlog": backlog, "edges": edges,
                "owned_ranges": owned, "protected_ranges": protected, "emissions": emissions,
                "source_owned": owned_bytes, "hashes": _map_hashes(db),
                "emission_bytes": {str(r[0]): int(r[1]) for r in db.execute(
                    "SELECT emission_type,SUM(end-start) FROM emission GROUP BY emission_type")}}
    finally:
        db.close()

def _materialize(base_root: Path, target: Path, entries: list[dict[str, Any]], rom: bytes,
                 replacement_sources: dict[int, Path], assembler: Path,
                 heartbeat: Callable[[], None] | None = None) -> tuple[bool, Path]:
    target = target.resolve()
    if target.exists(): shutil.rmtree(target)
    target.mkdir(parents=True, exist_ok=True)
    for index, entry in enumerate(entries):
        artifact = str(entry["artifact"])
        destination = target / artifact; destination.parent.mkdir(parents=True, exist_ok=True)
        source = replacement_sources.get(index, base_root / artifact)
        if str(entry.get("emitted_artifact_type")) == "asm":
            shutil.copyfile(source, destination)
        else:
            destination.write_bytes(rom[int(entry["start"]):int(entry["end"])])
    write_layout(target, entries)
    rebuilt = target / "rebuilt.rom"
    completed = _run([assembler, "-m68000", "-no-opt", "-Fbin", "-o", rebuilt,
                       target / "full_layout.asm"], target, heartbeat=heartbeat,
                       label="Stage 7 full-ROM materialization")
    if completed.returncode or not rebuilt.is_file():
        return False, rebuilt
    actual = rebuilt.read_bytes()
    return actual == rom, rebuilt

def _normalize_split_artifacts(entries: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Give every split blob its own deterministic filename."""
    normalized = []
    for entry in entries:
        item = dict(entry)
        if str(item.get("emitted_artifact_type")) != "asm":
            item["artifact"] = f"blobs/{int(item['start']):06X}_{int(item['end']):06X}.bin"
        normalized.append(item)
    return normalized

def _copy_db(source: Path, target: Path) -> None:
    read = sqlite3.connect(source.resolve().as_uri() + "?mode=ro", uri=True)
    write = sqlite3.connect(target)
    try:
        read.backup(write)
        write.commit()
    finally:
        read.close(); write.close()

def _promote_db(source: Path, target: Path, promotions: list[dict[str, Any]]) -> tuple[int, dict[str, str]]:
    _copy_db(source, target)
    db = sqlite3.connect(target); db.row_factory = sqlite3.Row
    before = int(db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0])
    try:
        for promotion in promotions:
            start, end = int(promotion["start"]), int(promotion["end"])
            rows = list(db.execute("SELECT * FROM emission WHERE start<? AND end>? ORDER BY start", (end, start)))
            if not rows:
                raise ValueError("STOP_STAGE7_PROMOTION_NOT_IN_EMISSION_PARTITION")
            db.execute("DELETE FROM emission WHERE start<? AND end>?", (end, start))
            pieces = []
            for row in rows:
                lo, hi = int(row["start"]), int(row["end"])
                if lo < start: pieces.append((lo, start, row))
                if end < hi: pieces.append((end, hi, row))
            pieces.append((start, end, None))
            for piece_start, piece_end, original in sorted(pieces, key=lambda item: item[0]):
                if original is None:
                    values = (piece_start, piece_end, "ASM", "MAP_DRIVEN_EXECUTED_ASM_CLOSURE",
                              "CODE_VERIFIED", 1, "asm", f"code/sub_{start:06X}.asm")
                else:
                    values = tuple(original[key] for key in
                                   ("start", "end", "emission_type", "classification", "source_kind",
                                    "source_owned", "artifact_type", "artifact"))
                    values = (piece_start, piece_end, *values[2:])
                db.execute("INSERT INTO emission VALUES (?,?,?,?,?,?,?,?)", values)
        for promotion in promotions:
            start, end = int(promotion["start"]), int(promotion["end"])
            for row in db.execute("""SELECT o.object_id,o.attributes_json FROM rom_object o
                JOIN rom_range r USING(range_id) JOIN claim c USING(object_id)
                WHERE o.object_type='M68K_INSTRUCTION' AND c.claim_type='EXECUTED_FROM_ROM'
                AND r.start>=? AND r.end<=? ORDER BY r.start""", (start, end)):
                attrs = json.loads(row[1]); attrs["source_owned_bytes"] = int(attrs["length"])
                db.execute("UPDATE rom_object SET attributes_json=? WHERE object_id=?",
                           (json.dumps(attrs, sort_keys=True, separators=(",", ":")), row[0]))
                claim = db.execute("SELECT claim_id FROM claim WHERE object_id=? AND claim_type='SOURCE_OWNED'",
                                   (row[0],)).fetchone()
                if claim:
                    db.execute("UPDATE claim SET value_json='true',status='STATIC_VERIFIED' WHERE claim_id=?", (claim[0],))
                else:
                    claim_id = stable_id("claim", {"object_id": row[0], "claim_type": "SOURCE_OWNED", "stage": "2i.3"})
                    db.execute("INSERT INTO claim VALUES (?,?,?,?,?)",
                               (claim_id, row[0], "SOURCE_OWNED", "true", "STATIC_VERIFIED"))
                db.execute("""UPDATE claim SET value_json=?,status='STATIC_VERIFIED'
                    WHERE object_id=? AND claim_type='SOURCE_CLASS'""",
                           (json.dumps({"classification": "CODE_VERIFIED", "confidence": "CONFIRMED",
                                        "source_kind": "CODE_VERIFIED"}, sort_keys=True, separators=(",", ":")), row[0]))
        db.execute("INSERT OR REPLACE INTO map_meta VALUES (?,?)",
                   ("stage7_last_promotion", PROMOTION_SOURCE))
        db.commit()
        after = int(db.execute("SELECT COALESCE(SUM(end-start),0) FROM emission WHERE source_owned=1").fetchone()[0])
        if after - before != sum(int(item["end"]) - int(item["start"]) for item in promotions):
            raise ValueError("STOP_SOURCE_OWNED_DELTA_MISMATCH")
        return after - before, _map_hashes(db)
    finally:
        db.close()


def _audit_materialized(materialized: Path, rom: bytes, assembler: Path,
                        heartbeat: Callable[[], None] | None = None) -> dict[str, Any]:
    manifest = json.loads((materialized / "manifest.json").read_text(encoding="utf-8"))
    # Windows can briefly retain the assembler output handle after the child
    # exits. Cleanup is best-effort here; the audit result is already validated
    # byte-for-byte and the temporary directory is outside repository evidence.
    with tempfile.TemporaryDirectory(prefix="thor-stage8-", ignore_cleanup_errors=True) as temp_name:
        temp = Path(temp_name)
        for entry in manifest["entries"]:
            src = materialized / str(entry["artifact"]); dst = temp / str(entry["artifact"])
            dst.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(src, dst)
        write_layout(temp, manifest["entries"])
        rebuilt = temp / "audit.rom"
        result = _run([assembler, "-m68000", "-no-opt", "-Fbin", "-o", rebuilt,
                       temp / "full_layout.asm"], temp, heartbeat=heartbeat,
                       label="Stage 8 full-ROM audit",
                       timeout_code=STOP_STAGE8_SUBPROCESS_TIMEOUT)
        if result.returncode or not rebuilt.is_file() or rebuilt.read_bytes() != rom:
            raise ValueError("STOP_FULL_ROM_REBUILD_MISMATCH")
        return {"state": "PASS", "status": PASS_AUDIT, "full_rom_sha256": _sha(rebuilt),
                "bytes": len(rom), "manifest_sha256": _sha(materialized / "manifest.json")}


def run_stage8(materialized: Path, rom_path: Path, assembler: Path,
               progress: Any = None) -> dict[str, Any]:
    """Run the independent full-ROM audit after Stage 7 has returned."""
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("STOP_STAGE8_ROM_IDENTITY_MISMATCH")
    return _audit_materialized(
        materialized, rom, assembler,
        heartbeat=_heartbeat(progress, phase="STAGE8_FULL_ROM_ASSEMBLY"))


def run_stage7(canonical_generation: Path, rom_path: Path, range_tool: Path,
               assembler: Path, output: Path, progress: Any = None,
               max_iterations: int = 4) -> dict[str, Any]:
    """Recompute candidates, promote exact closed islands, and persist the terminal receipt."""
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("STOP_CANONICAL_ROM_IDENTITY_MISMATCH")
    output.mkdir(parents=True, exist_ok=True)
    decoder_contract = validate_range_decoder(range_tool)
    initial_snapshot = _snapshot(canonical_generation / "knowledge.sqlite")
    if progress:
        progress.heartbeat("VALIDATING EMISSION PARTITION")
    try:
        base_root, base_entries, preflight = resolve_manifest(
            canonical_generation, ROM_SHA, ROM_SIZE, initial_snapshot["emissions"],
            initial_snapshot["source_owned"], initial_snapshot["hashes"]["map_hash"])
    except Stage7PreflightError as error:
        (output / "stage7-preflight.json").write_text(
            json.dumps(error.diagnostic, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        raise
    (output / "stage7-preflight.json").write_text(
        json.dumps(preflight, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    source_owned_before = initial_snapshot["source_owned"]
    emission_before = initial_snapshot["emission_bytes"]
    history, total_delta, all_records, decode_batches = [], 0, [], []
    current_generation = canonical_generation
    last_materialized = base_root
    for iteration in range(1, max_iterations + 1):
        snapshot_generation = current_generation
        snapshot = initial_snapshot if iteration == 1 else _snapshot(
            current_generation / "knowledge.sqlite")
        candidates = merge_overlapping_candidates(observed_chains(snapshot["backlog"], snapshot["edges"], ROM_SHA))
        records, exact = [], []
        if progress:
            progress.update(0, total=len(candidates),
                           detail=f"iteration {iteration}: {len(candidates)} candidate islands")
        decode_jobs = []
        for number, candidate in enumerate(candidates, 1):
            start, end = candidate["intervals"][0]
            if any(start < hi and lo < end for lo, hi in snapshot["protected_ranges"]):
                continue
            candidate_dir = output / f"iteration-{iteration}" / f"{start:06X}-{end:06X}"
            candidate_dir.mkdir(parents=True, exist_ok=True)
            decode_jobs.append((number, candidate_dir, candidate))
        decode_started = time.monotonic()
        decode_workers = min(4, len(decode_jobs)) if decode_jobs else 0
        decode_metrics: dict[str, Any] = {}
        decode_results = decode_candidates(
            decode_jobs, range_tool, rom_path,
            heartbeat=(lambda done, total: progress.heartbeat(
                f"parallel decode {done}/{total} islands") if progress else None),
            metrics=decode_metrics)
        decode_batches.append(decode_metrics)
        decode_elapsed = round(time.monotonic() - decode_started, 3)
        for number, candidate in enumerate(candidates, 1):
            start, end = candidate["intervals"][0]
            if progress:
                progress.update(number - 1, total=len(candidates),
                                detail=(f"Candidate {number}/{len(candidates)} "
                                        f"0x{start:06X}-0x{end:06X}; Phase: BOUNDARY_CHECK"))
            record = {"candidate_id": candidate["candidate_id"], "start": start, "end": end,
                      "instruction_count": len(candidate["instructions"]),
                      "observed_edge_count": candidate["observed_edge_count"], "status": "STOP_BOUNDARY_UNPROVEN"}
            blocker = None
            if any(start < hi and lo < end for lo, hi in snapshot["protected_ranges"]):
                blocker = "STOP_MIXED_CODE_DATA_BOUNDARY"
            if blocker:
                record.update(status=blocker, blockers=[blocker]); records.append(record); continue
            try:
                decoded, asm, decode_error = decode_results.get(number, (None, None, None))
                if decode_error is not None:
                    raise decode_error
                if decoded is None or asm is None:
                    raise ValueError("STOP_STAGE7_DECODE_RESULT_MISSING")
                backlog_starts = {int(item["start"]) for item in snapshot["backlog"]}
                proof = prove_decoded_island(candidate, decoded, rom, snapshot["owned_ranges"],
                                             snapshot["protected_ranges"], backlog_starts, [])
                record.update(status=proof["status"], blockers=proof["blockers"], flow_edges=proof["edges"],
                              decoded_instruction_count=len(decoded["instructions"]))
                if proof["status"] == "PASS_CLOSED_ASM_RANGE":
                    binary = candidate_dir / "candidate.bin"
                    assembled = _run([assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm],
                                     heartbeat=_heartbeat(progress, candidate=number,
                                                          total=len(candidates), start=start,
                                                          end=end, phase="ASM_ROUNDTRIP"),
                                     label=f"Stage 7 ASM roundtrip 0x{start:06X}-0x{end:06X}")
                    if assembled.returncode or not binary.is_file():
                        record.update(status="STOP_ASM_ROUNDTRIP_MISMATCH", blockers=["STOP_ASM_ROUNDTRIP_MISMATCH"])
                    else:
                        record["roundtrip"] = verify_roundtrip(rom[start:end], binary.read_bytes())
                        if record["roundtrip"]["status"] == "PASS_ASM_ROUNDTRIP_EXACT":
                            exact.append((candidate, asm, proof)); record["status"] = "PASS_CLOSED_ASM_RANGE"
                        else:
                            record.update(status="STOP_ASM_ROUNDTRIP_MISMATCH", blockers=["STOP_ASM_ROUNDTRIP_MISMATCH"])
            except ValueError as error:
                record.update(status=str(error).split(":", 1)[0], blockers=[str(error).split(":", 1)[0]])
            records.append(record)
            if progress: progress.update(number, detail=f"audited {number}/{len(candidates)} islands")
        all_records.extend({**record, "iteration": iteration} for record in records)
        selected, dependencies = dependency_closed_starts([{"start": int(item[0]["intervals"][0][0]),
            "edges": item[2]["edges"]} for item in exact])
        selected_starts = {int(item[0]["intervals"][0][0]) for item in exact
                           if int(item[0]["intervals"][0][0]) in selected}
        for record in records:
            if record.get("status") == "PASS_CLOSED_ASM_RANGE" and record["start"] not in selected_starts:
                record.update(status="STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED",
                              blockers=["STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED"])
        exact = [item for item in exact if int(item[0]["intervals"][0][0]) in selected]
        if not exact:
            result = {"state": "NO_DELTA" if total_delta == 0 else "PASS", "status": PASS_STATUS,
                      "decoder_contract": decoder_contract,
                      "iteration_count": iteration, "candidate_count": len(records),
                      "closed_count": sum(row["status"] == "PASS_CLOSED_ASM_RANGE" for row in records),
                      "blocked_count": sum(row["status"] != "PASS_CLOSED_ASM_RANGE" for row in records),
                      "promoted_bytes": total_delta, "candidates": records,
                      "before": snapshot["source_owned"], "after": snapshot["source_owned"],
                      "generation_dir": str(current_generation), "map_hash": snapshot["hashes"]["map_hash"],
                      "materialized": str(last_materialized), "history": history}
            break
        promotions = []
        replacement_sources = {}
        entries = base_entries
        for candidate, asm, proof in exact:
            start, end = candidate["intervals"][0]
            stable = output / f"iteration-{iteration}" / f"sub_{start:06X}.asm"
            text = asm.read_text(encoding="utf-8").splitlines()
            text[0] = "; M12 Stage 7 MAP-driven executed ASM; canonical ROM " + ROM_SHA
            stable.write_text("\n".join(text) + "\n", encoding="utf-8")
            promotions.append({"candidate_id": candidate["candidate_id"], "start": start, "end": end,
                "bytes": end - start, "instruction_count": len(candidate["instructions"]),
                "executed_instruction_count": len(candidate["instructions"]), "artifact": f"code/sub_{start:06X}.asm",
                "proof_edges": proof["edges"], "asm_sha256": _sha(stable)})
        updated_entries = _normalize_split_artifacts(promote_entries(entries, promotions))
        materialized = output / f"iteration-{iteration}" / "materialized"
        replacement_sources = {index: output / f"iteration-{iteration}" / f"sub_{int(p['start']):06X}.asm"
                               for index, p in enumerate(promotions) for index in [next(i for i,e in enumerate(updated_entries)
                               if int(e["start"]) == int(p["start"]) and int(e["end"]) == int(p["end"]))]}
        matched, rebuilt = _materialize(
            base_root, materialized, updated_entries, rom, replacement_sources, assembler,
            heartbeat=_heartbeat(progress, phase="STAGE7_FULL_ROM_REBUILD"))
        if not matched: raise ValueError("STOP_FULL_ROM_REBUILD_MISMATCH")
        manifest = {"schema": "oasis.full-rom-split.v1", "rom_sha256": ROM_SHA, "rom_size": ROM_SIZE,
                    "entries": updated_entries, "full_match": True, "promotions": promotions,
                    "source": PROMOTION_SOURCE, "hashes": {"sha256": _sha(rebuilt)}}
        (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        generations_root = current_generation.parent
        generation_root = generations_root.parent
        generation = generations_root / ("gen-" + hashlib.sha256((snapshot["hashes"]["map_hash"] +
                                                                   str(promotions)).encode()).hexdigest()[:16])
        staging = generations_root / (".staging-stage7-" + hashlib.sha256(os.urandom(16)).hexdigest()[:12])
        staging.mkdir()
        delta, hashes = _promote_db(current_generation / "knowledge.sqlite", staging / "knowledge.sqlite", promotions)
        try: os.link(current_generation / "master.sqlite", staging / "master.sqlite")
        except OSError: shutil.copyfile(current_generation / "master.sqlite", staging / "master.sqlite")
        metadata = {"schema": "oasis.m12.postrun-canonical-generation.v1", "generation_id": generation.name,
                    "parent_generation_id": current_generation.name, "rom_sha256": ROM_SHA,
                    "importer_version": PROMOTION_SOURCE, "stage7": {"promotions": promotions,
                    "before_map_hash": snapshot["hashes"]["map_hash"], "after_map_hash": hashes["map_hash"],
                    "ownership_delta": delta, "full_rom_sha256": _sha(rebuilt),
                    "materialized": str(materialized.resolve())}}
        (staging / "generation-metadata.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        if generation.exists(): shutil.rmtree(generation)
        staging.rename(generation)
        pointer = {"schema": "oasis.m12.postrun-canonical.current.v2", "generation_dir": str(generation.relative_to(generation_root)),
                   "master_sha256": _sha(generation / "master.sqlite"), "knowledge_sha256": _sha(generation / "knowledge.sqlite"),
                   "metadata_sha256": _sha(generation / "generation-metadata.json"), "last_session_source_sha256":
                   json.loads((generation_root / "current.json").read_text(encoding="utf-8")).get("last_session_source_sha256"),
                   "rom_sha256": ROM_SHA, "status": PASS_STATUS}
        temporary = generation_root / "current.json.stage7"
        temporary.write_text(json.dumps(pointer, indent=2, sort_keys=True) + "\n", encoding="utf-8"); os.replace(temporary, generation_root / "current.json")
        total_delta += delta; current_generation = generation
        last_materialized = materialized
        base_root, base_entries = materialized, updated_entries
        history.append({"iteration": iteration, "promotions": promotions, "map_hash": hashes["map_hash"]})
        result = {"state": "PASS", "status": PASS_STATUS, "iteration_count": iteration,
                  "candidate_count": len(records), "promoted_bytes": total_delta,
                  "promotions": promotions, "candidates": records, "generation_dir": str(generation),
                  "map_hash": hashes["map_hash"], "history": history, "materialized": str(materialized),
                  "before": snapshot["source_owned"], "after": snapshot["source_owned"] + delta}
    else:
        raise ValueError("STOP_STAGE7_FIXED_POINT_GUARD")
    final_snapshot = snapshot if snapshot_generation == current_generation else _snapshot(
        current_generation / "knowledge.sqlite")
    materialized = Path(result.get("materialized", last_materialized))
    if "materialized" not in result: result["materialized"] = str(base_root)
    closed = [record for record in all_records if record.get("status") == "PASS_CLOSED_ASM_RANGE"]
    blocked = [record for record in all_records if record.get("status") != "PASS_CLOSED_ASM_RANGE"]
    promoted_ranges = [
        {"start": int(item["start"]), "end": int(item["end"]),
         "bytes": int(item["end"]) - int(item["start"])}
        for item in [promotion for entry in history for promotion in entry["promotions"]]
    ]
    decode_performance = summarize_decode_metrics(decode_batches)
    result.update({
        "decoder_contract": decoder_contract,
        "candidate_islands": all_records,
        "closed_islands": closed,
        "blocked_islands": blocked,
        "backlog_before": len(initial_snapshot["backlog"]),
        "backlog_after": len(final_snapshot["backlog"]),
        "promoted_instruction_count": sum(int(item["instruction_count"])
                                           for entry in history for item in entry["promotions"]),
        "source_owned_before": source_owned_before,
        "source_owned_after": final_snapshot["source_owned"],
        "source_owned_delta": final_snapshot["source_owned"] - source_owned_before,
        "asm_before": emission_before.get("ASM", 0),
        "asm_after": final_snapshot["emission_bytes"].get("ASM", 0),
        "incbin_before": emission_before.get("INCBIN", 0),
        "incbin_after": final_snapshot["emission_bytes"].get("INCBIN", 0),
        "map_changed": initial_snapshot["hashes"]["map_hash"] != final_snapshot["hashes"]["map_hash"],
        "promoted_ranges": promoted_ranges,
        "blocked_ranges": [{"start": record["start"], "end": record["end"]} for record in blocked],
        "output_paths": {"materialized": str(materialized.resolve()),
                         "stage7_result": str((output / "stage7-result.json").resolve())},
        "hashes": {"before_map": initial_snapshot["hashes"]["map_hash"],
                   "after_map": final_snapshot["hashes"]["map_hash"], "rom": ROM_SHA},
        "stop_reason": None,
        "preflight": preflight,
        "performance": {"decode_candidates": decode_performance["decode_request_total"],
                         "decode_workers": decode_performance["pool_workers"],
                         "decode_wall_seconds": decode_performance["batch_wall_seconds"],
                         **decode_performance},
    })
    result["stage7_terminal"] = True
    _write_json_atomic(output / "stage7-result.json", result)
    return result

__all__ = ["run_stage7", "run_stage8", "PASS_STATUS", "PASS_AUDIT", "STOP_STAGE8_SUBPROCESS_TIMEOUT"]
