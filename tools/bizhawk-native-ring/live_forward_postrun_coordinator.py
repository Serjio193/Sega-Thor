"""In-process Stage 1-9 coordinator with structured progress events."""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import threading
import time
from typing import Any, Callable

from live_forward_progress import STAGES
from live_forward_rolling_master import compact
from master_startup_authority import MasterStartupState


def _atomic_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + f".tmp-{os.getpid()}-{time.time_ns()}")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    for attempt in range(32):
        try:
            os.replace(temporary, path)
            return
        except PermissionError:
            if attempt == 31:
                temporary.unlink(missing_ok=True)
                raise
            time.sleep(min(0.1, 0.01 * (attempt + 1)))


@dataclass(frozen=True)
class PostRunContext:
    run_id: int
    rom_path: Path
    master_root: Path
    receipt_path: Path
    decoder: Path | None = None
    startup: MasterStartupState | None = None
    segment_total: int = 0
    configuration: dict[str, Any] = None
    range_tool: Path | None = None
    bootstrap_knowledge: Path | None = None
    flow_session: Any | None = None
    flow_stats: dict[str, Any] | None = None
    flow_stage6: dict[str, Any] | None = None


@dataclass(frozen=True)
class PostRunResult:
    overall_status: str
    active_stage: str | None
    stages: dict[str, Any]
    report: dict[str, Any]
    events: tuple[dict[str, Any], ...]
    error: str | None = None
    new_instructions: int = 0
    new_relations: int = 0
    terminal_facts: int = 0
    source_owned_before: int | None = None
    source_owned_after: int | None = None
    master_generation: str | None = None
    canonical_generation: str | None = None
    cleanup: dict[str, Any] | None = None
    stop_code: str | None = None

    @property
    def complete(self) -> bool:
        return self.overall_status in {"ANALYSIS COMPLETE ✓",
                                       "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"}


def semantic_projection(report: dict[str, Any]) -> dict[str, Any]:
    """Return the stable result fields used by subprocess shadow comparison."""
    stages = report.get("stage_results", report.get("stages", {}))
    return {"stage_states": {name: row.get("state") for name, row in stages.items()},
            "new_instructions": report.get("new_instructions", 0),
            "new_relations": report.get("new_edges", report.get("new_relations", 0)),
            "terminal_facts": report.get("terminal_facts", 0),
            "source_owned_delta": report.get("source_owned_delta", 0),
            "master_generation": report.get("generation_id"),
            "canonical_generation": (stages.get("REFRESHING MAP", {}) or {}).get("generation_dir"),
            "cleanup": (stages.get("CLEANUP") or report.get("cleanup")),
            "stop": report.get("stop")}


def semantic_equal(left: dict[str, Any], right: dict[str, Any]) -> bool:
    """Compare coordinator results without volatile timestamps or hashes."""
    return semantic_projection(left) == semantic_projection(right)


def attach_analysis_performance(report: dict[str, Any], rows: dict[str, dict[str, Any]],
                                flow_stats: dict[str, Any] | None,
                                ram_peak_bytes: int = 0) -> None:
    """Attach deterministic post-run timing and R7 handoff invariants."""
    durations = {name: float(row.get("duration_seconds", 0.0) or 0.0)
                 for name, row in rows.items()}
    analysis_wall = sum(durations.values())
    flow = report.get("flow_handoff", {}) or {}
    stats = flow_stats or flow
    records = int(stats.get("records", flow.get("records", 0)) or 0)
    segments = int(stats.get("segments", flow.get("segments", 0)) or 0)
    report["analysis_performance"] = {
        "analysis_wall_seconds": round(analysis_wall, 3),
        "stage_wall_seconds": {name: round(value, 3) for name, value in durations.items()},
        "records_processed": records,
        "segments_processed": segments,
        "records_per_second": round(records / analysis_wall, 3) if analysis_wall else 0.0,
        "segments_per_second": round(segments / analysis_wall, 3) if analysis_wall else 0.0,
        "flow_chunks": int(stats.get("chunks_transferred", flow.get("chunks_transferred", 0)) or 0),
        "queue_peak_chunks": int(stats.get("buffer_peak_chunks", flow.get("buffer_peak_chunks", 0)) or 0),
        "queue_peak_bytes": int(stats.get("buffer_peak_bytes", flow.get("buffer_peak_bytes", 0)) or 0),
        "ram_peak_bytes": int(ram_peak_bytes or flow.get("ram_peak_bytes", 0) or 0),
        "r7_invariants": {
            "flow_handoff_mode": flow.get("mode", flow.get("flow_handoff_mode")),
            "flow_disk_reads": int(flow.get("disk_reads", flow.get("flow_disk_reads", 0)) or 0),
            "flow_disk_writes": int(flow.get("disk_writes", flow.get("flow_disk_writes", 0)) or 0),
            "legacy_reads": int(flow.get("legacy_reads", 0) or 0),
            "fallback": flow.get("fallback", "DISABLED"),
            "source_owned_delta": int(report.get("source_owned_delta", 0) or 0),
        },
    }


class ProgressEventSink:
    """Progress adapter for stage code; snapshots are optional diagnostics."""

    def __init__(self, run_id: int, status_path: Path | None = None):
        self.run_id = int(run_id)
        self.status_path = status_path
        self.rows: dict[str, dict[str, Any]] = {
            stage: {"state": "PENDING", "processed_units": 0,
                    "total_units": None, "unit_name": None, "detail": ""}
            for stage in STAGES}
        self.events: list[dict[str, Any]] = []
        self.current: str | None = None
        self.stage_started_at = 0.0
        self.overall_state = "ANALYSIS RUNNING…"
        self._lock = threading.RLock()

    def _event(self, kind: str, stage: str | None, status: str,
               message: str = "", stop_code: str | None = None) -> None:
        with self._lock:
            row = self.rows.get(stage or "", {})
            event = {"event": kind, "run_id": self.run_id,
                     "stage_number": STAGES.index(stage) + 1 if stage in STAGES else None,
                     "stage": stage, "status": status,
                     "processed": row.get("processed_units", 0),
                     "total": row.get("total_units"),
                     "elapsed": max(0.0, time.monotonic() - self.stage_started_at)
                     if stage and self.stage_started_at else 0.0,
                     "heartbeat": time.time(), "message": message,
                     "stop_code": stop_code}
            self.events.append(event)
            self._snapshot()

    def _snapshot(self) -> None:
        if self.status_path is None:
            return
        _atomic_json(self.status_path, {"schema": "oasis.m12.postrun.in-process.v1",
            "run_id": self.run_id, "overall_state": self.overall_state,
            "pipeline_state": ("RUNNING" if self.current else
                               "FAILED" if self.overall_state == "ANALYSIS FAILED ✗" else
                               "STOPPED" if self.overall_state == "ANALYSIS STOPPED ✗" else
                               "COMPLETE_WITH_UNRESOLVED" if self.overall_state ==
                               "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE" else
                               "COMPLETE" if self.overall_state == "ANALYSIS COMPLETE ✓" else "RUNNING"),
            "stage": self.current, "stage_index": STAGES.index(self.current) + 1
            if self.current in STAGES else len(STAGES), "stage_count": len(STAGES),
            "active_stage": self.current, "backend_pid": os.getpid(),
            "stages": self.rows, "events": len(self.events),
            "last_heartbeat": time.time()})

    def start(self, stage: str, total: int | None = None, unit: str | None = None,
              detail: str = "") -> None:
        self.current, self.stage_started_at = stage, time.monotonic()
        row = self.rows[stage]; row.update(state="ACTIVE", processed_units=0,
                   total_units=total, unit_name=unit, detail=detail)
        self._event("stage_started", stage, "ACTIVE", detail)

    def update(self, processed: int, total: int | None = None,
               processed_bytes: int | None = None, detail: str = "") -> None:
        if self.current is None:
            return
        row = self.rows[self.current]; row["processed_units"] = int(processed)
        if total is not None:
            row["total_units"] = int(total)
        if processed_bytes is not None:
            row["processed_bytes"] = int(processed_bytes)
        if detail:
            row["detail"] = detail
        self._event("stage_progress", self.current, "ACTIVE", detail)

    def heartbeat(self, message: str = "") -> None:
        self._event("stage_progress", self.current, "ACTIVE", message)

    def finish(self, status: str = "PASS", detail: str = "") -> None:
        stage = self.current
        if stage is None:
            return
        row = self.rows[stage]; row["state"] = status
        row["duration_seconds"] = max(0.0, time.monotonic() - self.stage_started_at)
        if detail:
            row["detail"] = detail
        self._event("stage_completed", stage, status, detail,
                    detail if status in {"STOP", "ERROR"} else None)
        self.current = None

    def complete(self, report: dict[str, Any] | None = None) -> None:
        self.current = None
        if isinstance(report, dict) and report.get("pipeline_state") not in {
                "ANALYSIS COMPLETE ✓", "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"}:
            self.overall_state = "ANALYSIS STOPPED ✗"
            self._event("pipeline_stopped", None, "STOP", str(report.get("stop", "")))
            return
        unresolved = any(row.get("state") == "UNRESOLVED" for row in self.rows.values())
        self.overall_state = ("ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE" if unresolved
                              else "ANALYSIS COMPLETE ✓")
        self._event("pipeline_complete", None,
                    "UNRESOLVED" if unresolved else "PASS", "semantic evidence remains unresolved")

    def fail(self, error: str) -> None:
        stage = self.current
        if stage is not None:
            self.rows[stage]["state"] = "ERROR"
            self.rows[stage]["detail"] = error
            self._event("pipeline_failed", stage, "ERROR", error, error)
            index = STAGES.index(stage)
            for later in STAGES[index + 1:]:
                self.rows[later]["state"] = "BLOCKED"
        self.current = None
        self.overall_state = "ANALYSIS FAILED ✗"
        self._event("pipeline_failed", None, "ERROR", error, error)


class PostRunCoordinator:
    """Own one background-capable post-run execution and its event stream."""

    def __init__(self) -> None:
        self._lock = threading.RLock()
        self._result: PostRunResult | None = None

    @property
    def result(self) -> PostRunResult | None:
        with self._lock:
            return self._result

    def run(self, context: PostRunContext, progress_sink: ProgressEventSink | None = None,
            diagnostic_report: Path | None = None) -> PostRunResult:
        receipt = json.loads(Path(context.receipt_path).read_text(encoding="utf-8"))
        runtime = receipt.get("runtime", {})
        spool = receipt.get("raw_segment_spool") or receipt.get("flow_handoff", {})
        flow = receipt.get("flow_handoff", {})
        if int(runtime.get("run_id", -1)) != int(context.run_id):
            raise ValueError("STOP_POSTRUN_CONTEXT_RUN_ID")
        if int((flow or spool).get("segments", -1)) != int(context.segment_total):
            raise ValueError("STOP_POSTRUN_CONTEXT_SEGMENT_TOTAL")
        if context.startup is not None and context.startup.master_sha256 == "":
            raise ValueError("STOP_MASTER_STARTUP_UNAVAILABLE")
        sink = progress_sink or ProgressEventSink(context.run_id)
        try:
            report = compact(context.receipt_path, context.master_root, context.rom_path,
                             context.bootstrap_knowledge, None, None, context.decoder,
                             progress=sink, flow_session=context.flow_session,
                             flow_stats=context.flow_stats, stage6_result=context.flow_stage6,
                             range_tool=context.range_tool)
            attach_analysis_performance(report, sink.rows, context.flow_stats,
                                        int(runtime.get("peak_working_set_bytes", 0) or 0))
            overall = (report.get("pipeline_state") if report.get("pipeline_state") in {
                "ANALYSIS COMPLETE ✓", "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"}
                else "ANALYSIS STOPPED ✗")
            result = PostRunResult(overall, sink.current,
                                   dict(report.get("stage_results", report.get("stages", {}))),
                                   report, tuple(sink.events), report.get("stop"),
                                   int(report.get("new_instructions", 0)),
                                   int(report.get("new_edges", report.get("new_relations", 0))),
                                   int(report.get("terminal_facts", 0)),
                                   report.get("source_owned_before"),
                                   report.get("source_owned_after"),
                                   report.get("generation_id"),
                                   (report.get("stage_results", {}).get("REFRESHING MAP", {}) or {}).get("generation_dir"),
                                   report.get("cleanup"), report.get("stop"))
            if diagnostic_report:
                _atomic_json(diagnostic_report, report)
        except Exception as error:
            sink.fail(str(error))
            result = PostRunResult("ANALYSIS FAILED ✗", None, dict(sink.rows),
                                   {"status": "ERROR", "error": str(error)},
                                   tuple(sink.events), str(error), stop_code=str(error))
            if diagnostic_report:
                _atomic_json(diagnostic_report, result.report)
        with self._lock:
            self._result = result
        return result

    def run_background(self, context: PostRunContext, progress_sink: ProgressEventSink | None = None,
                       diagnostic_report: Path | None = None) -> threading.Thread:
        thread = threading.Thread(target=self.run, args=(context, progress_sink, diagnostic_report),
                                  name=f"postrun-{context.run_id}", daemon=False)
        thread.start()
        return thread


__all__ = ["PostRunContext", "PostRunCoordinator", "PostRunResult", "ProgressEventSink",
           "semantic_equal", "semantic_projection"]
