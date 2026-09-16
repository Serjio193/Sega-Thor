"""Bounded diagnostic equality trace at AUTO67 Worker input boundaries."""

from __future__ import annotations

import hashlib
import json
import math
import struct
import threading
from typing import Any


TRACE_STAGES = ("DISPATCH_INPUT", "WORKER_RECEIVED", "MATERIALIZER_INPUT")
TRACE_FIELDS = (
    "worker_id", "investigation_id", "occurrence_id", "event_kind", "event_pc",
    "event_address", "event_epoch", "event_seq", "snapshot_identity",
    "snapshot_epoch", "first_sequence", "latest_sequence", "count",
    "frozen_records_sha256", "normalized_event_sha256",
)
RECORD_LAYOUT = struct.Struct("<QIHH")


def _normalized(value: Any) -> Any:
    if value is None or isinstance(value, (bool, int, str)):
        return value
    if isinstance(value, float):
        return value if math.isfinite(value) else {"$float": repr(value)}
    if isinstance(value, bytes):
        return {"$bytes_hex": value.hex()}
    if isinstance(value, dict):
        return {str(key): _normalized(value[key])
                for key in sorted(value, key=lambda item: str(item))}
    if isinstance(value, (list, tuple)):
        return [_normalized(item) for item in value]
    if isinstance(value, (set, frozenset)):
        items = [_normalized(item) for item in value]
        return sorted(items, key=lambda item: json.dumps(
            item, sort_keys=True, ensure_ascii=True, separators=(",", ":")))
    return {"$type": type(value).__qualname__, "$repr": repr(value)}


def normalized_event_sha256(event: dict[str, Any]) -> str:
    payload = json.dumps(_normalized(event), sort_keys=True, ensure_ascii=True,
                         separators=(",", ":"), allow_nan=False).encode("utf-8")
    return hashlib.sha256(payload).hexdigest()


def _integer(value: Any) -> int:
    return int(value, 0) if isinstance(value, str) else int(value)


def _record_sha256(snapshot_pool: Any, event: dict[str, Any]) -> str | None:
    identity = event.get("snapshot_identity")
    if not identity or snapshot_pool is None:
        return None
    try:
        occurrence = (_integer(event.get("epoch", 1)), _integer(event.get("seq")))
        snapshot = snapshot_pool.get(str(identity), occurrence)
        payload = b"".join(RECORD_LAYOUT.pack(
            record.sequence, record.pc, record.opcode, record.reserved)
            for record in snapshot.native_records)
    except (AttributeError, KeyError, TypeError, ValueError, struct.error):
        return None
    return hashlib.sha256(payload).hexdigest()


def _point_equal(left: dict[str, Any], right: dict[str, Any]) -> dict[str, bool]:
    return {field: left.get(field) == right.get(field) for field in TRACE_FIELDS}


class WorkerInputTrace:
    """Keep a bounded set of native BUS_WRITE observations through three boundaries."""

    def __init__(self, snapshot_pool: Any, *, preferred_pc: int = 0x26C,
                 fallback_limit: int = 24, preferred_limit: int = 24):
        if fallback_limit < 1 or preferred_limit < 1:
            raise ValueError("Worker input trace limits must be positive")
        self.snapshot_pool = snapshot_pool
        self.preferred_pc = preferred_pc
        self.fallback_limit = fallback_limit
        self.preferred_limit = preferred_limit
        self._lock = threading.Lock()
        self._traces: dict[tuple[str, str], dict[str, Any]] = {}
        self._fallback_count = 0
        self._preferred_count = 0
        self._trace_errors: list[dict[str, str]] = []

    def record(self, stage: str, worker_id: int, task: dict[str, Any]) -> None:
        if stage not in TRACE_STAGES:
            return
        event = task.get("event")
        if not isinstance(event, dict) or event.get("kind") != "BUS_WRITE_PC":
            return
        if not event.get("native_snapshot_mode") or not event.get("snapshot_identity"):
            return
        investigation_id = str(task.get("investigation_id", ""))
        occurrence_id = str(event.get("occurrence_id", ""))
        key = (investigation_id, occurrence_id)
        with self._lock:
            trace = self._traces.get(key)
            if stage == "DISPATCH_INPUT" and trace is None:
                try:
                    preferred = _integer(event.get("pc")) == self.preferred_pc
                except (TypeError, ValueError):
                    preferred = False
                if preferred and self._preferred_count >= self.preferred_limit:
                    return
                if not preferred and self._fallback_count >= self.fallback_limit:
                    return
                trace = {"worker_id": worker_id,
                         "investigation_id": investigation_id,
                         "occurrence_id": occurrence_id,
                         "preferred_pc_match": preferred,
                         "points": {}}
                self._traces[key] = trace
                if preferred:
                    self._preferred_count += 1
                else:
                    self._fallback_count += 1
            if trace is None:
                return
            try:
                point = {
                    "worker_id": worker_id,
                    "investigation_id": investigation_id,
                    "occurrence_id": occurrence_id,
                    "event_kind": event.get("kind"),
                    "event_pc": event.get("pc"),
                    "event_address": event.get("address"),
                    "event_epoch": event.get("epoch"),
                    "event_seq": event.get("seq"),
                    "snapshot_identity": event.get("snapshot_identity"),
                    "snapshot_epoch": event.get("snapshot_epoch"),
                    "first_sequence": event.get("first_sequence"),
                    "latest_sequence": event.get("latest_sequence"),
                    "count": event.get("count"),
                    "frozen_records_sha256": _record_sha256(self.snapshot_pool, event),
                    "normalized_event_sha256": normalized_event_sha256(event),
                }
            except (TypeError, ValueError, OverflowError, struct.error) as error:
                self._trace_errors.append({"stage": stage,
                                           "occurrence_id": occurrence_id,
                                           "error": f"{type(error).__name__}: {error}"})
                return
            trace["points"][stage] = point

    def snapshot(self) -> dict[str, Any]:
        with self._lock:
            traces = [dict(item, points=dict(item["points"]))
                      for item in self._traces.values()]
            errors = list(self._trace_errors)
        complete = [item for item in traces
                    if all(stage in item["points"] for stage in TRACE_STAGES)]
        preferred = next((item for item in complete if item["preferred_pc_match"]), None)
        selected = preferred or (complete[0] if complete else None)
        if selected is None:
            return {"classification": "INCOMPLETE_TRACE",
                    "candidate_count": len(traces),
                    "complete_candidate_count": len(complete),
                    "trace_errors": errors,
                    "selected_trace": None}
        points = selected["points"]
        dispatch_received = _point_equal(points["DISPATCH_INPUT"],
                                         points["WORKER_RECEIVED"])
        received_materializer = _point_equal(points["WORKER_RECEIVED"],
                                             points["MATERIALIZER_INPUT"])
        snapshot_fields = {"occurrence_id", "snapshot_identity", "snapshot_epoch",
                           "first_sequence", "latest_sequence", "count",
                           "frozen_records_sha256"}
        snapshot_changed = any(not dispatch_received[field] or
                               not received_materializer[field]
                               for field in snapshot_fields)
        if snapshot_changed:
            classification = "STOP_WORKER_SNAPSHOT_IDENTITY_MUTATED"
        elif not all(dispatch_received.values()):
            classification = "STOP_WORKER_MAILBOX_INPUT_MUTATED"
        elif not all(received_materializer.values()):
            classification = "STOP_WORKER_MATERIALIZER_INPUT_MUTATED"
        else:
            classification = "PASS_WORKER_INPUT_PRESERVED"
        return {
            "classification": classification,
            "candidate_count": len(traces),
            "complete_candidate_count": len(complete),
            "selection": ("preferred event PC 0x00026C" if preferred
                          else "first complete BUS_WRITE_PC trace"),
            "selected_trace": {
                "worker_id": selected["worker_id"],
                "investigation_id": selected["investigation_id"],
                "occurrence_id": selected["occurrence_id"],
                "preferred_pc_match": selected["preferred_pc_match"],
                "points": points,
                "comparisons": {
                    "DISPATCH_INPUT_equals_WORKER_RECEIVED": dispatch_received,
                    "WORKER_RECEIVED_equals_MATERIALIZER_INPUT": received_materializer,
                    "all_fields_equal": (all(dispatch_received.values()) and
                                         all(received_materializer.values())),
                },
            },
            "trace_errors": errors,
        }
