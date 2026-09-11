"""Deterministic Stage 1 orchestration for the M12 ROM ownership map."""

from copy import deepcopy
import hashlib
import json
from collections import defaultdict
from m12_carver_adapters import ADAPTERS, infer_adapter


ROM_START = 0
ROM_END = 0x300000
SCHEMA = "oasis.m68k.m12-carver.interval-db.v1"
REPORT_SCHEMA = "oasis.m68k.m12-carver.stage1-report.v1"
UNKNOWN = {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"}
NON_CONFIRMED = {"PROBABLE", "CANDIDATE", "UNVERIFIED"}
CONTEXT_FIELDS = ("consumer", "parser", "pointer_table", "runtime_reader",
                  "structural_format", "resource_block")


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def stable_id(prefix, value):
    digest = hashlib.sha256(canonical(value).encode("utf-8")).hexdigest()[:20]
    return f"{prefix}:{digest}"


def parse_int(value):
    if isinstance(value, str):
        return int(value, 0)
    return int(value)


def bounds(item):
    if "range" in item:
        value = item["range"]
        if isinstance(value, str) and ".." in value:
            left, right = value.split("..", 1)
            return parse_int(left), parse_int(right)
        if isinstance(value, (list, tuple)) and len(value) == 2:
            return parse_int(value[0]), parse_int(value[1])
    if "start" in item and "end" in item:
        return parse_int(item["start"]), parse_int(item["end"])
    if "address" in item:
        start = parse_int(item["address"])
        return start, start + 1
    return None


def intersects(left, right):
    return left[0] < right[1] and right[0] < left[1]


def confirmed_manifest_entry(entry):
    confidence = str(entry.get("confidence", "")).upper()
    return entry.get("kind") not in UNKNOWN and confidence not in NON_CONFIRMED


def node_type(value):
    value = str(value or "").lower()
    if "runtime" in value or "reader" in value:
        return "runtime_reader"
    if "table" in value:
        return "table"
    if "resource" in value or "graphics" in value:
        return "resource"
    if "code" in value or "68000" in value or "z80" in value:
        return "code"
    if "consumer" in value or "parser" in value:
        return "consumer"
    return "producer"


class IntervalDB:
    """Coverage-preserving interval map plus typed evidence and graph state."""

    def __init__(self, manifest, manifest_path="manifest.json", rom_end=ROM_END):
        self.rom_end = rom_end
        self.manifest = deepcopy(manifest)
        self.manifest_path = str(manifest_path)
        self.ranges = []
        self.evidence = {}
        self.nodes = {}
        self.edges = {}
        self.conflicts = {}
        self.candidates = {}
        self.expansion_queue = []
        self.adapter_counts = defaultdict(int)
        self._import_manifest()

    @classmethod
    def from_manifest(cls, manifest, manifest_path="manifest.json", rom_end=ROM_END):
        return cls(manifest, manifest_path, rom_end)

    def _import_manifest(self):
        if self.manifest.get("schema") != "oasis.full-rom-split.v1":
            raise ValueError("unsupported M12 manifest schema")
        if parse_int(self.manifest.get("start", -1)) != ROM_START:
            raise ValueError("manifest start must be zero")
        if parse_int(self.manifest.get("end", -1)) != self.rom_end:
            raise ValueError("manifest end does not match Carver interval")
        if parse_int(self.manifest.get("rom_size", -1)) != self.rom_end:
            raise ValueError("manifest ROM size does not match Carver interval")
        cursor = ROM_START
        for index, entry in enumerate(self.manifest.get("entries", [])):
            start, end = parse_int(entry["start"]), parse_int(entry["end"])
            if start != cursor or end <= start or end > self.rom_end:
                raise ValueError("manifest has a gap, overlap, or invalid interval")
            cursor = end
            rid = f"range:{index:04d}:{start:06X}:{end:06X}"
            owned = confirmed_manifest_entry(entry)
            classification = entry.get("classification", entry.get("kind", "UNKNOWN"))
            record = {"id": rid, "start": start, "end": end,
                      "classification": classification,
                      "confidence": entry.get("confidence", "UNKNOWN"),
                      "evidence": [], "conflicts": [],
                      "parser": entry.get("parser"), "consumer": entry.get("consumer"),
                      "destination": entry.get("destination"),
                      "discovered_by": [entry.get("source", "manifest")],
                      "provenance_parents": [], "provenance_children": [],
                      "promotion_transaction": entry.get("promotion_transaction") or
                      entry.get("transaction") or self.manifest.get("transaction") or
                      entry.get("source"),
                      "source_owned": owned, "source_kind": entry.get("kind", "UNKNOWN"),
                      "source_manifest_entry": deepcopy(entry)}
            self.ranges.append(record)
            self.range_node(rid, record)
            evidence = {"id": f"evidence:manifest:{index:04d}", "type": "MANIFEST_RANGE",
                        "producer": "current_manifest", "confidence": "CONFIRMED" if owned else "UNKNOWN",
                        "start": start, "end": end, "range_id": rid,
                        "details": {"manifest_path": self.manifest_path, "manifest_index": index}}
            self.add_evidence(evidence, attach=False)
            record["evidence"].append(evidence["id"])
            self.add_edge("current_manifest", rid, "IMPORTS", [evidence["id"]])
        if cursor != self.rom_end:
            raise ValueError("manifest does not cover the requested interval")
        expected = self.manifest.get("metrics", {}).get("SOURCE_OWNED_BYTES")
        actual = self.source_owned_bytes()
        if expected is not None and int(expected) != actual:
            raise ValueError("manifest source-owned byte count changed during import")
        self.manifest_source_owned_bytes = int(expected) if expected is not None else actual

    def range_node(self, rid, record):
        self.nodes[rid] = {"id": rid, "type": "range", "start": record["start"],
                           "end": record["end"], "classification": record["classification"],
                           "confirmed": record["source_owned"]}

    def source_owned_bytes(self):
        return sum(r["end"] - r["start"] for r in self.ranges if r["source_owned"])

    def find_ranges(self, start, end):
        return [r for r in self.ranges if intersects((start, end), (r["start"], r["end"]))]

    def add_evidence(self, record, attach=True):
        normalized = deepcopy(record)
        start, end = bounds(normalized) or (None, None)
        if start is not None:
            if start < ROM_START or end <= start or end > self.rom_end:
                raise ValueError("evidence interval is outside the Carver interval")
            normalized.update({"start": start, "end": end})
        normalized.setdefault("id", stable_id("evidence", normalized))
        normalized.setdefault("type", "UNSPECIFIED")
        normalized.setdefault("producer", "unknown_producer")
        normalized.setdefault("confidence", "EVIDENCE_ONLY")
        normalized.setdefault("details", {})
        if normalized["id"] in self.evidence:
            if canonical(self.evidence[normalized["id"]]) != canonical(normalized):
                self.add_conflict("DUPLICATE_EVIDENCE_ID", normalized, normalized,
                                  "same evidence id has different content")
            return normalized["id"]
        self.evidence[normalized["id"]] = normalized
        producer = str(normalized["producer"])
        self.nodes.setdefault(producer, {"id": producer, "type": node_type(producer), "label": producer})
        self.nodes[normalized["id"]] = {"id": normalized["id"], "type": "evidence",
                                         "evidence_type": normalized["type"]}
        self.add_edge(producer, normalized["id"], "PRODUCES", [normalized["id"]])
        related = self.find_ranges(start, end) if start is not None else []
        if attach:
            for target in related:
                target["evidence"].append(normalized["id"])
                for field in ("parser", "consumer", "destination"):
                    if target[field] is None and normalized.get(field) is not None:
                        target[field] = normalized[field]
                self.add_edge(normalized["id"], target["id"], "SUPPORTS", [normalized["id"]])
        if normalized.get("consumer"):
            consumer = str(normalized["consumer"])
            self.nodes.setdefault(consumer, {"id": consumer, "type": "consumer", "label": consumer})
            self.add_edge(consumer, normalized["id"], "READS", [normalized["id"]])
        if normalized.get("parser"):
            parser = str(normalized["parser"])
            self.nodes.setdefault(parser, {"id": parser, "type": "consumer", "label": parser})
            self.add_edge(parser, normalized["id"], "PARSERS", [normalized["id"]])
        candidate = (str(normalized.get("confidence", "")).upper() in NON_CONFIRMED or
                     str(normalized.get("classification", "")).upper() in NON_CONFIRMED)
        if start is not None and candidate and not any(r["source_owned"] for r in related):
            self.candidates[normalized["id"]] = normalized
        if start is not None and candidate:
            for target in related:
                if target["source_owned"]:
                    self.add_conflict("CANDIDATE_OVERLAPS_CONFIRMED", normalized, target,
                                      "candidate range overlaps confirmed ownership",
                                      [normalized["id"]])
        return normalized["id"]

    def add_conflict(self, kind, left, right, reason, evidence_ids=None):
        conflict = {"type": kind, "left": bounds(left), "right": bounds(right),
                    "reason": reason, "evidence": sorted(evidence_ids or [])}
        cid = stable_id("conflict", conflict)
        conflict["id"] = cid
        conflict["blocking"] = True
        self.conflicts[cid] = conflict
        for record in self.find_ranges(*(bounds(left) or (0, 0))):
            record["conflicts"].append(cid)
        for record in self.find_ranges(*(bounds(right) or (0, 0))):
            record["conflicts"].append(cid)
        return cid

    def add_edge(self, source, target, edge_type, evidence_ids=None):
        edge = {"source": str(source), "target": str(target), "type": str(edge_type),
                "evidence": sorted(evidence_ids or [])}
        eid = stable_id("edge", edge)
        edge["id"] = eid
        self.nodes.setdefault(str(source), {"id": str(source), "type": node_type(source)})
        self.nodes.setdefault(str(target), {"id": str(target), "type": node_type(target)})
        self.edges[eid] = edge
        if str(source) in self.nodes and self.nodes[str(source)].get("type") == "range":
            target_range = next((r for r in self.ranges if r["id"] == str(target)), None)
            if target_range and str(source) not in target_range["provenance_parents"]:
                target_range["provenance_parents"].append(str(source))
        if str(target) in self.nodes and self.nodes[str(target)].get("type") == "range":
            source_range = next((r for r in self.ranges if r["id"] == str(source)), None)
            if source_range and str(target) not in source_range["provenance_children"]:
                source_range["provenance_children"].append(str(target))
        return eid

    def ingest(self, payload, source):
        adapter = infer_adapter(payload)
        adapted = ADAPTERS[adapter](payload, source)
        self.adapter_counts[adapter] += len(adapted["records"])
        for node in sorted(adapted.get("nodes", []), key=canonical):
            if isinstance(node, dict) and node.get("id"):
                self.nodes[str(node["id"])] = deepcopy(node)
        for record in sorted(adapted["records"], key=lambda item: canonical(item)):
            evidence_id = self.add_evidence(record)
            for parent in sorted(record.get("provenance_parents", record.get("parents", []))):
                parent_range = next((r for r in self.ranges if r["id"] == str(parent)), None)
                if parent_range is None or not parent_range["source_owned"]:
                    self.add_conflict("NON_CONFIRMED_PROVENANCE_PARENT", record, record,
                                      "child candidate requires a confirmed provenance parent",
                                      [evidence_id])
                    continue
                self.add_edge(str(parent), evidence_id, "DERIVES_CANDIDATE", [evidence_id])
        for edge in sorted(adapted.get("edges", []), key=canonical):
            if not isinstance(edge, dict) or not edge.get("source") or not edge.get("target"):
                continue
            if edge.get("type") == "DERIVES_CANDIDATE":
                parent = next((r for r in self.ranges if r["id"] == str(edge["source"])), None)
                if parent is None or not parent["source_owned"]:
                    self.add_conflict("NON_CONFIRMED_PROVENANCE_PARENT", edge, edge,
                                      "candidate edge requires a confirmed provenance parent",
                                      edge.get("evidence", []))
                    continue
            self.add_edge(edge["source"], edge["target"], edge.get("type", "RELATED"),
                          edge.get("evidence", []))
        for item in payload.get("expansion_queue", []):
            self.expansion_queue.append(deepcopy(item))
        return adapted["adapter"]

    def fixed_point(self):
        added = 0
        edges_before = len(self.edges)
        while self.expansion_queue:
            item = self.expansion_queue.pop(0)
            parent = item.get("parent")
            parent_range = next((r for r in self.ranges if r["id"] == str(parent)), None)
            if parent_range is None or not parent_range["source_owned"]:
                self.add_conflict("EXPANSION_PARENT_NOT_CONFIRMED", item, item,
                                  "expansion queue parent is not confirmed")
                continue
            self.add_evidence(item)
            added += 1
        return {"reached": not self.expansion_queue, "new_evidence": added,
                "range_splits": 0, "range_reclassifications": 0,
                "provenance_edges_added": len(self.edges) - edges_before,
                "expansion_queue": 0}

    def _related_evidence(self, gap):
        result = []
        for item in self.evidence.values():
            item_bounds = bounds(item)
            targets = item.get("targets", item.get("pointer_targets", item.get("xrefs", [])))
            target_hits = []
            for target in targets if isinstance(targets, list) else []:
                target_bounds = bounds(target) if isinstance(target, dict) else (parse_int(target), parse_int(target) + 1)
                if intersects((gap["start"], gap["end"]), target_bounds):
                    target_hits.append(target_bounds)
            if (item_bounds and intersects((gap["start"], gap["end"]), item_bounds)) or target_hits:
                result.append((item, target_hits))
        return result

    def gap_report(self):
        gaps = [r for r in self.ranges if r["source_kind"] in UNKNOWN]
        output = []
        for gap in gaps:
            related = self._related_evidence(gap)
            summaries = [self.evidence_summary(item) for item, _ in related]
            pointers = [dict(self.evidence_summary(item), targets=targets)
                        for item, targets in related if targets or
                        any(word in str(item.get("type", "")).lower() for word in ("pointer", "xref", "table"))]
            runtime = [summary for summary in summaries if item_is_runtime(self.evidence.get(summary["id"], {}))]
            detectors = [summary for summary in summaries if "detector" in str(summary["type"]).lower()]
            consumers = sorted({str(item.get("consumer") or item.get("parser"))
                                for item, _ in related if item.get("consumer") or item.get("parser")})
            output.append({"id": gap["id"], "start": gap["start"], "end": gap["end"],
                           "size": gap["end"] - gap["start"], "rank": 0,
                           "left_neighbor": self.neighbor(gap, -1),
                           "right_neighbor": self.neighbor(gap, 1),
                           "pointers_xrefs": pointers, "runtime_reads_executions": runtime,
                           "detector_hits": detectors, "known_consumers": consumers,
                           "evidence": summaries, "conflicts": sorted(gap["conflicts"])})
        campaigns = self.campaigns(output)
        rank = {item["id"]: index + 1 for index, item in enumerate(campaigns)}
        for gap in output:
            gap["rank"] = rank.get(next((c["id"] for c in campaigns if gap["id"] in c["gap_ids"]), ""), 0)
        return output, campaigns

    def neighbor(self, gap, direction):
        index = self.ranges.index(gap) + direction
        if 0 <= index < len(self.ranges):
            item = self.ranges[index]
            return {"id": item["id"], "start": item["start"], "end": item["end"],
                    "classification": item["classification"], "source_kind": item["source_kind"]}
        return None

    def evidence_summary(self, item):
        return {key: item[key] for key in ("id", "type", "producer", "confidence") if key in item} | {
            "start": item.get("start"), "end": item.get("end"),
            "consumer": item.get("consumer"), "parser": item.get("parser"),
            "source_ref": item.get("source_ref")}

    def campaigns(self, gaps):
        parent = {gap["id"]: gap["id"] for gap in gaps}
        def root(value):
            while parent[value] != value:
                parent[value] = parent[parent[value]]
                value = parent[value]
            return value
        def union(left, right):
            left, right = root(left), root(right)
            if left != right:
                parent[right] = left
        keys = defaultdict(list)
        for gap in gaps:
            for item in gap["evidence"]:
                record = self.evidence.get(item["id"], {})
                for field in CONTEXT_FIELDS:
                    value = record.get(field)
                    if value not in (None, "", []):
                        keys[(field, canonical(value))].append(gap["id"])
            resource_blocks = {self.evidence[item["id"]].get("resource_block")
                               for item in gap["evidence"]
                               if self.evidence[item["id"]].get("resource_block")}
            for other in gaps:
                if gap["end"] != other["start"] or not resource_blocks:
                    continue
                other_blocks = {self.evidence[item["id"]].get("resource_block")
                                for item in other["evidence"]
                                if self.evidence[item["id"]].get("resource_block")}
                if resource_blocks & other_blocks:
                    union(gap["id"], other["id"])
        for members in keys.values():
            for member in members[1:]:
                union(members[0], member)
        groups = defaultdict(list)
        for gap in gaps:
            groups[root(gap["id"])].append(gap)
        result = []
        for members in groups.values():
            member_ids = sorted(gap["id"] for gap in members)
            contexts = sorted({f"{field}={self.evidence[item['id']].get(field)}"
                               for gap in members for item in gap["evidence"]
                               for field in CONTEXT_FIELDS if self.evidence[item["id"]].get(field) not in (None, "", [])})
            size = sum(gap["size"] for gap in members)
            score = size + sum(100 for gap in members if gap["runtime_reads_executions"])
            score += sum(80 for gap in members if gap["pointers_xrefs"])
            score += sum(70 for gap in members if gap["known_consumers"])
            score += sum(20 for gap in members if gap["detector_hits"])
            conflicts = sorted({cid for gap in members for cid in gap["conflicts"]})
            result.append({"id": stable_id("campaign", member_ids), "gap_ids": member_ids,
                           "start": min(gap["start"] for gap in members),
                           "end": max(gap["end"] for gap in members), "size": size,
                           "score": score, "rank": 0, "contexts": contexts,
                           "conflicts": conflicts, "promotion_blocked": bool(conflicts)})
        result.sort(key=lambda item: (-item["score"], -item["size"], item["start"], item["id"]))
        for index, campaign in enumerate(result, 1):
            campaign["rank"] = index
        return result

    def report(self):
        fixed = self.fixed_point()
        gaps, campaigns = self.gap_report()
        recommended = next((item["id"] for item in campaigns if not item["promotion_blocked"]), None)
        return {"schema": REPORT_SCHEMA, "deterministic": True,
                "fixed_point": fixed, "interval_coverage": {
                    "start": ROM_START, "end": self.rom_end, "total_bytes": self.rom_end,
                    "gaps": 0, "overlaps": 0},
                "source_owned_bytes": self.source_owned_bytes(),
                "source_owned_unchanged": self.source_owned_bytes() == self.manifest_source_owned_bytes,
                "evidence_records": len(self.evidence), "provenance_nodes": len(self.nodes),
                "provenance_edges": len(self.edges), "conflicts": sorted(self.conflicts.values(), key=canonical),
                "candidate_ranges": [self.evidence_summary(item) for item in
                                     sorted(self.candidates.values(), key=lambda x: x["id"])],
                "gaps": gaps, "campaigns": campaigns,
                "recommended_next_campaign": recommended,
                "adapters": dict(sorted(self.adapter_counts.items()))}

    def interval_db(self):
        self.fixed_point()
        return {"schema": SCHEMA, "rom_sha256": self.manifest.get("rom_sha256"),
                "rom_size": self.rom_end, "start": ROM_START, "end": self.rom_end,
                "ranges": sorted(self.ranges, key=lambda item: item["start"]),
                "evidence_records": [self.evidence[key] for key in sorted(self.evidence)],
                "provenance": {"nodes": [self.nodes[key] for key in sorted(self.nodes)],
                                "edges": [self.edges[key] for key in sorted(self.edges)]},
                "conflicts": [self.conflicts[key] for key in sorted(self.conflicts)],
                "promotion_policy": {"confirmed_only": True, "conflicts_block_promotion": True,
                                      "carver_stage": 1, "creates_source_owned": False}}
def item_is_runtime(item):
    text = str(item.get("type", "")).lower()
    return bool(item.get("runtime") or "runtime" in text or "read" in text or "execut" in text)
