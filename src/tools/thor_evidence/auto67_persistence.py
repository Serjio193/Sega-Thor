"""Single bounded AUTO67 output queue for the MAP-1 Cartographer."""

from __future__ import annotations

from pathlib import Path
import queue
import threading
import time
from typing import Any

try:
    from .auto67_cartographer import candidate_bundle, import_ref
    from .cartographer import Cartographer
    from .identity import ROM_SHA
except ImportError:
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    from auto67_cartographer import candidate_bundle, import_ref
    from cartographer import Cartographer
    from thor_evidence.identity import ROM_SHA


MAP_QUEUE_CAPACITY = 16384


class LiveMapSink:
    """One bounded local-chain queue and one Cartographer writer thread."""

    def __init__(self, map_db: Path, source_sha256: str = ""):
        self.map_db = Path(map_db).resolve()
        self.source_sha256 = source_sha256 or ROM_SHA
        self.items: queue.Queue[dict[str, Any] | None] = queue.Queue(
            maxsize=MAP_QUEUE_CAPACITY)
        self.stop_requested = threading.Event()
        self.ready = threading.Event()
        self.thread: threading.Thread | None = None
        self.lock = threading.Lock()
        self.submitted = 0
        self.dropped = 0
        self.local_chains_submitted = 0
        self.local_chains_processed = 0
        self.chains_with_accepted_proof = 0
        self.chains_without_accepted_proof = 0
        self.map_new_nodes = 0
        self.map_new_edges = 0
        self.map_promoted_nodes = 0
        self.map_promoted_edges = 0
        self.map_conflicts = 0
        self.map_component_joins = 0
        self.map_fragments_dropped = 0
        self.map_write_errors = 0
        self.last_map_delta: dict[str, Any] = {}
        self.cached_graph_hash = ""
        self.last_map_error: str | None = None

    def start(self) -> None:
        self.map_db.parent.mkdir(parents=True, exist_ok=True)
        self.thread = threading.Thread(target=self._run, name="auto67-map-writer",
                                       daemon=True)
        self.thread.start()
        self.ready.wait(timeout=5)

    def submit(self, local_chain: dict[str, Any]) -> bool:
        try:
            self.items.put_nowait(local_chain)
        except queue.Full:
            with self.lock:
                self.dropped += 1
                self.map_fragments_dropped += 1
            return False
        with self.lock:
            self.submitted += 1
            self.local_chains_submitted += 1
        return True

    def _run(self) -> None:
        cartographer = None
        try:
            cartographer = Cartographer(self.map_db, self.source_sha256)
            self.cached_graph_hash = cartographer.graph_hash()
            self.ready.set()
            while not self.stop_requested.is_set() or not self.items.empty():
                try:
                    local_chain = self.items.get(timeout=0.1)
                except queue.Empty:
                    continue
                if local_chain is None:
                    continue
                with self.lock:
                    self.local_chains_processed += 1
                candidate = candidate_bundle(local_chain)
                if candidate is None:
                    with self.lock:
                        self.chains_without_accepted_proof += 1
                    continue
                bundle, stable_hash = candidate
                try:
                    delta = cartographer.merge(bundle, import_ref(stable_hash),
                                               self.source_sha256)
                    with self.lock:
                        self.chains_with_accepted_proof += 1
                        self.map_new_nodes += delta.new_nodes
                        self.map_new_edges += delta.new_edges
                        self.map_promoted_nodes += delta.promoted_nodes
                        self.map_promoted_edges += delta.promoted_edges
                        self.map_conflicts += delta.new_conflicts
                        self.map_component_joins += delta.component_joins
                        self.last_map_delta = delta.as_dict()
                        self.cached_graph_hash = delta.graph_hash
                except Exception as error:
                    with self.lock:
                        self.map_write_errors += 1
                        self.last_map_error = f"{type(error).__name__}: {error}"
        except Exception as error:
            with self.lock:
                self.last_map_error = f"{type(error).__name__}: {error}"
            self.ready.set()
        finally:
            if cartographer is not None:
                try:
                    cartographer.close()
                except Exception as error:
                    with self.lock:
                        self.last_map_error = f"{type(error).__name__}: {error}"

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
                self.last_map_error = self.last_map_error or "writer did not drain before timeout"

    def snapshot(self) -> dict[str, Any]:
        with self.lock:
            return {
                "available": True,
                "database": str(self.map_db),
                "queue_depth": self.items.qsize(),
                "queue_capacity": self.items.maxsize,
                "submitted": self.submitted,
                "dropped": self.dropped,
                "local_chains_submitted": self.local_chains_submitted,
                "local_chains_processed": self.local_chains_processed,
                "chains_with_accepted_proof": self.chains_with_accepted_proof,
                "chains_without_accepted_proof": self.chains_without_accepted_proof,
                "map_new_nodes": self.map_new_nodes,
                "map_new_edges": self.map_new_edges,
                "map_promoted_nodes": self.map_promoted_nodes,
                "map_promoted_edges": self.map_promoted_edges,
                "map_conflicts": self.map_conflicts,
                "map_component_joins": self.map_component_joins,
                "map_fragments_dropped": self.map_fragments_dropped,
                "map_write_errors": self.map_write_errors,
                "last_map_delta": dict(self.last_map_delta),
                "graph_hash": self.cached_graph_hash,
                "last_map_error": self.last_map_error,
            }
