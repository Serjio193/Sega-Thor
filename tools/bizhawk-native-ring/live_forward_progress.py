"""Atomic post-run progress snapshots shared by the backend and UI."""

from __future__ import annotations

from datetime import datetime, timezone
import json
import os
from pathlib import Path
import tempfile
import time
from typing import Any


STAGES = (
    "FINALIZING RUN", "AUDITING FLOW", "MERGING MASTER", "COMPACTING EVIDENCE",
    "REFRESHING MAP", "CONTROL PROVENANCE", "AUDIO ANALYSIS", "VDP / DMA ANALYSIS",
    "SPRITE / SAT ANALYSIS", "GAMEPLAY RAM / ENTITY CANDIDATES", "CONTROLLED ENTITY PROVENANCE",
    "GENERIC RECURSIVE CLOSURE", "ASM CLOSURE",
    "FULL ROM AUDIT", "CLEANUP",
)
ACCEPTED_TERMINAL = {"PASS", "NO_DELTA", "SKIPPED_NOT_APPLICABLE"}
NONFATAL_TERMINAL = {"UNRESOLVED"}
PIPELINE_STATES = {"RUNNING", "UNRESPONSIVE", "PARTIAL_COMPLETE", "COMPLETE",
                   "COMPLETE_WITH_UNRESOLVED", "STOPPED", "FAILED"}
TERMINAL_PIPELINES = {"PARTIAL_COMPLETE", "COMPLETE", "COMPLETE_WITH_UNRESOLVED",
                      "STOPPED", "FAILED"}
TERMINAL_STAGE_STATES = ACCEPTED_TERMINAL | NONFATAL_TERMINAL | {"STOP", "ERROR", "BLOCKED"}


def _replace_with_retry(source: str, target: str, attempts: int = 32) -> None:
    """Keep atomic publication resilient to a short Windows reader lock."""
    for attempt in range(attempts):
        try:
            os.replace(source, target)
            return
        except PermissionError:
            if attempt + 1 == attempts:
                raise
            time.sleep(min(0.1, 0.01 * (attempt + 1)))


def _row(state: str = "PENDING", **values: Any) -> dict[str, Any]:
    return {"state": state, "processed_units": 0, "total_units": None,
            "unit_name": None, "processed_bytes": None, "detail": "",
            "duration_seconds": None, **values}


def _pipeline_from_display(display: str) -> str:
    if display == "ANALYSIS COMPLETE ✓":
        return "COMPLETE"
    if display == "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE":
        return "COMPLETE_WITH_UNRESOLVED"
    if display == "PARTIAL ANALYSIS COMPLETE ⚠":
        return "PARTIAL_COMPLETE"
    if display == "ANALYSIS STOPPED ✗":
        return "STOPPED"
    if display == "ANALYSIS FAILED ✗":
        return "FAILED"
    if display == "ANALYSIS UNRESPONSIVE":
        return "UNRESPONSIVE"
    return "RUNNING"


def _display_from_pipeline(pipeline: str) -> str:
    return {"PARTIAL_COMPLETE": "PARTIAL ANALYSIS COMPLETE ⚠",
            "COMPLETE": "ANALYSIS COMPLETE ✓", "STOPPED": "ANALYSIS STOPPED ✗",
            "COMPLETE_WITH_UNRESOLVED": "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE",
            "FAILED": "ANALYSIS FAILED ✗", "UNRESPONSIVE": "ANALYSIS UNRESPONSIVE",
            "RUNNING": "ANALYSIS RUNNING…"}[pipeline]


class ProgressPublisher:
    """Publishes durable status without making the UI depend on backend memory."""

    def __init__(self, path: Path) -> None:
        self.path = path
        self.pid = os.getpid()
        self.started = time.monotonic()
        self.stage_started = self.started
        self.last_progress_change = self.started
        self.current: str | None = None
        self.partial = False
        self.rows = {stage: _row() for stage in STAGES}
        self.last_write = 0.0
        self.backend_started_at = datetime.now(timezone.utc).isoformat()
        self.backend_exit_code: int | None = None
        self.backend_terminal_reason: str | None = None
        self.exception_type: str | None = None
        self.exception_message: str | None = None
        self.pipeline_state = "RUNNING"
        self.write(force=True)

    def _snapshot(self, overall: str = "ANALYSIS RUNNING…", error: str | None = None,
                  warning: str | None = None) -> dict[str, Any]:
        now = time.monotonic()
        row = self.rows.get(self.current or "", _row())
        index = STAGES.index(self.current) + 1 if self.current in STAGES else 0
        pipeline_state = self.pipeline_state
        active_stage = self.current if row["state"] == "ACTIVE" else None
        if pipeline_state in TERMINAL_PIPELINES and any(
                item["state"] == "ACTIVE" for item in self.rows.values()):
            raise ValueError("STOP_STATUS_ACTIVE_TERMINAL_CONFLICT")
        return {
            "timestamp_utc": datetime.now(timezone.utc).isoformat(),
            "timestamp_monotonic": now,
            "stage": self.current,
            "active_stage": active_stage,
            "stage_state": row["state"],
            "processed_units": row["processed_units"],
            "total_units": row["total_units"],
            "unit_name": row["unit_name"],
            "processed_bytes": row["processed_bytes"],
            "detail": row["detail"],
            "elapsed_seconds": round(now - self.stage_started, 3),
            "total_elapsed_seconds": round(now - self.started, 3),
            "last_progress_change_time": self.last_progress_change,
            "backend_pid": self.pid,
            "backend_started_at": self.backend_started_at,
            "backend_exit_code": self.backend_exit_code,
            "backend_terminal_reason": self.backend_terminal_reason,
            "exception_type": self.exception_type,
            "exception_message": self.exception_message,
            "last_heartbeat": datetime.now(timezone.utc).isoformat(),
            "error": error,
            "warning": warning,
            "overall_state": overall,
            "pipeline_state": pipeline_state,
            "stage_index": index,
            "stage_count": len(STAGES),
            "stages": self.rows,
        }

    def write(self, force: bool = False, overall: str = "ANALYSIS RUNNING…",
              error: str | None = None, warning: str | None = None) -> None:
        if self.pipeline_state in TERMINAL_PIPELINES and overall == "ANALYSIS RUNNING…":
            overall = _display_from_pipeline(self.pipeline_state)
        requested_pipeline = _pipeline_from_display(overall)
        self.pipeline_state = requested_pipeline
        now = time.monotonic()
        if not force and now - self.last_write < 0.25:
            return
        self.path.parent.mkdir(parents=True, exist_ok=True)
        handle, temporary = tempfile.mkstemp(prefix=self.path.name + ".tmp-",
                                              dir=self.path.parent)
        try:
            with os.fdopen(handle, "w", encoding="utf-8", newline="\n") as stream:
                json.dump(self._snapshot(overall, error, warning), stream,
                          indent=2, sort_keys=True)
                stream.write("\n")
                stream.flush()
                os.fsync(stream.fileno())
            _replace_with_retry(temporary, str(self.path))
            self.last_write = now
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)

    def start(self, stage: str, *, total: int | None = None,
              unit: str | None = None, detail: str = "") -> None:
        if stage not in self.rows:
            raise ValueError(f"unknown post-run stage: {stage}")
        self.current = stage
        self.stage_started = time.monotonic()
        self.rows[stage].update(state="ACTIVE", processed_units=0, total_units=total,
                                unit_name=unit, detail=detail)
        self.write(force=True)

    def update(self, processed: int, *, total: int | None = None,
               processed_bytes: int | None = None, detail: str = "") -> None:
        if self.current is None:
            return
        row = self.rows[self.current]
        changed = processed != row["processed_units"]
        row.update(processed_units=max(0, int(processed)),
                   total_units=total if total is not None else row["total_units"],
                   processed_bytes=processed_bytes if processed_bytes is not None else row["processed_bytes"],
                   detail=detail)
        if changed:
            self.last_progress_change = time.monotonic()
        self.write()

    def heartbeat(self, detail: str | None = None) -> None:
        """Publish liveness without pretending that a work counter advanced."""
        if self.current is not None and detail is not None:
            self.rows[self.current]["detail"] = detail
        self.write(force=True)

    def finish(self, state: str = "PASS", *, detail: str = "",
               warning: str | None = None) -> None:
        if self.current is None:
            return
        if state not in ACCEPTED_TERMINAL | NONFATAL_TERMINAL | {"WARNING", "STOP", "ERROR"}:
            raise ValueError(f"invalid post-run stage state: {state}")
        row = self.rows[self.current]
        row.update(state=state, detail=detail,
                   duration_seconds=round(time.monotonic() - self.stage_started, 3))
        if state == "STOP":
            self.finalize("STOPPED", detail or warning or "stage stopped")
            return
        if state == "ERROR":
            self.finalize("FAILED", detail or warning or "stage failed")
            return
        self.write(force=True, warning=warning)

    def skip_remaining(self, reason: str) -> None:
        self.partial = True
        self.finalize("PARTIAL_COMPLETE", reason)

    def complete(self, *, report: dict[str, Any] | None = None) -> None:
        del report
        if any(row["state"] == "STOP" for row in self.rows.values()):
            self.finalize("STOPPED", "a mandatory post-run stage stopped fail-closed")
            return
        if any(row["state"] == "ERROR" for row in self.rows.values()):
            self.finalize("FAILED", "a post-run stage failed")
            return
        if not self.partial and all(row["state"] in ACCEPTED_TERMINAL | NONFATAL_TERMINAL
                                    for row in self.rows.values()):
            target = ("COMPLETE_WITH_UNRESOLVED" if any(
                row["state"] == "UNRESOLVED" for row in self.rows.values()) else "COMPLETE")
            self.finalize(target, "pipeline completed with unresolved semantic evidence"
                          if target == "COMPLETE_WITH_UNRESOLVED" else
                          "all post-run stages reached terminal state")
        else:
            self.finalize("PARTIAL_COMPLETE", "mandatory stages were not executed")

    def fail(self, error: str, *, stage: str | None = None) -> None:
        self.finalize("FAILED", error, stage=stage, exception=error)

    def finalize(self, pipeline_state: str, reason: str = "", *,
                 stage: str | None = None, exit_code: int | None = None,
                 exception: str | None = None) -> None:
        """Atomically close a run and remove every impossible ACTIVE state."""
        if pipeline_state not in PIPELINE_STATES | {"COMPLETE_WITH_UNRESOLVED"} or pipeline_state == "RUNNING":
            raise ValueError(f"invalid terminal pipeline state: {pipeline_state}")
        if stage in self.rows:
            self.current = stage
        active = self.current if self.current in self.rows else next(
            (name for name, row in self.rows.items() if row["state"] == "ACTIVE"), None)
        if pipeline_state in {"COMPLETE", "COMPLETE_WITH_UNRESOLVED"}:
            for row in self.rows.values():
                if row["state"] == "ACTIVE":
                    row.update(state="PASS", duration_seconds=0.0)
        elif pipeline_state == "PARTIAL_COMPLETE":
            for row in self.rows.values():
                if row["state"] == "ACTIVE":
                    row.update(state="SKIPPED_NOT_APPLICABLE", duration_seconds=0.0)
                elif row["state"] == "PENDING":
                    row.update(state="SKIPPED_NOT_APPLICABLE", detail=reason,
                               duration_seconds=0.0)
        else:
            terminal_stage = "STOP" if pipeline_state == "STOPPED" else "ERROR"
            if active:
                self.rows[active].update(state=terminal_stage, detail=reason)
            for name, row in self.rows.items():
                if row["state"] == "PENDING":
                    row.update(state="BLOCKED", detail=f"blocked by {active or 'pipeline'}",
                               duration_seconds=0.0)
        self.backend_exit_code = exit_code
        self.backend_terminal_reason = reason
        if exception:
            self.exception_type = exception.split(":", 1)[0]
            self.exception_message = exception
        self.current = active or self.current
        display = {"PARTIAL_COMPLETE": "PARTIAL ANALYSIS COMPLETE ⚠",
                   "COMPLETE": "ANALYSIS COMPLETE ✓", "STOPPED": "ANALYSIS STOPPED ✗",
                   "COMPLETE_WITH_UNRESOLVED": "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE",
                   "FAILED": "ANALYSIS FAILED ✗"}[pipeline_state]
        self.pipeline_state = pipeline_state
        self.write(force=True, overall=display, error=exception,
                   warning=reason if pipeline_state == "PARTIAL_COMPLETE" else None)


def stage_percent(row: dict[str, Any]) -> int | None:
    """Return a factual percentage, or None for an indeterminate stage."""
    total = row.get("total_units")
    processed = row.get("processed_units")
    if not isinstance(total, int) or total <= 0 or not isinstance(processed, int):
        return None
    return min(100, max(0, (processed * 100) // total))


def heartbeat_state(snapshot: dict[str, Any], now: float | None = None) -> str:
    """Classify snapshot freshness without treating a quiet stage as a crash."""
    stamp = snapshot.get("timestamp_monotonic")
    current = time.monotonic() if now is None else now
    age = current - float(stamp) if isinstance(stamp, (int, float)) else float("inf")
    if age < 2:
        return "LIVE"
    if age <= 10:
        return "BUSY / WAITING"
    return "NO UPDATE"


def overall_from_stages(rows: dict[str, dict[str, Any]], pipeline_state: str | None = None) -> str:
    if pipeline_state == "COMPLETE":
        return "ANALYSIS COMPLETE ✓"
    if pipeline_state == "COMPLETE_WITH_UNRESOLVED":
        return "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"
    if pipeline_state == "PARTIAL_COMPLETE":
        return "PARTIAL ANALYSIS COMPLETE ⚠"
    if pipeline_state == "STOPPED":
        return "ANALYSIS STOPPED ✗"
    if pipeline_state == "FAILED":
        return "ANALYSIS FAILED ✗"
    states = {row.get("state") for row in rows.values()}
    if "ERROR" in states:
        return "ANALYSIS FAILED ✗"
    if "STOP" in states:
        return "ANALYSIS STOPPED ✗"
    if states & {"PENDING", "ACTIVE", "UNKNOWN"}:
        return "ANALYSIS RUNNING…"
    if "SKIPPED_NOT_APPLICABLE" in states:
        return "PARTIAL ANALYSIS COMPLETE ⚠"
    if "UNRESOLVED" in states and all(state in ACCEPTED_TERMINAL | {"UNRESOLVED"}
                                       for state in states):
        return "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE"
    if all(state in ACCEPTED_TERMINAL for state in states):
        return "ANALYSIS COMPLETE ✓"
    return "PARTIAL ANALYSIS COMPLETE ⚠"


def mark_failed(path: Path, error: str, exit_code: int | None = 1) -> None:
    """Finalize a crashed backend without leaving a dead ACTIVE stage."""
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(value, dict):
            value = {}
    except (OSError, json.JSONDecodeError):
        value = {}
    stage = value.get("active_stage") or value.get("stage")
    stages = value.get("stages")
    if not isinstance(stages, dict):
        stages = {stage_name: _row() for stage_name in STAGES}
    publisher = ProgressPublisher(path)
    publisher.pid = int(value.get("backend_pid", publisher.pid)) if isinstance(
        value.get("backend_pid"), int) else publisher.pid
    publisher.rows = stages
    publisher.current = stage if isinstance(stage, str) and stage in publisher.rows else None
    publisher.backend_started_at = str(value.get("backend_started_at", publisher.backend_started_at))
    publisher.finalize("FAILED", error, stage=publisher.current,
                       exit_code=exit_code, exception=error)


def repair_terminal_snapshot(path: Path) -> dict[str, Any]:
    """Repair an old terminal snapshot that still contains an ACTIVE stage."""
    value = json.loads(path.read_text(encoding="utf-8"))
    pipeline = value.get("pipeline_state")
    if pipeline not in TERMINAL_PIPELINES:
        return value
    stages = value.get("stages", {})
    if not any(isinstance(row, dict) and row.get("state") == "ACTIVE"
               for row in stages.values()):
        return value
    publisher = ProgressPublisher(path)
    publisher.rows = stages
    publisher.current = value.get("active_stage") or value.get("stage")
    publisher.pid = int(value.get("backend_pid", publisher.pid)) if isinstance(
        value.get("backend_pid"), int) else publisher.pid
    publisher.backend_started_at = str(value.get("backend_started_at", publisher.backend_started_at))
    target = pipeline if pipeline in {"COMPLETE", "PARTIAL_COMPLETE"} else (
        "STOPPED" if pipeline == "STOPPED" else "FAILED")
    publisher.finalize(target, "repaired terminal snapshot with ACTIVE stage",
                       stage=publisher.current)
    return json.loads(path.read_text(encoding="utf-8"))
