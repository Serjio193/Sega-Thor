"""Build a fail-closed catalog of frame/SAT-entry sprite observations."""

from __future__ import annotations

import argparse
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any, Mapping, Sequence

from m12_sprite_reconstruction import (  # type: ignore[import-not-found]
    ReconstructionError,
    compose_piece,
    decode_genesis_palette,
    decode_genesis_tile,
    decode_sat,
    reconstruction_metadata,
)


SCHEMA = "oasis.m68k.hardware-sprite-piece-catalog.v1"
CAPTURE_SCHEMA = "oasis.m68k.m12-sprite-piece-capture.v1"
EXPECTED_ROM = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"


def _fail(message: str) -> None:
    raise ReconstructionError(message)


def _bytes(value: Any, name: str) -> bytes:
    if not isinstance(value, list) or any(not isinstance(item, int) or not 0 <= item <= 255
                                          for item in value):
        _fail(f"{name} is not a byte array")
    return bytes(value)


def _hex(value: Any, name: str) -> int:
    if not isinstance(value, str) or not value.lower().startswith("0x"):
        _fail(f"{name} is not a hexadecimal address")
    try:
        return int(value, 16)
    except ValueError as exc:
        raise ReconstructionError(f"{name} is not a hexadecimal address") from exc


def _hash(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def _matrix(pixels: Sequence[Sequence[Any]], palette: Sequence[Sequence[int]]) -> tuple[list[list[Any]], list[list[bool]], list[list[Any]]]:
    values: list[list[Any]] = []
    transparency: list[list[bool]] = []
    cram_indices: list[list[Any]] = []
    for row in pixels:
        value_row: list[Any] = []
        transparent_row: list[bool] = []
        cram_row: list[Any] = []
        for pixel in row:
            if pixel is None:
                value_row.append(None)
                transparent_row.append(True)
                cram_row.append(None)
            else:
                value_row.append({"pixel_index": pixel.pixel_index,
                                  "palette_line": pixel.palette_line,
                                  "cram_index": pixel.cram_index,
                                  "priority": pixel.priority,
                                  "rgb": list(palette[pixel.pixel_index])})
                transparent_row.append(False)
                cram_row.append(pixel.cram_index)
        values.append(value_row)
        transparency.append(transparent_row)
        cram_indices.append(cram_row)
    return values, transparency, cram_indices


def _fingerprint(attributes: Mapping[str, Any], tiles: Sequence[Mapping[str, Any]],
                 pixel_hash: str) -> tuple[str, dict[str, Any]]:
    semantic = {
        "dimensions": [attributes["width_cells"], attributes["height_cells"]],
        "tile_indices": [tile["tile_index"] for tile in tiles],
        "tile_hashes": [tile["sha256"] for tile in tiles],
        "palette_line": attributes["palette_line"],
        "flip_h": attributes["flip_h"], "flip_v": attributes["flip_v"],
        "priority": attributes["priority"], "decoded_pixel_hash": pixel_hash,
    }
    encoded = json.dumps(semantic, sort_keys=True, separators=(",", ":")).encode()
    return _hash(encoded), semantic


def _publication(observation: Mapping[str, Any], sat: bytes) -> tuple[str, str, dict[str, Any]]:
    """Return status, reason, and exact publication evidence for entry zero."""
    publication = observation.get("publication")
    offset = observation.get("publication_offset") or 0
    if publication is None and observation.get("sat_entry") == 0:
        publication, offset = observation.get("dma"), 0
    if publication is None:
        return "absent", "SAT entry has no captured DMA publication", {}
    shadow = observation.get("shadow_bytes")
    if shadow is None or not isinstance(publication, Mapping):
        return "missing", "producer/shadow-to-VRAM publication evidence is absent", {}
    shadow_raw = _bytes(shadow, "shadow SAT bytes")
    source = _bytes(publication.get("source_bytes"), "DMA source bytes")
    if len(shadow_raw) != 8 or not isinstance(offset, int) or offset < 0 or len(source) < offset + 8:
        return "conflict", "publication byte lengths disagree", {}
    if publication.get("frame") != observation.get("frame"):
        return "conflict", "DMA frame differs from SAT observation frame", {}
    try:
        destination = _hex(publication.get("destination"), "DMA destination") + offset
        source_address = _hex(publication.get("source"), "DMA source") + offset
        shadow_address = _hex(observation.get("shadow_address"), "shadow address")
    except ReconstructionError as exc:
        return "conflict", str(exc), {}
    if destination != 0xD000 + int(observation.get("sat_entry", 0)) * 8 or source_address != shadow_address:
        return "conflict", "DMA source or destination does not identify the SAT publication", {}
    source_slice = source[offset:offset + 8]
    declared_slice = observation.get("publication_bytes")
    if declared_slice is not None and _bytes(declared_slice, "publication bytes") != source_slice:
        return "conflict", "publication slice disagrees with DMA source bytes", {}
    if shadow_raw == source_slice and shadow_raw != sat:
        return "conflict", "shadow and DMA source agree but VRAM SAT bytes disagree", {}
    if shadow_raw != source_slice or shadow_raw != sat:
        return "observed", "DMA was observed, but same-frame shadow/SAT values differ", {}
    evidence = {"shadow_address": observation.get("shadow_address"),
                "shadow_bytes": list(shadow_raw),
                "shadow_sha256": _hash(shadow_raw), "dma": dict(publication),
                "publication_offset": offset,
                "temporal_claim": "same_frame_value_linkage; producer_to_DMA_causality_not_generalized"}
    return "pass", "complete S2 publication linkage", evidence


def _entry(observation: Mapping[str, Any], run_id: str, capture_sha256: str) -> dict[str, Any]:
    frame, sat_entry = observation.get("frame"), observation.get("sat_entry")
    result: dict[str, Any] = {
        "key": {"run_id": run_id, "frame": frame, "sat_entry": sat_entry},
        "frame": frame, "sat_entry": sat_entry, "classification": "INCOMPLETE",
        "reason": "required evidence is missing", "evidence_references": {
            "capture_sha256": capture_sha256,
            "observation_id": f"{run_id}:{frame}:{sat_entry}",
        },
    }
    if isinstance(observation.get("causality"), Mapping):
        result["causality"] = dict(observation["causality"])
    try:
        sat = _bytes(observation.get("sat_bytes"), "SAT bytes")
        if len(sat) != 8:
            _fail("SAT entry must be exactly 8 bytes")
        decoded = decode_sat(sat, 0, 1)[0]
        attributes = {"x": decoded.x, "y": decoded.y,
                      "width_cells": decoded.width_cells, "height_cells": decoded.height_cells,
                      "link": decoded.link, "tile_index": decoded.tile.tile_index,
                      "palette_line": decoded.tile.palette, "priority": decoded.tile.priority,
                      "flip_h": decoded.tile.flip_h, "flip_v": decoded.tile.flip_v}
        result["vram_sat"] = {"address": observation.get("sat_address"),
                              "raw_bytes": list(sat), "sha256": _hash(sat)}
        result["decoded_attributes"] = attributes
        publication_status, publication_reason, publication = _publication(observation, sat)
        result["publication"] = publication
        result["shadow_sat"] = None
        if observation.get("shadow_bytes") is not None:
            result["shadow_sat"] = {
                "address": observation.get("shadow_address"),
                "raw_bytes": list(_bytes(observation["shadow_bytes"], "shadow SAT bytes")),
            }
        raw_tiles = observation.get("tiles")
        expected_tiles = decoded.width_cells * decoded.height_cells
        if not isinstance(raw_tiles, list) or len(raw_tiles) != expected_tiles:
            result["tiles"] = raw_tiles if isinstance(raw_tiles, list) else []
            result["reason"] = "VRAM tile list is incomplete for SAT dimensions"
            return result
        expected_indices = [decoded.tile.tile_index + cell_x * decoded.height_cells + cell_y
                            for cell_x in range(decoded.width_cells)
                            for cell_y in range(decoded.height_cells)]
        observed_indices = [tile.get("tile_index") for tile in raw_tiles
                            if isinstance(tile, Mapping)]
        if set(observed_indices) != set(expected_indices):
            result["classification"] = "CONFLICT"
            result["reason"] = "SAT tile index set disagrees with captured VRAM tiles"
            return result
        tile_records: list[dict[str, Any]] = []
        tile_map: dict[int, tuple[tuple[int, ...], ...]] = {}
        for raw_tile in raw_tiles:
            if not isinstance(raw_tile, Mapping):
                _fail("malformed tile record")
            if raw_tile.get("frame") not in (None, frame):
                result["classification"] = "CONFLICT"
                result["reason"] = "VRAM tile bytes are from the wrong frame"
                return result
            tile_index = raw_tile.get("tile_index")
            tile_raw = _bytes(raw_tile.get("bytes"), "VRAM tile bytes")
            if len(tile_raw) != 32 or tile_index in tile_map:
                _fail("missing or duplicate VRAM tile record")
            tile_address = raw_tile.get("vram_address")
            if tile_address != tile_index * 32:
                _fail("VRAM tile address disagrees with tile index")
            declared_hash = raw_tile.get("sha256")
            tile_hash = _hash(tile_raw)
            if declared_hash is not None and declared_hash.lower() != tile_hash:
                result["classification"] = "CONFLICT"
                result["reason"] = "declared tile hash disagrees with VRAM tile bytes"
                return result
            tile_map[tile_index] = decode_genesis_tile(tile_raw)
            tile_records.append({"tile_index": tile_index, "vram_address": tile_address,
                                 "raw_bytes": list(tile_raw), "sha256": tile_hash})
        cram_raw = _bytes(observation.get("cram_bytes"), "CRAM bytes")
        if len(cram_raw) != 32:
            result["reason"] = "exact CRAM snapshot is absent or incomplete"
            return result
        expected_cram = decoded.tile.palette * 32
        if observation.get("cram_base_address") != expected_cram:
            result["classification"] = "CONFLICT"
            result["reason"] = "CRAM snapshot is from the wrong palette line"
            return result
        if observation.get("cram_frame") not in (None, frame):
            result["classification"] = "CONFLICT"
            result["reason"] = "CRAM snapshot is from the wrong frame"
            return result
        palette = decode_genesis_palette(cram_raw)
        pixels = compose_piece(decoded, tile_map)
        matrix, transparency, cram_indices = _matrix(pixels, palette)
        pixel_hash = _hash(json.dumps(matrix, sort_keys=True, separators=(",", ":")).encode())
        fingerprint, semantic = _fingerprint(attributes, tile_records, pixel_hash)
        result.update({"tiles": tile_records,
                       "cram": {"base_address": expected_cram, "raw_bytes": list(cram_raw),
                                "sha256": _hash(cram_raw)},
                       "decoded_pixels": matrix, "transparency": transparency,
                       "palette_cram_indices": cram_indices, "pixel_sha256": pixel_hash,
                       "semantic_fingerprint": fingerprint, "semantic": semantic,
                       "priority_metadata": reconstruction_metadata()})
        result["frame_coherence"] = {
            "observation_frame": frame,
            "capture_phase": observation.get("capture_phase", "frame_end"),
            "shadow_sat": {"frame": frame, "capture_phase": observation.get("capture_phase", "frame_end")},
            "vram_sat": {"frame": frame, "capture_phase": observation.get("capture_phase", "frame_end")},
            "vram_tiles": {"frame": frame, "capture_phase": observation.get("capture_phase", "frame_end")},
            "cram": {"frame": frame, "capture_phase": observation.get("capture_phase", "frame_end")},
            "publication": ({"frame": publication.get("frame"), "sequence": publication.get("sequence")}
                             if isinstance(publication, Mapping) else None),
        }
        if publication_status == "conflict":
            result["classification"], result["reason"] = "CONFLICT", publication_reason
        elif publication_status == "observed":
            result["classification"], result["reason"] = "OBSERVED_LINKAGE_ONLY", publication_reason
        elif observation.get("linkage_only"):
            result["classification"], result["reason"] = "OBSERVED_LINKAGE_ONLY", publication_reason
        elif publication_status == "pass":
            result["classification"], result["reason"] = "PROVEN", "complete S2 chain closed"
        else:
            result["classification"], result["reason"] = "INCOMPLETE", publication_reason
        return result
    except ReconstructionError as exc:
        result["reason"] = str(exc)
        return result


def _gap_reasons(entry: Mapping[str, Any]) -> list[str]:
    if entry.get("classification") == "PROVEN":
        return []
    reasons: list[str] = []
    text = str(entry.get("reason", ""))
    attributes = entry.get("decoded_attributes") or {}
    expected_tiles = int(attributes.get("width_cells", 1)) * int(attributes.get("height_cells", 1))
    tiles = entry.get("tiles")
    if entry.get("classification") == "OBSERVED_LINKAGE_ONLY":
        reasons.append("AMBIGUOUS_TEMPORAL_LINK")
    if "tile bytes are from the wrong frame" in text.lower():
        reasons.append("WRONG_FRAME_TILE")
    elif "wrong frame" in text.lower():
        reasons.append("WRONG_FRAME_CRAM")
    if "SAT" in text and "disagree" in text:
        reasons.append("SAT_VALUE_MISMATCH")
    if entry.get("vram_sat") is None:
        reasons.append("MISSING_VRAM_SAT")
    if entry.get("shadow_sat") is None:
        reasons.append("MISSING_SHADOW_SAT")
    if not entry.get("publication"):
        reasons.append("MISSING_DMA_PUBLICATION")
    if entry.get("sat_entry") != 0 and not entry.get("publication"):
        reasons.append("NO_PRODUCER_OBSERVATION")
    if not isinstance(tiles, list) or not tiles:
        reasons.append("MISSING_TILE_BYTES")
    elif len(tiles) < expected_tiles:
        reasons.append("PARTIAL_MULTI_TILE_CAPTURE" if expected_tiles > 1 else "MISSING_TILE_BYTES")
    if entry.get("cram") is None:
        reasons.append("MISSING_CRAM")
    if "hash" in text.lower():
        reasons.append("TILE_HASH_MISMATCH")
    if not reasons:
        reasons.append("OTHER_EXACT_REASON")
    return sorted(set(reasons))


def _overlap_event(event: Mapping[str, Any], start: int, end: int) -> bool:
    try:
        event_start = int(event.get("destination_start"))
        event_end = int(event.get("destination_end"))
    except (TypeError, ValueError):
        return True
    return event_start < end and event_end > start


def _between(events: list[Mapping[str, Any]], start_frame: int, end_frame: int,
             start_sequence: int | None, start: int, end: int) -> list[dict[str, Any]]:
    result = []
    for event in events:
        frame = event.get("frame")
        if not isinstance(frame, int) or frame < start_frame or frame > end_frame:
            continue
        if frame == start_frame and start_sequence is not None and event.get("sequence", 0) <= start_sequence:
            continue
        if _overlap_event(event, start, end):
            result.append(dict(event))
    return result


def _persistence(entry: dict[str, Any], prior: dict[str, Any], capture: Mapping[str, Any]) -> tuple[bool, dict[str, Any], str]:
    start_frame, end_frame = prior.get("frame"), entry.get("frame")
    if not isinstance(start_frame, int) or not isinstance(end_frame, int) or end_frame <= start_frame:
        return False, {}, "no later frame to persist"
    if capture.get("sat_mutation_observation_complete") is not True:
        return False, {"sat_mutation_observation_complete": False}, "SAT mutation coverage is incomplete"
    if capture.get("shadow_mutation_observation_complete") is not True:
        return False, {"shadow_mutation_observation_complete": False}, "shadow mutation coverage is incomplete"
    prior_sat, later_sat = prior.get("vram_sat"), entry.get("vram_sat")
    if not prior_sat or not later_sat or prior_sat.get("raw_bytes") != later_sat.get("raw_bytes"):
        return False, {}, "VRAM SAT bytes changed across persistence interval"
    try:
        sat_start = _hex(later_sat.get("address"), "SAT address")
        shadow_start = _hex(entry["shadow_sat"]["address"], "shadow address")
    except (KeyError, ReconstructionError) as exc:
        return False, {}, str(exc)
    publication = prior.get("publication") or {}
    publication_sequence = publication.get("dma", {}).get("sequence") if isinstance(publication, Mapping) else None
    sat_events = _between(list(capture.get("sat_mutation_events", [])), start_frame, end_frame,
                          publication_sequence, sat_start, sat_start + 8)
    base_events = [dict(event) for event in capture.get("sat_base_events", [])
                   if isinstance(event.get("frame"), int) and start_frame <= event["frame"] <= end_frame]
    shadow_events = _between(list(capture.get("shadow_mutation_events", [])), start_frame, end_frame,
                             None, shadow_start, shadow_start + 8)
    shadow_persisted = bool(entry.get("shadow_sat") and prior.get("shadow_sat") and
                            entry["shadow_sat"].get("raw_bytes") == prior["shadow_sat"].get("raw_bytes") and
                            not shadow_events)
    evidence = {
        "relation": "PROVEN_PERSISTED_SAT_STATE", "publication_frame": start_frame,
        "observation_frame": end_frame, "persistence_distance_frames": end_frame - start_frame,
        "sat_bytes_at_publication": prior_sat.get("raw_bytes"),
        "sat_bytes_at_observation": later_sat.get("raw_bytes"),
        "mutation_events_between": sat_events, "sat_base_changes_between": base_events,
        "sat_mutation_observation_complete": True,
        "shadow_persistence": {
            "relation": "PROVEN_PERSISTED_SHADOW_SAT_STATE" if shadow_persisted else "UNPROVEN",
            "mutation_events_between": shadow_events,
            "bytes_unchanged": shadow_persisted,
            "shadow_mutation_observation_complete": True,
        },
    }
    if sat_events or base_events:
        return False, evidence, "intervening SAT mutation or SAT-base change invalidates persistence"
    if not shadow_persisted:
        return False, evidence, "shadow SAT persistence is not proven"
    return True, evidence, "SAT state persisted from an earlier direct publication"


def build_catalog(capture: Mapping[str, Any], capture_sha256: str,
                  run_id: str | None = None) -> dict[str, Any]:
    if capture.get("schema") != CAPTURE_SCHEMA:
        _fail("unsupported runtime capture schema")
    if str(capture.get("canonical_rom_sha256", "")).lower() != EXPECTED_ROM:
        _fail("runtime capture ROM identity is not canonical Beyond Oasis")
    if capture.get("state_writes_emitted") is not False:
        _fail("state-writing runtime capture is not accepted")
    resolved_run_id = run_id or str(capture.get("run_id", "m12_unknown"))
    observations = capture.get("catalog_observations", [])
    if not isinstance(observations, list):
        _fail("catalog observations are missing")
    entries = [_entry(observation, resolved_run_id, capture_sha256)
               for observation in observations]
    for entry in entries:
        entry["publication_provenance"] = (
            "DIRECT_PUBLICATION" if entry.get("classification") == "PROVEN" else "UNPROVEN")
        if entry.get("causality", {}).get("status") != "ORDERED_PRODUCER_WRITE_TO_DMA":
            entry.setdefault("causality", {"status": "UNPROVEN"})
    by_entry: dict[tuple[Any, Any], list[dict[str, Any]]] = defaultdict(list)
    for entry in sorted(entries, key=lambda item: (item.get("sat_entry", -1), item.get("frame", -1))):
        by_entry[(resolved_run_id, entry.get("sat_entry"))].append(entry)
    for entry in entries:
        if entry.get("classification") != "INCOMPLETE":
            continue
        candidates = [candidate for candidate in by_entry[(resolved_run_id, entry.get("sat_entry"))]
                      if candidate.get("frame", -1) < entry.get("frame", -1) and
                      candidate.get("publication_provenance") in ("DIRECT_PUBLICATION", "PERSISTED_FROM_PUBLICATION")]
        if not candidates:
            continue
        prior = candidates[-1]
        persisted, evidence, reason = _persistence(entry, prior, capture)
        entry["persistence"] = evidence
        entry["shadow_persistence"] = evidence.get("shadow_persistence")
        if persisted:
            entry["classification"] = "PROVEN"
            entry["reason"] = reason
            entry["publication_provenance"] = "PERSISTED_FROM_PUBLICATION"
    for entry in entries:
        entry["gap_reasons"] = _gap_reasons(entry)
    grouped: dict[tuple[Any, Any, Any], list[dict[str, Any]]] = defaultdict(list)
    for entry in entries:
        key = (entry["key"]["run_id"], entry["frame"], entry["sat_entry"])
        grouped[key].append(entry)
    for same_key in grouped.values():
        if len(same_key) < 2:
            continue
        signatures = {json.dumps(item.get("vram_sat"), sort_keys=True) for item in same_key}
        if len(signatures) > 1:
            for item in same_key:
                item["classification"], item["reason"] = "CONFLICT", "duplicate observation key disagrees"
    fingerprint_counts = Counter(item["semantic_fingerprint"] for item in entries
                                 if item.get("semantic_fingerprint"))
    counts = Counter(item["classification"] for item in entries)
    provenance_counts = Counter(item.get("publication_provenance") for item in entries)
    causality_counts = Counter((item.get("causality") or {}).get("status", "UNPROVEN") for item in entries)
    frames = sorted({item["frame"] for item in entries if isinstance(item["frame"], int)})
    return {
        "schema": SCHEMA, "run_id": resolved_run_id,
        "canonical_rom_sha256": EXPECTED_ROM, "capture_sha256": capture_sha256,
        "frames_captured": capture.get("frames_executed"),
        "frames_with_sat_observations": len(frames),
        "sat_observations": len(entries), "entries_total": len(entries),
        "proven_count": counts["PROVEN"], "incomplete_count": counts["INCOMPLETE"],
        "observed_linkage_only_count": counts["OBSERVED_LINKAGE_ONLY"],
        "conflict_count": counts["CONFLICT"],
        "direct_publication_count": provenance_counts["DIRECT_PUBLICATION"],
        "persisted_publication_count": provenance_counts["PERSISTED_FROM_PUBLICATION"],
        "unproven_publication_count": provenance_counts["UNPROVEN"],
        "ordered_causality_count": causality_counts["ORDERED_PRODUCER_WRITE_TO_DMA"],
        "unproven_causality_count": causality_counts["UNPROVEN"],
        "sat_mutation_observation_complete": capture.get("sat_mutation_observation_complete") is True,
        "shadow_mutation_observation_complete": capture.get("shadow_mutation_observation_complete") is True,
        "entries": entries,
        "semantic_fingerprints": [{"fingerprint": fingerprint, "observation_count": count}
                                  for fingerprint, count in sorted(fingerprint_counts.items())],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--run-id")
    args = parser.parse_args()
    raw = args.capture.read_bytes()
    catalog = build_catalog(json.loads(raw), _hash(raw), args.run_id)
    args.output.write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
