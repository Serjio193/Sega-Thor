"""Bounded frozen-capsule pool for AUTO67.1 developer-only experiments."""

from __future__ import annotations

import threading
import time
from collections import deque
from dataclasses import dataclass, field
from pathlib import Path

from auto67_capsule_codec import CapsuleFormatError, DecodedCapsule, decode_capsule


CAPSULE_SIZE = 128 * 1024
CAPSULE_COUNT = 16
MAX_LIVE_CAPTURES = 4
RECORD_SIZE = 16


def _number(value: object, default: int = 0) -> int:
    try:
        return int(str(value), 0)
    except (TypeError, ValueError):
        return default


@dataclass
class Capsule:
    capsule_id: int
    state: str = "FREE"
    worker_id: int | None = None
    investigation_id: str | None = None
    lease_id: str | None = None
    filter_type: str | None = None
    filter_value: int = 0
    start_frame: int = 0
    last_frame: int = 0
    bytes_used: int = 0
    event_count: int = 0
    capture_duration: float = 0.0
    frames_covered: int = 0
    freeze_reason: str | None = None
    full: bool = False
    truncated: bool = False
    format_version: int = 2
    logical_header_bytes: int = 64
    physical_header_bytes: int = 24
    record_size: int = 20
    capsule_path: str | None = None
    known: bool = False
    merged: bool = False
    proven: bool = False
    bounded_unresolved: bool = False
    buffer: bytearray = field(default_factory=lambda: bytearray(CAPSULE_SIZE),
                              repr=False)

    def claim(self, worker_id: int, investigation_id: str, lease_id: str,
              event: dict) -> None:
        self.state = "CLAIMED"
        self.worker_id = worker_id
        self.investigation_id = investigation_id
        self.lease_id = lease_id
        address = event.get("address")
        self.filter_type = "address" if address not in (None, "", "null") else "pc"
        self.filter_value = _number(address if self.filter_type == "address"
                                    else event.get("pc"))
        self.start_frame = _number(event.get("frame"))
        self.last_frame = self.start_frame
        self.state = "CAPTURING"

    def sync(self, item: dict) -> bool:
        item_lease = item.get("lease_id")
        if self.lease_id is not None and "lease_id" in item:
            if item_lease != self.lease_id:
                return False
        if self.state == "FREE" and item.get("state") != "FREE":
            return False
        if self.lease_id is not None and item.get("state") == "FREE":
            return False
        previous = self.state
        for name in ("state", "worker_id", "investigation_id", "lease_id",
                     "filter_type", "filter_value", "start_frame", "last_frame", "bytes_used",
                     "event_count", "capture_duration", "frames_covered",
                     "freeze_reason", "full", "truncated", "format_version",
                     "logical_header_bytes", "physical_header_bytes", "record_size",
                     "capsule_path"):
            if name in item:
                setattr(self, name, item[name])
        return previous != self.state

    def reset(self) -> None:
        self.state = "FREE"
        self.worker_id = self.investigation_id = self.lease_id = None
        self.filter_type = None
        self.filter_value = self.start_frame = self.last_frame = 0
        self.bytes_used = self.event_count = self.frames_covered = 0
        self.capture_duration = 0.0
        self.freeze_reason = None
        self.full = self.truncated = False
        self.format_version = 2
        self.logical_header_bytes = 64
        self.physical_header_bytes = 24
        self.record_size = 20
        self.capsule_path = None
        self.known = self.merged = self.proven = self.bounded_unresolved = False

    def snapshot(self) -> dict:
        return {
            "capsule_id": self.capsule_id, "state": self.state,
            "worker_id": self.worker_id, "investigation_id": self.investigation_id,
            "lease_id": self.lease_id, "filter_type": self.filter_type,
            "filter_value": self.filter_value, "start_frame": self.start_frame,
            "last_frame": self.last_frame, "bytes_used": self.bytes_used,
            "capacity": CAPSULE_SIZE, "utilization": self.bytes_used / CAPSULE_SIZE,
            "event_count": self.event_count, "capture_duration": self.capture_duration,
            "frames_covered": self.frames_covered, "freeze_reason": self.freeze_reason,
            "format_version": self.format_version, "logical_header_bytes": self.logical_header_bytes,
            "physical_header_bytes": self.physical_header_bytes, "record_size": self.record_size,
            "capsule_path": self.capsule_path, "full": self.full, "truncated": self.truncated,
            "known": self.known,
            "merged": self.merged, "proven": self.proven,
            "bounded_unresolved": self.bounded_unresolved,
        }


class CommandPublisher:
    """Writes only the replaceable command state outside callback/claim paths."""

    def __init__(self, path: Path):
        self.path = path
        self.latest = ""
        self.changed = threading.Event()
        self.stopped = threading.Event()
        self.thread = threading.Thread(target=self._run, name="auto67-capsule-commands",
                                       daemon=True)
        self.thread.start()

    def publish(self, text: str) -> None:
        self.latest = text
        self.changed.set()

    def _run(self) -> None:
        while not self.stopped.is_set():
            self.changed.wait(0.25)
            self.changed.clear()
            if not self.latest:
                continue
            temporary = self.path.with_suffix(".tmp")
            try:
                temporary.write_text(self.latest, encoding="utf-8")
                temporary.replace(self.path)
            except OSError:
                pass

    def stop(self) -> None:
        self.stopped.set()
        self.changed.set()
        self.thread.join(timeout=1)


class CapsulePool:
    def __init__(self, count: int = CAPSULE_COUNT,
                 max_live: int = MAX_LIVE_CAPTURES,
                 command_path: Path | None = None):
        if count != CAPSULE_COUNT:
            raise ValueError("AUTO67.1 requires exactly 16 capsules")
        if max_live < 0 or max_live > count:
            raise ValueError("max live captures must be between 0 and 16")
        self.max_live = max_live
        self.capsules = [Capsule(i) for i in range(count)]
        self.lock = threading.RLock()
        self.ready = threading.Condition(self.lock)
        self.commands: deque[str] = deque(maxlen=64)
        self.command_sequence = 0
        self.publisher = CommandPublisher(command_path) if command_path else None
        self.samples: deque[int] = deque(maxlen=4096)
        self.metrics = {"capsules_created": 0, "capsules_frozen": 0,
                        "capsules_reused": 0, "capsules_full": 0,
                        "capsules_truncated": 0, "known_early_release": 0,
                        "merge_early_release": 0, "frozen_samples": 0}

    def _publish(self, op: str, capsule: Capsule) -> None:
        self.command_sequence += 1
        fields = [op, str(self.command_sequence), str(capsule.capsule_id),
                  capsule.lease_id or "-", str(capsule.worker_id or 0),
                  capsule.investigation_id or "-", capsule.filter_type or "-",
                  str(capsule.filter_value), "30"]
        self.commands.append("|".join(fields))
        if self.publisher:
            self.publisher.publish("\n".join(self.commands) + "\n")

    def claim(self, worker_id: int, investigation_id: str, event: dict) -> Capsule | None:
        with self.lock:
            live = sum(item.state in {"CLAIMED", "CAPTURING"} for item in self.capsules)
            if live >= self.max_live:
                return None
            capsule = next((item for item in self.capsules if item.state == "FREE"), None)
            if capsule is None:
                return None
            lease_id = f"L{self.command_sequence + 1:08X}"
            capsule.claim(worker_id, investigation_id, lease_id, event)
            self.metrics["capsules_created"] += 1
            self._publish("START", capsule)
            return capsule

    def capacity_state(self) -> tuple[int, int]:
        with self.lock:
            live = sum(item.state in {"CLAIMED", "CAPTURING"}
                       for item in self.capsules)
            free = sum(item.state == "FREE" for item in self.capsules)
            return live, free

    def sync(self, items: list[dict]) -> None:
        with self.ready:
            for item in items:
                cid = _number(item.get("capsule_id"), -1)
                if not 0 <= cid < len(self.capsules):
                    continue
                capsule = self.capsules[cid]
                changed = capsule.sync(item)
                if changed and capsule.state == "FROZEN":
                    self.metrics["capsules_frozen"] += 1
                    self.metrics["frozen_samples"] += 1
                    self.samples.append(capsule.bytes_used)
                    self.metrics["capsules_full"] += int(capsule.full)
                    self.metrics["capsules_truncated"] += int(capsule.truncated)
            self.ready.notify_all()

    def wait_frozen(self, capsule_id: int, lease_id: str,
                    stop_event: threading.Event) -> bool:
        with self.ready:
            while not stop_event.is_set():
                capsule = self.capsules[capsule_id]
                if capsule.lease_id != lease_id:
                    return False
                if capsule.state in {"FROZEN", "ANALYZING", "DONE"}:
                    return True
                self.ready.wait(0.2)
        return False

    def analyzing(self, capsule_id: int) -> None:
        with self.lock:
            if self.capsules[capsule_id].state == "FROZEN":
                self.capsules[capsule_id].state = "ANALYZING"

    def decode(self, capsule_id: int, lease_id: str,
               investigation_id: str) -> DecodedCapsule:
        with self.lock:
            capsule = self.capsules[capsule_id]
            if capsule.lease_id != lease_id or capsule.investigation_id != investigation_id:
                raise CapsuleFormatError("capsule metadata lease/investigation mismatch")
            if not capsule.capsule_path:
                raise CapsuleFormatError("frozen capsule path is missing")
            path = Path(capsule.capsule_path)
            expected_id = capsule.capsule_id
        decoded = decode_capsule(path, expected_id, lease_id)
        if decoded.format_version != 2:
            raise CapsuleFormatError("worker materialization requires capsule format v2")
        return decoded

    def release(self, capsule_id: int, result: str) -> None:
        with self.lock:
            capsule = self.capsules[capsule_id]
            capsule.bounded_unresolved = result == "BOUNDED_UNRESOLVED"
            capsule.known = result == "KNOWN"
            capsule.merged = result == "MERGED"
            capsule.proven = result == "PROVEN"
            capsule.state = "DONE"
            self._publish("RELEASE", capsule)
            self.metrics["capsules_reused"] += 1
            self.metrics["known_early_release"] += int(result == "KNOWN")
            self.metrics["merge_early_release"] += int(result == "MERGED")
            self.ready.notify_all()
            capsule.reset()

    @staticmethod
    def _percentile(values: list[int], fraction: float) -> int:
        if not values:
            return 0
        return values[min(len(values) - 1, int((len(values) - 1) * fraction))]

    def snapshot(self) -> dict:
        with self.lock:
            samples = sorted(self.samples)
            state_counts = {state: 0 for state in
                            ("FREE", "CLAIMED", "CAPTURING", "FROZEN",
                             "ANALYZING", "DONE", "MERGED", "ABORTED")}
            for item in self.capsules:
                state_counts[item.state] = state_counts.get(item.state, 0) + 1
            return {"configured": len(self.capsules), "capacity": CAPSULE_SIZE,
                    "max_simultaneous_live": self.max_live,
                    "active_live": sum(item.state in {"CLAIMED", "CAPTURING"}
                                       for item in self.capsules),
                    "capsules_free": state_counts["FREE"],
                    "state_counts": state_counts,
                    "items": [item.snapshot() for item in self.capsules],
                    "metrics": dict(self.metrics),
                    "bytes_used": {"min": samples[0] if samples else 0,
                                    "p50": self._percentile(samples, .50),
                                    "p90": self._percentile(samples, .90),
                                    "p95": self._percentile(samples, .95),
                                    "p99": self._percentile(samples, .99),
                                    "max": samples[-1] if samples else 0,
                                    "mean": sum(samples) / len(samples) if samples else 0},
                    "utilization": {"p50": self._percentile(samples, .50) / CAPSULE_SIZE,
                                     "p90": self._percentile(samples, .90) / CAPSULE_SIZE,
                                     "p95": self._percentile(samples, .95) / CAPSULE_SIZE,
                                     "p99": self._percentile(samples, .99) / CAPSULE_SIZE,
                                     "max": samples[-1] / CAPSULE_SIZE if samples else 0}}

    def stop(self) -> None:
        if self.publisher:
            self.publisher.stop()
