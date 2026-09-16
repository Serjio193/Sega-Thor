"""Developer-only 1:1 immutable snapshot admission probe for AUTO67."""

from __future__ import annotations

import inspect
import threading
import time
from collections import Counter, deque
from dataclasses import dataclass
from typing import Any

from auto67_live import Dispatcher


SNAPSHOT_POOL_CAPACITY = 16


@dataclass(frozen=True)
class FrozenSnapshot:
    snapshot_identity: str
    occurrence_identity: tuple[int, int]
    snapshot_epoch: int
    first_sequence: int
    latest_sequence: int
    count: int
    frozen_at: float


class SnapshotPool:
    """Fixed immutable slots; no overwrite and no semantic admission test."""

    def __init__(self, capacity: int = SNAPSHOT_POOL_CAPACITY):
        if capacity != SNAPSHOT_POOL_CAPACITY:
            raise ValueError("1:1 admission requires exactly 16 snapshot slots")
        self.capacity = capacity
        self.lock = threading.RLock()
        self.slots: list[FrozenSnapshot | None] = [None] * capacity
        self._sequence = 0
        self._full_started: float | None = None
        self._full_duration = 0.0
        self._occupancy_peak = 0
        self._occupancy_samples: deque[dict[str, Any]] = deque(maxlen=4096)
        self._occupancy_histogram: Counter[int] = Counter()
        self.metrics = {
            "occurrences_seen": 0, "occurrences_frozen": 0,
            "snapshots_dispatched": 0, "snapshots_completed": 0,
            "snapshot_pool_full_count": 0,
            "occurrences_without_snapshot": 0,
        }

    @staticmethod
    def _identity(event: dict[str, Any]) -> tuple[int, int]:
        epoch = int(event.get("epoch", 1))
        seq = int(event["seq"])
        expected = f"epoch={epoch}:seq={seq}"
        if event.get("occurrence_id", expected) != expected:
            raise ValueError("invalid occurrence identity")
        return epoch, seq

    def _sample(self, operation: str) -> None:
        depth = sum(item is not None for item in self.slots)
        self._occupancy_peak = max(self._occupancy_peak, depth)
        self._occupancy_histogram[depth] += 1
        self._occupancy_samples.append({"operation": operation,
                                        "depth": depth,
                                        "at": time.monotonic()})

    def freeze(self, event: dict[str, Any], snapshot_epoch: int,
               first_sequence: int, latest_sequence: int, count: int
               ) -> FrozenSnapshot | None:
        """Freeze every occurrence at admission; only identity/slot checks run."""
        with self.lock:
            self.metrics["occurrences_seen"] += 1
            epoch, seq = self._identity(event)
            if snapshot_epoch != epoch or first_sequence > latest_sequence:
                raise ValueError("invalid snapshot identity metadata")
            slot = next((index for index, item in enumerate(self.slots)
                         if item is None), None)
            if slot is None:
                now = time.monotonic()
                self.metrics["snapshot_pool_full_count"] += 1
                self.metrics["occurrences_without_snapshot"] += 1
                if self._full_started is None:
                    self._full_started = now
                self._sample("SNAPSHOT_POOL_FULL")
                return None
            self._sequence += 1
            frozen = FrozenSnapshot(
                snapshot_identity=f"SNAP-{self._sequence:08X}",
                occurrence_identity=(epoch, seq), snapshot_epoch=snapshot_epoch,
                first_sequence=first_sequence, latest_sequence=latest_sequence,
                count=count, frozen_at=time.time())
            self.slots[slot] = frozen
            self.metrics["occurrences_frozen"] += 1
            self._sample("FREEZE")
            return frozen

    def mark_dispatched(self, snapshot_identity: str) -> None:
        with self.lock:
            for item in self.slots:
                if item and item.snapshot_identity == snapshot_identity:
                    self.metrics["snapshots_dispatched"] += 1
                    self._sample("DISPATCH")
                    return
            raise ValueError("snapshot identity is not owned by this pool")

    def get(self, snapshot_identity: str,
            occurrence_identity: tuple[int, int]) -> FrozenSnapshot:
        with self.lock:
            for item in self.slots:
                if item and item.snapshot_identity == snapshot_identity:
                    if item.occurrence_identity != occurrence_identity:
                        raise ValueError("snapshot occurrence identity mismatch")
                    return item
        raise KeyError(snapshot_identity)

    def release(self, snapshot_identity: str,
                occurrence_identity: tuple[int, int]) -> None:
        with self.lock:
            for index, item in enumerate(self.slots):
                if item and item.snapshot_identity == snapshot_identity:
                    if item.occurrence_identity != occurrence_identity:
                        raise ValueError("snapshot release identity mismatch")
                    self.slots[index] = None
                    self.metrics["snapshots_completed"] += 1
                    now = time.monotonic()
                    if self._full_started is not None:
                        self._full_duration += now - self._full_started
                        self._full_started = None
                    self._sample("RELEASE")
                    return
        raise KeyError(snapshot_identity)

    def snapshot(self) -> dict[str, Any]:
        with self.lock:
            full_duration = self._full_duration
            if self._full_started is not None:
                full_duration += time.monotonic() - self._full_started
            return {
                **self.metrics,
                "snapshot_pool_capacity": self.capacity,
                "current_snapshot_depth": sum(item is not None for item in self.slots),
                "peak_snapshot_depth": self._occupancy_peak,
                "snapshot_pool_full_duration": full_duration,
                "occupancy_histogram": {str(k): v for k, v in
                                         sorted(self._occupancy_histogram.items())},
                "occupancy_trace": list(self._occupancy_samples),
            }


class SnapshotAdmissionProbe:
    """Uses the existing Dispatcher while keeping snapshot state diagnostic-only."""

    def __init__(self, worker_count: int = 16, window_capacity: int = 8,
                 processing_delay: float = 0.05):
        if worker_count != SNAPSHOT_POOL_CAPACITY:
            raise ValueError("1:1 experiment requires exactly 16 workers")
        self.pool = SnapshotPool()
        self.dispatcher = Dispatcher(worker_count=worker_count,
                                     capacity=window_capacity,
                                     processing_delay=processing_delay)
        self._snapshots: dict[tuple[int, int], FrozenSnapshot] = {}
        self._event_wall_times: dict[tuple[int, int], float] = {}
        self._seen_transitions: set[tuple[str, str, str]] = set()
        self._completed: set[tuple[int, int]] = set()
        self._worker_idle_min = worker_count
        self._worker_busy_peak = 0
        self._ages: dict[str, list[float]] = {
            "occurrence_at_lease": [], "occurrence_at_worker_start": [],
            "frozen_at_worker_start": [],
        }

    def start(self) -> None:
        self.dispatcher.start()
        self._sample_workers()

    def _sample_workers(self) -> None:
        state = self.dispatcher.snapshot()
        metrics = state["metrics"]
        busy = metrics["workers_busy"]
        self._worker_busy_peak = max(self._worker_busy_peak, busy)
        self._worker_idle_min = min(self._worker_idle_min,
                                    self.dispatcher.worker_count - busy)

    def ingest(self, event: dict[str, Any], first_sequence: int,
               latest_sequence: int, count: int) -> None:
        event = dict(event)
        epoch = int(event.get("epoch", 1))
        seq = int(event["seq"])
        event.setdefault("occurrence_id", f"epoch={epoch}:seq={seq}")
        event["captured_wall"] = time.time()
        self._event_wall_times[(epoch, seq)] = event["captured_wall"]
        frozen = self.pool.freeze(event, epoch, first_sequence,
                                  latest_sequence, count)
        if frozen is not None:
            self._snapshots[(epoch, seq)] = frozen
            event.update({"snapshot_identity": frozen.snapshot_identity,
                          "snapshot_epoch": frozen.snapshot_epoch,
                          "first_sequence": frozen.first_sequence,
                          "latest_sequence": frozen.latest_sequence,
                          "count": frozen.count,
                          "snapshot_count": frozen.count})
        else:
            event["snapshot_status"] = "SNAPSHOT_POOL_FULL"
        self.dispatcher.ingest(event)
        self._reconcile()

    def _reconcile(self) -> None:
        state = self.dispatcher.snapshot()
        now = time.time()
        idle_occurrences: set[tuple[int, int]] = set()
        for transition in state["transition_history"]:
            key = (transition["state"], transition["occurrence_id"],
                   transition["investigation_id"] or "")
            if key in self._seen_transitions:
                continue
            self._seen_transitions.add(key)
            identity = transition["occurrence_id"]
            if not identity or not identity.startswith("epoch="):
                continue
            epoch, seq = (int(part.split("=", 1)[1]) for part in
                          identity.split(":", 1))
            frozen = self._snapshots.get((epoch, seq))
            if frozen is None:
                continue
            age = max(0.0, now - self._event_wall(epoch, seq))
            if transition["state"] == "LEASED":
                self.pool.mark_dispatched(frozen.snapshot_identity)
                self._ages["occurrence_at_lease"].append(age)
            elif transition["state"] == "WORKING":
                self._ages["occurrence_at_worker_start"].append(age)
                self._ages["frozen_at_worker_start"].append(
                    max(0.0, transition["at"] - frozen.frozen_at))
            elif transition["state"] == "IDLE":
                idle_occurrences.add((epoch, seq))
        for investigation in state["investigations"]:
            identity = investigation.get("occurrence_id", "")
            if not identity.startswith("epoch="):
                continue
            epoch, seq = (int(part.split("=", 1)[1]) for part in
                          identity.split(":", 1))
            occurrence = (epoch, seq)
            frozen = self._snapshots.get(occurrence)
            if (frozen is not None and occurrence not in self._completed
                    and occurrence in idle_occurrences):
                self.pool.get(frozen.snapshot_identity, occurrence)
                self.pool.release(frozen.snapshot_identity, occurrence)
                self._completed.add(occurrence)
        self._sample_workers()

    def wait_for_returns(self, count: int, timeout: float = 5.0) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            self._reconcile()
            if self.dispatcher.snapshot()["metrics"]["worker_returns"] >= count:
                self._reconcile()
                return
            time.sleep(0.005)
        raise AssertionError("workers did not complete the admission burst")

    def stop(self) -> dict[str, Any]:
        self._reconcile()
        self.dispatcher.stop()
        self._reconcile()
        result = self.pool.snapshot()
        result.update({"workers_busy_current": self.dispatcher.snapshot()["metrics"]["workers_busy"],
                       "workers_busy_peak": self._worker_busy_peak,
                       "workers_idle_current": self.dispatcher.worker_count - self.dispatcher.snapshot()["metrics"]["workers_busy"],
                       "workers_idle_min": self._worker_idle_min,
                       "worker_leases": self.dispatcher.snapshot()["metrics"]["worker_leases"],
                       "worker_returns": self.dispatcher.snapshot()["metrics"]["worker_returns"],
                       "rolling_window_overwrites": self.dispatcher.snapshot()["rolling_window"]["overwrites"],
                       "occurrence_age_at_worker_lease": self._ages["occurrence_at_lease"],
                       "occurrence_age_at_worker_start": self._ages["occurrence_at_worker_start"],
                       "frozen_snapshot_age_at_worker_start": self._ages["frozen_at_worker_start"]})
        return result

    def _event_wall(self, epoch: int, seq: int) -> float:
        return self._event_wall_times.get((epoch, seq), time.time())


def admission_static_audit() -> dict[str, Any]:
    """Mechanically verify admission has no semantic pre-worker selector."""
    source = inspect.getsource(Dispatcher._choose_current)
    module_source = inspect.getsource(SnapshotPool.freeze)
    forbidden = ("global_map", "cartographer", "frontier", "interesting",
                 "useful", "not-needed", "known", "proven", "duplicate",
                 "conflict", "REJECT_ACTIVE_CLAIM")
    found = [token for token in forbidden if token.lower() in source.lower()]
    return {"dispatcher_admission_forbidden_tokens": found,
            "existing_order_is_preserved": "prehistory" in source,
            "obsolete_active_claim_counter_in_admission": "REJECT_ACTIVE_CLAIM" in source,
            "snapshot_freeze_reads_only_identity_and_slot": not any(
                token in module_source for token in ("kind", "address", "pc")),
            "global_map_or_cartographer_in_admission": any(
                token in source.lower() for token in ("global_map", "cartographer")),
            "semantic_filter_absent": not found}


def run_admission_experiment() -> dict[str, Any]:
    probe = SnapshotAdmissionProbe()
    probe.start()
    try:
        for seq in range(16):
            probe.ingest({"epoch": 1, "seq": seq, "occurrence_id":
                          f"epoch=1:seq={seq}", "kind": "RAM_WRITE",
                          "pc": "0x100", "address": "0x200"},
                         max(0, seq - 3), seq, 4)
        probe.wait_for_returns(16)
        return probe.stop()
    except Exception:
        probe.dispatcher.stop()
        raise
