"""AUTO67 BizHawk launcher and bounded snapshot lifecycle."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import threading
import time
from collections import deque
from pathlib import Path
from typing import Any

import auto67_live as auto67_live_module
from auto67_capsule import CapsulePool
from auto67_dashboard import DASHBOARD_HTML
from auto67_live import BASELINE, ROM_SHA, Dispatcher
from auto67_materializer import register_provenance_targets
from auto67_predecessor import register_writer_candidate_report
from auto67_persistence import LiveMapSink
from map_merge import merge_session_map
from auto67_status import StatusPublisher
from auto67_transport import PreDispatchTransport


def run_live(args: argparse.Namespace) -> dict[str, Any]:
    rom = Path(args.rom).resolve()
    emulator = Path(args.emulator).resolve()
    lua = Path(args.lua).resolve()
    output = Path(args.output).resolve()
    rom_bytes = rom.read_bytes()
    if hashlib.sha256(rom_bytes).hexdigest() != ROM_SHA:
        raise SystemExit("canonical ROM identity mismatch")
    output.parent.mkdir(parents=True, exist_ok=True)
    target_path = output.with_suffix(".register-targets.txt")
    targets = register_provenance_targets(rom_bytes)
    target_path.write_text("".join(f"{pc:06X}|{mask}\n"
                                   for pc, mask in sorted(targets.items())),
                           encoding="ascii")
    writer_target_path = output.with_suffix(".register-writer-targets.txt")
    writer_report = register_writer_candidate_report(rom_bytes)
    writer_candidates = writer_report["candidates"]
    writer_limit = max(0, int(args.writer_hook_limit))
    writer_items = sorted(writer_candidates.items())
    if writer_limit:
        writer_items = writer_items[:writer_limit]
    writer_target_path.write_text("".join(f"{pc:06X}|{mask}\n"
                                          for pc, mask in writer_items),
                                  encoding="ascii")
    status_path = output.with_suffix(".status.json")
    final_path = output.with_suffix(".lua.json")
    stop_path = output.with_suffix(".stop")
    command_path = output.with_suffix(".capsule.commands")
    native_experiment = bool(getattr(args, "native_snapshot_experiment", False))
    if native_experiment and (not args.capsule_mode or args.max_live_captures < 1
                              or lua.name.lower() != "live_capsule.lua"):
        raise SystemExit("native snapshot experiment requires live capsule capture")
    for path in (status_path, final_path, stop_path, command_path):
        if path.exists():
            path.unlink()
    native_snapshot_pool = None
    if native_experiment:
        from auto67_snapshot_admission import SnapshotPool
        native_snapshot_pool = SnapshotPool()
    capsule_pool = None
    capsule_dir = output.parent / (output.stem + ".capsules")
    if args.capsule_mode or args.prehistory_mode in {"continuous", "targeted_idle", "targeted_burst"}:
        capsule_dir.mkdir(parents=True, exist_ok=True)
    if args.capsule_mode:
        capsule_pool = CapsulePool(
            count=args.capsule_count, max_live=args.max_live_captures,
            command_path=command_path,
            native_snapshot_pool=native_snapshot_pool)
    worker_input_trace = None
    if native_experiment:
        from auto67_worker_input_trace import WorkerInputTrace
        worker_input_trace = WorkerInputTrace(native_snapshot_pool)
    map_db = getattr(args, "map_db", None)
    session_map = getattr(args, "session_map_out", None)
    if map_db and session_map is None:
        session_map = output.with_name(output.stem + ".session-map.sqlite")
    map_sink = LiveMapSink(map_db, source_sha256=ROM_SHA, session_map=session_map) if map_db else None
    if map_sink is not None:
        map_sink.start()
    dispatcher = Dispatcher(args.workers, args.window, args.worker_delay, capsule_pool,
                            map_sink, rom_bytes, worker_input_trace)
    dispatcher.start()
    native_admission = None
    worker_chain_receipts: deque[dict[str, Any]] = deque(maxlen=64)
    receipt_lock = threading.Lock()
    original_local_chain = auto67_live_module.local_chain
    original_materialize = auto67_live_module.materialize
    worker_materialization_durations: dict[str, int] = {}
    if native_experiment:
        from auto67_native_snapshot import NATIVE_RECORD, NativeSnapshotAdmission
        native_admission = NativeSnapshotAdmission(dispatcher, native_snapshot_pool,
                                                   rom_bytes)

        def time_worker_materialize(seed: dict[str, Any], *values: Any,
                                    **options: Any) -> dict[str, Any]:
            started_ns = time.perf_counter_ns()
            result = original_materialize(seed, *values, **options)
            if seed.get("native_snapshot_mode"):
                with receipt_lock:
                    worker_materialization_durations[
                        str(seed.get("occurrence_id"))] = time.perf_counter_ns() - started_ns
            return result

        auto67_live_module.materialize = time_worker_materialize

        def observe_worker_chain(event: dict[str, Any], materialized: dict[str, Any] | None,
                                 investigation_id: str, lease_id: str) -> dict[str, Any]:
            chain = original_local_chain(event, materialized, investigation_id, lease_id)
            if event.get("native_snapshot_mode") and materialized is not None:
                materialized_provenance = materialized.get("register_provenance", {})
                output_type = ("REGISTER_REACHING_DEFINITION" if chain["chain_steps"]
                               else "FAIL_CLOSED:" + str(materialized_provenance.get(
                                   "reason", "NO_REGISTER_PROVENANCE"))
                               if materialized_provenance.get("status") != "NOT_REQUIRED"
                               else "RESOLVER_NOT_REQUIRED")
                worker_id = next((item["worker_id"] for item in dispatcher.worker_info
                                  if item.get("occurrence_id") == event.get("occurrence_id")), None)
                producer_records = []
                occurrence = (int(event.get("epoch", 1)), int(event["seq"]))
                snapshot_id = native_admission.by_occurrence.get(occurrence)
                snapshot_receipt = None
                if snapshot_id is not None:
                    snapshot = native_snapshot_pool.get(snapshot_id, occurrence)
                    digest = hashlib.sha256(b"".join(
                        NATIVE_RECORD.pack(record.sequence, record.pc,
                                           record.opcode, record.reserved)
                        for record in snapshot.native_records)).hexdigest()
                    snapshot_receipt = {
                        "epoch": snapshot.snapshot_epoch,
                        "first_sequence": snapshot.first_sequence,
                        "latest_sequence": snapshot.latest_sequence,
                        "count": snapshot.count,
                        "consumer_sequence": snapshot.consumer_sequence,
                        "consumer_pc": f"0x{snapshot.consumer_pc:06X}",
                        "last_record_pc": f"0x{snapshot.native_records[-1].pc:06X}",
                        "last_record_opcode": f"0x{snapshot.native_records[-1].opcode:04X}",
                        "records_sha256_at_worker_start": digest,
                        "frozen_snapshot_read_identical_at_worker_start":
                            digest == event.get("native_records_sha256"),
                        "live_ring_latest_seen": native_admission.latest_native_sequence,
                        "live_ring_advanced_after_freeze":
                            native_admission.latest_native_sequence > snapshot.latest_sequence,
                        "read_duration_ns": snapshot.read_duration_ns,
                        "copy_duration_ns": snapshot.copy_duration_ns,
                        "compression_duration_ns": snapshot.compression_duration_ns,
                        "freeze_duration_ns": snapshot.freeze_duration_ns,
                    }
                    for step in chain["chain_steps"]:
                        producer = step["producer_occurrence"]
                        sequence = int(producer["sequence"])
                        pc = int(producer["pc"], 16)
                        record = next((item for item in snapshot.native_records
                                       if item.sequence == sequence and item.pc == pc), None)
                        producer_records.append({
                            "register": step["register"],
                            "sequence": sequence,
                            "pc": producer["pc"],
                            "opcode": f"0x{record.opcode:04X}" if record else None,
                            "present_in_frozen_snapshot": record is not None,
                            "intervening_register_write": step["evidence"][
                                "intervening_register_write"],
                        })
                receipt = {"occurrence_id": event.get("occurrence_id"),
                           "snapshot_identity": event.get("snapshot_identity"),
                           "consumer_pc": event.get("pc"),
                           "event_kind": event.get("kind"),
                           "event_address": event.get("address"),
                           "worker_id": worker_id,
                           "lease_id": lease_id,
                           "investigation_id": investigation_id,
                           "output_evidence_type": output_type,
                           "occurrence_identity": occurrence,
                           "resolver_executed": (bool(materialized_provenance.get("requested"))
                                                  and materialized_provenance.get("capture")
                                                  is not None),
                           "resolver_status": materialized_provenance.get("status"),
                           "resolver_reason": materialized_provenance.get("reason"),
                           "resolver_requested_registers": materialized_provenance.get(
                               "requested", []),
                           "resolver_capture": materialized_provenance.get("capture"),
                           "capture_diagnostics": materialized.get("capture_diagnostics"),
                           "worker_materialize_duration_ns":
                               worker_materialization_durations.pop(
                                   str(event.get("occurrence_id")), None),
                           "native_snapshot": snapshot_receipt,
                           "producer_records_in_frozen_snapshot": producer_records,
                           "worker_chain": chain}
                with receipt_lock:
                    worker_chain_receipts.append(receipt)
            return chain

        auto67_live_module.local_chain = observe_worker_chain
    environment = os.environ.copy()
    environment.update({
        "OASIS_LIVE_STATUS": str(status_path),
        "OASIS_LIVE_FINAL": str(final_path),
        "OASIS_LIVE_STOP": str(stop_path),
        "OASIS_LIVE_MAX_FRAMES": str(args.max_frames),
        "OASIS_LIVE_DEMO_INPUTS": args.demo_inputs,
        "OASIS_LIVE_WINDOW": str(args.window),
        "OASIS_LIVE_CAPTURE_DISABLED": "1" if args.capture_disabled else "0",
        "OASIS_LIVE_CAPTURE_MODE": args.capture_mode,
        "OASIS_CAPSULE_COMMANDS": str(command_path),
        "OASIS_CAPSULE_DIR": str(capsule_dir),
        "OASIS_AUTO67_HOOK_METRICS": str(lua.parent / "auto67_hook_metrics.lua"),
        "OASIS_AUTO67_REGISTER_TARGETS": str(target_path),
        "OASIS_AUTO67_REGISTER_WRITER_TARGETS": str(writer_target_path),
        "OASIS_AUTO67_PREHISTORY_RING": "4096",
        "OASIS_AUTO67_PREHISTORY_MODE": args.prehistory_mode,
        "OASIS_AUTO67_BURST_BUDGET": str(args.burst_budget),
        "OASIS_AUTO67_WRITER_HOOK_LIMIT": str(writer_limit),
        "OASIS_AUTO67_WRITER_CANDIDATE_COUNT": str(writer_report["unique_count"]),
    })
    if native_experiment:
        environment["OASIS_AUTO67_NATIVE_SNAPSHOT"] = "1"
        environment["OASIS_AUTO67_PREHISTORY_MODE"] = "disabled"
    command = [str(emulator)]
    emulator_config = getattr(args, "emulator_config", None)
    if emulator_config:
        command.append(f"--config={Path(emulator_config).resolve()}")
    command.extend((f"--lua={lua}", str(rom)))
    started = time.monotonic()
    transport = PreDispatchTransport()
    launcher_log = output.with_suffix(".launcher.log")
    view_path = output.with_suffix(".view.json")
    view_mode = "none" if args.no_view else args.view_mode
    publisher = StatusPublisher(
        dispatcher, view_path, DASHBOARD_HTML,
        args.view_port if view_mode == "browser" else None,
        window_enabled=view_mode == "window")
    view_url = publisher.url
    global_before = _global_file_state(Path(map_db)) if map_db else _global_file_state(None)

    with launcher_log.open("w", encoding="utf-8") as log:
        process = subprocess.Popen(command, cwd=emulator.parent, env=environment,
                                    stdout=log, stderr=subprocess.STDOUT, text=True)
        while process.poll() is None:
            lua_status = None
            if status_path.exists():
                try:
                    lua_status = json.loads(status_path.read_text(encoding="utf-8"))
                except (json.JSONDecodeError, OSError):
                    lua_status = None
            if lua_status:
                dispatcher.metrics["capture_poll_count"] += 1
                if dispatcher.capsule_pool is not None:
                    dispatcher.capsule_pool.sync(lua_status.get("capsules", []))
                for event in transport.consume(lua_status):
                    if native_admission is None:
                        dispatcher.ingest(event)
                    else:
                        native_admission.ingest(event)
            if lua_status:
                publisher.publish(lua_status)
            time.sleep(args.poll_interval)
        process.wait(timeout=10)
    global_after_runtime = _global_file_state(Path(map_db)) if map_db else _global_file_state(None)
    if final_path.exists():
        lua_final = json.loads(final_path.read_text(encoding="utf-8"))
    else:
        lua_final = dict(publisher.latest, capture_complete=False)
    if capsule_pool is not None:
        capsule_pool.sync(lua_final.get("capsules", []))
    for event in transport.consume(lua_final):
        if native_admission is None:
            dispatcher.ingest(event)
        else:
            native_admission.ingest(event)
    if native_admission is not None:
        native_admission.reconcile()
    dispatcher.stop()
    native_snapshot_result = native_admission.snapshot() if native_admission else None
    if native_experiment:
        for receipt in worker_chain_receipts:
            occurrence = receipt.get("occurrence_identity")
            receipt["snapshot_slot_released_after_worker_completion"] = (
                isinstance(occurrence, tuple) and
                occurrence not in native_admission.by_occurrence)
        auto67_live_module.local_chain = original_local_chain
        auto67_live_module.materialize = original_materialize
    publisher.publish(lua_final)
    if map_sink is not None:
        map_sink.stop()
        try:
            merge_result = merge_session_map(Path(map_db), Path(session_map))
        except Exception as error:
            merge_result = {"status": "ERROR", "error": f"{type(error).__name__}: {error}"}
        map_sink.set_global_merge_result(merge_result)
    else:
        merge_result = {"status": "DISABLED"}
    if capsule_pool is not None:
        capsule_pool.stop()
    publisher.stop()
    elapsed = time.monotonic() - started
    result = {"schema": "oasis.m12.auto67.live-session.v1", "baseline": BASELINE,
              "rom_sha256": ROM_SHA, "command": command, "returncode": process.returncode,
              "session_seconds": elapsed, "workers_configured": args.workers,
              "dispatcher": dispatcher.snapshot(), "lua": lua_final,
              "raw_event_backlog": 0, "raw_event_backlog_structure": "NONEXISTENT",
              "predispatch_transport": transport.snapshot(),
              "launcher_log": str(launcher_log), "status_path": str(status_path),
              "prehistory_config": {"ring_capacity": 4096,
                                     "mode": args.prehistory_mode,
                                     "burst_budget": args.burst_budget,
                                     "target_path": str(target_path),
                                     "target_pc_count": len(targets),
                                     "writer_target_path": str(writer_target_path),
                                     "writer_candidate_count": writer_report["unique_count"],
                                     "writer_a4_count": writer_report["a4_count"],
                                     "writer_a5_count": writer_report["a5_count"],
                                     "writer_installed_count": len(writer_items),
                                     "writer_hook_limit": writer_limit,
                                     "writer_distribution": writer_report["distribution"],
                                     "writer_unsupported": writer_report["unsupported"],
                                     "writer_duplicate_pcs": writer_report["duplicate_pcs"]},
              "capture_path": str(final_path), "capture_disabled": args.capture_disabled,
              "view_url": view_url, "view_snapshot": str(view_path),
              "view_mode": view_mode,
              "map_sink": map_sink.snapshot() if map_sink else {
                   "available": False},
              "map_db": str(map_db) if map_db else None,
              "session_map": str(session_map) if session_map else None,
              "global_map": {"path": str(map_db) if map_db else None,
                              "before_runtime": global_before,
                              "after_runtime": global_after_runtime,
                              "merge": merge_result},
              "capsule_mode": args.capsule_mode,
              "native_snapshot_experiment": ({
                  "enabled": True,
                  "prehistory_mode": "disabled",
                  "savestate_used": False,
                  "snapshot_admission": native_snapshot_result,
                  "worker_input_trace": worker_input_trace.snapshot(),
                  "worker_chain_receipts": list(worker_chain_receipts),
                  "worker_id_by_occurrence": {
                      item["occurrence_id"]: item["worker_id"]
                      for item in dispatcher.snapshot()["investigations"]
                      if item.get("occurrence_id") is not None},
              } if native_experiment else None),
              "capsule_config": {"count": args.capsule_count,
                                  "capacity": 128 * 1024,
                                  "max_live": args.max_live_captures},
              "visualization_proof": {"two_workers_working":
                                       dispatcher.metrics["peak_workers_working"] >= 2,
                                       "worker_returned_idle": dispatcher.metrics["worker_returns"] > 0,
                                       "rolling_window_active":
                                       dispatcher.metrics["events_observed"] > 0}}
    sink_snapshot = result["map_sink"]
    result.update({"session_graph_hash": sink_snapshot.get("session_graph_hash", ""),
                   "global_graph_hash_before": merge_result.get("global_graph_hash_before"),
                   "global_graph_hash_after": merge_result.get("global_graph_hash_after"),
                   "session_delta": sink_snapshot.get("last_map_delta", {}),
                   "global_merge_delta": merge_result.get("global_merge_delta", {}),
                   "session_save_status": sink_snapshot.get("session_save_status", "DISABLED"),
                   "global_merge_status": merge_result.get("status", "DISABLED")})
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return result


def _global_file_state(path: Path | None) -> dict[str, Any]:
    if path is None or not path.exists():
        return {"exists": False, "sha256": None, "mtime_ns": None}
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return {"exists": True, "sha256": digest.hexdigest(),
            "mtime_ns": path.stat().st_mtime_ns}


def main() -> int:
    parser = argparse.ArgumentParser(description="AUTO67 live opportunistic RE launcher")
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--emulator-config", type=Path, default=None,
                        help="optional isolated BizHawk configuration")
    parser.add_argument("--lua", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--workers", type=int, choices=(1, 2, 4, 8, 16, 32, 64), default=16)
    parser.add_argument("--window", type=int, default=256)
    parser.add_argument("--worker-delay", type=float, default=0.003)
    parser.add_argument("--poll-interval", type=float, default=0.05)
    parser.add_argument("--max-frames", type=int, default=0,
                        help="bounded validation limit; zero means play until emulator closes")
    parser.add_argument("--demo-inputs", default="",
                        help="optional validation-only frame:buttons list, not a scenario")
    parser.add_argument("--capture-disabled", action="store_true")
    parser.add_argument("--capture-mode", choices=("continuous", "burst"), default="continuous",
                        help="burst is lossy discovery only; neither mode proves complete chains")
    parser.add_argument("--prehistory-mode", choices=("disabled", "targeted_idle",
                                                       "targeted_burst", "continuous"),
                        default="continuous", help="AUTO67.6 prehistory source mode")
    parser.add_argument("--burst-budget", type=int, choices=(32, 64, 128), default=64)
    parser.add_argument("--writer-hook-limit", type=int, default=0,
                        help="bounded generic writer-hook prefix for scale tests; zero means all")
    parser.add_argument("--capsule-mode", action="store_true",
                        help="AUTO67.1 fixed 16-capsule path; no raw event FIFO")
    parser.add_argument("--native-snapshot-experiment", action="store_true",
                        help="developer-only native-ring freeze-to-Worker proof")
    parser.add_argument("--capsule-count", type=int, default=16)
    parser.add_argument("--max-live-captures", type=int, choices=range(0, 17), default=1)
    parser.add_argument("--view-port", type=int, default=0)
    parser.add_argument("--view-mode", choices=("window", "browser", "none"), default="window",
                        help="native operator window by default; browser is explicit fallback")
    parser.add_argument("--no-view", action="store_true")
    parser.add_argument("--map-db", type=Path, default=None,
                        help="canonical GLOBAL MAP destination; merged only after runtime")
    parser.add_argument("--session-map-out", type=Path, default=None,
                        help="optional durable session-map artifact path")
    args = parser.parse_args()
    result = run_live(args)
    print(json.dumps({"returncode": result["returncode"],
                      "metrics": result["dispatcher"]["metrics"],
                      "lua": result["lua"]}, sort_keys=True))
    return int(result["returncode"] or 0)


if __name__ == "__main__":
    raise SystemExit(main())
