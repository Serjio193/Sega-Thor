import hashlib, json
from datetime import datetime, timezone
import os; from pathlib import Path
import shutil
import sqlite3
import sys
import time
import uuid
from typing import Any
EVIDENCE_TOOLS = Path(__file__).parents[2] / "src" / "tools" / "thor_evidence"
if str(EVIDENCE_TOOLS) not in sys.path:
    sys.path.insert(0, str(EVIDENCE_TOOLS))
if str(EVIDENCE_TOOLS.parent) not in sys.path:
    sys.path.insert(0, str(EVIDENCE_TOOLS.parent))
from cartographer import Cartographer  # noqa: E402
from identity import ROM_SHA, ROM_SIZE  # noqa: E402
from live_forward_rom_link import LiveForwardRomLinker; from live_forward_archivist import archive_session_graph  # noqa: E402
from live_forward_control_provenance_stage import run_control_provenance  # noqa: E402
from live_forward_scaling_audit import RECORD  # noqa: E402
from rom_knowledge_live_delta import import_archivist_graph  # noqa: E402
from rom_knowledge_map import KnowledgeStore  # noqa: E402
from map_driven_asm_stage7 import run_stage7, run_stage8  # noqa: E402
from live_forward_absorption_cleanup import (CleanupStop, reclaim_absorbed_run,
    reclaim_in_memory_run)  # noqa: E402
from pipeline_outcomes import (finish_unabsorbed_cleanup,
                               record_pipeline_failure)  # noqa: E402
from master_canonical_view import resolve_master_canonical_view  # noqa: E402
from canonical_map_fallback import materialize_canonical_map_fallback  # noqa: E402
from stage5_session_memory import Stage5SessionMemory, _lineage as _lineage_summary, _touch as _touch_fact  # noqa: E402
from stage5_disk_legacy import run_stage5_disk as _stage5_disk  # noqa: E402
from live_forward_audio_stage import run_audio_stage  # noqa: E402
from live_forward_vdp_stage import run_vdp_stage  # noqa: E402
from live_forward_sprite_stage import run_sprite_stage  # noqa: E402
from semantic_stage_scheduler import run_semantic_stages  # noqa: E402
STOP_MAP_FACT_MISSING, STOP_ROM_MISMATCH, STOP_IDENTITY_CONFLICT, STOP_COUNT_MISMATCH, STOP_OWNERSHIP_CHANGED = (
    "STOP_POSTRUN_MAP_FACT_MISSING", "STOP_POSTRUN_MAP_ROM_MISMATCH", "STOP_POSTRUN_MAP_IDENTITY_CONFLICT",
    "STOP_POSTRUN_MAP_COUNT_MISMATCH", "STOP_POSTRUN_MAP_OWNERSHIP_CHANGED")
IMPORTER_VERSION = "M12-2I.2b-stage5-v1"
def _sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()
def _utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z")
def _json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))
def _copy_db(source: Path, target: Path) -> None:
    target.parent.mkdir(parents=True, exist_ok=True)
    read = sqlite3.connect(source.resolve().as_uri() + "?mode=ro", uri=True)
    write = sqlite3.connect(target)
    try:
        read.backup(write)
        write.commit()
    finally:
        read.close()
        write.close()
def _rename_with_retry(source: Path, target: Path, attempts: int = 32) -> None:
    for attempt in range(attempts):
        try:
            source.rename(target)
            return
        except PermissionError:
            if attempt + 1 == attempts: raise
            time.sleep(min(0.1, 0.01 * (attempt + 1)))
def _compact_session(receipt: dict[str, Any], output: Path, rom_path: Path,
                     decoder: Path, progress: Any = None) -> tuple[Path, dict[str, Any]]:
    spool = receipt.get("raw_segment_spool", {})
    raw, index = Path(spool.get("raw_path", "")), Path(spool.get("index_path", ""))
    if not raw.is_file() or not index.is_file():
        raise ValueError(f"{STOP_MAP_FACT_MISSING}:ordered FLOW spool is missing")
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError(STOP_ROM_MISMATCH)
    if not decoder.is_file():
        raise ValueError(f"{STOP_MAP_FACT_MISSING}:exact-range decoder is missing")
    runtime = receipt.get("runtime", {})
    run_id = int(runtime.get("run_id", 0))
    identity = str(receipt.get("instrumentation_identity", ""))
    if run_id <= 0 or len(identity) != 64:
        raise ValueError(f"{STOP_MAP_FACT_MISSING}:runtime identity is incomplete")
    session_path = output / "session-rom-link.sqlite"
    if session_path.is_file():
        graph = Cartographer(session_path, ROM_SHA)
        meta = {row[0]: row[1] for row in graph.db.execute("SELECT key,value FROM map_meta")}
        graph.close()
        if meta.get("live_forward_session_state") == "CLOSED" and meta.get("live_forward_run_id") == str(run_id):
            return session_path, {"replayed_session": True, "accepted_segments": int(spool.get("segments", 0)),
                                  "accepted_records": int(raw.stat().st_size // RECORD.size),
                                  "unique_instructions": 0, "unique_edges": 0,
                                  "rejected_facts": 0, "identity_conflicts": 0, "rom_mismatches": 0}
        raise ValueError(f"{STOP_IDENTITY_CONFLICT}:existing session belongs to another run")
    instruction_facts: dict[tuple[int, int], dict[str, Any]] = {}
    next_facts: dict[tuple[tuple[int, int], tuple[int, int]], dict[str, Any]] = {}
    terminal_facts: dict[tuple[tuple[int, int], int], dict[str, Any]] = {}
    seen_segments: set[tuple[int, int, int, int]] = set()
    previous_entry = -1
    accepted_segments = accepted_records = 0
    source_run: int | None = None
    with index.open("r", encoding="utf-8") as index_stream, raw.open("rb") as raw_stream:
        for line in index_stream:
            item = json.loads(line)
            segment = item.get("segment", {})
            if segment.get("valid") is not True or segment.get("ready_for_cartographer") is not True:
                raise ValueError(f"{STOP_COUNT_MISMATCH}:segment readiness")
            pos_keys = ("run_id", "capture_id", "generation", "entry_stream_sequence", "exit_stream_sequence", "record_count")
            if any(not isinstance(segment.get(k), int) or segment[k] <= 0 for k in pos_keys) or \
                    any(not isinstance(segment.get(k), int) or segment[k] < 0 for k in ("epoch", "worker_id")):
                raise ValueError(f"{STOP_COUNT_MISMATCH}:segment identity")
            segment_id = tuple(int(segment[key]) for key in ("run_id", "epoch", "worker_id", "capture_id"))
            if segment_id + (int(segment["generation"]),) in seen_segments:
                raise ValueError(STOP_IDENTITY_CONFLICT)
            seen_segments.add(segment_id + (int(segment["generation"]),))
            if source_run is None:
                source_run = int(segment["run_id"])
            if source_run != int(segment["run_id"]) or source_run != run_id:
                raise ValueError(STOP_IDENTITY_CONFLICT)
            if int(segment["entry_stream_sequence"]) <= previous_entry:
                raise ValueError(f"{STOP_COUNT_MISMATCH}:segment ordering")
            previous_entry = int(segment["entry_stream_sequence"])
            raw_offset, raw_length = int(item["raw_offset"]), int(item["raw_length"])
            raw_stream.seek(raw_offset)
            data = raw_stream.read(raw_length)
            if len(data) != raw_length or hashlib.sha256(data).hexdigest() != item.get("raw_sha256"):
                raise ValueError(f"{STOP_COUNT_MISMATCH}:segment hash")
            rows = list(RECORD.iter_unpack(data))
            if len(rows) != int(segment["record_count"]) or raw_length != len(rows) * RECORD.size:
                raise ValueError(f"{STOP_COUNT_MISMATCH}:record count")
            if any(right[0] != left[0] + 1 for left, right in zip(rows, rows[1:])):
                raise ValueError(f"{STOP_COUNT_MISMATCH}:record ordering")
            instructions = [(i, row) for i, row in enumerate(rows) if row[5] & 1]
            for record_index, row in instructions:
                key = (int(row[2]), int(row[4]))
                _touch_fact(instruction_facts, key,
                            _lineage_summary(segment, record_index, row, raw_offset))
            for (left_index, left), (right_index, right) in zip(instructions, instructions[1:]):
                if right_index != left_index + 1:
                    continue
                source, target = (int(left[2]), int(left[4])), (int(right[2]), int(right[4]))
                _touch_fact(next_facts, (source, target),
                            _lineage_summary(segment, left_index, left, raw_offset))
            if instructions:
                record_index, row = instructions[-1]
                key = ((int(row[2]), int(row[4])), int(row[3]))
                _touch_fact(terminal_facts, key,
                            _lineage_summary(segment, record_index, row, raw_offset))
            accepted_segments += 1
            accepted_records += len(rows)
            if progress and accepted_segments % 256 == 0:
                progress.update(accepted_segments, detail=f"validated {accepted_segments:,} FLOW segments")
    decode_dir = output / "decoder-evidence"
    decode_dir.mkdir(parents=True, exist_ok=True)
    resolutions = LiveForwardRomLinker._decode_batch(decoder, rom_path,
        {pc for pc, _ in instruction_facts}, decode_dir / "rom-decode")
    scope = "rom:" + ROM_SHA
    nodes: dict[str, dict[str, Any]] = {}
    edges: dict[str, dict[str, Any]] = {}
    id_graph = Cartographer.in_memory(ROM_SHA)
    source_ids: dict[tuple[int, int], str] = {}
    try:
        for (pc, opcode), fact in instruction_facts.items():
            resolution = resolutions[pc]
            if resolution["memory_region"] != "ROM" or resolution["decode_status"] != "DECODED" or \
                    resolution["opcode"] != opcode or resolution["rom_offset"] is None:
                raise ValueError(STOP_ROM_MISMATCH)
            offset, length, data = int(resolution["rom_offset"]), int(resolution["length"]), resolution["bytes"]
            if length < 2 or len(data) != length or not 0 <= offset < offset + length <= len(rom) or \
                    rom[offset:offset + length] != data or int.from_bytes(data[:2], "big") != opcode:
                raise ValueError(STOP_ROM_MISMATCH)
            source = {"kind": "M68K_INSTRUCTION", "key": f"{pc:08X}:{opcode:04X}",
                      "scope": scope, "status": "OBSERVED",
                      "attributes": {"cpu": "M68K", "pc": pc, "opcode": opcode,
                                     "record_kind": "instruction"},
                      "lineage": [fact["lineage"]]}
            range_node = {"kind": "ROM_INSTRUCTION_RANGE",
                          "key": f"{offset:08X}:{offset + length:08X}:{data.hex().upper()}",
                          "scope": ROM_SHA, "status": "OBSERVED",
                          "attributes": {"rom_sha256": ROM_SHA, "start_offset": offset,
                                         "end_offset_exclusive": offset + length, "length": length,
                                         "opcode": opcode, "bytes_hex": data.hex().upper(),
                                         "bytes_sha256": hashlib.sha256(data).hexdigest()}, "lineage": []}
            source_id = id_graph._node_id(source)
            range_id = id_graph._node_id(range_node)
            source_ids[(pc, opcode)] = source_id
            nodes[source_id] = source
            nodes[range_id] = range_node
            edge = {"source": source_id, "target": range_id, "relation": "EXECUTED_FROM_ROM",
                    "scope": ROM_SHA, "status": "OBSERVED",
                    "rule": "FLOW_V1 opcode matched exact canonical ROM instruction bytes",
                    "assumptions": [], "lineage": [fact["lineage"]]}
            edges[id_graph._edge_id(edge)] = edge
        for (source, target), fact in next_facts.items():
            edge = {"source": source_ids[source], "target": source_ids[target],
                    "relation": "EXECUTED_NEXT", "scope": scope, "status": "OBSERVED",
                    "rule": "adjacent FLOW_V1 instruction records", "assumptions": [],
                    "lineage": [fact["lineage"]]}
            edges[id_graph._edge_id(edge)] = edge
        for (source, address), fact in terminal_facts.items():
            target = {"kind": "M68K_TARGET_ADDRESS", "key": f"{address:08X}", "scope": scope,
                      "status": "OBSERVED", "attributes": {"cpu": "M68K", "raw_next_pc": address,
                      "meaning": "observed terminal FLOW_V1 next_pc only"},
                      "lineage": [fact["lineage"]]}
            target_id = id_graph._node_id(target)
            nodes[target_id] = target
            edge = {"source": source_ids[source], "target": target_id,
                    "relation": "OBSERVED_NEXT_PC", "scope": scope, "status": "OBSERVED",
                    "rule": "terminal next_pc address fact", "assumptions": [],
                    "lineage": [fact["lineage"]]}
            edges[id_graph._edge_id(edge)] = edge
    finally:
        id_graph.close()
    graph = Cartographer.in_memory(ROM_SHA, source_owned_bytes=0)
    try:
        graph._put_meta("live_forward_session_schema", "oasis.m12.live-forward-session.v1")
        graph._put_meta("live_forward_session_id", f"postrun-{run_id}")
        graph._put_meta("live_forward_instrumentation_identity", identity)
        graph._put_meta("live_forward_created_utc", _utc_now())
        graph._put_meta("live_forward_session_state", "OPEN")
        graph._put_meta("live_forward_run_id", str(run_id))
        graph.db.commit()
        graph.merge({"nodes": list(nodes.values()), "edges": list(edges.values()),
                     "frontiers": [], "resolves_frontiers": []},
                    f"stage5:{run_id}:{spool.get('raw_sha256')}", str(spool.get("raw_sha256", "")),
                    compute_graph_hash=False, compute_components=False)
        graph.db.execute("INSERT OR REPLACE INTO map_meta VALUES (?,?)",
                         ("live_forward_session_state", "CLOSED"))
        graph.db.execute("INSERT OR REPLACE INTO map_meta VALUES (?,?)",
                         ("live_forward_closed_utc", _utc_now()))
        graph.db.execute("INSERT OR REPLACE INTO map_meta VALUES (?,?)",
                         ("live_forward_graph_sha256", graph.graph_hash()))
        graph.db.commit()
        temporary = session_path.with_name(session_path.name + ".tmp")
        output.mkdir(parents=True, exist_ok=True)
        target = sqlite3.connect(temporary)
        graph.db.backup(target)
        target.commit()
        target.close()
        os.replace(temporary, session_path)
    finally:
        graph.close()
    return session_path, {"replayed_session": False, "accepted_segments": accepted_segments,
        "accepted_records": accepted_records, "unique_instructions": len(instruction_facts),
        "unique_edges": len(next_facts), "terminal_facts": len(terminal_facts),
        "rejected_facts": 0, "identity_conflicts": 0, "rom_mismatches": 0}
def _ownership_snapshot(path: Path) -> dict[str, Any]:
    store = KnowledgeStore(path, ROM_SHA, ROM_SIZE)
    try:
        metrics = store.metrics()
        # Hash ordered canonical rows incrementally; KnowledgeStore.hashes materializes the full table.
        emission_digest = hashlib.sha256()
        emission_digest.update(b"[")
        first = True
        for row in store.db.execute(
                "SELECT start,end,emission_type,classification,source_kind,source_owned,artifact_type,artifact "
                "FROM emission ORDER BY start,end"):
            if not first:
                emission_digest.update(b",")
            first = False
            emission_digest.update(json.dumps(list(row), sort_keys=True,
                                               separators=(",", ":"), ensure_ascii=True).encode("utf-8"))
        emission_digest.update(b"]")
        hashes = {"emission_hash": emission_digest.hexdigest()}
        emission = {str(row[0]): int(row[1]) for row in store.db.execute(
            "SELECT emission_type,SUM(end-start) FROM emission GROUP BY emission_type")}
        rows = [(int(row[0]), int(row[1])) for row in store.db.execute(
            "SELECT start,end FROM emission ORDER BY start,end")]
        if not rows or rows[0][0] != 0 or rows[-1][1] != ROM_SIZE or any(
                right[0] != left[1] for left, right in zip(rows, rows[1:])):
            raise ValueError(f"{STOP_OWNERSHIP_CHANGED}:emission partition")
        return {"metrics": metrics, "hashes": hashes, "emission_bytes": emission,
                "source_owned_bytes": metrics["source_owned_bytes"]}
    finally:
        store.close()
def _validate_rolling_master(receipt_path: Path, receipt: dict[str, Any],
                             generation: Path) -> dict[str, Any]:
    report_path, master = generation / "report.json", generation / "master.sqlite"
    if not report_path.is_file() or not master.is_file():
        raise ValueError(f"{STOP_MAP_FACT_MISSING}:accepted rolling master is missing")
    report = _json(report_path)
    spool = receipt.get("raw_segment_spool") or receipt.get("flow_handoff", {})
    flow = receipt.get("flow_handoff", {})
    raw_sha = flow.get("raw_sha256") or spool.get("raw_sha256")
    idx_sha = flow.get("logical_sha256") or flow.get("index_sha256") or spool.get("index_sha256")
    expected = {"status": "PASS_END_GAME_ROLLING_MASTER_COMPACT_V1",
                "run_id": int(receipt.get("runtime", {}).get("run_id", 0)),
                "input_raw_sha256": raw_sha, "input_index_sha256": idx_sha,
                "rom_sha256": ROM_SHA, "master_sha256": _sha(master)}
    if any(report.get(key) != value for key, value in expected.items()):
        raise ValueError(f"{STOP_IDENTITY_CONFLICT}:rolling master proof mismatch")
    return {"generation_dir": str(generation), "report_sha256": _sha(report_path),
            "master_sha256": expected["master_sha256"], "report": report}
def _stage5(receipt_path: Path, receipt: dict[str, Any], output: Path, rom_path: Path,
            master_generation: Path, decoder: Path, progress: Any = None,
            memory: Stage5SessionMemory | None = None,
            flow_stats: dict[str, Any] | None = None) -> dict[str, Any]:
    rolling = _validate_rolling_master(receipt_path, receipt, master_generation)
    memory = memory or Stage5SessionMemory.from_receipt(receipt, rom_path, decoder, progress)
    try:
        source_sha = memory.source_sha256
        view = resolve_master_canonical_view(Path(__file__).parents[2])
        scratch_root = output / "canonical-master-v2"
        if (scratch_root / "current.json").is_file():
            base_pointer = _json(scratch_root / "current.json")
            base_generation = (scratch_root / base_pointer["generation_dir"]).resolve()
        else:
            base_generation = view.materialize(scratch_root, rom_path) if view else materialize_canonical_map_fallback(scratch_root, rom_path)
            base_pointer = _json(scratch_root / "current.json")
        base_master, base_knowledge = base_generation / "master.sqlite", base_generation / "knowledge.sqlite"
        base_snapshot = _ownership_snapshot(base_knowledge)
        generation_root, current = scratch_root, scratch_root / "current.json"
        stage5_gen = (generation_root / "generations" / ("gen-" + source_sha[:16])).resolve()
        if stage5_gen.is_dir() and (stage5_gen / "knowledge.sqlite").is_file():
            meta = _json(stage5_gen / "generation-metadata.json") if (stage5_gen / "generation-metadata.json").is_file() else {}
            if meta.get("session_source_sha256") == source_sha or base_pointer.get("last_session_source_sha256") == source_sha:
                storage = meta.get("stage5_storage", {"scratch_mode": "RAM", "sqlite_writes_bytes": 0, "temp_files": 0,
                                                      "flow_disk_read_bytes": 0, "flow_handoff_mode": "IN_MEMORY_STREAM" if flow_stats else "DISK_SPOOL",
                                                      "flow_disk_reads": int((flow_stats or {}).get("disk_reads", 0)), "flow_disk_writes": int((flow_stats or {}).get("disk_writes", 0)),
                                                      "flow_buffer_peak_bytes": int((flow_stats or {}).get("buffer_peak_bytes", 0))})
                return {"state": "NO_DELTA", "replay_status": "PASS_IDEMPOTENT_NOOP", "session_path": None,
                        "session_sha256": source_sha, "generation_dir": str(stage5_gen), "rolling_master": rolling,
                        "before": base_snapshot, "after": base_snapshot, "stage5_storage": storage}
        staging = generation_root / "generations" / (".staging-" + uuid.uuid4().hex)
        staging.mkdir(parents=True, exist_ok=False)
        try:
            merge_master, staged_knowledge = staging / "merge-master.sqlite", staging / "knowledge.sqlite"
            empty_master = Cartographer(merge_master, ROM_SHA)
            empty_master.close()
            _copy_db(base_knowledge, staged_knowledge)
            merged = archive_session_graph(merge_master, memory.graph, ROM_SHA, source_sha)
            rom = rom_path.read_bytes()
            imported = import_archivist_graph(memory.graph, merge_master, staged_knowledge,
                                               rom, ROM_SHA, merged["merge_receipt"], source_sha)
            replay = import_archivist_graph(memory.graph, merge_master, staged_knowledge,
                                             rom, ROM_SHA, merged["merge_receipt"], source_sha)
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
            generation = generation_root / "generations" / ("gen-" + source_sha[:16])
            if generation.exists():
                raise ValueError(f"{STOP_IDENTITY_CONFLICT}:generation already exists")
            metadata = {
                "schema": "oasis.m12.postrun-canonical-generation.v1", "generation_id": generation.name,
                "parent_generation_id": base_pointer.get("generation_dir"), "rom_sha256": ROM_SHA, "rom_size": ROM_SIZE,
                "source_run_id": int(receipt["runtime"]["run_id"]), "session_source_sha256": source_sha,
                "rolling_master": rolling, "importer_version": IMPORTER_VERSION,
                "session": {"replayed_session": False, "accepted_segments": memory.segments_processed,
                            "accepted_records": memory.records_processed, "unique_instructions": len(memory.instruction_occurrences),
                            "unique_edges": len(memory.executed_next), "terminal_facts": len(memory.terminal_facts), "source_sha256": source_sha},
                "stage5_storage": {"scratch_mode": "RAM", "sqlite_writes_bytes": 0, "temp_files": 0,
                                   "flow_disk_read_bytes": 0 if flow_stats else memory.flow_bytes_read,
                                   "flow_handoff_mode": "IN_MEMORY_STREAM" if flow_stats else "DISK_SPOOL",
                                   "flow_disk_reads": int((flow_stats or {}).get("disk_reads", 0)),
                                   "flow_disk_writes": int((flow_stats or {}).get("disk_writes", 0)),
                                   "ram_peak_bytes": memory.ram_peak_bytes, "flow_buffer_peak_bytes": int((flow_stats or {}).get("buffer_peak_bytes", 0))},
                "before": base_snapshot, "after": after, "import": imported, "replay": replay, "audit": "PASS"
            }
            (staging / "generation-metadata.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            _rename_with_retry(staging, generation)
            pointer = {"schema": "oasis.m12.postrun-canonical.current.v2",
                       "generation_dir": str(generation.relative_to(generation_root)),
                       "master_sha256": _sha(generation / "master.sqlite"),
                       "knowledge_sha256": _sha(generation / "knowledge.sqlite"),
                       "metadata_sha256": _sha(generation / "generation-metadata.json"),
                       "last_session_source_sha256": source_sha, "rom_sha256": ROM_SHA,
                       "status": "PASS_POSTRUN_CANONICAL_MAP_REFRESH_V1"}
            temporary = current.with_suffix(".tmp-" + uuid.uuid4().hex)
            temporary.write_text(json.dumps(pointer, indent=2, sort_keys=True) + "\n", encoding="utf-8")
            os.replace(temporary, current)
            return {"state": "PASS", "status": "PASS_POSTRUN_CANONICAL_MAP_REFRESH_V1",
                    "session_path": None, "session_sha256": source_sha, "generation_dir": str(generation),
                    "pointer": pointer, "rolling_master": rolling, "before": base_snapshot, "after": after,
                    "import": imported, "replay": replay, "source_owned_delta": 0,
                    "emission_unchanged": True, "stage5_storage": metadata["stage5_storage"]}
        except Exception:
            shutil.rmtree(staging, ignore_errors=True)
            raise
    finally:
        memory.close()
def run_remaining(receipt_path: Path, report: dict[str, Any], master_generation: Path,
                  analysis_dir: Path, rom_path: Path, progress: Any,
                  decoder: Path | None = None, flow_session: Stage5SessionMemory | None = None,
                  flow_stats: dict[str, Any] | None = None,
                  stage6_result: dict[str, Any] | None = None,
                  range_tool: Path | None = None) -> dict[str, Any]:
    result = {"stages": {}, "cleanup": {"status": "RAW_RETAINED", "deleted": []},
              "pipeline_state": "ANALYSIS STOPPED ✗"}
    try:
        receipt = _json(receipt_path)
        source = receipt.get("flow_handoff") or receipt.get("raw_segment_spool", {})
        total_segments = int(source.get("segments", 0)) or None
        progress.start("REFRESHING MAP", total=total_segments, unit="FLOW segments",
                       detail="building a closed MAP-1 session from ordered FLOW")
        def_dec, def_rng = Path(__file__).parents[2] / "build" / "oasis_re_rom_range_decode.exe", Path(__file__).parents[2] / "build" / "oasis_re_assemble_range.exe"
        decoder = decoder if (decoder and decoder.is_file()) else (def_dec if def_dec.is_file() else None)
        range_tool = range_tool if (range_tool and Path(range_tool).is_file()) else (def_rng if def_rng.is_file() else None)
        if not decoder or not range_tool: raise ValueError(f"{STOP_MAP_FACT_MISSING}:exact-range decoder or range tool is missing")
        run_key = f"run-{receipt['runtime']['run_id']}-{_sha(receipt_path)[:16]}"
        run_analysis_dir = analysis_dir / run_key
        stage5 = _stage5(receipt_path, receipt, run_analysis_dir, rom_path,
                         master_generation, decoder, progress, flow_session, flow_stats)
        progress.update(total_segments or 1, detail="canonical generation accepted")
        progress.finish(detail="canonical generation accepted")
        result["stages"]["REFRESHING MAP"] = stage5
    except Exception as error:
        reason = str(error)
        progress.finish("STOP", detail=reason)
        result["stop"] = reason
        result["stages"]["REFRESHING MAP"] = {"state": "STOP", "stop": reason}
        return result
    progress.start("CONTROL PROVENANCE", total=total_segments, unit="FLOW segments",
                   detail="checking actual indirect JMP/JSR consumers")
    if stage6_result is not None:
        stage6 = dict(stage6_result)
        stage6["input_generation"] = stage6["output_generation_or_same"] = stage6["canonical_generation"] = stage5.get("generation_dir")
        stage6["raw_flow_sha256"] = (flow_stats or {}).get("raw_sha256")
    else:
        stage6 = run_control_provenance(receipt, rom_path, stage5.get("generation_dir"), total_segments, progress, run_analysis_dir / "stage6-control-provenance.json")
    stage6["state"] = stage6["status"]
    result["stages"]["CONTROL PROVENANCE"] = stage6
    if stage6["status"] in {"PASS", "NO_DELTA"}:
        progress.finish(stage6["status"], detail="no new executed indirect consumers" if stage6["status"] == "NO_DELTA" else "proven consumers accepted")
    else:
        reason = stage6.get("stop_reason") or stage6.get("error") or "Stage 6 failed"
        progress.finish("ERROR" if stage6["status"] == "ERROR" else "STOP", detail=reason); result["stop"] = reason; return result
    progress.start("AUDIO ANALYSIS", total=total_segments, unit="FLOW segments",
                   detail="analyzing Z80 audio runtime provenance and resource roundtrip")
    stage_audio = run_audio_stage(receipt, rom_path, run_analysis_dir, total_segments, progress)
    stage_audio["state"] = stage_audio["status"]
    result["stages"]["AUDIO ANALYSIS"] = stage_audio
    if stage_audio["status"] in {"PASS", "NO_DELTA"}:
        progress.finish(stage_audio["status"], detail="audio resources and provenance verified")
    else:
        reason = stage_audio.get("stop_reason") or stage_audio.get("error") or "Audio analysis failed"
        progress.finish("STOP", detail=reason); result["stop"] = reason; return result
    progress.start("VDP / DMA ANALYSIS", total=total_segments, unit="FLOW segments",
                   detail="analyzing VDP control protocol and DMA provenance")
    stage_vdp = run_vdp_stage(receipt, rom_path, run_analysis_dir, total_segments, progress)
    stage_vdp["state"] = stage_vdp["status"]
    result["stages"]["VDP / DMA ANALYSIS"] = stage_vdp
    if stage_vdp["status"] in {"PASS", "NO_DELTA"}:
        progress.finish(stage_vdp["status"], detail="VDP and DMA provenance verified")
    else:
        reason = stage_vdp.get("stop_reason") or stage_vdp.get("error") or "VDP analysis failed"
        progress.finish("STOP", detail=reason); result["stop"] = reason; return result
    progress.start("SPRITE / SAT ANALYSIS", total=total_segments, unit="FLOW segments",
                   detail="analyzing hardware sprites and SAT state")
    stage_sprite = run_sprite_stage(receipt, rom_path, run_analysis_dir, total_segments, progress)
    stage_sprite["state"] = stage_sprite["status"]
    result["stages"]["SPRITE / SAT ANALYSIS"] = stage_sprite
    if stage_sprite["status"] in {"PASS", "NO_DELTA"}:
        progress.finish(stage_sprite["status"], detail="hardware sprites and SAT state verified")
    else:
        reason = stage_sprite.get("stop_reason") or stage_sprite.get("error") or "Sprite/SAT analysis failed"
        progress.finish("STOP", detail=reason); result["stop"] = reason; return result
    semantic_stop = run_semantic_stages(receipt, rom_path, run_analysis_dir,
                                        total_segments, progress, stage_sprite, result)
    if semantic_stop: result["stop"] = semantic_stop; return result
    try:
        progress.start("ASM CLOSURE", total=None, unit="candidate islands", detail="VALIDATING EMISSION PARTITION")
        canonical_generation = Path(stage5["generation_dir"])
        if not (canonical_generation / "knowledge.sqlite").is_file(): raise ValueError(f"{STOP_MAP_FACT_MISSING}:MASTER V2 canonical scratch unavailable")
        assembler = Path(r"C:\Github\Sega-Thor\build\m11-9\vasm\vasmm68k_mot.exe")
        if not assembler.is_file(): raise ValueError("STOP_STAGE7_ASSEMBLER_MISSING:vasmm68k_mot.exe is missing")
        stage7 = run_stage7(canonical_generation, rom_path, Path(range_tool), assembler, analysis_dir / run_key / "stage7", progress)
        progress.finish("PASS" if stage7.get("state") == "PASS" else "NO_DELTA", detail=f"{stage7.get('promoted_bytes', 0)} bytes promoted")
        result["stages"]["ASM CLOSURE"] = stage7
        progress.start("FULL ROM AUDIT", total=ROM_SIZE, unit="canonical ROM bytes", detail="independent Stage 8 rebuild of the Stage 7 materialized split")
        stage8 = run_stage8(Path(stage7["materialized"]), rom_path, assembler, progress)
        progress.update(int(stage8.get("bytes", 0)), total=ROM_SIZE, detail="full ROM audit exact")
        progress.finish("PASS", detail=stage8.get("status", "full ROM audit exact"))
        result["stages"]["FULL ROM AUDIT"] = stage8
        progress.start("CLEANUP", total=None, unit="files", detail="proving absorption before permanent deletion")
        reclaimer = reclaim_in_memory_run if flow_stats else reclaim_absorbed_run
        cleanup = reclaimer(receipt_path, report, result["stages"], master_generation.parents[1], run_analysis_dir, rom_path, flow_stats if flow_stats else progress)
        progress.finish("PASS", detail=f"ABSORBED RUN; deleted {cleanup['files_deleted']} files")
        result["cleanup"] = cleanup; result["stages"]["CLEANUP"] = cleanup
        result["pipeline_state"] = ("ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"
                                    if result.get("semantic_unresolved") else "ANALYSIS COMPLETE ✓")
        return result
    except CleanupStop as error:
        if finish_unabsorbed_cleanup(result, progress, error):
            return result
        raise
    except Exception as error:
        record_pipeline_failure(result, progress, error)
        return result
