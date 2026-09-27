"""Pure proof rules for M12 MAP-driven executed ASM closure."""

from __future__ import annotations

import copy
import hashlib
import json
from typing import Any


def stable_id(rom_sha256: str, intervals: list[tuple[int, int]]) -> str:
    payload = json.dumps({"rom_sha256": rom_sha256,
                          "intervals": [[a, b] for a, b in intervals]},
                         sort_keys=True, separators=(",", ":"))
    return "m12-2f:" + hashlib.sha256(payload.encode()).hexdigest()


def canonical_hash(value: Any) -> str:
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"),
                          ensure_ascii=True).encode()
    return hashlib.sha256(encoded).hexdigest()


def verify_roundtrip(expected: bytes, rebuilt: bytes) -> dict[str, Any]:
    if expected == rebuilt:
        return {"status": "PASS_ASM_ROUNDTRIP_EXACT", "expected_bytes": len(expected),
                "actual_bytes": len(rebuilt), "sha256": hashlib.sha256(rebuilt).hexdigest()}
    common = min(len(expected), len(rebuilt))
    offset = next((i for i in range(common) if expected[i] != rebuilt[i]), common)
    return {"status": "STOP_ASM_ROUNDTRIP_MISMATCH", "expected_bytes": len(expected),
            "actual_bytes": len(rebuilt), "first_difference": offset}


def validate_partition(entries: list[dict[str, Any]], rom_size: int) -> None:
    cursor = 0
    for entry in sorted(entries, key=lambda item: int(item["start"])):
        start, end = int(entry["start"]), int(entry["end"])
        if start != cursor or end <= start or end > rom_size:
            raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")
        cursor = end
    if cursor != rom_size:
        raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")


def observed_chains(instructions: list[dict[str, Any]],
                    edges: list[dict[str, Any]], rom_sha256: str) -> list[dict[str, Any]]:
    """Group only exact adjacent ranges linked by observed runtime edges."""
    by_id = {str(row["object_id"]): row for row in instructions}
    outgoing: dict[str, set[str]] = {oid: set() for oid in by_id}
    incoming: dict[str, set[str]] = {oid: set() for oid in by_id}
    accepted_edges: set[tuple[str, str]] = set()
    for edge in edges:
        source, target = str(edge["source_object_id"]), str(edge["target_object_id"])
        if source not in by_id or target not in by_id:
            continue
        if int(by_id[source]["end"]) != int(by_id[target]["start"]):
            continue
        accepted_edges.add((source, target))
        outgoing[source].add(target)
        incoming[target].add(source)

    paths: list[list[str]] = []
    visited_edges: set[tuple[str, str]] = set()
    heads = sorted((oid for oid in by_id if not incoming[oid]),
                   key=lambda oid: (int(by_id[oid]["start"]), oid))
    for head in heads:
        path = [head]
        current = head
        while len(outgoing[current]) == 1:
            target = next(iter(outgoing[current]))
            edge = (current, target)
            if edge in visited_edges or target in path:
                break
            visited_edges.add(edge)
            path.append(target)
            current = target
        paths.append(path)

    # Handle directed cycles and branch remnants deterministically.
    for source, target in sorted(accepted_edges):
        if (source, target) in visited_edges:
            continue
        path = [source]
        current = source
        seen = {source}
        while len(outgoing[current]) == 1:
            next_id = next(iter(outgoing[current]))
            edge = (current, next_id)
            if edge in visited_edges or next_id in seen:
                break
            visited_edges.add(edge)
            path.append(next_id)
            seen.add(next_id)
            current = next_id
        paths.append(path)

    unique: dict[tuple[str, ...], dict[str, Any]] = {}
    for path in paths:
        ids = tuple(path)
        if ids in unique:
            continue
        rows = sorted((by_id[oid] for oid in path), key=lambda row: int(row["start"]))
        intervals = [(int(rows[0]["start"]), int(rows[-1]["end"]))]
        unique[ids] = {"candidate_id": stable_id(rom_sha256, intervals),
                       "rom_sha256": rom_sha256,
                       "intervals": intervals, "instruction_ids": list(ids),
                       "instructions": rows, "observed_edge_count": max(0, len(ids) - 1)}
    return sorted(unique.values(), key=lambda item: (item["intervals"], item["candidate_id"]))


def classify_target(target: int, interval: tuple[int, int],
                    owned_ranges: list[tuple[int, int]],
                    candidate_starts: set[int]) -> str:
    if interval[0] <= target < interval[1]:
        return "INTERNAL"
    if any(start <= target < end for start, end in owned_ranges):
        return "SOURCE_OWNED"
    if target in candidate_starts:
        return "CANDIDATE_ISLAND"
    return "UNKNOWN"


def dependency_closed_starts(candidates: list[dict[str, Any]]) -> tuple[set[int], dict[int, list[int]]]:
    """Keep only exact candidates whose candidate targets also pass every gate."""
    by_start = {int(item["start"]): item for item in candidates}
    selected = set(by_start)
    dependencies = {start: sorted({int(edge["target"]) for edge in item.get("edges", [])
        if edge.get("status") == "DERIVED_EXACT" and
        edge.get("classification") == "CANDIDATE_ISLAND"})
        for start, item in by_start.items()}
    changed = True
    while changed:
        changed = False
        for start in sorted(selected):
            missing = [target for target in dependencies[start] if target not in selected]
            if missing:
                selected.remove(start)
                changed = True
    return selected, dependencies


def prove_decoded_island(candidate: dict[str, Any], decoded: dict[str, Any],
                         rom: bytes, owned_ranges: list[tuple[int, int]],
                         data_ranges: list[tuple[int, int]],
                         candidate_starts: set[int] | None = None,
                         observed_edges: list[tuple[int, int]] | None = None) -> dict[str, Any]:
    """Check bytes, map boundaries and every direct/fallthrough control edge."""
    candidate_starts = candidate_starts or set()
    interval = tuple(candidate["intervals"][0])
    start, end = interval
    reasons: list[str] = []
    if any(start < data_end and data_start < end
           for data_start, data_end in data_ranges):
        reasons.append("STOP_MIXED_CODE_DATA_BOUNDARY")
    if decoded.get("source_decoder") != "re_slice_decoder" or \
            decoded.get("start") != start or decoded.get("end") != end:
        reasons.append("STOP_BOUNDARY_UNPROVEN")

    expected = sorted(candidate["instructions"], key=lambda row: int(row["start"]))
    if not expected:
        reasons.append("STOP_BOUNDARY_UNPROVEN")
        return {"status": reasons[0], "blockers": reasons, "edges": []}
    instructions = decoded.get("instructions", [])
    expected_addresses = [int(row["start"]) for row in expected]
    actual_addresses = [int(row["address"]) for row in instructions]
    if actual_addresses != expected_addresses:
        reasons.append("STOP_BOUNDARY_UNPROVEN")

    cursor = start
    edges: list[dict[str, Any]] = []
    for index, instruction in enumerate(instructions):
        address = int(instruction["address"])
        words = instruction.get("raw_words", [])
        raw = b"".join(int(word).to_bytes(2, "big") for word in words)
        instruction_end = address + len(raw)
        if not instruction.get("supported", False) or instruction.get("flow") == "unsupported":
            reasons.append("STOP_UNSUPPORTED_M68K_DECODE")
        if address != cursor or instruction_end > end or len(raw) < 2:
            reasons.append("STOP_BOUNDARY_UNPROVEN")
        if address < 0 or instruction_end > len(rom) or rom[address:instruction_end] != raw:
            reasons.append("STOP_CANONICAL_BYTES_MISMATCH")
        if index < len(expected):
            row = expected[index]
            if address != int(row["start"]) or instruction_end != int(row["end"]) or \
                    int(instruction["opcode"]) != int(row["attributes"]["opcode"]) or \
                    hashlib.sha256(raw).hexdigest() != row["attributes"]["bytes_sha256"]:
                reasons.append("STOP_MAP_INSTRUCTION_IDENTITY_MISMATCH")
        cursor = instruction_end

        flow = instruction.get("flow")
        operation = str(instruction.get("operation", "")).lower()
        target = instruction.get("branch_target")
        if flow in {"indirect_call", "indirect_jump"}:
            reasons.append("STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED")
            continue
        if flow == "unsupported":
            continue
        if flow in {"direct_branch", "direct_call", "direct_jump"}:
            if target is None:
                reasons.append("STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED")
            else:
                target = int(target)
                classification = classify_target(target, interval, owned_ranges, candidate_starts)
                edges.append({"source": address, "target": target,
                              "kind": "DIRECT_ENCODED", "status": "DERIVED_EXACT",
                              "classification": classification})
                if classification == "INTERNAL" and target not in actual_addresses:
                    reasons.append("STOP_BOUNDARY_UNPROVEN")
                elif classification == "UNKNOWN":
                    reasons.append("STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED")
        elif flow == "return":
            continue
        elif flow != "none":
            reasons.append("STOP_UNSUPPORTED_M68K_DECODE")
            continue

        has_fallthrough = flow == "none" or flow == "direct_call" or \
            (flow == "direct_branch" and operation not in {"bra"})
        if has_fallthrough:
            classification = classify_target(instruction_end, interval, owned_ranges,
                                             candidate_starts)
            edges.append({"source": address, "target": instruction_end,
                          "kind": "FALLTHROUGH", "status": "DERIVED_EXACT",
                          "classification": classification})
            if classification == "INTERNAL" and instruction_end not in actual_addresses:
                reasons.append("STOP_BOUNDARY_UNPROVEN")
            elif classification == "UNKNOWN":
                reasons.append("STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED")

    if cursor != end or (start, end) != (int(expected[0]["start"]), int(expected[-1]["end"])):
        reasons.append("STOP_BOUNDARY_UNPROVEN")
    # Runtime status comes only from MAP relations; decode-derived edges stay static.
    for source, target in observed_edges or []:
        edges.append({"source": int(source), "target": int(target),
                      "kind": "EXECUTED_NEXT", "status": "OBSERVED_RUNTIME"})
    reasons = sorted(set(reasons))
    return {"status": "PASS_CLOSED_ASM_RANGE" if not reasons else reasons[0],
            "blockers": reasons, "edges": sorted(edges, key=lambda edge:
                (edge["source"], edge["target"], edge["kind"], edge["status"]))}


def merge_overlapping_candidates(candidates: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Deduplicate same interval candidates; merge overlaps only at exact boundaries."""
    ordered = sorted(candidates, key=lambda item: (item["intervals"], item["candidate_id"]))
    merged: list[dict[str, Any]] = []
    for candidate in ordered:
        current = copy.deepcopy(candidate)
        if not merged or current["intervals"][0][0] >= merged[-1]["intervals"][-1][1]:
            merged.append(current)
            continue
        prior = merged[-1]
        ranges = prior["intervals"] + current["intervals"]
        boundaries = {int(row["start"]) for row in prior.get("instructions", []) +
                      current.get("instructions", [])}
        starts = sorted({int(row["start"]) for row in
                         prior.get("instructions", []) + current.get("instructions", [])})
        ends = {int(row["end"]) for row in prior.get("instructions", []) +
                current.get("instructions", [])}
        for start, end in ranges:
            if start not in boundaries or end not in ends:
                raise ValueError("STOP_OVERLAPPING_OBJECT_CONFLICT")
        combined = {int(row["start"]): row for row in
                    prior.get("instructions", []) + current.get("instructions", [])}
        union_start, union_end = min(a for a, _ in ranges), max(b for _, b in ranges)
        cursor = union_start
        for start in starts:
            row = combined[start]
            if start != cursor:
                raise ValueError("STOP_OVERLAPPING_OBJECT_CONFLICT")
            cursor = int(row["end"])
        if cursor != union_end:
            raise ValueError("STOP_OVERLAPPING_OBJECT_CONFLICT")
        rows = [combined[start] for start in starts]
        interval = [(union_start, union_end)]
        ids = sorted(set(prior.get("instruction_ids", []) + current.get("instruction_ids", [])))
        rom_sha256 = str(current.get("rom_sha256", prior.get("rom_sha256", "")))
        merged[-1] = {"candidate_id": stable_id(rom_sha256, interval),
                      "rom_sha256": rom_sha256,
                      "intervals": interval, "instruction_ids": ids, "instructions": rows,
                      "observed_edge_count": max(prior.get("observed_edge_count", 0),
                                                  current.get("observed_edge_count", 0))}
    return merged


def promote_entries(entries: list[dict[str, Any]], promotions: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Return an isolated exact-range manifest update; input remains untouched."""
    current = copy.deepcopy(sorted(entries, key=lambda item: int(item["start"])))
    ranges = sorted((int(p["start"]), int(p["end"])) for p in promotions)
    if len(set(ranges)) != len(ranges) or any(a[1] > b[0] for a, b in zip(ranges, ranges[1:])):
        raise ValueError("STOP_OVERLAPPING_OBJECT_CONFLICT")
    for start, end in ranges:
        already = next((entry for entry in current if int(entry["start"]) == start and
                        int(entry["end"]) == end and
                        entry.get("source") == "M12_MAP_DRIVEN_EXECUTED_ASM_CLOSURE_2F"), None)
        if already:
            continue
        replacement = None
        for index, entry in enumerate(current):
            if entry.get("kind") == "UNKNOWN" and int(entry["start"]) <= start <= end <= int(entry["end"]):
                replacement = index
                break
        if replacement is None:
            raise ValueError("STOP_ALREADY_OWNED_NO_DELTA")
        old = current[replacement]
        inserted = []
        if int(old["start"]) < start:
            inserted.append({**old, "end": start})
        promotion = next(p for p in promotions if int(p["start"]) == start and int(p["end"]) == end)
        inserted.append({"start": start, "end": end, "kind": "CODE_VERIFIED",
            "source": "M12_MAP_DRIVEN_EXECUTED_ASM_CLOSURE_2F", "confidence": "CONFIRMED",
            "classification": "MAP_DRIVEN_EXECUTED_ASM_STATIC_VERIFIED",
            "emitted_artifact_type": "asm", "artifact": promotion["artifact"],
            "ownership_reason": "Exact MAP lineage, bounded decoder, control-flow closure, and vasm round-trip"})
        if end < int(old["end"]):
            inserted.append({**old, "start": end})
        current[replacement:replacement + 1] = inserted
    cursor = 0
    for index, entry in enumerate(current):
        start, end = int(entry["start"]), int(entry["end"])
        if start != cursor or end <= start:
            raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")
        entry["size"], entry["manifest_index"] = end - start, index
        cursor = end
    return current


def ownership_delta(before: list[dict[str, Any]], after: list[dict[str, Any]],
                    promotions: list[dict[str, Any]], rom_size: int) -> int:
    def mask(entries: list[dict[str, Any]]) -> bytearray:
        result = bytearray(rom_size)
        cursor = 0
        for entry in sorted(entries, key=lambda item: int(item["start"])):
            start, end = int(entry["start"]), int(entry["end"])
            if start != cursor or end <= start or end > rom_size:
                raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")
            owned = entry.get("kind") not in {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"} and \
                str(entry.get("confidence", "")).upper() not in {"PROBABLE", "CANDIDATE", "UNVERIFIED"}
            if owned:
                result[start:end] = b"\1" * (end - start)
            cursor = end
        if cursor != rom_size:
            raise ValueError("STOP_ROM_PARTITION_GAP_OR_OVERLAP")
        return result
    old, new = mask(before), mask(after)
    added = [index for index, (a, b) in enumerate(zip(old, new)) if b and not a]
    removed = [index for index, (a, b) in enumerate(zip(old, new)) if a and not b]
    expected = {i for item in promotions for i in range(int(item["start"]), int(item["end"]))}
    if removed or set(added) != expected:
        raise ValueError("STOP_SOURCE_OWNED_DELTA_MISMATCH")
    return len(added)
