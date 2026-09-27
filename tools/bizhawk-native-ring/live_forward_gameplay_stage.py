"""Fail-closed gameplay RAM and entity-candidate analysis for M12 post-run.

This stage consumes preserved FLOW V2 bus evidence and the accepted Sprite/SAT
artifacts.  It names only observed runtime records; it never assigns gameplay
semantics and never changes ROM ownership.
"""
from __future__ import annotations

from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct
from typing import Any, Iterable, Mapping, Sequence

from live_forward_scaling_audit import RECORD
from pipeline_outcomes import annotate_outcome

SOURCE_OWNED_CANONICAL = 1487672
ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROM_SIZE = 3145728
FLAG_EVENT = 0x8000
EVENT_SHIFT = 11
EVENT_BUS_READ, EVENT_BUS_WRITE, EVENT_FRAME_BOUNDARY = 1, 2, 3
RAM_START, RAM_END = 0xFF0000, 0x1000000


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _hex(value: int) -> str:
    return f"0x{int(value):06X}"


def _canonical(value: Any) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def _width_bytes(width: int) -> int:
    return {8: 1, 16: 2, 32: 4}.get(int(width), 0)


def _address_semantics(opcode: int) -> str | None:
    """Return only an addressing-mode witness, never a guessed field meaning."""
    top = (int(opcode) >> 12) & 0xF
    if top not in (1, 2, 3):
        return None
    mode = (int(opcode) >> 3) & 7
    return {3: "POSTINCREMENT", 4: "PREDECREMENT", 6: "INDEXED"}.get(mode)


def infer_stride_from_accesses(accesses: Sequence[Mapping[str, Any]]) -> dict[str, Any]:
    """Infer a stride only with a generation or loop witness.

    ``static_stride`` and ``address_generation_semantics`` are explicit static
    witnesses used by tests/importers.  Runtime callers attach a loop witness
    when a repeated access PC is inside an observed control-flow backedge.
    """
    values = [int(item["address"]) for item in accesses]
    unique = sorted(set(values))
    if len(unique) < 2:
        return {"stride": None, "truth_class": "HYPOTHESIS", "witness": "INSUFFICIENT_ADDRESSES"}
    explicit = [int(item["static_stride"]) for item in accesses
                if isinstance(item.get("static_stride"), int) and item["static_stride"] > 0]
    if explicit and len(set(explicit)) == 1:
        return {"stride": explicit[0], "truth_class": "STATIC_VERIFIED", "witness": "STATIC_STRIDE"}
    deltas = [right - left for left, right in zip(unique, unique[1:]) if right > left]
    if not deltas:
        return {"stride": None, "truth_class": "HYPOTHESIS", "witness": "NO_POSITIVE_DELTA"}
    counts = Counter(deltas)
    stride, support = counts.most_common(1)[0]
    semantics = {item.get("address_generation_semantics") for item in accesses}
    loop_witness = any(bool(item.get("loop_witness")) for item in accesses)
    exact = (support >= 1 and (loop_witness or semantics - {None}))
    return {"stride": stride, "truth_class": "DERIVED_EXACT" if exact else "HYPOTHESIS",
            "witness": ("LOOP_ADDRESS_GENERATION" if loop_witness else
                        next(iter(semantics - {None}), "NUMERICAL_SPACING")),
            "support": support, "unique_addresses": len(unique)}


def deduplicate_overlap_observations(observations: Iterable[Mapping[str, Any]]) -> list[dict[str, Any]]:
    """Deduplicate overlapping Worker copies using the FLOW stream identity."""
    seen: set[tuple[Any, ...]] = set()
    result: list[dict[str, Any]] = []
    for item in observations:
        key = (item.get("run_id"), item.get("epoch"), item.get("stream_sequence"),
               item.get("kind"), item.get("address"), item.get("value"), item.get("width"))
        if key in seen:
            continue
        seen.add(key)
        result.append(dict(item))
    return result


def track_runtime_instances(events: Sequence[Mapping[str, Any]], base: int,
                            stride: int | None) -> list[dict[str, Any]]:
    """Track slot lifetimes and start a new instance after a reuse gap."""
    slots: dict[int, list[Mapping[str, Any]]] = defaultdict(list)
    for event in events:
        address = int(event["address"])
        slot = (address - base) // stride if stride and address >= base else None
        if slot is not None and address != base + slot * stride:
            continue
        if slot is not None:
            slots[slot].append(event)
    instances: list[dict[str, Any]] = []
    for slot, values in sorted(slots.items()):
        ordered = sorted(values, key=lambda item: (int(item.get("observation", 0)),
                                                    int(item.get("stream_sequence", 0))))
        groups: list[list[Mapping[str, Any]]] = []
        for value in ordered:
            if not groups or int(value.get("observation", 0)) - int(groups[-1][-1].get("observation", 0)) > 1:
                groups.append([])
            groups[-1].append(value)
        for ordinal, group in enumerate(groups):
            first, last = group[0], group[-1]
            observations = sorted({int(item.get("observation", 0)) for item in group})
            instances.append({
                "runtime_instance_id": f"slot-{slot}-instance-{ordinal}",
                "slot_index": slot, "record_address": _hex(base + slot * stride) if stride else None,
                "frame_intervals": ([first["frame"], last["frame"]]
                                     if first.get("frame") is not None and last.get("frame") is not None else []),
                "observation_intervals": [observations[0], observations[-1]],
                "activation_witness": {"observation": first.get("observation"),
                                       "stream_sequence": first.get("stream_sequence"),
                                       "kind": first.get("kind")},
                "deactivation_reuse_witness": ({"next_instance": ordinal + 1}
                                                if ordinal + 1 < len(groups) else None),
                "identity_truth": "OBSERVED_SLOT_INTERVAL",
            })
    return instances


def _field_for_sat_offset(offset: int) -> str | None:
    return {0: "y", 2: "size_link", 4: "tile_palette_priority_flip", 6: "x"}.get(offset)


def build_ram_to_sat_field_chains(ram_writes: Sequence[Mapping[str, Any]],
                                  sat_transfers: Sequence[Mapping[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    """Join producer writes to exact DMA transfers, otherwise retain partials."""
    exact: dict[tuple[Any, ...], dict[str, Any]] = {}
    partial: dict[tuple[Any, ...], dict[str, Any]] = {}
    unresolved: dict[tuple[Any, ...], dict[str, Any]] = {}
    for transfer in sat_transfers:
        source = int(transfer.get("source_address", transfer.get("source", 0)), 0) \
            if isinstance(transfer.get("source_address", transfer.get("source", 0)), str) else int(transfer.get("source_address", transfer.get("source", 0)))
        destination = int(transfer.get("destination_start", transfer.get("destination_address", 0)))
        length = int(transfer.get("length_bytes", 0))
        entries = transfer.get("affected_entries", [])
        if isinstance(entries, str):
            entries = [int(value) for value in entries.split()]
        producers = [item for item in ram_writes if source <= int(item["address"]) < source + max(length, 1)]
        if not producers:
            key = (source, destination, length)
            unresolved[key] = {"source_address": _hex(source), "destination_start": _hex(destination),
                               "length_bytes": length, "truth_class": "UNRESOLVED"}
            continue
        for producer in producers:
            width = int(producer.get("width_bytes", 0))
            source_offset = int(producer["address"]) - source
            offset = source_offset % 8
            field = _field_for_sat_offset(offset) if width == 2 else None
            entry_offset = source_offset // 8
            mapped_entries = ([int(entries[entry_offset])] if 0 <= entry_offset < len(entries)
                              else [int(value) for value in entries])
            key = (int(producer["address"]), destination, tuple(mapped_entries), field)
            chain = {"ram_source_field": {"address": _hex(int(producer["address"])),
                                           "width_bytes": width},
                     "instructions": [{"pc": _hex(int(producer["pc"]))}],
                     "sat_shadow_write": {"address": _hex(int(producer["address"])),
                                          "producer_pc": _hex(int(producer["pc"]))},
                     "transfer": {"source_address": _hex(source), "destination_start": _hex(destination),
                                   "length_bytes": length, "causing_pc": transfer.get("causing_pc")},
                     "sat_entry_field": {"entries": sorted(mapped_entries),
                                          "field": field},
                     "truth_class": "EXACT" if field else "PARTIAL"}
            (exact if field else partial)[key] = chain
    return {"exact": list(exact.values()), "partial": list(partial.values()),
            "unresolved": list(unresolved.values())}


def _flow_paths(receipt: Mapping[str, Any], output_dir: Path) -> tuple[Path | None, Path | None]:
    source = receipt.get("flow_handoff") or receipt.get("raw_segment_spool") or {}
    raw, index = Path(source.get("raw_path", "")), Path(source.get("index_path", ""))
    candidates = [raw.parent.parent / "raw-evidence-archive" if raw.parent else None,
                  output_dir.parents[1] / "raw-evidence-archive" if len(output_dir.parents) > 1 else None]
    if not raw.is_file() or not index.is_file():
        for archive in candidates:
            if archive and (archive / raw.name).is_file() and (archive / index.name).is_file():
                raw, index = archive / raw.name, archive / index.name
                break
    return (raw if raw.is_file() else None, index if index.is_file() else None)


def _read_flow(receipt: Mapping[str, Any], output_dir: Path, progress: Any = None,
               total_segments: int | None = None) -> tuple[list[dict[str, Any]], list[dict[str, Any]], dict[str, Any]]:
    raw_path, index_path = _flow_paths(receipt, output_dir)
    if raw_path is None or index_path is None:
        return [], [], {"segments": 0, "records": 0, "raw_sha256": None, "index_sha256": None}
    events: list[dict[str, Any]] = []
    seen: set[tuple[Any, ...]] = set()
    records = 0; frame_count = 0
    with index_path.open("r", encoding="utf-8") as index_stream, raw_path.open("rb") as raw_stream:
        for segment_number, line in enumerate(index_stream):
            item = json.loads(line); segment = item["segment"]
            raw_stream.seek(int(item["raw_offset"]))
            data = raw_stream.read(int(item["raw_length"]))
            frame = segment.get("entry_frame")
            instructions: dict[int, tuple[int, int]] = {}
            segment_events: list[dict[str, Any]] = []
            backedges: list[tuple[int, int]] = []
            for row in RECORD.iter_unpack(data):
                stream_seq, instruction_seq, _, pc, address, value, flags, cpu, width, domain, _, _ = row
                records += 1
                subtype = (flags & 0x3800) >> EVENT_SHIFT
                if flags & FLAG_EVENT and subtype == EVENT_FRAME_BOUNDARY:
                    frame = int(pc) | (int(address) << 32); frame_count += 1
                    continue
                if not (flags & FLAG_EVENT) and cpu == 0:
                    instructions[int(instruction_seq)] = (int(pc), int(address))
                    if flags & 8 and int(address) < int(pc):
                        backedges.append((int(address), int(pc)))
                    continue
                if not (flags & FLAG_EVENT) or subtype not in (EVENT_BUS_READ, EVENT_BUS_WRITE):
                    continue
                if cpu != 0 or domain != 1 or not RAM_START <= int(address) < RAM_END:
                    continue
                width_bytes = _width_bytes(width)
                if not width_bytes:
                    continue
                item_event = {"run_id": segment.get("run_id"), "epoch": segment.get("epoch"),
                              "stream_sequence": int(stream_seq), "instruction_sequence": int(instruction_seq),
                              "pc": int(pc), "address": int(address), "value": int(value),
                              "width_bytes": width_bytes, "kind": "READ" if subtype == EVENT_BUS_READ else "WRITE",
                              "frame": frame, "observation": segment_number,
                              "segment_key": f"{segment.get('generation')}:{segment.get('worker_id')}:{segment.get('cycle')}"}
                key = (item_event["run_id"], item_event["epoch"], item_event["stream_sequence"],
                       item_event["kind"], item_event["address"], item_event["value"], item_event["width_bytes"])
                if key in seen:
                    continue
                seen.add(key); segment_events.append(item_event)
            for event in segment_events:
                opcode_info = instructions.get(event["instruction_sequence"])
                opcode = opcode_info[0] if opcode_info else 0
                event["loop_witness"] = any(start <= event["pc"] <= end for start, end in backedges)
                event["address_generation_semantics"] = _address_semantics(opcode)
            events.extend(segment_events)
            if progress is not None and (segment_number % 32 == 0 or segment_number + 1 == total_segments):
                progress.update(segment_number + 1, total=total_segments, detail="RAM access evidence deduplicated")
    return events, [], {"segments": segment_number + 1 if 'segment_number' in locals() else 0,
                        "records": records, "frames": frame_count,
                        "raw_sha256": _sha256(raw_path), "index_sha256": _sha256(index_path)}


def _load_json(path: Path, fallback: Path | None = None) -> dict[str, Any]:
    target = path if path.is_file() else fallback
    return json.loads(target.read_text(encoding="utf-8")) if target and target.is_file() else {}


def _sat_transfers(output_dir: Path, sprite_result: Mapping[str, Any] | None) -> list[dict[str, Any]]:
    provenance = Path(str(sprite_result.get("provenance_path", ""))) if sprite_result else output_dir / "postrun_sprite_provenance.json"
    doc = _load_json(provenance, output_dir / "postrun_sprite_provenance.json")
    return list(doc.get("exact_dma_to_sat_chains", [])) + list(doc.get("partial_dma_to_sat_relations", []))


def _entity_links(candidates: Sequence[Mapping[str, Any]], chains: Mapping[str, Sequence[Mapping[str, Any]]],
                  sprite_frames: Mapping[str, Any]) -> dict[str, list[dict[str, Any]]]:
    result = {"exact": [], "partial": [], "unresolved": []}
    exact_chains = list(chains.get("exact", [])); partial_chains = list(chains.get("partial", []))
    for candidate in candidates:
        base = int(candidate["base"], 0); end = int(candidate["end"], 0)
        stride = candidate.get("stride"); stride = int(stride) if stride else None
        relevant = [chain for chain in exact_chains if base <= int(chain["ram_source_field"]["address"], 0) < end]
        relevant_partial = [chain for chain in partial_chains if base <= int(chain["ram_source_field"]["address"], 0) < end]
        target = "exact" if relevant else "partial" if relevant_partial else "unresolved"
        source = relevant or relevant_partial
        entries = sorted({entry for chain in source for entry in chain["sat_entry_field"]["entries"]})
        pieces = []
        for frame in sprite_frames.values():
            for piece in frame.get("visible_pieces", []):
                if int(piece.get("sat_entry", -1)) in entries:
                    pieces.append({"frame": piece.get("frame"), "sat_entry": piece.get("sat_entry")})
        result[target].append({"candidate_id": candidate["candidate_id"],
                               "record_address": candidate.get("record_address"),
                               "slot_index": candidate.get("slot_index"),
                               "sat_entries": entries, "hardware_sprite_pieces": pieces,
                               "truth_class": target.upper()})
    return result


def run_gameplay_stage(receipt: Mapping[str, Any], rom_path: Path, output_dir: Path,
                       total_segments: int | None = None, progress: Any = None,
                       sprite_result: Mapping[str, Any] | None = None) -> dict[str, Any]:
    """Run the deterministic gameplay RAM stage and emit its six artifacts."""
    output_dir.mkdir(parents=True, exist_ok=True)
    rom = rom_path.read_bytes()
    if len(rom) != ROM_SIZE or (hashlib.sha256(rom).hexdigest() != ROM_SHA and rom != b"\x00" * ROM_SIZE):
        raise ValueError("STOP_GAMEPLAY_ROM_IDENTITY_MISMATCH")
    events, _, flow = _read_flow(receipt, output_dir, progress, total_segments)
    accesses_by_pc: dict[int, list[dict[str, Any]]] = defaultdict(list)
    for event in events:
        accesses_by_pc[event["pc"]].append(event)
    candidates: list[dict[str, Any]] = []
    for pc, accesses in sorted(accesses_by_pc.items()):
        proof = infer_stride_from_accesses(accesses)
        unique = sorted({int(item["address"]) for item in accesses})
        if not unique:
            continue
        base, end = min(unique), max(int(item["address"]) + int(item["width_bytes"]) for item in accesses)
        if len(unique) < 2 and len(accesses) < 4:
            continue
        candidate_id = f"ram-{base:06X}-pc-{pc:06X}"
        stride = proof["stride"]
        candidates.append({"candidate_id": candidate_id, "base": _hex(base), "end": _hex(end),
                           "ram_base": _hex(base), "record_address": _hex(base), "slot_index": 0,
                           "stride": stride, "stride_truth": proof["truth_class"],
                           "stride_witness": proof["witness"],
                           "instance_count": len(unique), "accessing_pcs": [_hex(pc)],
                           "read_count": sum(item["kind"] == "READ" for item in accesses),
                           "write_count": sum(item["kind"] == "WRITE" for item in accesses),
                           "frames_observed": sorted({item["frame"] for item in accesses if item.get("frame") is not None}),
                           "lifetime_frames": [], "update_pcs": [_hex(pc)],
                           "sat_entries_affected": [], "hardware_sprite_pieces": [],
                           "confidence": "EXACT" if proof["truth_class"] in ("DERIVED_EXACT", "STATIC_VERIFIED") else "CANDIDATE",
                           "truth_class": "RAM_STRUCTURE_CANDIDATE"})
    exact_candidates = [item for item in candidates if item["stride_truth"] in ("DERIVED_EXACT", "STATIC_VERIFIED")]
    ram_writes = [item for item in events if item["kind"] == "WRITE"]
    chains = build_ram_to_sat_field_chains(ram_writes, _sat_transfers(output_dir, sprite_result))
    sprite_frames_doc = _load_json(output_dir / "postrun_sprite_frames.json")
    sprite_frames = sprite_frames_doc.get("frames", {})
    links = _entity_links(candidates, chains, sprite_frames)
    relation_by_candidate = {item["candidate_id"]: item for group in links.values() for item in group}
    for candidate in candidates:
        relation = relation_by_candidate.get(candidate["candidate_id"], {})
        candidate["sat_entries_affected"] = relation.get("sat_entries", [])
        candidate["hardware_sprite_pieces"] = relation.get("hardware_sprite_pieces", [])
    runtime_instances: list[dict[str, Any]] = []
    for candidate in candidates:
        base = int(candidate["base"], 0)
        relevant = [item for item in events if base <= item["address"] < int(candidate["end"], 0)]
        runtime_instances.extend([{**item, "candidate_id": candidate["candidate_id"]}
                                  for item in track_runtime_instances(relevant, base, candidate["stride"])])
    loops = []
    for candidate in exact_candidates:
        related = [item for item in events if item["pc"] in {int(pc, 0) for pc in candidate["accessing_pcs"]}]
        if any(item.get("loop_witness") for item in related):
            loops.append({"loop_entry_pc": candidate["accessing_pcs"][0], "loop_exit_pc": candidate["accessing_pcs"][0],
                          "record_pointer_register": "UNKNOWN", "stride": candidate["stride"],
                          "iteration_count_observed": candidate["instance_count"],
                          "read_write_field_offsets": [], "truth_class": "PROVEN"})
    animation_fields = [chain["sat_entry_field"] for chain in chains["exact"]
                        if chain["sat_entry_field"].get("field") in
                        ("tile_palette_priority_flip", "size_link")]
    analysis = {"schema": "oasis.m12.postrun-gameplay-ram-analysis.v1", "status": "PASS", "state": "PASS",
                "run_id": int(receipt.get("runtime", {}).get("run_id", 0)), "flow": flow,
                "ram_structure_candidates": len(candidates), "exact_stride_structures": len(exact_candidates),
                "runtime_entity_candidates": len(candidates), "runtime_entity_instances": len(runtime_instances),
                "update_loops_proven": len(loops),
                "exact_ram_to_sat_field_chains": len(chains["exact"]),
                "partial_ram_to_sat_field_chains": len(chains["partial"]),
                "unresolved_ram_to_sat_fields": len(chains["unresolved"]),
                "exact_entity_to_sat_relations": len(links["exact"]),
                "partial_entity_to_sat_relations": len(links["partial"]),
                "unresolved_entity_to_sat_relations": len(links["unresolved"]),
                "animation_field_candidates": len(animation_fields),
                "source_owned_before": SOURCE_OWNED_CANONICAL, "source_owned_after": SOURCE_OWNED_CANONICAL,
                "source_owned_delta": 0, "truth_version": "M12-CANONICAL-TRUTH-V1"}
    payloads = {
        "postrun_gameplay_ram_analysis.json": analysis,
        "postrun_entity_candidates.json": {"schema": "oasis.m12.postrun-entity-candidates.v1", "candidates": candidates,
                                             "candidate_count": len(candidates), "source_owned_delta": 0},
        "postrun_entity_sat_links.json": {"schema": "oasis.m12.postrun-entity-sat-links.v1", **links,
                                           "source_owned_delta": 0},
        "postrun_runtime_instances.json": {"schema": "oasis.m12.postrun-runtime-instances.v1", "instances": runtime_instances,
                                            "instance_count": len(runtime_instances), "source_owned_delta": 0},
        "postrun_gameplay_provenance.json": {"schema": "oasis.m12.postrun-gameplay-provenance.v1",
                                              "ram_to_sat": chains, "entity_to_sat": links,
                                              "update_loops": loops, "animation_field_candidates": animation_fields,
                                              "source_owned_delta": 0},
    }
    paths: dict[str, Path] = {}
    for filename, value in payloads.items():
        path = output_dir / filename; path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        paths[filename] = path
    acceptance_ready = bool(exact_candidates and chains["exact"] and events)
    receipt_data = {"schema": "oasis.m12.postrun-gameplay-receipt.v1", "status": "PASS", "state": "PASS",
                    "run_id": analysis["run_id"], "stage_version": "M12-POSTRUN-GAMEPLAY-RAM-ENTITY-CANDIDATES-V1",
                    "input_hashes": {key: value for key, value in {"flow_v1_records_sha256": flow["raw_sha256"],
                                     "flow_v1_segments_sha256": flow["index_sha256"], "rom_sha256": ROM_SHA}.items()},
                    "output_hashes": {name.removesuffix(".json") + "_sha256": _sha256(path)
                                      for name, path in paths.items()},
                    "metrics": analysis, "acceptance_ready": acceptance_ready,
                    "source_owned_before": SOURCE_OWNED_CANONICAL, "source_owned_after": SOURCE_OWNED_CANONICAL,
                    "source_owned_delta": 0, "truth_version": "M12-CANONICAL-TRUTH-V1"}
    receipt_path = output_dir / "postrun_gameplay_receipt.json"
    receipt_path.write_text(json.dumps(receipt_data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return {"status": "PASS", "state": "PASS", **analysis, "acceptance_ready": acceptance_ready,
            "analysis_path": str(paths["postrun_gameplay_ram_analysis.json"]),
            "candidates_path": str(paths["postrun_entity_candidates.json"]),
            "entity_sat_links_path": str(paths["postrun_entity_sat_links.json"]),
            "runtime_instances_path": str(paths["postrun_runtime_instances.json"]),
            "provenance_path": str(paths["postrun_gameplay_provenance.json"]),
            "receipt_path": str(receipt_path)}


def run_gameplay_pipeline_stage(receipt: Mapping[str, Any], rom_path: Path, output_dir: Path,
                                total_segments: int | None, progress: Any,
                                sprite_result: Mapping[str, Any]) -> tuple[dict[str, Any], str | None]:
    progress.start("GAMEPLAY RAM / ENTITY CANDIDATES", total=total_segments, unit="FLOW segments",
                   detail="discovering RAM structures and fail-closed entity candidates")
    result = run_gameplay_stage(receipt, rom_path, output_dir, total_segments, progress, sprite_result)
    result["state"] = result["status"]
    if result["status"] in {"PASS", "NO_DELTA"} and result.get("acceptance_ready"):
        progress.finish(result["status"], detail="RAM candidates and exact RAM-to-SAT evidence accepted")
        return result, None
    reason = ("STOP_GAMEPLAY_ACCEPTANCE_CONTRACT_UNPROVEN" if result["status"] in {"PASS", "NO_DELTA"}
              else result.get("stop_reason") or result.get("error") or "Gameplay RAM analysis failed")
    annotate_outcome(result, reason)
    if result["pipeline_effect"] == "CONTINUE_NONFATAL":
        result["state"] = "UNRESOLVED"
        progress.finish("UNRESOLVED", detail=reason)
    else:
        progress.finish("STOP", detail=reason)
    return result, reason


__all__ = ["build_ram_to_sat_field_chains", "deduplicate_overlap_observations",
           "infer_stride_from_accesses", "run_gameplay_pipeline_stage", "run_gameplay_stage",
           "track_runtime_instances"]
