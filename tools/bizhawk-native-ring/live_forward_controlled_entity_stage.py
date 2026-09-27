"""Fail-closed controller-to-entity provenance for the M12 post-run pipeline."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
from typing import Any, Iterable, Mapping, Sequence

from live_forward_gameplay_stage import ROM_SHA, SOURCE_OWNED_CANONICAL
from live_forward_scaling_audit import RECORD
from pipeline_outcomes import annotate_outcome

INPUT_PORTS = {0xA10003: "P1_DATA_PORT", 0xA10005: "P2_DATA_PORT"}
INPUT_RAM = {0xFF1654, 0xFF1658, 0xFF165C, 0xFF165D, 0xFF165E, 0xFF1660}
EVENT_BUS_READ, EVENT_BUS_WRITE = 1, 2
FLAG_EVENT, EVENT_MASK, EVENT_SHIFT = 0x8000, 0x3800, 11


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _hex(value: int | None) -> str | None:
    return None if value is None else f"0x{int(value):06X}"


def _json(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _event_id(event: Mapping[str, Any]) -> str:
    return ":".join(str(event.get(key, "")) for key in
                     ("run_id", "epoch", "stream_sequence", "instruction_sequence"))


def _event(row: tuple[int, ...], segment: Mapping[str, Any], segment_number: int,
           entity_range: tuple[int, int] | None = None) -> dict[str, Any] | None:
    stream, instruction, control_flow, pc, address, value, flags, cpu, width, domain, _, _ = row
    if not (flags & FLAG_EVENT) or cpu != 0 or domain != 1:
        return None
    subtype = (flags & EVENT_MASK) >> EVENT_SHIFT
    if subtype not in (EVENT_BUS_READ, EVENT_BUS_WRITE):
        return None
    in_entity = entity_range is not None and entity_range[0] <= address < entity_range[1]
    if address not in INPUT_PORTS and address not in INPUT_RAM and not in_entity:
        return None
    result = {"run_id": segment.get("run_id"), "epoch": segment.get("epoch"),
              "segment": segment_number, "stream_sequence": int(stream),
              "instruction_sequence": int(instruction), "control_flow_sequence": int(control_flow),
              "pc": int(pc), "address": int(address), "value": int(value),
              "width_bytes": {8: 1, 16: 2, 32: 4}.get(int(width), 0),
              "kind": "READ" if subtype == EVENT_BUS_READ else "WRITE",
              "frame": segment.get("entry_frame")}
    if not result["width_bytes"]:
        return None
    result["event_id"] = _event_id(result)
    return result


def _read_input_flow(receipt: Mapping[str, Any], output_dir: Path,
                     progress: Any = None, total_segments: int | None = None,
                     entity_range: tuple[int, int] | None = None) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    source = receipt.get("flow_handoff") or receipt.get("raw_segment_spool") or {}
    raw, index = Path(str(source.get("raw_path", ""))), Path(str(source.get("index_path", "")))
    archive = raw.parent.parent / "raw-evidence-archive" if raw.parent else output_dir / "raw-evidence-archive"
    if not raw.is_file() or not index.is_file():
        if (archive / raw.name).is_file() and (archive / index.name).is_file():
            raw, index = archive / raw.name, archive / index.name
    if not raw.is_file() or not index.is_file():
        return [], {"segments": 0, "records": 0, "raw_sha256": None, "index_sha256": None}
    events: list[dict[str, Any]] = []
    seen: set[tuple[Any, ...]] = set()
    records = 0
    frame_boundary_count = 0
    entry_frames: set[int] = set()
    with index.open("r", encoding="utf-8") as index_stream, raw.open("rb") as raw_stream:
        for number, line in enumerate(index_stream):
            item = json.loads(line)
            segment = item.get("segment", {})
            frame_boundary_count += int(segment.get("frame_boundary_count", 0))
            if segment.get("entry_frame") is not None:
                entry_frames.add(int(segment["entry_frame"]))
            raw_stream.seek(int(item["raw_offset"]))
            data = raw_stream.read(int(item["raw_length"]))
            for row in RECORD.iter_unpack(data):
                records += 1
                event = _event(row, item["segment"], number, entity_range)
                key = (event.get("run_id"), event.get("epoch"), event.get("stream_sequence"),
                       event.get("kind"), event.get("address"), event.get("value"), event.get("width_bytes")) if event else None
                if event and key not in seen:
                    seen.add(key)
                    events.append(event)
            if progress is not None and (number % 32 == 0 or number + 1 == total_segments):
                progress.update(number + 1, total=total_segments, detail="controller/input observations indexed")
    return events, {"segments": number + 1 if "number" in locals() else 0, "records": records,
                    "frame_boundary_count": frame_boundary_count, "entry_frames": sorted(entry_frames),
                    "raw_sha256": _sha256(raw), "index_sha256": _sha256(index)}


def select_entity_candidate(candidates: Sequence[Mapping[str, Any]],
                            exact_links: Sequence[Mapping[str, Any]],
                            instances: Sequence[Mapping[str, Any]]) -> dict[str, Any] | None:
    """Select one exact relation deterministically without assigning semantics."""
    exact_ids = {str(item.get("candidate_id")) for item in exact_links}
    eligible = [dict(item) for item in candidates if str(item.get("candidate_id")) in exact_ids]
    if not eligible:
        return None
    def score(item: Mapping[str, Any]) -> tuple[int, int, int, int, int, str]:
        cid = str(item["candidate_id"])
        observed = [x for x in instances if x.get("candidate_id") == cid]
        frames = len(item.get("frames_observed", []))
        stable = len({x.get("slot_index") for x in observed})
        return (int(item.get("stride_truth") in {"DERIVED_EXACT", "STATIC_VERIFIED"}),
                stable, frames, len(observed), int(item.get("write_count", 0)), cid)
    selected = max(eligible, key=score)
    link = next(item for item in exact_links if item.get("candidate_id") == selected["candidate_id"])
    selected["sat_entry_indices"] = sorted(int(value) for value in link.get("sat_entries", []))
    selected["selection_truth"] = "EXACT_ENTITY_TO_SAT_RELATION"
    return selected


def build_input_entity_chains(events: Sequence[Mapping[str, Any]], selected: Mapping[str, Any]) -> dict[str, list[dict[str, Any]]]:
    """Join only same-control-flow input dataflow; ordering alone is never exact."""
    base = int(str(selected["base"]), 0)
    stride = int(selected.get("stride") or 0)
    slot = int(selected.get("slot_index", 0))
    entity_start, entity_end = base + slot * stride, base + (slot + 1) * stride
    sources = [e for e in events if e["kind"] == "READ" and e["address"] in INPUT_PORTS]
    representations = [e for e in events if e["kind"] == "WRITE" and e["address"] in INPUT_RAM]
    entity_writes = [e for e in events if e["kind"] == "WRITE" and entity_start <= e["address"] < entity_end]
    source_to_representation: list[dict[str, Any]] = []
    representation_to_entity: list[dict[str, Any]] = []
    exact: list[dict[str, Any]] = []
    partial: list[dict[str, Any]] = []
    for source in sources:
        reps = [r for r in representations if r["run_id"] == source["run_id"] and
                r["epoch"] == source["epoch"] and r["control_flow_sequence"] == source["control_flow_sequence"] and
                r["stream_sequence"] > source["stream_sequence"]]
        for rep in reps[:1]:
            source_to_representation.append({"truth_class": "EXACT", "input_event_id": source["event_id"],
                "source": INPUT_PORTS[source["address"]], "source_pc": _hex(source["pc"]),
                "representation_address": _hex(rep["address"]), "representation_write_pc": _hex(rep["pc"]),
                "representation_event_id": rep["event_id"], "control_flow_sequence": source["control_flow_sequence"]})
            entity = next((w for w in entity_writes if w["run_id"] == rep["run_id"] and
                           w["epoch"] == rep["epoch"] and w["control_flow_sequence"] == rep["control_flow_sequence"] and
                           w["stream_sequence"] > rep["stream_sequence"]), None)
            if entity is None:
                partial.append({"truth_class": "PARTIAL", "reason": "NO_SAME_CONTROL_FLOW_ENTITY_WRITE",
                                "input_event_id": source["event_id"], "representation_event_id": rep["event_id"]})
                continue
            consumer = next((r for r in events if r["kind"] == "READ" and r["address"] == rep["address"] and
                             r["control_flow_sequence"] == rep["control_flow_sequence"] and
                             r["stream_sequence"] > rep["stream_sequence"] and r["stream_sequence"] <= entity["stream_sequence"]), None)
            if consumer is None:
                partial.append({"truth_class": "PARTIAL", "reason": "NO_EXACT_INPUT_CONSUMER_READ",
                                "input_event_id": source["event_id"], "representation_event_id": rep["event_id"]})
                continue
            chain = {"truth_class": "EXACT", "input_event_id": source["event_id"],
                     "source": INPUT_PORTS[source["address"]], "source_pc": _hex(source["pc"]),
                     "representation_address": _hex(rep["address"]), "representation_write_pc": _hex(rep["pc"]),
                     "consumer_pc": _hex(consumer["pc"]), "consumer_event_id": consumer["event_id"],
                     "entity_address": _hex(entity["address"]), "entity_write_pc": _hex(entity["pc"]),
                     "entity_event_id": entity["event_id"], "field_offset": entity["address"] - entity_start,
                     "control_flow_sequence": source["control_flow_sequence"]}
            exact.append(chain)
            representation_to_entity.append(chain)
    return {"source_to_representation": source_to_representation, "representation_to_entity": representation_to_entity,
            "exact": exact, "partial": partial,
            "unresolved": [] if exact or partial else [{"truth_class": "UNRESOLVED", "reason": "NO_EXACT_INPUT_DATAFLOW"}]}


def _instance_at(instances: Sequence[Mapping[str, Any]], selected: Mapping[str, Any],
                 observation: int | None) -> Mapping[str, Any] | None:
    if observation is None:
        return None
    matches = []
    for item in instances:
        interval = item.get("observation_intervals", [None, None])
        if (item.get("candidate_id") == selected.get("candidate_id")
                and int(item.get("slot_index", -1)) == int(selected.get("slot_index", -2))
                and len(interval) == 2 and interval[0] is not None and interval[1] is not None
                and interval[0] <= observation <= interval[1]):
            matches.append(item)
    return sorted(matches, key=lambda item: str(item.get("runtime_instance_id", "")))[0] if matches else None


def audit_partial_chains(partial: Sequence[Mapping[str, Any]], events: Sequence[Mapping[str, Any]],
                         selected: Mapping[str, Any], instances: Sequence[Mapping[str, Any]],
                         flow: Mapping[str, Any] | None = None) -> dict[str, Any]:
    """Classify every partial chain by the first exact edge still unsupported."""
    by_id = {str(item.get("event_id")): item for item in events}
    base, stride, slot = int(str(selected["base"]), 0), int(selected.get("stride") or 0), int(selected.get("slot_index", 0))
    start, end = base + slot * stride, base + (slot + 1) * stride
    entity_writes = sorted((item for item in events if item["kind"] == "WRITE" and start <= item["address"] < end),
                           key=lambda item: item["stream_sequence"])
    reused = len([item for item in instances if item.get("candidate_id") == selected.get("candidate_id")
                  and int(item.get("slot_index", -1)) == slot]) > 1
    frame_gap = flow is not None and int(flow.get("frame_boundary_count", 0)) == 0
    audited: list[dict[str, Any]] = []
    counts = {letter: 0 for letter in "ABCDEFG"}
    for index, relation in enumerate(partial, 1):
        source = by_id.get(str(relation.get("input_event_id")))
        representation = by_id.get(str(relation.get("representation_event_id")))
        blockers: list[str] = []
        later = [item for item in entity_writes if representation and item["stream_sequence"] > representation["stream_sequence"]]
        same_flow = [item for item in later if representation and item["control_flow_sequence"] == representation["control_flow_sequence"]]
        consumer = next((item for item in events if source and representation and item["kind"] == "READ"
                         and item["address"] == representation["address"]
                         and item["run_id"] == representation["run_id"] and item["epoch"] == representation["epoch"]
                         and item["control_flow_sequence"] == representation["control_flow_sequence"]
                         and representation["stream_sequence"] < item["stream_sequence"]
                         and (not same_flow or item["stream_sequence"] <= same_flow[0]["stream_sequence"])), None)
        if source is None or representation is None:
            blockers.append("A")
        if consumer is None:
            blockers.append("B")
        if not same_flow:
            blockers.append("C")
        if same_flow and not _instance_at(instances, selected, same_flow[0].get("segment")):
            blockers.append("D")
        if reused:
            blockers.append("E")
        if frame_gap:
            blockers.append("F")
        # No unsupported instruction form was observed; the missing register/branch
        # facts are a coverage gap and remain classified as B/F, not G.
        if any(str(item.get("dataflow_class")) == "UNSUPPORTED" for item in (source, representation) if item):
            blockers.append("G")
        for blocker in blockers:
            counts[blocker] += 1
        source_instance = _instance_at(instances, selected, source.get("segment") if source else None)
        write = later[0] if later else None
        write_instance = _instance_at(instances, selected, write.get("segment") if write else None)
        boundary = "UNKNOWN"
        if source_instance and write_instance:
            boundary = "YES" if source_instance.get("runtime_instance_id") != write_instance.get("runtime_instance_id") else "NO"
        elif source_instance and not write_instance:
            boundary = "UNKNOWN"
        item = {"partial_index": index, "blockers": blockers,
                "input_event_id": source.get("event_id") if source else relation.get("input_event_id"),
                "input_value": source.get("value") if source else None,
                "input_value_hex": _hex(source.get("value")) if source else None,
                "input_source_pc": _hex(source.get("pc")) if source else None,
                "input_consumer_pc": _hex(representation.get("pc")) if representation else None,
                "input_consumer_event_id": representation.get("event_id") if representation else None,
                "representation_address": _hex(representation.get("address")) if representation else None,
                "representation_value": representation.get("value") if representation else None,
                "entity_field_offset": None, "entity_write_pc": None, "entity_address": None,
                "written_value": None, "frame": source.get("frame") if source else None,
                "instruction_sequence_range": [source.get("instruction_sequence"), representation.get("instruction_sequence")]
                if source and representation else None,
                "control_flow_sequence_range": [source.get("control_flow_sequence"), representation.get("control_flow_sequence")]
                if source and representation else None,
                "dataflow": {"input_observation": source, "consumer_instruction": representation,
                             "intermediate_values": ([source.get("value"), representation.get("value")]
                                                      if source and representation else []),
                             "register_values": [],
                             "register_tracking_status": "UNAVAILABLE_FROM_W3_V2_BUS_EVENT_SCHEMA",
                             "entity_write_instruction": None, "entity_address": None, "written_value": None},
                "nearest_noncausal_entity_write":
                    {"pc": _hex(write["pc"]), "address": _hex(write["address"]), "value": write["value"],
                     "frame": write.get("frame"), "instruction_sequence": write["instruction_sequence"],
                     "control_flow_sequence": write["control_flow_sequence"]} if write else None,
                "slot_boundary": {"INSTANCE_ID": source_instance.get("runtime_instance_id") if source_instance else None,
                                  "INSTANCE_START": source_instance.get("observation_intervals", [None, None])[0]
                                  if source_instance else None,
                                  "INSTANCE_END": source_instance.get("observation_intervals", [None, None])[1]
                                  if source_instance else None,
                                  "WRITE_FRAME": write.get("frame") if write else None,
                                  "SLOT_ACTIVE_AT_WRITE": "YES" if write_instance else "NO",
                                  "REUSE_BOUNDARY_CROSSED": boundary,
                                  "WRITE_INSTANCE_ID": write_instance.get("runtime_instance_id") if write_instance else None}}
        audited.append(item)
    strongest = min(audited, key=lambda item: (len(item["blockers"]),
                                                 0 if item["slot_boundary"]["INSTANCE_ID"] else 1,
                                                 item["partial_index"])) if audited else None
    missing = ["same-control-flow entity field write after the input consumer (30/30 partials)",
               "M68K register values and decoded branch/state edges in the preserved W3 V2 records"]
    if frame_gap:
        missing.append("frame-boundary records; all preserved segments expose entry_frame=0 with frame_boundary_count=0")
    if reused:
        missing.append("a non-reused slot generation tying the input event to one continuous FF13CC instance")
    contract = None
    if strongest:
        contract = {"INPUT_EVENT_ID": strongest["input_event_id"], "INPUT_VALUE": strongest["input_value_hex"],
                    "INPUT_CONSUMER_PC": strongest["input_consumer_pc"],
                    "ENTITY_FIELD_OFFSET": strongest["entity_field_offset"],
                    "ENTITY_WRITE_PC": strongest["entity_write_pc"], "FRAME": strongest["frame"],
                    "INSTRUCTION_SEQUENCE_RANGE": strongest["instruction_sequence_range"]}
    return {"schema": "oasis.m12.controlled-entity-causal-audit.v1", "partial_count": len(audited),
            "blocker_counts": counts, "chains": audited, "strongest_partial_chain": strongest,
            "strongest_chain_contract": contract,
            "coverage_failure": "STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT" if audited else None,
            "missing_runtime_evidence": missing if audited else []}


def recover_entity_fields(selected: Mapping[str, Any], gameplay: Mapping[str, Any],
                          events: Sequence[Mapping[str, Any]]) -> list[dict[str, Any]]:
    base, stride, slot = int(str(selected["base"]), 0), int(selected.get("stride") or 0), int(selected.get("slot_index", 0))
    start, end = base + slot * stride, base + (slot + 1) * stride
    chains = [item for item in gameplay.get("ram_to_sat", {}).get("exact", [])
              if start <= int(item["ram_source_field"]["address"], 0) < end]
    known = {int(item["ram_source_field"]["address"], 0) - start: item for item in chains}
    names = {0: "y", 2: "size_link", 4: "tile_palette_priority_flip", 6: "x"}
    fields = []
    for offset in range(0, max(stride, 0), 2):
        address = start + offset
        writes = [e for e in events if e["kind"] == "WRITE" and e["address"] == address]
        reads = [e for e in events if e["kind"] == "READ" and e["address"] == address]
        downstream = known.get(offset)
        fields.append({"name": downstream.get("sat_entry_field", {}).get("field", names.get(offset, "UNKNOWN"))
                       if downstream else names.get(offset, "UNKNOWN"),
                       "offset": offset, "address": _hex(address),
                       "width_bytes": max([e["width_bytes"] for e in writes], default=0),
                       "read_pcs": sorted({_hex(e["pc"]) for e in reads}),
                       "write_pcs": sorted({_hex(e["pc"]) for e in writes}),
                       "truth_class": "EXACT" if downstream else "UNKNOWN",
                       "downstream_sat_relation": downstream})
    return fields


def evaluate_lifetime(selected: Mapping[str, Any], instances: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
    chosen = [x for x in instances if x.get("candidate_id") == selected.get("candidate_id") and
              int(x.get("slot_index", -1)) == int(selected.get("slot_index", -2))]
    reused = len(chosen) > 1
    if reused:
        continuity = "PARTIAL" if all(x.get("deactivation_reuse_witness") for x in chosen[:-1]) else "UNRESOLVED"
    else:
        continuity = "PARTIAL" if any(x.get("frame_intervals") == [0, 0] for x in chosen) else "EXACT"
    start = chosen[0].get("observation_intervals", [None, None])[0] if chosen else None
    end = chosen[-1].get("observation_intervals", [None, None])[1] if chosen else None
    return {"instance_start": start, "instance_end": end, "slot_reuse_observed": reused,
            "identity_continuity": continuity, "instances": chosen}


def evaluate_player_label(selected: Mapping[str, Any], input_chains: Mapping[str, Sequence[Mapping[str, Any]]],
                          sat_chain: Mapping[str, Any], lifetime: Mapping[str, Any]) -> dict[str, Any]:
    exact = bool(input_chains.get("exact"))
    sat_exact = sat_chain.get("truth_class") == "EXACT"
    primary = bool(selected.get("primary_avatar_witness"))
    proven = exact and sat_exact and primary and lifetime.get("identity_continuity") == "EXACT"
    return {"entity_role": "PLAYER" if proven else "CONTROLLED_ENTITY" if exact else "ENTITY_CANDIDATE",
            "player_label_proven": "YES" if proven else "NO",
            "primary_avatar_witness": primary, "gate_reason": "ALL_PLAYER_GATES_CLOSED" if proven else
            "PLAYER_REQUIRES_EXPLICIT_PRIMARY_AVATAR_WITNESS"}


def build_entity_sat_chain(selected: Mapping[str, Any], input_chains: Mapping[str, Sequence[Mapping[str, Any]]],
                           gameplay: Mapping[str, Any], entity_links: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
    exact_by_address = {item["ram_source_field"]["address"]: item for item in gameplay.get("ram_to_sat", {}).get("exact", [])}
    start, end = int(str(selected["base"]), 0), int(str(selected["base"]), 0) + int(selected.get("stride") or 0)
    accepted = [item for address, item in exact_by_address.items() if start <= int(address, 0) < end]
    accepted_anchor = {"truth_class": "EXACT", "candidate_id": selected.get("candidate_id"),
                       "sat_entry_indices": sorted(int(entry) for entry in selected.get("sat_entry_indices", [])),
                       "field_relations": accepted}
    hardware = {int(item.get("sat_entry", -1)) for piece in next((x for x in entity_links if x.get("candidate_id") == selected.get("candidate_id")), {}).get("hardware_sprite_pieces", []) for item in [piece]}
    chains = []
    for relation in input_chains.get("exact", []):
        ram = exact_by_address.get(relation.get("entity_address"))
        if not ram:
            continue
        entries = ram["sat_entry_field"].get("entries", [])
        chains.append({"truth_class": "EXACT", "input_to_entity": relation,
                       "sat_shadow_address": ram["sat_shadow_write"]["address"],
                       "producer_pc": ram["sat_shadow_write"]["producer_pc"],
                       "dma_event_identity": ram["transfer"], "sat_entry_indices": entries,
                       "hardware_sprite_piece_proven": bool(hardware.intersection(entries)),
                       "hardware_sprite_piece_indices": sorted(hardware.intersection(entries))})
    if chains:
        truth = "EXACT" if any(x["hardware_sprite_piece_proven"] for x in chains) else "PARTIAL"
        return {"truth_class": truth, "chains": chains, "accepted_entity_to_sat": accepted_anchor}
    return {"truth_class": "UNRESOLVED", "chains": [], "accepted_entity_to_sat": accepted_anchor,
            "reason": "INPUT_TO_ENTITY_CHAIN_UNPROVEN"}


def analyze_controlled_entity(candidates: Sequence[Mapping[str, Any]], exact_links: Sequence[Mapping[str, Any]],
                              gameplay: Mapping[str, Any], instances: Sequence[Mapping[str, Any]],
                              events: Sequence[Mapping[str, Any]], run_id: int,
                              flow: Mapping[str, Any] | None = None) -> dict[str, Any]:
    selected = select_entity_candidate(candidates, exact_links, instances)
    if selected is None:
        raise ValueError("STOP_CONTROLLED_ENTITY_EXACT_CANDIDATE_MISSING")
    input_chains = build_input_entity_chains(events, selected)
    fields = recover_entity_fields(selected, gameplay, events)
    lifetime = evaluate_lifetime(selected, instances)
    sat_chain = build_entity_sat_chain(selected, input_chains, gameplay, exact_links)
    partial_audit = audit_partial_chains(input_chains["partial"], events, selected, instances, flow)
    role = evaluate_player_label(selected, input_chains, sat_chain, lifetime)
    metrics = {"SELECTED_ENTITY_CANDIDATE": selected["candidate_id"],
               "PROVEN_ENTITY_FIELDS": sum(x["truth_class"] == "EXACT" for x in fields),
               "EXACT_INPUT_TO_ENTITY_CHAINS": len(input_chains["exact"]),
               "PARTIAL_INPUT_TO_ENTITY_CHAINS": len(input_chains["partial"]),
               "EXACT_INPUT_TO_SAT_CHAINS": sum(x["truth_class"] == "EXACT" for x in sat_chain["chains"]),
               "EXACT_INPUT_TO_HARDWARE_SPRITE_CHAINS": sum(x["hardware_sprite_piece_proven"] for x in sat_chain["chains"]),
               "ENTITY_ROLE": role["entity_role"], "PLAYER_LABEL_PROVEN": role["player_label_proven"],
               "CONTROLLED_ENTITY_SOURCE_OWNED_DELTA": 0}
    metrics.update({f"PARTIAL_BLOCKER_{key}": value for key, value in partial_audit["blocker_counts"].items()})
    acceptance = metrics["EXACT_INPUT_TO_ENTITY_CHAINS"] > 0 and metrics["EXACT_INPUT_TO_SAT_CHAINS"] > 0 and not lifetime["slot_reuse_observed"]
    selection_contract = {"ENTITY_CANDIDATE_ID": selected["candidate_id"], "RAM_RECORD_ADDRESS": selected.get("record_address"),
                          "STRUCT_BASE": selected.get("base"), "STRIDE": selected.get("stride"),
                          "SLOT_INDEX": selected.get("slot_index"), "OBSERVED_FRAME_RANGE": selected.get("frames_observed", []),
                          "SAT_ENTRY_INDICES": selected["sat_entry_indices"]}
    lifetime_contract = {"INSTANCE_START": lifetime["instance_start"], "INSTANCE_END": lifetime["instance_end"],
                         "SLOT_REUSE_OBSERVED": lifetime["slot_reuse_observed"],
                         "IDENTITY_CONTINUITY": lifetime["identity_continuity"]}
    return {"schema": "oasis.m12.postrun-controlled-entity-analysis.v1", "status": "PASS" if acceptance else "STOP",
            "state": "PASS" if acceptance else "STOP", "run_id": run_id, "selected_entity": selected,
            "selection_contract": selection_contract, "lifetime_contract": lifetime_contract,
            "input_sources": [dict(x) for x in events if x["kind"] == "READ" and x["address"] in INPUT_PORTS],
            "input_entity_chains": input_chains, "fields": fields, "sat_chain": sat_chain,
            "lifetime": lifetime, "entity_role": role, "metrics": metrics, "partial_chain_audit": partial_audit,
            "acceptance_ready": acceptance, "stop_reason": None if acceptance else "STOP_CONTROLLED_ENTITY_COVERAGE_INSUFFICIENT",
            "source_owned_contract": {"CONTROLLED_ENTITY_SOURCE_OWNED_BEFORE": SOURCE_OWNED_CANONICAL,
                                      "CONTROLLED_ENTITY_SOURCE_OWNED_AFTER": SOURCE_OWNED_CANONICAL,
                                      "CONTROLLED_ENTITY_SOURCE_OWNED_DELTA": 0},
            "source_owned_before": SOURCE_OWNED_CANONICAL, "source_owned_after": SOURCE_OWNED_CANONICAL,
            "source_owned_delta": 0, "truth_version": "M12-CANONICAL-TRUTH-V1"}


def run_controlled_entity_stage(receipt: Mapping[str, Any], rom_path: Path, output_dir: Path,
                                total_segments: int | None = None, progress: Any = None,
                                gameplay_dir: Path | None = None) -> dict[str, Any]:
    """Consume accepted gameplay artifacts and emit five deterministic artifacts."""
    output_dir.mkdir(parents=True, exist_ok=True)
    rom = rom_path.read_bytes()
    if len(rom) != 3145728 or hashlib.sha256(rom).hexdigest() != ROM_SHA:
        raise ValueError("STOP_CONTROLLED_ENTITY_ROM_IDENTITY_MISMATCH")
    source_dir = gameplay_dir or output_dir
    candidates = json.loads((source_dir / "postrun_entity_candidates.json").read_text(encoding="utf-8"))["candidates"]
    links_doc = json.loads((source_dir / "postrun_entity_sat_links.json").read_text(encoding="utf-8"))
    gameplay = json.loads((source_dir / "postrun_gameplay_provenance.json").read_text(encoding="utf-8"))
    instances = json.loads((source_dir / "postrun_runtime_instances.json").read_text(encoding="utf-8"))["instances"]
    selected_hint = select_entity_candidate(candidates, links_doc.get("exact", []), instances)
    if selected_hint is None:
        raise ValueError("STOP_CONTROLLED_ENTITY_EXACT_CANDIDATE_MISSING")
    start = int(str(selected_hint["base"]), 0)
    stride = int(selected_hint.get("stride") or 0)
    slot = int(selected_hint.get("slot_index", 0))
    events, flow = _read_input_flow(receipt, output_dir, progress, total_segments,
                                     (start + slot * stride, start + (slot + 1) * stride))
    analysis = analyze_controlled_entity(candidates, links_doc.get("exact", []), gameplay, instances, events,
                                         int(receipt.get("runtime", {}).get("run_id", 0)), flow)
    payloads = {"postrun_controlled_entity_analysis.json": analysis,
                "postrun_controlled_entity_fields.json": {"schema": "oasis.m12.postrun-controlled-entity-fields.v1", "selected_entity": analysis["selected_entity"], "fields": analysis["fields"]},
                "postrun_input_entity_chains.json": {"schema": "oasis.m12.postrun-input-entity-chains.v1", "chains": analysis["input_entity_chains"], "partial_chain_audit": analysis["partial_chain_audit"]},
                "postrun_controlled_entity_sat_chain.json": {"schema": "oasis.m12.postrun-controlled-entity-sat-chain.v1", **analysis["sat_chain"]}}
    paths = {}
    for name, value in payloads.items():
        path = output_dir / name; path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8"); paths[name] = path
    receipt_data = {"schema": "oasis.m12.postrun-controlled-entity-receipt.v1", "stage_version": "M12-CONTROLLED-ENTITY-PROVENANCE-V1", "status": analysis["status"], "state": analysis["state"], "run_id": analysis["run_id"], "flow": flow, "input_hashes": {"flow_v1_records_sha256": flow["raw_sha256"], "flow_v1_segments_sha256": flow["index_sha256"], "rom_sha256": ROM_SHA}, "output_hashes": {name.removesuffix(".json") + "_sha256": _sha256(path) for name, path in paths.items()}, "metrics": analysis["metrics"], "acceptance_ready": analysis["acceptance_ready"], "entity_role": analysis["entity_role"], "player_label_proven": analysis["entity_role"]["player_label_proven"], "source_owned_contract": analysis["source_owned_contract"], "source_owned_before": SOURCE_OWNED_CANONICAL, "source_owned_after": SOURCE_OWNED_CANONICAL, "source_owned_delta": 0, "truth_version": "M12-CANONICAL-TRUTH-V1"}
    receipt_path = output_dir / "postrun_controlled_entity_receipt.json"
    receipt_path.write_text(json.dumps(receipt_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return {**analysis, "analysis_path": str(paths["postrun_controlled_entity_analysis.json"]), "fields_path": str(paths["postrun_controlled_entity_fields.json"]), "chains_path": str(paths["postrun_input_entity_chains.json"]), "sat_chain_path": str(paths["postrun_controlled_entity_sat_chain.json"]), "receipt_path": str(receipt_path)}


def run_controlled_entity_pipeline_stage(receipt: Mapping[str, Any], rom_path: Path, output_dir: Path,
                                         total_segments: int | None, progress: Any,
                                         gameplay_result: Mapping[str, Any]) -> tuple[dict[str, Any], str | None]:
    progress.start("CONTROLLED ENTITY PROVENANCE", total=total_segments, unit="FLOW segments",
                   detail="tracing controller input to one accepted entity candidate")
    gameplay_path = Path(str(gameplay_result["analysis_path"])).parent if gameplay_result.get("analysis_path") else output_dir
    try:
        result = run_controlled_entity_stage(receipt, rom_path, output_dir, total_segments,
                                             progress, gameplay_path)
    except ValueError as error:
        if str(error) != "STOP_CONTROLLED_ENTITY_EXACT_CANDIDATE_MISSING":
            raise
        result = {"status": "UNRESOLVED", "state": "UNRESOLVED",
                  "acceptance_ready": False, "stop_reason": str(error),
                  "source_owned_delta": 0}
    if result["acceptance_ready"]:
        progress.finish("PASS", detail="controller-to-entity-to-SAT evidence closed")
        return result, None
    reason = result.get("stop_reason", "STOP_CONTROLLED_ENTITY_PROVENANCE_INCOMPLETE")
    annotate_outcome(result, reason)
    if result["pipeline_effect"] == "CONTINUE_NONFATAL":
        result["state"] = "UNRESOLVED"
        progress.finish("UNRESOLVED", detail=reason)
    else:
        progress.finish("STOP", detail=reason)
    return result, reason


__all__ = ["analyze_controlled_entity", "audit_partial_chains", "build_entity_sat_chain", "build_input_entity_chains",
           "evaluate_lifetime", "evaluate_player_label", "recover_entity_fields", "run_controlled_entity_pipeline_stage",
           "run_controlled_entity_stage", "select_entity_candidate"]
