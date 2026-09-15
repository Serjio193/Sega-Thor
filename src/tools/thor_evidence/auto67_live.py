"""AUTO67 live opportunistic runtime sampler and bounded worker dispatcher."""

from __future__ import annotations

import argparse
import hashlib
import threading
import time
from collections import deque
from typing import Any
from auto67_status import StatusPublisher
from auto67_capsule import CapsulePool
from auto67_capsule_codec import CapsuleFormatError
from auto67_materializer import materialize, required_registers
from auto67_profile import DispatchProfiler
from auto67_persistence import LivePersistenceSink, descriptor, materialized_descriptor


ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
BASELINE = "019fed68d7e906daabe184f3b74017c853b78ce3"
DISPATCH_REASONS = ("DISPATCHED", "REJECT_ACTIVE_CLAIM", "REJECT_NO_FREE_CAPSULE",
                    "REJECT_CAPTURE_LIMIT", "REJECT_INVALID", "REJECT_OTHER")


def digest(value: Any) -> str:
    if isinstance(value, dict):
        payload = "|".join(f"{key}={value[key]!r}" for key in sorted(value))
    else:
        payload = repr(value)
    return hashlib.blake2s(payload.encode("utf-8"), digest_size=12).hexdigest()


class RollingWindow:
    """Bounded current context; it never becomes a pending-event backlog."""

    def __init__(self, capacity: int):
        if capacity < 2:
            raise ValueError("rolling window capacity must be at least two")
        self.capacity = capacity
        self.items: deque[dict[str, Any]] = deque(maxlen=capacity)
        self.overwrites = 0
        self.retained = 0

    def append(self, event: dict[str, Any]) -> None:
        if len(self.items) == self.capacity:
            self.overwrites += 1
        self.items.append(event)

    def current(self) -> list[dict[str, Any]]:
        return list(self.items)


class Dispatcher:
    """Single claim authority with one mailbox per worker and no raw-event FIFO."""

    def __init__(self, worker_count: int = 16, capacity: int = 256,
                 processing_delay: float = 0.003,
                 capsule_pool: CapsulePool | None = None,
                 chain_sink: LivePersistenceSink | None = None,
                 rom: bytes | None = None):
        if worker_count not in {1, 2, 4, 8, 16, 32, 64}:
            raise ValueError("worker count must be one of 1,2,4,8,16,32,64")
        self.window = RollingWindow(capacity)
        self.worker_count = worker_count
        self.processing_delay = processing_delay
        self.capsule_pool = capsule_pool
        self.chain_sink = chain_sink
        self.rom = rom
        self.lock = threading.RLock()
        self.stop_event = threading.Event()
        self.wake = [threading.Event() for _ in range(worker_count)]
        self.mailboxes: list[dict[str, Any] | None] = [None] * worker_count
        self.worker_states = ["STARTING"] * worker_count
        self.threads: list[threading.Thread] = []
        self._window_item_sequence = 0
        self.metrics: dict[str, Any] = {
            "events_observed": 0,
            "seeds_considered": 0, "seeds_dispatched": 0,
            "known_rejected_before_dispatch": 0,
            "active_collisions": 0,
            "investigation_merges": 0, "new_chains": 0, "new_branches": 0,
            "new_edges": 0, "investigations_created": 0,
            "worker_leases": 0, "worker_returns": 0, "peak_workers_busy": 0,
            "peak_workers_working": 0,
            "capture_poll_count": 0, "worker_cpu_seconds": 0.0,
            "persistence_submit_errors": 0,
            "seed_age_sum": 0.0, "max_seed_age": 0.0, "same_session_known_replay": False,
            "new_roots": 0, "new_consumers": 0, "new_writers": 0,
            "structures_enumerated": 0, "promotion_candidates": 0,
            "bytes_promoted": 0,
            "dispatch_reasons": {reason: 0 for reason in DISPATCH_REASONS},
            "dispatch_starved_no_free_capsule": False,
            "latest_assignment": None,
            "worker_pool_prestarted": False,
            "workers_started": 0,
            "hunt_attempts": 0, "hunt_successes": 0,
            "focused_capture_requested": 0, "focused_capture_completed": 0,
            "focused_capture_slot_waits": 0,
            "capsule_decode_errors": 0, "capsule_records_decoded": 0,
            "materialized_chains": 0, "materialized_observed_facts": 0,
            "materialized_causal_facts": 0, "materialized_chain_steps": 0,
            "unsupported_causal_facts": 0,
            "predecessor_captures_installed": 0, "predecessor_records_captured": 0,
            "predecessor_decode_attempts": 0, "predecessor_decode_none": 0,
            "predecessor_max_ring_utilization": 0, "predecessor_truncations": 0,
            "predecessor_gaps": 0, "register_provenance_resolved": 0,
            "register_provenance_unresolved": 0,
            "duplicate_active_claims": 0,
        }
        self.recent_investigations: deque[dict[str, Any]] = deque(maxlen=16)
        self.dispatch_profiler = DispatchProfiler()
        self.event_times: deque[float] = deque(maxlen=512)
        self.worker_info = [{"worker_id": i, "state": "STARTING",
                             "investigation_id": None, "chain_fingerprint": None,
                             "stage": "DISPATCH", "seed_age": 0.0,
                             "task_runtime": 0.0, "last_result": None,
                             "transitions": []} for i in range(worker_count)]
        self.transition_history: deque[dict[str, Any]] = deque(maxlen=500)

    @staticmethod
    def _branch(event: dict[str, Any]) -> str:
        return digest({"kind": event.get("kind"), "address": event.get("address"),
                       "pc": event.get("pc")})

    @staticmethod
    def _seed(event: dict[str, Any], branch: str | None = None) -> str:
        return digest({"branch": branch or Dispatcher._branch(event),
                       "occurrence_id": event.get("occurrence_id"),
                       "window_item_id": event.get("window_item_id")})

    @staticmethod
    def _context(event: dict[str, Any], branch: str) -> str:
        fields = ("epoch", "scene", "state", "caller_pc", "consumer_pc",
                  "source_address", "destination_address", "selector", "index",
                  "branch_suffix", "pointer_target", "rom_target", "ram_target",
                  "predecessor", "successor")
        context = {name: event.get(name) for name in fields if event.get(name) is not None}
        context["branch"] = branch
        return digest(context)

    def _ensure_occurrence_identity(self, event: dict[str, Any]) -> None:
        if "window_item_id" not in event:
            self._window_item_sequence += 1
            event["window_item_id"] = self._window_item_sequence
        event.setdefault("epoch", 1)
        event["occurrence_id"] = str(event.get("occurrence_id") or
                                      f"epoch={event['epoch']}:seq={event.get('seq')}")

    def _decision(self, event: dict[str, Any], reason: str,
                  state: str | None = None) -> None:
        self.metrics["dispatch_reasons"][reason] += 1
        if state:
            event["dispatch_state"] = state
        event["dispatch_reason"] = reason

    def start(self) -> None:
        for worker_id in range(self.worker_count):
            thread = threading.Thread(target=self._worker, args=(worker_id,),
                                      name=f"auto67-worker-{worker_id}", daemon=True)
            self.threads.append(thread)
            thread.start()
        with self.lock:
            self.metrics["worker_pool_prestarted"] = True
            self.metrics["workers_started"] = len(self.threads)
            self.worker_states = ["IDLE"] * self.worker_count
            for item in self.worker_info:
                self._transition(item, "IDLE", "DISPATCH")

    def stop(self) -> None:
        self.stop_event.set()
        for item in self.wake:
            item.set()
        for thread in self.threads:
            thread.join(timeout=5)

    def _choose_current(self) -> tuple[int, dict[str, Any]] | None:
        free = [i for i, state in enumerate(self.worker_states)
                if state in {"IDLE", "HUNTING"}]
        if not free:
            return None
        current = list(reversed(self.window.items))
        prehistory = [event for event in current if event.get("prehistory_path")]
        ordinary = [event for event in current if not event.get("prehistory_path")]
        for event in prehistory + ordinary:
            if event.get("dispatch_state"):
                continue
            self._ensure_occurrence_identity(event)
            self.metrics["seeds_considered"] += 1
            t0 = time.perf_counter_ns()
            branch = self._branch(event)
            context = self._context(event, branch)
            event["branch_fingerprint"] = branch
            event["context_fingerprint"] = context
            t1 = time.perf_counter_ns()
            worker_id = free.pop(0)
            age = max(0.0, time.monotonic() - float(event.get("captured_monotonic", time.monotonic())))
            occurrence_id = event["occurrence_id"]
            task = {"branch": branch, "context": context, "occurrence_id": occurrence_id,
                    "worker_id": worker_id, "seed": self._seed(event, branch),
                    "assigned_ns": time.time_ns(), "dispatch_trace": {
                        "timestamps_ns": {"t0": t0, "t1": t1, "t2": 0},
                        "frame": event.get("frame")}}
            task["dispatch_trace"]["timestamps_ns"]["t2"] = time.perf_counter_ns()
            task["investigation_id"] = "INV-AUTO67-" + task["seed"]
            if self.capsule_pool is not None and self.capsule_pool.max_live > 0:
                self.metrics["focused_capture_requested"] += 1
            if self.capsule_pool is not None:
                capsule = self.capsule_pool.claim(
                    worker_id, task["investigation_id"], event)
                if capsule is None:
                    live, free_capsules = self.capsule_pool.capacity_state()
                    if live >= self.capsule_pool.max_live or not free_capsules:
                        task["capture_status"] = ("WAITING_CAPTURE_SLOT" if
                                                    self.capsule_pool.max_live > 0 else
                                                    "DISABLED")
                        self.metrics["focused_capture_slot_waits"] += int(
                            self.capsule_pool.max_live > 0)
                    else:
                        free.insert(0, worker_id)
                        self._decision(event, "REJECT_OTHER")
                        return None
                else:
                    task["capsule_id"] = capsule.capsule_id
                    task["capsule_lease"] = capsule.lease_id
                    self.metrics["predecessor_captures_installed"] += int(
                        capsule.predecessor_enabled)
            task["dispatch_trace"]["timestamps_ns"]["t3"] = time.perf_counter_ns()
            task["event"] = dict(event)
            task["dispatch_trace"]["timestamps_ns"]["t4"] = time.perf_counter_ns()
            task["dispatch_trace"]["timestamps_ns"]["t5"] = time.perf_counter_ns()
            task["lease_id"] = task.get("capsule_lease") or (
                f"L{self.metrics['worker_leases'] + 1:08X}-{digest(occurrence_id)}")
            event["dispatch_state"] = "LEASED"
            event["lease_worker"] = worker_id
            self.mailboxes[worker_id] = task
            task["dispatch_trace"]["timestamps_ns"]["t6"] = time.perf_counter_ns()
            self.worker_states[worker_id] = "LEASED"
            info = self.worker_info[worker_id]
            info.update({"investigation_id": task["investigation_id"], "chain_fingerprint": branch,
                         "stage": "DISPATCH", "seed_age": age, "task_runtime": 0.0, "last_result": None})
            self._transition(info, "LEASED", "DISPATCH")
            self.metrics["seeds_dispatched"] += 1
            self.metrics["worker_leases"] += 1
            if self.chain_sink is not None:
                try:
                    self.chain_sink.set_runtime_leases(self.metrics["worker_leases"])
                except Exception:
                    self.metrics["persistence_submit_errors"] += 1
            self.metrics["dispatch_starved_no_free_capsule"] = False
            self.metrics["dispatch_reasons"]["DISPATCHED"] += 1
            self.metrics["latest_assignment"] = {"worker_id": worker_id, "chain": branch,
                "context": context, "occurrence_id": occurrence_id,
                "investigation_id": info["investigation_id"], "lease_id": task["lease_id"]}
            busy = self.worker_count - len(free)
            self.metrics["peak_workers_busy"] = max(self.metrics["peak_workers_busy"], busy)
            self.metrics["seed_age_sum"] += age
            self.metrics["max_seed_age"] = max(self.metrics["max_seed_age"], age)
            return worker_id, task
        return None

    def _dispatch_current(self, hunting: bool = False) -> None:
        if hunting:
            self.metrics["hunt_attempts"] += 1
        assigned: list[tuple[int, dict[str, Any]]] = []
        claim_acquired_ns = 0
        with self.lock:
            claim_acquired_ns = time.perf_counter_ns()
            while True:
                chosen = self._choose_current()
                if chosen is None:
                    break
                assigned.append(chosen)
        released_ns = time.perf_counter_ns()
        self.dispatch_profiler.record_claim_lock(released_ns - claim_acquired_ns)
        if hunting:
            self.metrics["hunt_successes"] += len(assigned)
        for worker_id, task in assigned:
            task["dispatch_trace"]["timestamps_ns"]["t7"] = released_ns
            self.wake[worker_id].set()

    def ingest(self, event: dict[str, Any]) -> None:
        event = dict(event)
        event.setdefault("captured_monotonic", time.monotonic())
        if self.capsule_pool is not None and self.rom is not None:
            event["register_provenance_registers"] = required_registers(event, self.rom)
        with self.lock:
            self._ensure_occurrence_identity(event)
            self.window.append(event)
            self.metrics["events_observed"] += 1
            self.event_times.append(time.monotonic())
        self._dispatch_current()

    def _worker(self, worker_id: int) -> None:
        while not self.stop_event.is_set():
            self.wake[worker_id].wait(0.2)
            self.wake[worker_id].clear()
            with self.lock:
                task = self.mailboxes[worker_id]
                self.mailboxes[worker_id] = None
                if task is None:
                    if self.stop_event.is_set():
                        break
                    self.worker_states[worker_id] = "HUNTING"
                    self._transition(self.worker_info[worker_id], "HUNTING", "HUNT")
            if task is None:
                self._dispatch_current(hunting=True)
                with self.lock:
                    if self.mailboxes[worker_id] is None:
                        self._transition(self.worker_info[worker_id], "IDLE", "DISPATCH")
                continue
            with self.lock:
                self.worker_states[worker_id] = "WORKING"
                self.metrics["peak_workers_working"] = max(
                    self.metrics["peak_workers_working"], self.worker_states.count("WORKING"))
                info = self.worker_info[worker_id]
                self._transition(info, "WORKING", "CAPTURE" if "capsule_id" in task else "MATERIALIZE")
                task["dispatch_trace"]["timestamps_ns"]["t8"] = time.perf_counter_ns()
                task["dispatch_trace"]["lease_id"] = task.get("lease_id")
                task["dispatch_trace"]["investigation_id"] = info["investigation_id"]
                self.dispatch_profiler.record(task["dispatch_trace"])
            started = time.perf_counter()
            cpu_started = time.thread_time()
            event = task["event"]
            capsule_id = task.get("capsule_id")
            capsule_evidence = None
            predecessor_evidence = None
            capture_completed = False
            decode_failed = False
            if capsule_id is not None:
                with self.lock:
                    self._transition(self.worker_info[worker_id], "WORKING", "CAPTURING")
                capture_completed = self.capsule_pool.wait_frozen(
                    capsule_id, task["capsule_lease"], self.stop_event)
                self.metrics["focused_capture_completed"] += int(capture_completed)
                self.capsule_pool.analyzing(capsule_id)
                if capture_completed:
                    try:
                        capsule_evidence = self.capsule_pool.decode(
                            capsule_id, task["capsule_lease"], task["investigation_id"])
                        self.metrics["capsule_records_decoded"] += capsule_evidence.event_count
                        self.metrics["predecessor_decode_attempts"] += 1
                        predecessor_evidence = self.capsule_pool.decode_predecessor(
                            capsule_id, task["capsule_lease"], task["investigation_id"])
                        self.metrics["predecessor_decode_none"] += int(
                            predecessor_evidence is None)
                        if predecessor_evidence is not None:
                            self.metrics["predecessor_records_captured"] += len(
                                predecessor_evidence.records)
                            self.metrics["predecessor_max_ring_utilization"] = max(
                                self.metrics["predecessor_max_ring_utilization"],
                                len(predecessor_evidence.records))
                            self.metrics["predecessor_truncations"] += int(
                                predecessor_evidence.truncated)
                            self.metrics["predecessor_gaps"] += int(predecessor_evidence.gap)
                    except CapsuleFormatError:
                        decode_failed = True
                        self.metrics["capsule_decode_errors"] += 1
                with self.lock:
                    self._transition(self.worker_info[worker_id], "WORKING", "ANALYZING")
            else:
                with self.lock:
                    if task.get("capture_status") == "WAITING_CAPTURE_SLOT":
                        self._transition(self.worker_info[worker_id], "WORKING",
                                         "WAITING_CAPTURE_SLOT")
                    self._transition(self.worker_info[worker_id], "WORKING", "QUICK_CHECK")
            if self.processing_delay:
                time.sleep(self.processing_delay)
            branch = task["branch"]
            inv_id = "INV-AUTO67-" + task["seed"]
            worker_outcome = ("EVIDENCE_MATERIALIZED" if capsule_evidence is not None
                              else "DECODE_FAILED" if decode_failed
                              else "CAPTURE_UNAVAILABLE" if capsule_id is not None and not capture_completed
                              else "EVIDENCE_CAPTURED" if capsule_id is not None
                              else "EVIDENCE_OBSERVED")
            persistence_status = "BOUNDED_UNRESOLVED"
            persistence_event = dict(event, worker_outcome=worker_outcome)
            if capsule_evidence is not None:
                materialized = materialize(event, capsule_evidence, self.rom,
                                           predecessor_evidence, live_worker=True)
                persistence_item = materialized_descriptor(
                    persistence_event, materialized, persistence_status, event.get("frame"), worker_id,
                    task.get("lease_id"), inv_id)
            else:
                materialized = None
                persistence_item = descriptor(
                    persistence_event, persistence_status, event.get("frame"), worker_id,
                    task.get("lease_id"), inv_id)
            with self.lock:
                investigation = {"id": inv_id, "branch": branch,
                                 "context": task["context"],
                                 "occurrence_id": task["occurrence_id"],
                                 "window_item_id": event.get("window_item_id"),
                                 "lease_id": task.get("lease_id"),
                                 "seed_sequence": event.get("seq"),
                                 "outcome": worker_outcome, "evidence": [event]}
                if materialized is not None:
                    investigation["materialization"] = {
                        "capsule_format_version": materialized["capsule_format_version"],
                        "capsule_record_count": materialized["capsule_record_count"],
                        "runtime_observation_count": len(materialized["runtime_observations"]),
                        "observed_fact_count": len(materialized["observed_facts"]),
                        "causal_fact_count": len(materialized["causal_facts"]),
                        "chain_step_count": len(materialized["chain_steps"]),
                    }
                    self.metrics["materialized_chains"] += 1
                    self.metrics["materialized_observed_facts"] += len(
                        materialized["observed_facts"])
                    self.metrics["materialized_causal_facts"] += len(
                        materialized["causal_facts"])
                    self.metrics["materialized_chain_steps"] += len(
                        materialized["chain_steps"])
                    provenance = materialized.get("register_provenance", {})
                    self.metrics["register_provenance_resolved"] += len(
                        provenance.get("resolved", []))
                    self.metrics["register_provenance_unresolved"] += len(
                        provenance.get("unresolved", []))
                self.recent_investigations.append(investigation)
                self.metrics["investigations_created"] += 1
                self.worker_states[worker_id] = "RETURNING"
                info = self.worker_info[worker_id]
                info["task_runtime"] = time.perf_counter() - started
                info["last_result"] = worker_outcome
                self._transition(info, "RETURNING", worker_outcome)
                self.metrics["worker_returns"] += 1
                self.worker_states[worker_id] = "IDLE"
                self._transition(info, "IDLE", worker_outcome)
                self.metrics["worker_cpu_seconds"] += time.thread_time() - cpu_started
                if capsule_id is not None:
                    self.capsule_pool.release(capsule_id)
            if self.chain_sink is not None:
                try:
                    self.chain_sink.submit(persistence_item)
                    self.chain_sink.set_runtime_leases(self.metrics["worker_leases"])
                except Exception:
                    self.metrics["persistence_submit_errors"] += 1
            self._dispatch_current(hunting=True)

    def _transition(self, info: dict[str, Any], state: str, stage: str) -> None:
        info["state"] = state
        info["stage"] = stage
        transition = {"worker_id": info["worker_id"], "state": state,
                      "stage": stage, "chain": info["chain_fingerprint"],
                      "investigation_id": info["investigation_id"],
                      "at": time.time()}
        info["transitions"].append(transition)
        if len(info["transitions"]) > 24:
            del info["transitions"][:-24]
        self.transition_history.append(transition)

    def snapshot(self, lightweight: bool = False) -> dict[str, Any] | None:
        if not self.lock.acquire(blocking=not lightweight):
            return None
        try:
            metrics = dict(self.metrics)
            total_age = metrics.pop("seed_age_sum")
            metrics["average_seed_age"] = total_age / max(1, metrics["worker_leases"])
            now = time.monotonic()
            metrics["event_rate"] = sum(item >= now - 1.0 for item in self.event_times)
            metrics["workers_busy"] = sum(state in {"LEASED", "WORKING", "RETURNING"}
                                           for state in self.worker_states)
            metrics["workers_configured"] = self.worker_count
            metrics["active_claims"] = sum(state in {"LEASED", "WORKING", "RETURNING"} for state in self.worker_states)
            metrics["duplicate_active_claims"] = 0
            metrics["branch_suppression_policy"] = "OBSOLETE_NO_SCHEDULING"
            metrics["fresh_candidates_available"] = sum(
                not item.get("dispatch_state") for item in self.window.items)
            capsules = self.capsule_pool.snapshot() if self.capsule_pool else None
            if capsules:
                metrics["capsules_free"] = capsules["capsules_free"]
                metrics["focused_capture_completed"] = capsules["metrics"]["capsules_frozen"]
            persistence = self.chain_sink.snapshot() if self.chain_sink else {
                "available": False}
            return {"metrics": metrics, "worker_states": list(self.worker_states),
                    "workers": [dict(item, transitions=list(item["transitions"]))
                                for item in self.worker_info],
                    "transition_history": list(self.transition_history) if not lightweight
                    else list(self.transition_history)[-32:],
                    "rolling_window": {"capacity": self.window.capacity,
                                        "utilization": len(self.window.items),
                                        "max_utilization": self.window.capacity,
                                        "overwrites": self.window.overwrites,
                                        "retained": self.window.retained},
                    "investigations": list(self.recent_investigations),
                    "chain_store": persistence,
                    "dispatch_profile": self.dispatch_profiler.snapshot(),
                    "capsules": capsules}
        finally:
            self.lock.release()
def run_live(args: argparse.Namespace) -> dict[str, Any]:
    from auto67_runner import run_live as runner
    return runner(args)
def main() -> int:
    from auto67_runner import main as runner_main
    return runner_main()
if __name__ == "__main__":
    raise SystemExit(main())
