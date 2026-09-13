"""V7 bounded frontier inventory and deterministic evidence scheduler."""
from dataclasses import asdict, dataclass

from .identity import ROM_SIZE, STATUSES, canonical, digest

MAX_REQUEST_SPAN = 0x10000
ACTIVE = {"UNKNOWN", "PROVISIONAL", "CONFLICT"}


@dataclass
class Frontier:
    id: str
    kind: str
    start: int
    end: int
    status: str
    information_gain: int
    confidence: int
    cost: int
    risk: int
    evidence_classes: tuple
    attempts: int = 0
    exhausted: bool = False


class FrontierScheduler:
    """Schedule bounded requests and stop after two non-progress passes."""

    def __init__(self, rom_size=ROM_SIZE, max_request_span=MAX_REQUEST_SPAN):
        self.rom_size = rom_size
        self.max_request_span = max_request_span
        self.frontiers = {}
        self.requests = []
        self.results = []
        self._last_state = None
        self._fixed_point = False

    def add(self, kind, start, end, status="UNKNOWN", information_gain=0,
            confidence=0, cost=0, risk=0, evidence_classes=("STATIC",)):
        if type(start) is not int or type(end) is not int or not 0 <= start < end <= self.rom_size:
            raise ValueError("frontier range is invalid")
        if end - start > self.max_request_span:
            raise ValueError("frontier exceeds bounded request span")
        if status not in ACTIVE:
            raise ValueError("scheduler accepts unresolved frontiers only")
        values = (information_gain, confidence, cost, risk)
        if any(type(value) is not int or value < 0 for value in values):
            raise ValueError("scheduler scores must be non-negative integers")
        if not evidence_classes or any(not isinstance(item, str) or not item for item in evidence_classes):
            raise ValueError("evidence class is required")
        ident = digest({"kind": "thor-v7-frontier", "range": [start, end], "query": kind,
                        "status": status, "information_gain": information_gain,
                        "confidence": confidence, "cost": cost, "risk": risk,
                        "evidence_classes": list(evidence_classes)})
        if ident in self.frontiers:
            raise ValueError("duplicate frontier")
        item = Frontier(ident, kind, start, end, status, information_gain, confidence,
                        cost, risk, tuple(evidence_classes))
        self.frontiers[ident] = item
        return ident

    def rank(self):
        active = [item for item in self.frontiers.values() if item.status in ACTIVE and not item.exhausted]
        active.sort(key=lambda item: (-item.information_gain, item.confidence,
                                      item.cost, item.risk, item.id))
        return [{"frontier_id": item.id, "information_gain": item.information_gain,
                 "confidence": item.confidence, "cost": item.cost, "risk": item.risk,
                 "score": [-item.information_gain, item.confidence, item.cost,
                           item.risk, item.id]} for item in active]

    def next_request(self):
        ranked = self.rank()
        if not ranked:
            return None
        choice = self.frontiers[ranked[0]["frontier_id"]]
        request = {"schema": "thor.evidence.v7.request", "frontier_id": choice.id,
                   "start": choice.start, "end": choice.end,
                   "evidence_class": choice.evidence_classes[0], "attempt": choice.attempts + 1,
                   "bounded": True, "whole_rom_trace": False}
        self.requests.append(request)
        return request

    def record(self, frontier_id, status="UNKNOWN", progress=False, evidence_ids=()):
        if frontier_id not in self.frontiers:
            raise ValueError("unknown frontier")
        item = self.frontiers[frontier_id]
        if item.exhausted:
            raise ValueError("frontier is exhausted")
        if status not in STATUSES | {"PROVISIONAL"}:
            raise ValueError("invalid frontier result status")
        before = self._state()
        item.attempts += 1
        item.status = status
        if not progress and item.attempts >= 2:
            item.exhausted = True
        self.results.append({"frontier_id": frontier_id, "status": status,
                             "progress": bool(progress), "evidence_ids": list(evidence_ids),
                             "attempt": item.attempts})
        after = self._state()
        self._fixed_point = before == after or (not progress and item.exhausted)
        self._last_state = after
        return dict(self.results[-1])

    def fixed_point(self):
        return self._fixed_point or not self.rank()

    def _state(self):
        return tuple((item.id, item.status, item.attempts, item.exhausted)
                     for item in sorted(self.frontiers.values(), key=lambda value: value.id))

    def export(self):
        return {"schema": "thor.evidence.v7.frontier-scheduler", "rom_size": self.rom_size,
                "frontiers": [_plain(asdict(item)) for item in sorted(self.frontiers.values(), key=lambda x: x.id)],
                "rank": self.rank(), "requests": list(self.requests), "results": list(self.results),
                "fixed_point": self.fixed_point(), "whole_rom_trace": False,
                "ownership": {"source_owned_delta": 0, "promotion": False}}


def _plain(value):
    if isinstance(value, tuple):
        return [_plain(item) for item in value]
    if isinstance(value, dict):
        return {key: _plain(item) for key, item in value.items()}
    return value
