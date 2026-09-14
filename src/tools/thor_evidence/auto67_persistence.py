"""Bounded persistent worker-chain writer for the existing SQLite sidecar."""

from __future__ import annotations

from collections import Counter
import hashlib
import json
from pathlib import Path
import queue
import threading
import time
import uuid
from typing import Any

try:
    from .identity import ROM_SHA
    from .store import Store
except ImportError:  # direct AUTO67 launcher/tests put this directory on sys.path
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    from thor_evidence.identity import ROM_SHA
    from thor_evidence.store import Store


CAUSAL_FIELDS = (
    "kind", "pc", "address", "caller_pc", "consumer_pc", "source_address",
    "destination_address", "selector", "index", "branch_suffix", "pointer_target",
    "rom_target", "ram_target", "predecessor", "successor", "writer_pc", "reader_pc",
    "consumer", "producer", "relation_source", "relation_target", "relation_kind",
)
EXPLICIT_CHAIN_FIELDS = ("chain_steps", "causal_facts", "unresolved_frontier",
                         "terminal_root")
CHAIN_QUEUE_CAPACITY = 16384


def canonical_chain(event: dict[str, Any]) -> str:
    """Serialize only observed causal facts; temporal/UI fields are excluded."""
    observed = {name: event[name] for name in CAUSAL_FIELDS
                if event.get(name) is not None}
    record: dict[str, Any] = {"observed": observed}
    for name in EXPLICIT_CHAIN_FIELDS:
        if event.get(name) is not None:
            record[name] = event[name]
    materialized = event.get("materialized")
    if materialized is not None:
        record["materialized_schema_version"] = materialized.get(
            "materialized_schema_version", 1)
        record["seed"] = {name: materialized.get("seed", {}).get(name)
                           for name in CAUSAL_FIELDS
                           if materialized.get("seed", {}).get(name) is not None}
        record["runtime_observations"] = [
            {name: item[name] for name in ("kind", "pc", "address") if item.get(name) is not None}
            for item in materialized.get("runtime_observations", [])]
        record["observed_facts"] = [
            {name: value for name, value in item.items() if name != "provenance"}
            for item in materialized.get("observed_facts", [])]
        record["causal_facts"] = [
            {name: value for name, value in item.items() if name != "provenance"}
            for item in materialized.get("causal_facts", [])]
        record["chain_steps"] = materialized.get("chain_steps", [])
        if materialized.get("unresolved_frontier") is not None:
            record["unresolved_frontier"] = {
                name: materialized["unresolved_frontier"][name]
                for name in ("status", "missing", "next", "reason", "evidence")
                if name in materialized["unresolved_frontier"]}
    return json.dumps(record, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def chain_descriptor(event: dict[str, Any], status: str, frame: int | None,
                     worker_id: int | None = None, lease_id: str | None = None,
                     investigation_id: str | None = None,
                     materialization_provenance: dict[str, Any] | None = None) -> dict[str, Any]:
    canonical = canonical_chain(event)
    chain_hash = hashlib.sha256(canonical.encode("utf-8")).hexdigest()
    provenance_data: dict[str, Any] = {
        "source": "AUTO67-live-worker",
        "status": status,
        "frame": frame,
        "worker_id": worker_id,
        "lease_id": lease_id,
        "investigation_id": investigation_id,
    }
    if materialization_provenance:
        provenance_data["capsule"] = materialization_provenance
    record_class = "MATERIALIZED_CHAIN" if event.get("materialized") is not None else "SEED_ONLY"
    provenance = json.dumps(provenance_data, sort_keys=True, separators=(",", ":"))
    return {"chain_hash": chain_hash, "canonical_payload": canonical,
            "status": status, "frame": frame, "provenance": provenance,
            "record_class": record_class}


def materialized_descriptor(seed: dict[str, Any], materialized: dict[str, Any],
                           status: str, frame: int | None,
                           worker_id: int | None = None, lease_id: str | None = None,
                           investigation_id: str | None = None) -> dict[str, Any]:
    event = dict(seed)
    event["materialized"] = materialized
    capsule_provenance = {name: materialized[name] for name in (
        "capsule_format_version", "capsule_record_count") if name in materialized}
    return chain_descriptor(event, status, frame, worker_id, lease_id,
                            investigation_id, capsule_provenance)


def _store_monitor(store: Store, session_id: str) -> dict[str, Any]:
    """Read objective growth from the writer-owned Store connection."""
    session = store.connection.execute(
        "SELECT payload FROM live_session WHERE id=?", (session_id,)).fetchone()
    if session is None:
        return {"available": False}
    metrics = json.loads(session["payload"])
    total = store.connection.execute("SELECT COUNT(*) FROM live_chain").fetchone()[0]
    unresolved = store.connection.execute(
        "SELECT COUNT(*) FROM live_chain WHERE last_status != 'PROVEN'").fetchone()[0]
    rooted = store.connection.execute(
        "SELECT COUNT(*) FROM live_chain WHERE last_status = 'PROVEN'").fetchone()[0]
    seed_only = store.connection.execute(
        "SELECT COUNT(*) FROM live_chain WHERE record_class='SEED_ONLY'").fetchone()[0]
    materialized = store.connection.execute(
        "SELECT COUNT(*) FROM live_chain WHERE record_class='MATERIALIZED_CHAIN'").fetchone()[0]
    return {"available": True, "total_unique_chains": total,
            "new_unique_chains_this_session": metrics.get("unique_chain_inserts", 0),
            "exact_duplicates_rejected": metrics.get("exact_duplicate_observations", 0),
            "completed_worker_chains": metrics.get("completed_worker_chains", 0),
            "unresolved_chains": unresolved, "rooted_chains": rooted,
            "seed_only_chains": seed_only, "materialized_chains": materialized}


class LivePersistenceSink:
    """One bounded non-BizHawk queue and one transactional SQLite writer."""

    def __init__(self, path: Path, scenario_key: str = "AUTO67-live"):
        self.path = Path(path).resolve()
        self.scenario_key = scenario_key
        self.session_id = "auto67-session-" + uuid.uuid4().hex
        self.items: queue.Queue[dict[str, Any] | None] = queue.Queue(
            maxsize=CHAIN_QUEUE_CAPACITY)
        self.stop_requested = threading.Event()
        self.ready = threading.Event()
        self.thread: threading.Thread | None = None
        self.lock = threading.Lock()
        self.submitted = 0
        self.dropped = 0
        self.persisted = 0
        self.write_errors = 0
        self.last_error: str | None = None
        self.last_result: dict[str, Any] | None = None
        self.runtime_leases = 0
        self.monitor: dict[str, Any] = {
            "available": False, "total_unique_chains": 0,
            "new_unique_chains_this_session": 0,
            "exact_duplicates_rejected": 0, "completed_worker_chains": 0,
            "unresolved_chains": 0, "rooted_chains": 0,
            "seed_only_chains": 0, "materialized_chains": 0,
        }
        self.statuses: Counter[str] = Counter()

    def start(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.thread = threading.Thread(target=self._run, name="auto67-chain-writer",
                                       daemon=True)
        self.thread.start()
        self.ready.wait(timeout=5)

    def set_runtime_leases(self, leases: int) -> None:
        with self.lock:
            self.runtime_leases = leases

    def submit(self, item: dict[str, Any]) -> bool:
        try:
            self.items.put_nowait(item)
        except queue.Full:
            with self.lock:
                self.dropped += 1
            return False
        with self.lock:
            self.submitted += 1
        return True

    def _run(self) -> None:
        store = None
        try:
            store = Store(self.path)
            store.begin_live_session(
                self.session_id, self.scenario_key, str(time.time()),
                json.dumps({"schema": "oasis.m12.auto67.chain-session.v1",
                            "chain_status": {}}, sort_keys=True,
                           separators=(",", ":")))
            self.monitor = _store_monitor(store, self.session_id)
            self.ready.set()
            processed = 0
            while not self.stop_requested.is_set() or not self.items.empty():
                try:
                    item = self.items.get(timeout=0.1)
                except queue.Empty:
                    continue
                if item is None:
                    continue
                try:
                    result = store.record_live_chain(
                        self.session_id, item["chain_hash"], item["canonical_payload"],
                        item["status"], item["frame"], item["provenance"],
                        item.get("record_class", "SEED_ONLY"))
                    processed += 1
                    with self.lock:
                        self.persisted += 1
                        self.last_result = result
                        self.statuses[item["status"]] += 1
                    if processed % 64 == 0:
                        self.monitor = _store_monitor(store, self.session_id)
                except Exception as error:
                    with self.lock:
                        self.write_errors += 1
                        self.last_error = f"{type(error).__name__}: {error}"
        except Exception as error:  # keep the live machine running if DB is unavailable
            with self.lock:
                self.last_error = f"{type(error).__name__}: {error}"
            self.ready.set()
        finally:
            if store is not None:
                try:
                    self.monitor = _store_monitor(store, self.session_id)
                    with store.connection:
                        store.connection.execute("UPDATE live_session SET ended_at=? WHERE id=?",
                                                 (str(time.time()), self.session_id))
                    store.close()
                except Exception as error:
                    with self.lock:
                        self.last_error = f"{type(error).__name__}: {error}"

    def stop(self) -> None:
        if self.thread is None:
            return
        self.stop_requested.set()
        while True:
            try:
                self.items.put_nowait(None)
                break
            except queue.Full:
                time.sleep(0.01)
        self.thread.join(timeout=10)
        if self.thread.is_alive():
            with self.lock:
                self.last_error = self.last_error or "writer did not drain before timeout"

    def snapshot(self) -> dict[str, Any]:
        with self.lock:
            leases = self.runtime_leases
            monitor = dict(self.monitor)
            completed = monitor.get("completed_worker_chains", 0)
            monitor["chains_per_1000_leases"] = (
                completed * 1000.0 / leases if leases else 0.0)
            monitor["db_size_bytes"] = self.path.stat().st_size if self.path.exists() else 0
            return {"available": monitor.get("available", False),
                    "database": str(self.path), "session_id": self.session_id,
                    "queue_depth": self.items.qsize(),
                    "queue_capacity": self.items.maxsize, "submitted": self.submitted,
                    "dropped": self.dropped, "persisted": self.persisted,
                    "write_errors": self.write_errors, "monitor": monitor,
                    "statuses": dict(self.statuses), "last_result": dict(self.last_result or {}),
                    "error": self.last_error}


# Compatibility name for callers that imported the old bridge directly.
descriptor = chain_descriptor
