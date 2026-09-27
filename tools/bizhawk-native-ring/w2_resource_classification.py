"""Worker-side W2 primitive classification over immutable W1 records."""

from __future__ import annotations

import hashlib
import json
import struct
from collections import defaultdict
from statistics import mean
from typing import Any, Iterable, Mapping

from live_forward_scaling_audit import (
    EVENT_BUS_READ, EVENT_BUS_WRITE, EVENT_FRAME_BOUNDARY, EVENT_SUBTYPE_MASK,
    EVENT_SUBTYPE_SHIFT, FLAG_EVENT, FLAG_INSTRUCTION, RECORD,
)
from w2_vdp_decoder import VdpProtocolDecoder
from w2_frame_coherence import UNRESOLVED, resolve_segment_frames


DOMAIN_NAMES = {
    0: "ROM", 1: "68K_RAM", 2: "Z80_WINDOW", 3: "VDP",
    4: "YM2612", 5: "PSG", 6: "OTHER",
}
SHADOW_SAT_START = 0xFF13CC
SHADOW_SAT_END = SHADOW_SAT_START + 0x30
PRIMITIVE_CLASSES = (
    "ROM_DATA_READ", "RAM_READ", "RAM_WRITE", "Z80_WINDOW_READ",
    "Z80_WINDOW_WRITE", "VDP_CONTROL_READ", "VDP_CONTROL_WRITE",
    "VDP_DATA_READ", "VDP_DATA_WRITE", "YM2612_MAPPED_ACCESS",
    "PSG_MAPPED_ACCESS", "OTHER_BUS_ACCESS",
)


def _width(auxiliary: int) -> int:
    return {1: 8, 2: 16, 3: 32}.get((auxiliary >> 16) & 3, 0)


def _domain(auxiliary: int) -> int:
    return (auxiliary >> 18) & 7


def _value(row: tuple[int, ...], width: int) -> int:
    value = int(row[5]) | ((int(row[11]) & 0xFFFF) << 16)
    return value & ((1 << width) - 1) if width else value


def _event_kind(flags: int) -> str | None:
    if not flags & FLAG_EVENT:
        return None
    subtype = (flags & EVENT_SUBTYPE_MASK) >> EVENT_SUBTYPE_SHIFT
    return {EVENT_BUS_READ: "READ", EVENT_BUS_WRITE: "WRITE",
            EVENT_FRAME_BOUNDARY: "FRAME"}.get(subtype)


def _primitive(kind: str, address: int, domain: int) -> str:
    if domain == 0:
        return "ROM_DATA_READ" if kind == "READ" else "OTHER_BUS_ACCESS"
    if domain == 1:
        return "RAM_READ" if kind == "READ" else "RAM_WRITE"
    if domain == 2:
        return "Z80_WINDOW_READ" if kind == "READ" else "Z80_WINDOW_WRITE"
    if domain == 3:
        port = address & 0x1F
        if port in (0x00, 0x02):
            return "VDP_DATA_READ" if kind == "READ" else "VDP_DATA_WRITE"
        if port in (0x04, 0x06):
            return "VDP_CONTROL_READ" if kind == "READ" else "VDP_CONTROL_WRITE"
        return "OTHER_BUS_ACCESS"
    if domain == 4:
        return "YM2612_MAPPED_ACCESS"
    if domain == 5:
        return "PSG_MAPPED_ACCESS"
    return "OTHER_BUS_ACCESS"


def _event(row: tuple[int, ...], identity: Mapping[str, Any] | None = None) -> dict[str, Any] | None:
    kind = _event_kind(int(row[6]))
    if kind not in ("READ", "WRITE"):
        return None
    width = _width(int(row[11]))
    domain = _domain(int(row[11]))
    address = int(row[4]) & 0xFFFFFF
    identity = identity or {"run_id": None, "epoch": None, "frame": UNRESOLVED}
    return {
        "stream_sequence": int(row[0]), "instruction_sequence": int(row[1]),
        "pc": int(row[3]), "address": address, "value": _value(row, width),
        "width": width, "domain": DOMAIN_NAMES.get(domain, "OTHER"),
        "domain_code": domain, "cpu": (int(row[11]) >> 21) & 3,
        "direction": kind, "primitive": _primitive(kind, address, domain),
        "flags": int(row[6]), "auxiliary": int(row[11]), "truth": "OBSERVED",
        "run_id": identity.get("run_id"), "epoch": identity.get("epoch"),
        "frame": identity.get("frame", UNRESOLVED),
    }


def _move_shape(opcode: int) -> dict[str, Any] | None:
    top = (opcode >> 12) & 0xF
    width = {1: 8, 2: 32, 3: 16}.get(top)
    if width is None:
        return None
    source_mode, source_reg = (opcode >> 3) & 7, opcode & 7
    dest_mode, dest_reg = (opcode >> 6) & 7, (opcode >> 9) & 7
    if dest_mode == 1:  # MOVEA has an address-register destination, not a bus write.
        return None
    source_memory = source_mode in (2, 3, 4, 5, 6) or (source_mode == 7 and source_reg <= 1)
    dest_memory = dest_mode in (2, 3, 4, 5, 6) or (dest_mode == 7 and dest_reg <= 1)
    if not source_memory or not dest_memory:
        return None
    return {"opcode": opcode, "width": width, "source_mode": source_mode,
            "source_register": source_reg, "destination_mode": dest_mode,
            "destination_register": dest_reg}


def _exact_relations(instructions: Mapping[int, dict[str, Any]]) -> list[dict[str, Any]]:
    relations: list[dict[str, Any]] = []
    for sequence in sorted(instructions):
        item = instructions[sequence]
        shape = _move_shape(item["opcode"])
        events = item["events"]
        reads = [event for event in events if event["direction"] == "READ"]
        writes = [event for event in events if event["direction"] == "WRITE"]
        if shape is None or len(reads) != 1 or len(writes) != 1:
            continue
        source, destination = reads[0], writes[0]
        if source["width"] != shape["width"] or destination["width"] != shape["width"]:
            continue
        mask = (1 << shape["width"]) - 1
        if (source["value"] & mask) != (destination["value"] & mask):
            continue
        relations.append({
            "relation": "EXACT_DATA_TRANSFER", "truth": "DERIVED_EXACT",
            "instruction_sequence": sequence, "pc": item["pc"],
            "opcode": item["opcode"], "width": shape["width"],
            "source_event_sequence": source["stream_sequence"],
            "destination_event_sequence": destination["stream_sequence"],
            "source_address": source["address"], "destination_address": destination["address"],
            "value": source["value"], "decoder": "M68K_MOVE_MEMORY_TO_MEMORY",
            "run_id": item.get("run_id"), "epoch": item.get("epoch"),
            "frame": item.get("frame", UNRESOLVED),
        })
    return relations


def _statistics(values: list[int]) -> dict[str, float | int | None]:
    if not values:
        return {"mean": None, "p95": None, "max": 0}
    ordered = sorted(values)
    return {"mean": mean(values), "p95": ordered[max(0, (len(ordered) * 95 + 99) // 100 - 1)],
            "max": max(values)}


def _candidate_rows(events: Iterable[Mapping[str, Any]]) -> list[dict[str, Any]]:
    grouped: dict[tuple[str, int, int, str], list[Mapping[str, Any]]] = defaultdict(list)
    for event in events:
        key = (str(event["primitive"]), int(event["address"]), int(event["width"]),
               str(event["domain"]))
        grouped[key].append(event)
    result = []
    for key, rows in sorted(grouped.items()):
        identities = {(row.get("run_id"), row.get("epoch"), row.get("frame", UNRESOLVED))
                     for row in rows}
        identity = next(iter(identities)) if len(identities) == 1 else (None, None, UNRESOLVED)
        result.append({
            "primitive": key[0], "address": key[1], "width": key[2], "domain": key[3],
            "count": len(rows), "first_stream_sequence": rows[0]["stream_sequence"],
            "last_stream_sequence": rows[-1]["stream_sequence"], "truth": "OBSERVED",
            "semantic_label": None,
            "run_id": identity[0], "epoch": identity[1], "frame": identity[2],
        })
    return result


def _z80_report(events: Iterable[Mapping[str, Any]]) -> dict[str, Any]:
    rows = [dict(event) for event in events if event["domain"] == "Z80_WINDOW"]
    handoff_addresses = {0xA11100, 0xA11200}
    handoffs = [event for event in rows if event["direction"] == "WRITE" and
                event["address"] in handoff_addresses]
    return {
        "observed_writes": [event for event in rows if event["direction"] == "WRITE"],
        "audio_handoff_candidates": [{**event, "candidate": "AUDIO_HANDOFF_CANDIDATE",
                                      "truth": "OBSERVED"} for event in handoffs],
        "song_semantics": "NOT_CLAIMED", "instrument_semantics": "NOT_CLAIMED",
    }


def _shadow_sat(events: Iterable[Mapping[str, Any]]) -> list[dict[str, Any]]:
    return [{**event, "evidence_class": "SHADOW_SAT_RAM_WRITE",
             "truth": "OBSERVED"} for event in events
            if event["primitive"] == "RAM_WRITE" and
            SHADOW_SAT_START <= event["address"] < SHADOW_SAT_END]


def _witnesses(events: Iterable[Mapping[str, Any]]) -> dict[str, dict[str, Any] | None]:
    rows = list(events)
    names = ("ROM_DATA_READ", "RAM_WRITE", "VDP_CONTROL_WRITE", "Z80_WINDOW_WRITE",
             "SHADOW_SAT_RAM_WRITE")
    result: dict[str, dict[str, Any] | None] = {}
    for name in names:
        if name == "SHADOW_SAT_RAM_WRITE":
            match = next((event for event in rows if event["primitive"] == "RAM_WRITE" and
                          SHADOW_SAT_START <= event["address"] < SHADOW_SAT_END), None)
            result[name] = ({**match, "evidence_class": name} if match else None)
        else:
            result[name] = next((dict(event) for event in rows if event["primitive"] == name), None)
    return result


def classify_rows(rows: Iterable[tuple[int, ...]], run_id: int | None = None,
                  segments: Iterable[Mapping[str, Any]] | None = None) -> dict[str, Any]:
    ordered = sorted(rows, key=lambda row: int(row[0]))
    frame_resolution = resolve_segment_frames(segments or (), ordered)
    frame_by_stream = frame_resolution["by_stream"]
    raw = b"".join(RECORD.pack(*row) for row in ordered)
    instructions: dict[int, dict[str, Any]] = {}
    events: list[dict[str, Any]] = []
    events_by_instruction: dict[int, list[dict[str, Any]]] = defaultdict(list)
    frames: list[dict[str, Any]] = []
    errors: list[str] = []
    for row in ordered:
        flags = int(row[6])
        if flags & FLAG_INSTRUCTION:
            sequence = int(row[1])
            if sequence in instructions:
                errors.append(f"duplicate instruction_sequence {sequence}")
            instructions[sequence] = {"instruction_sequence": sequence,
                                      "pc": int(row[3]), "opcode": int(row[5]),
                                      "events": [],
                                      **frame_by_stream.get(int(row[0]), {
                                          "run_id": run_id, "epoch": None,
                                          "frame": UNRESOLVED}),}
        kind = _event_kind(flags)
        if kind == "FRAME":
            identity = frame_by_stream.get(int(row[0]), {
                "run_id": run_id, "epoch": None, "frame": UNRESOLVED})
            frames.append({"stream_sequence": int(row[0]), "frame": int(row[3]),
                           "run_id": identity.get("run_id"), "epoch": identity.get("epoch"),
                           "truth": "OBSERVED"})
            continue
        event = _event(row, frame_by_stream.get(int(row[0]), {
            "run_id": run_id, "epoch": None, "frame": UNRESOLVED}))
        if event is None:
            continue
        events_by_instruction[event["instruction_sequence"]].append(event)
        events.append(event)
    for sequence, item in instructions.items():
        item["events"] = sorted(events_by_instruction.get(sequence, []),
                                 key=lambda event: event["stream_sequence"])
    for sequence, attached in events_by_instruction.items():
        if sequence not in instructions:
            errors.append(f"bus event group {sequence} lacks retained instruction")
    relations = _exact_relations(instructions)
    vdp = VdpProtocolDecoder()
    vdp.consume_all(event for event in events if event["domain"] == "VDP")
    event_counts: dict[str, int] = {name: 0 for name in PRIMITIVE_CLASSES}
    widths: dict[str, int] = defaultdict(int)
    domains: dict[str, int] = defaultdict(int)
    for event in events:
        event_counts[event["primitive"]] += 1
        widths[str(event["width"])] += 1
        domains[event["domain"]] += 1
    per_instruction = [len(item["events"]) for item in instructions.values() if item["events"]]
    return {
        "schema": "oasis.m68k.w2-active-resource-classification.v1",
        "run_id": run_id,
        "raw_w1_record_count": len(ordered),
        "raw_w1_record_bytes": len(raw),
        "raw_w1_records_sha256": hashlib.sha256(raw).hexdigest(),
        "counts": {"instruction_records": len(instructions), "bus_events": len(events),
                   "frame_records": len(frames), "primitive_classes": dict(sorted(event_counts.items())),
                   "widths": dict(sorted(widths.items())), "domains": dict(sorted(domains.items()))},
        "instruction_accesses": [
            {"instruction_sequence": item["instruction_sequence"], "pc": item["pc"],
             "opcode": item["opcode"], "events": item["events"],
             "run_id": item.get("run_id"), "epoch": item.get("epoch"),
             "frame": item.get("frame", UNRESOLVED)}
            for item in sorted(instructions.values(), key=lambda item: item["instruction_sequence"])
            if item["events"]
        ],
        "relations": relations,
        "vdp": vdp.report(),
        "candidates": _candidate_rows(events),
        "witnesses": _witnesses(events),
        "shadow_sat_writes": _shadow_sat(events),
        "z80": _z80_report(events),
        "frames": frames,
        "frame_coherence": {
            "segments": frame_resolution["segments"],
            "unresolved_streams": frame_resolution["unresolved_streams"],
            "errors": frame_resolution["errors"],
        },
        "truth_policy": {"observed": "raw W1 event or instruction fact",
                         "derived_exact": "closed decoder relation with matching ordered events",
                         "hypothesis": "not emitted by this checkpoint", "source_owned_delta": 0},
        "latency": {"bus_events_per_instruction": _statistics(per_instruction)},
        "errors": sorted(set(errors)),
    }


def canonical_json(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode()


__all__ = ["classify_rows", "canonical_json", "SHADOW_SAT_START", "SHADOW_SAT_END"]
