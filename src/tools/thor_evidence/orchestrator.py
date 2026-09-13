"""V9 bounded operational cycle joining capture, graph, static and scheduler state."""
import hashlib
import json

from .frontier import FrontierScheduler
from .identity import ROM_SHA, canonical, digest, require_hash
from .static_bridge import StaticBridge
from .store import Store


class OperationalCycle:
    """One deterministic, non-owning M12 evidence cycle."""

    def __init__(self, database_path, carver=None, rom_sha256=ROM_SHA):
        self.rom_sha256 = require_hash(rom_sha256)
        self.store = Store(database_path)
        self.carver = carver
        self.scheduler = FrontierScheduler()
        self.trace = None
        self.static = None
        self._static_requests = {}
        self.captures = []
        self.static_responses = []

    def close(self):
        self.store.close()

    def capture(self, path):
        trace = self.store.import_capture(path)
        if self.trace is not None and trace != self.trace:
            raise ValueError("one operational cycle cannot splice traces")
        self.trace = trace
        self.static = StaticBridge(trace, self.rom_sha256)
        if trace not in self.captures:
            self.captures.append(trace)
        return trace

    def seed_frontier(self, kind, start, end, **scores):
        return self.scheduler.add(kind, start, end, **scores)

    def next_request(self):
        request = self.scheduler.next_request()
        if request is None:
            return None
        if self.trace is not None:
            request["trace"] = self.trace
            static_request = self.static.request(request["start"], request["end"],
                                                 request["evidence_class"],
                                                 (request["frontier_id"],))
            self._static_requests[request["frontier_id"]] = static_request
            request["static_request_id"] = static_request.id
        return request

    def static_response(self, request, records=(), certificates=(), unresolved=()):
        if self.static is None:
            raise ValueError("capture must be imported before static request")
        if isinstance(request, dict):
            request = self._static_requests[request["frontier_id"]]
        return self.static.response(request, records, certificates, unresolved)

    def ingest_static(self, response_id, frontier_id=None, progress=False):
        if self.static is None or self.carver is None:
            raise ValueError("static bridge and Carver database are required")
        merged = self.static.merge_to_carver(self.carver, response_id)
        response = self.static.responses[response_id]
        if frontier_id is not None:
            self.scheduler.record(frontier_id, status="PROVISIONAL" if progress else "UNKNOWN",
                                  progress=progress, evidence_ids=merged["evidence_ids"])
        self.static_responses.append(response_id)
        return merged

    def persist_v4(self, graph):
        if self.trace is None:
            raise ValueError("capture must be imported before V4 persistence")
        self.store.import_v4(graph, self.trace)

    def export(self):
        if self.trace is None:
            raise ValueError("capture is required for operational manifest")
        store_payload = self.store.export()
        manifest = {"schema": "thor.evidence.v9.operational-cycle",
                    "rom_sha256": self.rom_sha256, "trace": self.trace,
                    "captures": sorted(self.captures),
                    "store_sha256": hashlib.sha256(store_payload.encode()).hexdigest(),
                    "scheduler": self.scheduler.export(),
                    "static_bridge": self.static.export(),
                    "static_response_ids": sorted(self.static_responses),
                    "carver": self.carver.interval_db() if self.carver is not None else None,
                    "ownership": {"source_owned_delta": 0, "promotion": False}}
        canonical(manifest)
        return manifest

    def manifest_bytes(self):
        return (json.dumps(self.export(), sort_keys=True, separators=(",", ":"),
                           ensure_ascii=True) + "\n").encode()

    def identity(self):
        return digest({"kind": "thor-v9-cycle", "manifest": self.export()})
