"""Build a bounded, read-only provenance trace for selected ROM ranges."""

from __future__ import annotations

import json
from bisect import bisect_left, bisect_right
from collections import defaultdict, deque
from typing import Any, Iterable


RUNTIME_EDGE_TYPES = {"EXECUTED_NEXT", "OBSERVED_NEXT_PC"}
PATH_KEYS = ("path", "output_path", "artifact_path", "file")
HASH_KEYS = ("sha256", "output_sha256", "artifact_sha256")


class RangeTraceIndex:
    """Join canonical provenance by IDs and explicit intervals only."""

    @classmethod
    def from_sqlite(cls, connection, rom_sha256: str, rom_size: int):
        def rows(table: str):
            exists = connection.execute(
                "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?", (table,)
            ).fetchone()
            if not exists:
                return iter(())
            return (dict(row) for row in connection.execute(f'SELECT * FROM "{table}"'))

        ranges = (row for row in rows("rom_range")
                  if row.get("rom_sha256") == rom_sha256 and int(row["start"]) < rom_size)
        return cls(ranges, rows("rom_object"), rows("claim"), rows("relation"),
                   rows("evidence_ref"), rows("source_artifact"), rows("emission"),
                   rows("derivation"), rows("derivation_input"), rom_size)

    def __init__(self, ranges: Iterable[dict[str, Any]],
                 objects: Iterable[dict[str, Any]],
                 claims: Iterable[dict[str, Any]],
                 relations: Iterable[dict[str, Any]],
                 evidence_refs: Iterable[dict[str, Any]],
                 source_artifacts: Iterable[dict[str, Any]],
                 emissions: Iterable[dict[str, Any]],
                 derivations: Iterable[dict[str, Any]],
                 derivation_inputs: Iterable[dict[str, Any]],
                 rom_size: int | None = None):
        self.ranges: dict[str, tuple[int, int]] = {}
        for row in ranges:
            start, end = int(row["start"]), int(row["end"])
            if rom_size is not None:
                end = min(end, rom_size)
            if start >= 0 and end > start:
                self.ranges[str(row["range_id"])] = (start, end)

        self.objects: dict[str, str] = {}
        self.object_at: dict[str, tuple[int, int]] = {}
        for row in objects:
            object_id = str(row["object_id"])
            bounds = self.ranges.get(str(row["range_id"]))
            if bounds:
                self.objects[object_id] = str(row["object_type"])
                self.object_at[object_id] = bounds
        self.object_intervals = sorted(
            (bounds[0], bounds[1], object_id) for object_id, bounds in self.object_at.items())
        self.object_starts, self.object_prefix_ends = _interval_index(self.object_intervals)

        sources = {str(row["source_sha256"]): (
            str(row["artifact_type"]), str(row["artifact_name"]))
            for row in source_artifacts}
        claim_rows = []
        claim_owner: dict[str, str] = {}
        needed_subjects = set(self.objects)
        for row in claims:
            object_id = str(row["object_id"])
            if object_id in self.objects:
                compact = (str(row["claim_id"]), object_id, str(row["claim_type"]),
                           str(row["status"]), str(row["value_json"]))
                claim_rows.append(compact)
                needed_subjects.add(compact[0])
                claim_owner[compact[0]] = object_id

        direct_relations = []
        runtime_edges = []
        relation_owners: dict[str, set[str]] = {}
        for row in relations:
            relation_id = str(row["relation_id"])
            source = str(row.get("source_object_id") or "")
            target = str(row.get("target_object_id") or "")
            relation_type = str(row.get("relation_type") or "")
            address = row.get("target_address")
            status = str(row.get("status") or "UNKNOWN")
            if ({source, target} & self.objects.keys()) or relation_type in RUNTIME_EDGE_TYPES:
                needed_subjects.add(relation_id)
            owners = {source, target} & self.objects.keys()
            if owners:
                relation_owners[relation_id] = owners
            if {source, target} & self.objects.keys():
                direct_relations.append((relation_id, relation_type, source, target,
                                         address, status))
            if relation_type in RUNTIME_EDGE_TYPES and source and target:
                runtime_edges.append((relation_id, relation_type, source, target, status))

        refs_by_subject: dict[str, list[str]] = defaultdict(list)
        runtime_refs: dict[str, list[str]] = defaultdict(list)
        for row in evidence_refs:
            subject_id = str(row["subject_id"])
            if subject_id not in needed_subjects:
                continue
            source_hash = str(row["source_sha256"])
            artifact_type, artifact_name = sources.get(
                source_hash, ("UNKNOWN", "source artifact unavailable"))
            prefix = (f"EVIDENCE {row['ref_id']} · {row['fact_kind']} · "
                      f"{artifact_type}:{artifact_name} · sha256={source_hash[:16]}…")
            refs_by_subject[subject_id].append(prefix)
            if str(row["fact_kind"]).startswith("RUNTIME"):
                refs_by_subject[subject_id].append(
                    "  runtime witness · " +
                    _runtime_locator_summary(str(row["locator_json"])))
                runtime_refs[subject_id].append(
                    f"witness {row['ref_id']} "
                    f"{_runtime_locator_summary(str(row['locator_json']))}")

        self.claim_lines: dict[str, list[str]] = defaultdict(list)
        for claim_id, object_id, claim_type, status, value_json in claim_rows:
            self.claim_lines[object_id].append(
                f"CLAIM {claim_id} · {claim_type} · {status} · {_short(value_json)}")
            self.claim_lines[object_id].extend(
                "  " + line for line in refs_by_subject.get(claim_id, ()))

        self.relation_lines: dict[str, list[str]] = defaultdict(list)
        self.runtime_adjacency: dict[str, list[tuple[str, str]]] = defaultdict(list)
        for relation_id, relation_type, source, target, address, status in direct_relations:
            destination = target or (f"{int(address):#08x}" if address is not None
                                     else "unresolved target")
            label = (f"RELATION {relation_id} · {relation_type} · {status} · "
                     f"{source} -> {destination}")
            relation_refs = refs_by_subject.get(relation_id, ())
            for object_id in {source, target} & self.objects.keys():
                self.relation_lines[object_id].append(label)
                self.relation_lines[object_id].extend(
                    "  " + line for line in relation_refs)
        for relation_id, relation_type, source, target, status in runtime_edges:
            witnesses = runtime_refs.get(relation_id)
            if witnesses:
                edge = (f"{source} -[{relation_type}/{status}]-> {target} "
                        f"(relation {relation_id}; {'; '.join(witnesses[:2])})")
                self.runtime_adjacency[source].append((target, edge))

        subject_owners = {object_id: {object_id} for object_id in self.objects}
        subject_owners.update({key: {value} for key, value in claim_owner.items()})
        subject_owners.update(relation_owners)
        inputs: dict[str, set[str]] = defaultdict(set)
        input_labels: dict[str, list[tuple[str, str]]] = defaultdict(list)
        for row in derivation_inputs:
            derivation_id = str(row["derivation_id"])
            subject_id = str(row["subject_id"])
            linked = subject_owners.get(subject_id, set())
            inputs[derivation_id].update(linked)
            label = (f"INPUT {row['subject_type']}:{subject_id} role={row['role']}")
            input_labels[derivation_id].extend((object_id, label) for object_id in linked)
        self.derivation_lines: dict[str, list[str]] = defaultdict(list)
        for row in derivations:
            derivation_id = str(row["derivation_id"])
            linked = inputs.get(derivation_id, set())
            output_id = str(row["output_id"])
            output_objects = subject_owners.get(output_id, set())
            for object_id in linked | output_objects:
                input_chain = [label for owner, label in input_labels.get(derivation_id, ())
                               if owner == object_id]
                self.derivation_lines[object_id].extend(
                    _derivation_lines(row, input_chain))

        emission_rows = []
        for row in emissions:
            start, end = int(row["start"]), int(row["end"])
            if start >= 0 and end > start:
                label = (f"CANONICAL EMISSION [{start:#08x},{end:#08x}) · "
                         f"{row['classification']} · {row['artifact_type']} · {row['artifact']}")
                emission_rows.append((start, end, label))
        self.emission_intervals = sorted(emission_rows)
        self.emission_starts, self.emission_prefix_ends = _interval_index(
            self.emission_intervals)
        self.object_refs = {key: tuple(value) for key, value in refs_by_subject.items()}

    def describe(self, start: int, end: int, line_limit: int = 32) -> tuple[str, ...]:
        object_rows = _overlapping(self.object_intervals, self.object_starts,
                                   self.object_prefix_ends, start, end)
        object_ids = [object_id for _left, _right, object_id in object_rows]
        lines: list[str] = []
        for object_id in object_ids:
            left, right = self.object_at[object_id]
            lines.append(f"OBJECT {object_id} · {self.objects[object_id]} · "
                         f"[{left:#08x},{right:#08x})")
            lines.extend("  " + line for line in self.claim_lines.get(object_id, ()))
            lines.extend("  " + line for line in self.object_refs.get(object_id, ()))
            lines.extend("  " + line for line in self.relation_lines.get(object_id, ()))
            lines.extend("  " + line for line in self.derivation_lines.get(object_id, ()))
        for _left, _right, label in _overlapping(
                self.emission_intervals, self.emission_starts,
                self.emission_prefix_ends, start, end):
            lines.append(label)
        lines.extend(self._runtime_path_segments(set(object_ids)))
        if not any(line.startswith("RECORDED EXTRACTION OUTPUT REFERENCE")
                   for line in lines):
            lines.append("RECORDED EXTRACTION OUTPUT · no linked derivation contains "
                         "both an output path and SHA-256 in this generation")
        unique = tuple(dict.fromkeys(lines))
        if len(unique) <= line_limit:
            return unique
        priority = tuple(line for line in unique if line.startswith((
            "RUNTIME PATH SEGMENT", "RUNTIME PATH SEGMENTS",
            "RECORDED EXTRACTION OUTPUT")))
        ordinary = tuple(line for line in unique if line not in priority)
        kept = ordinary[:max(0, line_limit - len(priority))] + priority[:line_limit]
        return kept + (f"… {len(unique) - len(kept)} additional trace rows omitted",)

    def _runtime_path_segments(self, object_ids: set[str]) -> list[str]:
        lines: list[str] = []
        expanded = 0
        truncated = False
        for root in sorted(object_ids):
            frontier = deque([(root, (), frozenset((root,)))])
            while frontier and len(lines) < 6 and expanded < 500:
                current, path, visited = frontier.popleft()
                expanded += 1
                outgoing = self.runtime_adjacency.get(current, ())
                if path and not outgoing:
                    lines.append("RUNTIME PATH SEGMENT · " + " -> ".join(path))
                    continue
                if len(path) >= 12:
                    if path:
                        lines.append("RUNTIME PATH SEGMENT (depth limit) · " +
                                     " -> ".join(path))
                    continue
                extended = False
                for target, edge in outgoing:
                    if target in visited:
                        continue
                    extended = True
                    frontier.append((target, path + (edge,), visited | {target}))
                if path and not extended:
                    lines.append("RUNTIME PATH SEGMENT (cycle/branch limit) · " +
                                 " -> ".join(path))
            if frontier:
                truncated = True
            if len(lines) >= 6:
                truncated = True
                break
        if truncated:
            lines.append("RUNTIME PATH SEGMENTS · additional branches omitted by viewer limit")
        return lines or ["RUNTIME PATH · no linked, evidence-backed execution edge"]


def _runtime_locator_summary(locator_json: str) -> str:
    try:
        locator = json.loads(locator_json)
    except (TypeError, json.JSONDecodeError):
        return "locator malformed"
    if not isinstance(locator, dict):
        return "locator is not an object"
    event = locator.get("event") if isinstance(locator.get("event"), dict) else locator
    fields = []
    for key in ("capture_id", "run_id", "epoch", "cpu_id", "event_kind", "pc",
                "native_sequence", "instruction_sequence", "occurrence_id"):
        value = event.get(key, locator.get(key))
        if value is not None:
            shown = f"{int(value):#x}" if key == "pc" and isinstance(value, int) else str(value)
            fields.append(f"{key}={shown}")
    captures = locator.get("capture_ids")
    if isinstance(captures, list) and captures:
        fields.append("capture_ids=" + ",".join(str(item) for item in captures[:3]))
    windows = locator.get("windows")
    if isinstance(windows, list):
        for window in windows[:2]:
            if isinstance(window, dict) and window.get("capture_id") is not None:
                fields.append(f"window={window['capture_id']}/worker-"
                              f"{window.get('worker_id')}/gen-{window.get('generation')}")
    return ", ".join(fields) if fields else "identity fields unavailable"


def _derivation_lines(row: dict[str, Any], input_chain: list[str]) -> list[str]:
    lines = [f"DERIVATION {row['derivation_id']} · {row['rule_id']}@{row['rule_version']} · "
             f"{row['output_type']}:{row['output_id']} · impl={str(row['implementation_hash'])[:16]}…"]
    lines.extend("  " + item for item in input_chain)
    try:
        result = json.loads(str(row["result_json"]))
    except (TypeError, json.JSONDecodeError):
        result = None
    if isinstance(result, dict):
        path = next((result[key] for key in PATH_KEYS if isinstance(result.get(key), str)), None)
        digest = next((result[key] for key in HASH_KEYS if isinstance(result.get(key), str)), None)
        if (path and digest and len(digest) == 64 and
                all(char in "0123456789abcdefABCDEF" for char in digest)):
            lines.append("RECORDED EXTRACTION OUTPUT REFERENCE · "
                         f"{path} · sha256={digest} · file not verified by viewer")
    return lines


def _short(value: Any) -> str:
    try:
        text = json.dumps(json.loads(str(value)), ensure_ascii=False,
                          sort_keys=True, separators=(",", ":"))
    except (TypeError, json.JSONDecodeError):
        text = str(value)
    return text[:120]


def _interval_index(rows: list[tuple]) -> tuple[list[int], list[int]]:
    starts, prefix_ends = [], []
    maximum = -1
    for row in rows:
        starts.append(row[0])
        maximum = max(maximum, row[1])
        prefix_ends.append(maximum)
    return starts, prefix_ends


def _overlapping(rows: list[tuple], starts: list[int], prefix_ends: list[int],
                 start: int, end: int) -> list[tuple]:
    if not rows:
        return []
    right = bisect_left(starts, end)
    left = bisect_right(prefix_ends, start, 0, right)
    return [row for row in rows[left:right] if row[1] > start]
