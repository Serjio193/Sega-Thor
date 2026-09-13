"""V6 controlled-scenario differential evidence without cross-run causality."""
from dataclasses import asdict, dataclass

from .identity import STATUSES, canonical, digest, require_hash

ARMS = {"NEUTRAL_REPEAT", "MOTIVATED_ALTERNATE"}


def _plain(value):
    if isinstance(value, tuple):
        return [_plain(item) for item in value]
    if isinstance(value, dict):
        return {key: _plain(item) for key, item in value.items()}
    return value


@dataclass(frozen=True)
class Scenario:
    id: str
    environment_id: str
    trace_id: str
    arm: str
    motivation: str
    repeat_of: str
    graph_id: str


class DifferentialSet:
    """Keep each scenario graph separate and classify repeatable observations."""

    def __init__(self, rom_sha256):
        self.rom_sha256 = require_hash(rom_sha256)
        self.scenarios = {}
        self.facts = {}
        self.events = {}
        self.edges = {}

    def add_scenario(self, scenario_id, environment_id, trace_id, arm="NEUTRAL_REPEAT",
                     motivation="", repeat_of=""):
        for value in (scenario_id, environment_id, trace_id):
            if not isinstance(value, str) or not value:
                raise ValueError("scenario identity fields are required")
        if arm not in ARMS:
            raise ValueError("unknown scenario arm")
        if arm == "MOTIVATED_ALTERNATE" and not motivation:
            raise ValueError("alternate arm requires motivation")
        if arm == "NEUTRAL_REPEAT" and motivation:
            raise ValueError("neutral repeat cannot carry motivation")
        if scenario_id in self.scenarios:
            raise ValueError("duplicate scenario identity")
        graph_id = digest({"kind": "thor-v6-scenario-graph", "rom": self.rom_sha256,
                           "scenario": scenario_id, "environment": environment_id,
                           "trace": trace_id, "arm": arm, "motivation": motivation,
                           "repeat_of": repeat_of})
        self.scenarios[scenario_id] = Scenario(
            scenario_id, environment_id, require_hash(trace_id), arm, motivation,
            repeat_of, graph_id)
        self.events[scenario_id] = []
        self.facts[scenario_id] = []
        self.edges[scenario_id] = []
        return self.scenarios[scenario_id]

    def _scenario(self, scenario_id):
        try:
            return self.scenarios[scenario_id]
        except KeyError as exc:
            raise ValueError("unknown scenario") from exc

    def add_event(self, scenario_id, event_id, frame, kind, payload=None):
        self._scenario(scenario_id)
        if not isinstance(event_id, str) or not event_id or type(frame) is not int or frame < 0:
            raise ValueError("invalid scenario event")
        if any(item["id"] == event_id for item in self.events[scenario_id]):
            raise ValueError("duplicate event in scenario")
        event = {"id": event_id, "frame": frame, "kind": kind, "payload": payload or {}}
        canonical(event)
        self.events[scenario_id].append(event)
        return event

    def add_fact(self, scenario_id, fact):
        self._scenario(scenario_id)
        item = dict(fact)
        required = {"kind", "location", "value", "status", "event_ref"}
        if set(item) != required:
            raise ValueError("scenario fact fields are incomplete")
        if item["status"] not in STATUSES | {"PROVISIONAL"}:
            raise ValueError("invalid scenario fact status")
        if item["event_ref"] not in {event["id"] for event in self.events[scenario_id]}:
            raise ValueError("fact event belongs to another scenario or is missing")
        canonical(item)
        item["id"] = digest({"kind": "thor-v6-fact", "scenario": scenario_id,
                              "fact": item})
        self.facts[scenario_id].append(item)
        return item["id"]

    def add_edge(self, scenario_id, source, target, role, status="UNKNOWN"):
        self._scenario(scenario_id)
        if status not in STATUSES | {"PROVISIONAL"}:
            raise ValueError("invalid edge status")
        event_ids = {item["id"] for item in self.events[scenario_id]}
        fact_ids = {item["id"] for item in self.facts[scenario_id]}
        if source not in event_ids | fact_ids or target not in event_ids | fact_ids:
            raise ValueError("edge endpoint is outside scenario graph")
        edge = {"id": digest({"kind": "thor-v6-edge", "scenario": scenario_id,
                               "source": source, "target": target, "role": role}),
                "source": source, "target": target, "role": role, "status": status}
        self.edges[scenario_id].append(edge)
        return edge["id"]

    @staticmethod
    def _fact_key(fact):
        return digest({key: fact[key] for key in ("kind", "location", "value", "status")})

    def compare(self):
        if len(self.scenarios) < 2:
            raise ValueError("differential comparison requires two scenarios")
        by_key = {}
        for scenario_id, facts in self.facts.items():
            for fact in facts:
                by_key.setdefault(self._fact_key(fact), []).append((scenario_id, fact))
        common, specific = [], {}
        for key, occurrences in sorted(by_key.items()):
            scenario_ids = sorted({item[0] for item in occurrences})
            entry = {"fact_key": key, "scenarios": scenario_ids,
                     "observation_only": True, "causal": False,
                     "event_ids": {sid: sorted(f["event_ref"] for s, f in occurrences if s == sid)
                                   for sid in scenario_ids}}
            if len(scenario_ids) == len(self.scenarios):
                common.append(entry)
            else:
                for scenario_id in scenario_ids:
                    specific.setdefault(scenario_id, []).append(entry)
        return {"common": common, "scenario_specific": {
                    sid: specific.get(sid, []) for sid in sorted(self.scenarios)},
                "cross_scenario_edges": [], "causal_claims": []}

    def export(self):
        comparison = self.compare() if len(self.scenarios) >= 2 else None
        return {"schema": "thor.evidence.v6.differential", "rom_sha256": self.rom_sha256,
                "scenarios": [_plain(asdict(item)) for item in
                               sorted(self.scenarios.values(), key=lambda x: x.id)],
                "graphs": {sid: {"events": sorted(self.events[sid], key=lambda x: (x["frame"], x["id"])),
                                 "facts": sorted(self.facts[sid], key=lambda x: x["id"]),
                                 "edges": sorted(self.edges[sid], key=lambda x: x["id"])}
                           for sid in sorted(self.scenarios)},
                "differential": comparison,
                "ownership": {"source_owned_delta": 0, "promotion": False}}
