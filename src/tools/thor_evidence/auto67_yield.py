"""Bounded AUTO67 knowledge-yield and anti-churn accounting."""

from __future__ import annotations

from collections import Counter, deque
import time
from typing import Any


YIELD_CLASSES = (
    "DUPLICATE_EXACT", "KNOWN_COMPLETE", "MERGED_NO_NEW_FACT",
    "NEW_CONTEXT", "NEW_EDGE", "NEW_BRANCH", "NEW_WRITER", "NEW_READER",
    "NEW_CONSUMER", "NEW_PRODUCER", "NEW_POINTER_TARGET", "NEW_SELECTOR_VALUE",
    "NEW_DOMAIN_MEMBER", "NEW_ROOT", "NEW_STRUCTURE_FACT",
    "INVESTIGATION_ADVANCED", "INVESTIGATION_CLOSED", "CONFLICT",
    "BOUNDED_UNRESOLVED_NO_GAIN",
)


def _key(*values: Any) -> str:
    return "|".join("<none>" if value is None else str(value) for value in values)


class YieldTracker:
    """Session ledger; it is bounded and is not a persistent Knowledge DB."""

    def __init__(self, history_size: int = 1000, chain_limit: int = 4096):
        self.history: deque[dict[str, Any]] = deque(maxlen=history_size)
        self.chain_limit = chain_limit
        self.chain_order: deque[str] = deque()
        self.chains: dict[str, dict[str, Any]] = {}
        self.knowledge = {name: set() for name in (
            "observations", "contexts", "edges", "nodes", "relations", "claims",
            "conflicts", "proof_advanced", "proof_closed", "structures_advanced",
            "structures_closed", "promotion_candidates", "branches", "writers",
            "readers", "consumers", "producers", "pointer_targets", "selectors",
            "domain_members", "roots")}
        self.class_counts = Counter()
        self.reject_counts = Counter()
        self.total_leases = 0
        self.useful_leases = 0
        self.total_facts = 0
        self.started_at = 0.0

    def _chain(self, branch: str) -> dict[str, Any]:
        item = self.chains.get(branch)
        if item is not None:
            return item
        if len(self.chains) >= self.chain_limit:
            old = self.chain_order.popleft()
            self.chains.pop(old, None)
        item = {"chain": branch, "times_seen": 0, "times_leased": 0,
                "unique_contexts": set(), "new_facts": 0, "new_edges": 0,
                "new_branches": 0, "investigation_advances": 0,
                "last_gain_frame": None, "consecutive_no_gain": 0,
                "current_status": "UNSEEN"}
        self.chains[branch] = item
        self.chain_order.append(branch)
        return item

    def observe(self, branch: str, context: str) -> None:
        item = self._chain(branch)
        item["times_seen"] += 1
        item["unique_contexts"].add(context)

    def reject_duplicate(self, branch: str, context: str, frame: int | None) -> None:
        self._chain(branch)
        self.reject_counts["DUPLICATE_EXACT"] += 1

    def _add(self, name: str, value: str, facts: list[str]) -> None:
        if value not in self.knowledge[name]:
            self.knowledge[name].add(value)
            facts.append(name)

    def record_return(self, branch: str, context: str, event: dict[str, Any],
                      status: str, frame: int | None) -> dict[str, Any]:
        self.total_leases += 1
        item = self._chain(branch)
        item["times_leased"] += 1
        item["unique_contexts"].add(context)
        facts: list[str] = []
        if status == "MERGED":
            item["consecutive_no_gain"] += 1
            item["current_status"] = "MERGED_NO_NEW_FACT"
            self.class_counts["MERGED_NO_NEW_FACT"] += 1
            result = {"primary_result": "MERGED_NO_NEW_FACT",
                      "facts_added_count": 0, "useful": False,
                      "knowledge_delta": self._delta([]),
                      "yield_class": "MERGED_NO_NEW_FACT"}
            self.history.append({"at": time.monotonic(), "frame": frame,
                                 "chain": branch, "context": context, **result})
            return result
        observation_fields = ("kind", "pc", "address", "caller_pc", "consumer_pc",
                              "source_address", "destination_address", "selector",
                              "index", "pointer_target", "rom_target", "ram_target",
                              "predecessor", "successor")
        observation_key = _key(*(event.get(name) for name in observation_fields))
        self._add("observations", observation_key, facts)
        context_was_new = context not in self.knowledge["contexts"]
        self._add("contexts", context, facts)
        edge_was_new = False
        relation = event.get("relation")
        if isinstance(relation, dict) and relation.get("source") is not None and \
                relation.get("target") is not None and relation.get("kind") is not None:
            edge = _key(relation["source"], relation["target"], relation["kind"])
            edge_was_new = edge not in self.knowledge["edges"]
            self._add("edges", edge, facts)
        branch_was_new = branch not in self.knowledge["branches"]
        self._add("branches", branch, facts)
        fields = (("writers", "writer_pc", "NEW_WRITER"),
                  ("readers", "reader_pc", "NEW_READER"),
                  ("consumers", "consumer_pc", "NEW_CONSUMER"),
                  ("producers", "producer_pc", "NEW_PRODUCER"),
                  ("pointer_targets", "pointer_target", "NEW_POINTER_TARGET"),
                  ("selectors", "selector", "NEW_SELECTOR_VALUE"),
                  ("domain_members", "domain_member", "NEW_DOMAIN_MEMBER"))
        field_classes: list[str] = []
        for name, field, result in fields:
            value = event.get(field)
            if value is not None:
                before = len(self.knowledge[name])
                self._add(name, _key(branch, value), facts)
                if len(self.knowledge[name]) > before:
                    field_classes.append(result)
        root_was_new = False
        if event.get("root") is True:
            root_was_new = branch not in self.knowledge["roots"]
            self._add("roots", branch, facts)
        if status == "CONFLICT":
            self._add("conflicts", _key(branch, context), facts)
        if status in {"WAITING_RUNTIME", "WAITING_STATIC", "BLOCKED"}:
            self._add("proof_advanced", _key(branch, context), facts)
        if status == "PROVEN":
            self._add("proof_closed", _key(branch, context), facts)
        structure_was_new = False
        if event.get("structure") is not None:
            structure_key = _key(branch, event["structure"])
            structure_was_new = structure_key not in self.knowledge["structures_advanced"]
            self._add("structures_advanced", _key(branch, event["structure"]), facts)
            if status == "PROVEN":
                self._add("structures_closed", _key(branch, event["structure"]), facts)
        if event.get("promotion_candidate"):
            self._add("promotion_candidates", _key(branch, context), facts)
        if status == "KNOWN":
            primary = "KNOWN_COMPLETE"
        elif status == "MERGED":
            primary = "MERGED_NO_NEW_FACT"
        elif status == "CONFLICT":
            primary = "CONFLICT"
        elif status == "PROVEN":
            primary = "INVESTIGATION_CLOSED"
        elif status in {"WAITING_RUNTIME", "WAITING_STATIC", "BLOCKED"}:
            primary = "INVESTIGATION_ADVANCED"
        elif field_classes:
            primary = field_classes[0]
        elif structure_was_new:
            primary = "NEW_STRUCTURE_FACT"
        elif root_was_new:
            primary = "NEW_ROOT"
        elif branch_was_new:
            primary = "NEW_BRANCH"
        elif context_was_new:
            primary = "NEW_CONTEXT"
        elif edge_was_new:
            primary = "NEW_EDGE"
        else:
            primary = "BOUNDED_UNRESOLVED_NO_GAIN"
        # Session novelty is evidence for accounting only. It is not durable
        # knowledge until the asynchronous sidecar classifies the result.
        useful = primary in {"INVESTIGATION_CLOSED", "CONFLICT"}
        if useful:
            self.useful_leases += 1
            item["new_facts"] += len(facts)
            item["new_edges"] += int(edge_was_new)
            item["new_branches"] += int(branch_was_new)
            item["investigation_advances"] += int(primary == "INVESTIGATION_ADVANCED")
            item["last_gain_frame"] = frame
            item["consecutive_no_gain"] = 0
        else:
            item["consecutive_no_gain"] += 1
        item["current_status"] = primary
        self.class_counts[primary] += 1
        self.total_facts += len(facts)
        result = {"primary_result": primary, "facts_added_count": len(facts),
                  "useful": useful, "knowledge_delta": self._delta(facts),
                  "yield_class": primary}
        self.history.append({"at": time.monotonic(), "frame": frame,
                             "chain": branch, "context": context,
                             **result})
        return result

    @staticmethod
    def _delta(facts: list[str]) -> dict[str, int]:
        names = ("observations", "contexts", "edges", "nodes", "relations", "claims",
                 "conflicts", "proof_advanced", "proof_closed", "structures_advanced",
                 "structures_closed", "promotion_candidates")
        return {name + "_added": facts.count(name) for name in names}

    def _ratio(self, values: list[dict[str, Any]]) -> dict[str, Any]:
        useful = sum(item["useful"] for item in values)
        total = len(values)
        return {"leases": total, "useful": useful, "no_gain": total - useful,
                "useful_percent": round(100 * useful / max(1, total), 3),
                "no_gain_percent": round(100 * (total - useful) / max(1, total), 3)}

    def snapshot(self, now: float | None = None) -> dict[str, Any]:
        recent = list(self.history)
        counts = {name: self.class_counts[name] for name in YIELD_CLASSES}
        useful_per_second = 0
        if now is not None:
            useful_per_second = sum(item["useful"] for item in recent
                                    if now - item["at"] <= 1.0)
        knowledge_delta = {name + "_added": len(values)
                           for name, values in self.knowledge.items()}
        churn_candidates = [item for item in self.chains.values()
                            if item["consecutive_no_gain"] > 0]
        top_churn = sorted(churn_candidates,
                           key=lambda item: (item["consecutive_no_gain"], item["times_leased"]),
                           reverse=True)[:10]
        top_yield = sorted(self.chains.values(),
                           key=lambda item: (item["new_facts"], item["new_edges"]),
                           reverse=True)[:10]
        def chain_view(item: dict[str, Any]) -> dict[str, Any]:
            return {key: (len(value) if key == "unique_contexts" else value)
                    for key, value in item.items() if key != "unique_contexts"} | {
                        "unique_contexts": len(item["unique_contexts"])}
        gain = {"unique_contexts": knowledge_delta.get("contexts_added", 0),
                "observations": knowledge_delta.get("observations_added", 0),
                "edges": knowledge_delta.get("edges_added", 0),
                "branches": len(self.knowledge["branches"]),
                "nodes": knowledge_delta.get("nodes_added", 0),
                "relations": knowledge_delta.get("relations_added", 0),
                "claims": knowledge_delta.get("claims_added", 0),
                "conflicts": knowledge_delta.get("conflicts_added", 0),
                "advanced_investigations": knowledge_delta.get("proof_advanced_added", 0),
                "closed_investigations": knowledge_delta.get("proof_closed_added", 0),
                "structures_advanced": knowledge_delta.get("structures_advanced_added", 0),
                "structures_closed": knowledge_delta.get("structures_closed_added", 0),
                "promotion_candidates": knowledge_delta.get("promotion_candidates_added", 0),
                "writers": knowledge_delta.get("writers_added", 0),
                "readers": knowledge_delta.get("readers_added", 0),
                "consumers": knowledge_delta.get("consumers_added", 0),
                "producers": knowledge_delta.get("producers_added", 0),
                "pointer_targets": knowledge_delta.get("pointer_targets_added", 0),
                "selector_values": knowledge_delta.get("selectors_added", 0),
                "domain_members": knowledge_delta.get("domain_members_added", 0),
                "roots": knowledge_delta.get("roots_added", 0)}
        ratios = {"last_100": self._ratio(recent[-100:]),
                  "last_1000": self._ratio(recent[-1000:]),
                  "whole_session": {"leases": self.total_leases,
                                    "useful": self.useful_leases,
                                    "no_gain": self.total_leases - self.useful_leases,
                                    "useful_percent": round(
                                        100 * self.useful_leases / max(1, self.total_leases), 3),
                                    "no_gain_percent": round(
                                        100 * (self.total_leases - self.useful_leases) /
                                        max(1, self.total_leases), 3)}}
        useful_ratio = ratios["whole_session"]["useful_percent"]
        classification = ("PRODUCTIVE" if useful_ratio >= 25 else
                          "CHURNING" if useful_ratio <= 5 and self.total_leases >= 100 else
                          "LOW_YIELD")
        return {"yield_per_second": useful_per_second, "total_leases": self.total_leases,
                "useful_leases": self.useful_leases, "no_gain_leases": self.total_leases - self.useful_leases,
                "exact_duplicate_rejects": self.reject_counts["DUPLICATE_EXACT"],
                "yield_classes": counts, "knowledge_delta": knowledge_delta,
                "knowledge_gain_per_1000_leases": {key: round(value * 1000 / max(1, self.total_leases), 3)
                                                    for key, value in gain.items()},
                "knowledge_gain_totals": gain, "ratios": ratios,
                "top_churn_chains": [chain_view(item) for item in top_churn],
                "top_productive_chains": [chain_view(item) for item in top_yield],
                "classification": classification,
                "ledger": "bounded_session_in_memory; durable gain requires the sidecar",
                "persistent_db_delta_available": False,
                "session_only": {"contexts": knowledge_delta.get("contexts_added", 0),
                                 "branches": knowledge_delta.get("branches_added", 0),
                                 "observations": knowledge_delta.get("observations_added", 0)}}
