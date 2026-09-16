"""Bounded frozen-capsule pool for AUTO67.1 developer-only experiments."""

from __future__ import annotations

import threading
import time
from collections import deque
from dataclasses import dataclass, field
from pathlib import Path

from auto67_capsule_codec import CapsuleFormatError, DecodedCapsule, decode_capsule
from auto67_predecessor import PredecessorCapture, decode as decode_predecessor


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
    predecessor_enabled: bool = False
    predecessor_registers: tuple[str, ...] = ()
    predecessor_target_pc: int = 0
    predecessor_id: int | None = None
    predecessor_path: str | None = None
    predecessor_record_count: int = 0
    predecessor_complete: bool = False
    predecessor_truncated: bool = False
    predecessor_gap: bool = False
    native_snapshot_mode: bool = False
    native_snapshot_identity: str | None = None
    native_snapshot_occurrence: tuple[int, int] | None = None
    native_snapshot_epoch: int = 0
    native_snapshot_first_sequence: int = 0
    native_snapshot_latest_sequence: int = 0
    native_snapshot_count: int = 0
    native_snapshot_pc: int = 0
    native_snapshot_registers: tuple[tuple[str, int], ...] = ()
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
        requested = tuple(str(item) for item in event.get(
            "register_provenance_registers", ()) if str(item) in {"A4", "A5"})
        path = event.get("prehistory_path")
        self.native_snapshot_mode = bool(event.get("native_snapshot_mode"))
        self.native_snapshot_identity = event.get("snapshot_identity")
        self.native_snapshot_occurrence = (
            _number(event.get("epoch", 1)), _number(event.get("seq")))
        self.native_snapshot_epoch = _number(event.get("snapshot_epoch"))
        self.native_snapshot_first_sequence = _number(event.get("first_sequence"))
        self.native_snapshot_latest_sequence = _number(event.get("latest_sequence"))
        self.native_snapshot_count = _number(event.get("count"))
        self.native_snapshot_pc = _number(event.get("pc"))
        registers = event.get("native_snapshot_registers") or {}
        self.native_snapshot_registers = tuple(sorted(
            (name, _number(value)) for name, value in registers.items()
            if name in {"A4", "A5"} and value is not None))
        self.predecessor_enabled = bool(path and path != "null") or self.native_snapshot_mode
        self.predecessor_registers = requested
        self.predecessor_target_pc = _number(event.get("pc"))
        self.predecessor_id = _number(event.get("prehistory_id"), 0) or None
        self.predecessor_path = str(path) if path and path != "null" else None
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
        for name in ("predecessor_enabled", "predecessor_registers",
                     "predecessor_target_pc", "predecessor_id", "predecessor_path",
                     "predecessor_record_count", "predecessor_complete",
                     "predecessor_truncated", "predecessor_gap"):
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
        self.predecessor_enabled = False
        self.predecessor_registers = ()
        self.predecessor_target_pc = 0
        self.predecessor_id = None
        self.predecessor_path = None
        self.predecessor_record_count = 0
        self.predecessor_complete = self.predecessor_truncated = self.predecessor_gap = False
        self.native_snapshot_mode = False
        self.native_snapshot_identity = None
        self.native_snapshot_occurrence = None
        self.native_snapshot_epoch = 0
        self.native_snapshot_first_sequence = 0
        self.native_snapshot_latest_sequence = 0
        self.native_snapshot_count = 0
        self.native_snapshot_pc = 0
        self.native_snapshot_registers = ()

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
            "predecessor_enabled": self.predecessor_enabled,
            "predecessor_registers": list(self.predecessor_registers),
            "predecessor_target_pc": self.predecessor_target_pc,
            "predecessor_id": self.predecessor_id,
            "predecessor_path": self.predecessor_path,
            "predecessor_record_count": self.predecessor_record_count,
            "predecessor_complete": self.predecessor_complete,
            "predecessor_truncated": self.predecessor_truncated,
            "predecessor_gap": self.predecessor_gap,
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
                 command_path: Path | None = None,
                 native_snapshot_pool: Any | None = None):
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
        self.native_snapshot_pool = native_snapshot_pool
        self.samples: deque[int] = deque(maxlen=4096)
        self.metrics = {"capsules_created": 0, "capsules_frozen": 0,
                        "capsules_reused": 0, "capsules_full": 0,
                        "capsules_truncated": 0, "frozen_samples": 0}
        self.metrics["predecessor_decode_path_samples"] = []

    def _publish(self, op: str, capsule: Capsule) -> None:
        self.command_sequence += 1
        fields = [op, str(self.command_sequence), str(capsule.capsule_id),
                  capsule.lease_id or "-", str(capsule.worker_id or 0),
                  capsule.investigation_id or "-", capsule.filter_type or "-",
                  str(capsule.filter_value), "30",
                  "1" if capsule.predecessor_enabled else "0",
                  str(capsule.predecessor_target_pc),
                  ",".join(capsule.predecessor_registers) or "-",
                  str(capsule.predecessor_id or 0), capsule.predecessor_path or "-"]
        self.commands.append("|".join(fields))
        if self.publisher:
            self.publisher.publish("\n".join(self.commands) + "\n")

    def claim(self, worker_id: int, investigation_id: str, event: dict) -> Capsule | None:
        with self.lock:
            if event.get("native_snapshot_mode") and event.get("snapshot_identity"):
                self._validate_native_snapshot_event(event)
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

    def _validate_native_snapshot_event(self, event: dict[str, Any]) -> None:
        if self.native_snapshot_pool is None:
            raise CapsuleFormatError("native snapshot pool is unavailable")
        try:
            occurrence = (_number(event.get("epoch", 1)), _number(event.get("seq")))
            snapshot = self.native_snapshot_pool.get(
                str(event["snapshot_identity"]), occurrence)
            if (snapshot.snapshot_epoch != _number(event.get("snapshot_epoch"))
                    or snapshot.first_sequence != _number(event.get("first_sequence"))
                    or snapshot.latest_sequence != _number(event.get("latest_sequence"))
                    or snapshot.count != _number(event.get("count"))):
                raise ValueError("native snapshot metadata mismatch")
        except (KeyError, ValueError) as error:
            raise CapsuleFormatError(
                f"native snapshot occurrence mismatch: {error}") from error

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

    def decode_predecessor(self, capsule_id: int, lease_id: str,
                           investigation_id: str) -> PredecessorCapture | None:
        with self.lock:
            capsule = self.capsules[capsule_id]
            if capsule.lease_id != lease_id or capsule.investigation_id != investigation_id:
                raise CapsuleFormatError("predecessor lease/investigation mismatch")
            if len(self.metrics["predecessor_decode_path_samples"]) < 8:
                self.metrics["predecessor_decode_path_samples"].append({
                    "capsule_id": capsule_id,
                    "enabled": capsule.predecessor_enabled or capsule.native_snapshot_mode,
                    "path": capsule.predecessor_path,
                    "native_snapshot_identity": capsule.native_snapshot_identity})
            if capsule.native_snapshot_mode:
                if not capsule.native_snapshot_identity or self.native_snapshot_pool is None:
                    self.metrics.setdefault("native_snapshot_missing", 0)
                    self.metrics["native_snapshot_missing"] += 1
                    return None
                try:
                    occurrence = capsule.native_snapshot_occurrence
                    if occurrence is None:
                        raise ValueError("native snapshot occurrence identity missing")
                    snapshot = self.native_snapshot_pool.get(
                        capsule.native_snapshot_identity, occurrence)
                    event = {
                        "epoch": occurrence[0], "seq": occurrence[1],
                        "occurrence_id": f"epoch={occurrence[0]}:seq={occurrence[1]}",
                        "frame": capsule.start_frame, "pc": capsule.native_snapshot_pc,
                        "snapshot_identity": capsule.native_snapshot_identity,
                        "snapshot_epoch": capsule.native_snapshot_epoch,
                        "first_sequence": capsule.native_snapshot_first_sequence,
                        "latest_sequence": capsule.native_snapshot_latest_sequence,
                        "count": capsule.native_snapshot_count,
                    }
                    from auto67_native_snapshot import to_predecessor_capture
                    capture = to_predecessor_capture(
                        snapshot, event, capsule.predecessor_registers)
                except (KeyError, ValueError) as error:
                    raise CapsuleFormatError(
                        f"native snapshot identity/range mismatch: {error}") from error
                self.metrics.setdefault("native_snapshot_resolver_inputs", 0)
                self.metrics["native_snapshot_resolver_inputs"] += 1
                return capture
            if capsule.predecessor_path in {None, "", "-", "null"}:
                return None
            path = Path(capsule.predecessor_path)
        return decode_predecessor(path)

    def release(self, capsule_id: int) -> None:
        with self.lock:
            capsule = self.capsules[capsule_id]
            capsule.state = "DONE"
            self._publish("RELEASE", capsule)
            self.metrics["capsules_reused"] += 1
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
