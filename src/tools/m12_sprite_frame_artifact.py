"""Compose one complete, fail-closed Genesis SAT sprite frame."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Mapping, Sequence

from m12_sprite_reconstruction import ReconstructionError  # type: ignore[import-not-found]


SCHEMA = "oasis.m68k.hardware-sprite-frame.v1"
CATALOG_SCHEMA = "oasis.m68k.hardware-sprite-piece-catalog.v1"
EXPECTED_ROM = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
PROVENANCE = ("DIRECT_PUBLICATION", "PERSISTED_FROM_PUBLICATION")


def _fail(message: str) -> None:
    raise ReconstructionError(message)


def _canonical(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


def _hash(value: Any) -> str:
    return hashlib.sha256(_canonical(value)).hexdigest()


def _int(value: Any, name: str) -> int:
    if not isinstance(value, int):
        _fail(f"{name} is not an integer")
    return value


def _frame_entries(catalog: Mapping[str, Any], run_id: str, frame: int) -> dict[int, dict[str, Any]]:
    result: dict[int, dict[str, Any]] = {}
    for item in catalog.get("entries", []):
        if not isinstance(item, Mapping) or item.get("frame") != frame:
            continue
        key = item.get("key") or {}
        item_run = key.get("run_id", catalog.get("run_id"))
        if item_run != run_id:
            continue
        sat_entry = item.get("sat_entry")
        if not isinstance(sat_entry, int) or sat_entry in result:
            _fail("STOP_SPRITE_FRAME_SAT_CHAIN_INVALID: duplicate SAT entry")
        result[sat_entry] = dict(item)
    if not result:
        _fail("STOP_SPRITE_FRAME_NOT_FOUND")
    return result


def _chain(entries: Mapping[int, Mapping[str, Any]]) -> list[int]:
    order: list[int] = []
    current = 0
    while True:
        if current in order or current not in entries:
            _fail("STOP_SPRITE_FRAME_SAT_CHAIN_INVALID")
        order.append(current)
        attributes = entries[current].get("decoded_attributes") or {}
        link = attributes.get("link")
        if not isinstance(link, int) or link < 0 or link >= 80:
            _fail("STOP_SPRITE_FRAME_SAT_CHAIN_INVALID")
        if link == 0:
            break
        current = link
    if set(order) != set(entries):
        _fail("STOP_SPRITE_FRAME_SAT_CHAIN_INVALID")
    return order


def _select_frame(catalog: Mapping[str, Any], run_id: str, requested: int | None) -> tuple[int, dict[int, dict[str, Any]], list[int]]:
    if requested is not None:
        entries = _frame_entries(catalog, run_id, requested)
        return requested, entries, _chain(entries)
    candidates: list[tuple[int, int, dict[int, dict[str, Any]], list[int]]] = []
    frames = sorted({item.get("frame") for item in catalog.get("entries", [])
                     if isinstance(item, Mapping) and isinstance(item.get("frame"), int)})
    for frame in frames:
        try:
            entries = _frame_entries(catalog, run_id, frame)
            chain = _chain(entries)
        except ReconstructionError:
            continue
        if all(item.get("classification") == "PROVEN" for item in entries.values()):
            candidates.append((len(chain), frame, entries, chain))
    if not candidates:
        _fail("STOP_SPRITE_FRAME_NO_COMPLETE_CHAIN")
    count, frame, entries, chain = sorted(candidates, key=lambda item: (-item[0], item[1]))[0]
    del count
    return frame, entries, chain


def _require_proven(entry: Mapping[str, Any]) -> None:
    if entry.get("classification") != "PROVEN":
        _fail("STOP_SPRITE_FRAME_NOT_FULLY_PROVEN")
    if entry.get("publication_provenance") not in PROVENANCE:
        _fail("STOP_SPRITE_FRAME_NOT_FULLY_PROVEN")
    for field in ("vram_sat", "shadow_sat", "tiles", "cram", "decoded_pixels", "transparency"):
        if not entry.get(field):
            _fail("STOP_SPRITE_FRAME_NOT_FULLY_PROVEN")


def _piece_metadata(entry: Mapping[str, Any], order_index: int) -> dict[str, Any]:
    _require_proven(entry)
    attrs = dict(entry["decoded_attributes"])
    width, height = attrs["width_cells"], attrs["height_cells"]
    pixels = entry["decoded_pixels"]
    if len(pixels) != height * 8 or any(len(row) != width * 8 for row in pixels):
        _fail("STOP_SPRITE_FRAME_PIXEL_MATRIX_INVALID")
    tile_refs = []
    for tile in entry["tiles"]:
        tile_refs.append({"tile_index": tile["tile_index"], "vram_address": tile["vram_address"],
                          "raw_bytes": list(tile["raw_bytes"]), "sha256": tile["sha256"]})
    return {
        "traversal_index": order_index,
        "sat_entry": entry["sat_entry"],
        "sat": dict(entry["vram_sat"]),
        "shadow_sat": dict(entry["shadow_sat"]),
        "decoded_attributes": attrs,
        "screen_position": {"x": attrs["x"], "y": attrs["y"],
                             "coordinate_space": "SAT_9BIT_UNCLIPPED"},
        "dimensions_pixels": {"width": width * 8, "height": height * 8},
        "tiles": tile_refs,
        "cram": {"base_address": entry["cram"]["base_address"],
                 "raw_bytes": list(entry["cram"]["raw_bytes"]),
                 "sha256": entry["cram"]["sha256"],
                 "palette_line": attrs["palette_line"]},
        "pixel_sha256": entry.get("pixel_sha256"),
        "semantic_fingerprint": entry.get("semantic_fingerprint"),
        "publication_provenance": entry["publication_provenance"],
        "persistence": entry.get("persistence"),
        "evidence_references": dict(entry.get("evidence_references") or {}),
        "causality": dict(entry.get("causality") or {"status": "UNPROVEN"}),
    }


def _source_tile(entry: Mapping[str, Any], x: int, y: int) -> tuple[int, Any]:
    attrs = entry["decoded_attributes"]
    width, height = attrs["width_cells"], attrs["height_cells"]
    cell_x, cell_y = x // 8, y // 8
    if attrs["flip_h"]:
        cell_x = width - 1 - cell_x
    if attrs["flip_v"]:
        cell_y = height - 1 - cell_y
    tile_index = attrs["tile_index"] + cell_x * height + cell_y
    for tile in entry["tiles"]:
        if tile.get("tile_index") == tile_index:
            return tile_index, tile
    _fail("STOP_SPRITE_FRAME_TILE_REFERENCE_INVALID")


def _compose(entries: Mapping[int, Mapping[str, Any]], order: Sequence[int]) -> dict[str, Any]:
    pieces = [_piece_metadata(entries[sat_entry], index) for index, sat_entry in enumerate(order)]
    left = min(piece["screen_position"]["x"] for piece in pieces)
    top = min(piece["screen_position"]["y"] for piece in pieces)
    right = max(piece["screen_position"]["x"] + piece["dimensions_pixels"]["width"] for piece in pieces)
    bottom = max(piece["screen_position"]["y"] + piece["dimensions_pixels"]["height"] for piece in pieces)
    width, height = right - left, bottom - top
    pixels = [[None for _ in range(width)] for _ in range(height)]
    winners = [[None for _ in range(width)] for _ in range(height)]
    cram = [[None for _ in range(width)] for _ in range(height)]
    rgb = [[None for _ in range(width)] for _ in range(height)]
    refs = [[None for _ in range(width)] for _ in range(height)]
    provenance: list[dict[str, Any]] = []
    overlap = 0
    for sat_entry in order:
        entry = entries[sat_entry]
        attrs = entry["decoded_attributes"]
        matrix = entry["decoded_pixels"]
        for local_y, row in enumerate(matrix):
            for local_x, pixel in enumerate(row):
                if pixel is None:
                    continue
                out_x = attrs["x"] - left + local_x
                out_y = attrs["y"] - top + local_y
                if winners[out_y][out_x] is not None:
                    overlap += 1
                    continue
                tile_index, _tile = _source_tile(entry, local_x, local_y)
                ref_index = len(provenance)
                provenance.append({
                    "witness_id": entry["evidence_references"]["observation_id"],
                    "frame": entry["frame"], "sat_entry": sat_entry,
                    "screen_x": attrs["x"] + local_x, "screen_y": attrs["y"] + local_y,
                    "piece_local_x": local_x, "piece_local_y": local_y,
                    "tile_index": tile_index, "tile_pixel_index": pixel["pixel_index"],
                    "palette_line": pixel["palette_line"], "cram_index": pixel["cram_index"],
                    "priority": pixel["priority"], "rgb": list(pixel["rgb"]),
                })
                pixels[out_y][out_x] = pixel["pixel_index"]
                winners[out_y][out_x] = sat_entry
                cram[out_y][out_x] = pixel["cram_index"]
                rgb[out_y][out_x] = list(pixel["rgb"])
                refs[out_y][out_x] = ref_index
    return {"pieces": pieces, "bounding_box": {"left": left, "top": top, "right": right, "bottom": bottom,
                                                   "width": width, "height": height,
                                                   "coordinate_space": "SAT_9BIT_UNCLIPPED"},
            "sat_traversal_order": list(order), "sprite_only_pixel_matrix": pixels,
            "winning_sat_entry_matrix": winners, "palette_cram_index_matrix": cram,
            "rgb_matrix": rgb, "pixel_provenance": provenance,
            "pixel_provenance_index_matrix": refs,
            "transparency_mask": [[value is None for value in row] for row in winners],
            "overlap_pixel_count": overlap,
            "visible_pixel_count": len(provenance)}


def build_frame_artifact(catalog: Mapping[str, Any], run_id: str | None = None,
                         frame: int | None = None) -> dict[str, Any]:
    if catalog.get("schema") != CATALOG_SCHEMA:
        _fail("unsupported sprite-piece catalog schema")
    if str(catalog.get("canonical_rom_sha256", "")).lower() != EXPECTED_ROM:
        _fail("sprite-piece catalog ROM identity is not canonical Beyond Oasis")
    resolved_run_id = run_id or str(catalog.get("run_id", "m12_unknown"))
    selected_frame, entries, order = _select_frame(catalog, resolved_run_id, frame)
    for sat_entry in order:
        _require_proven(entries[sat_entry])
    composed = _compose(entries, order)
    result: dict[str, Any] = {
        "schema": SCHEMA, "status": "PASS_ONE_PROVEN_HARDWARE_SPRITE_FRAME_V1",
        "run_id": resolved_run_id, "frame": selected_frame,
        "canonical_rom_sha256": EXPECTED_ROM,
        "catalog_capture_sha256": catalog.get("capture_sha256"),
        "catalog_schema": CATALOG_SCHEMA,
        "frame_kind": "SPRITE_ONLY_FRAME",
        "sat": {"base_address": entries[order[0]].get("vram_sat", {}).get("address"),
                "entry_count": len(order), "active_entry_count": len(entries),
                "traversal_order": list(order),
                "all_active_entries_reachable": True},
        **composed,
        "limitations": {
            "producer_to_dma_causality_proven": False,
            "sprite_vs_background_priority_modelled": False,
            "background_layers_modelled": False,
            "scanline_limit_modelled": False,
            "coordinate_clipping_applied": False,
            "statement": "SAT-linked sprite pieces only; no full-screen rendering claim",
        },
    }
    result["artifact_sha256"] = _hash(result)
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--run-id")
    parser.add_argument("--frame", type=int)
    args = parser.parse_args()
    raw = args.catalog.read_bytes()
    artifact = build_frame_artifact(json.loads(raw), args.run_id, args.frame)
    args.output.write_text(json.dumps(artifact, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
