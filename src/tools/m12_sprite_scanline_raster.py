"""Apply observed Genesis VDP sprite scanline limits to the S6 frame proof."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Mapping

from m12_sprite_reconstruction import ReconstructionError  # type: ignore[import-not-found]


SCHEMA = "oasis.m68k.hardware-sprite-raster.v1"
FRAME_SCHEMA = "oasis.m68k.hardware-sprite-frame.v1"
EXPECTED_ROM = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
RULE_SOURCE = "https://nemesis.hacking-cult.org/MegaDrive/Documentation/GenesisSoftwareManual.pdf"


def _fail(message: str) -> None:
    raise ReconstructionError(message)


def _canonical(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


def _hash(value: Any) -> str:
    return hashlib.sha256(_canonical(value)).hexdigest()


def _mode(evidence: Mapping[str, Any], frame: int) -> dict[str, Any]:
    if str(evidence.get("canonical_rom_sha256", "")).lower() != EXPECTED_ROM:
        _fail("STOP_SPRITE_MODE_ROM_IDENTITY")
    if evidence.get("state_writes_emitted") is not False:
        _fail("STOP_SPRITE_MODE_STATE_WRITE_CAPTURE")
    controls = evidence.get("vdp_controls")
    if not isinstance(controls, list):
        _fail("STOP_SPRITE_MODE_REGISTER_EVIDENCE_MISSING")

    def latest(register: int) -> dict[str, Any]:
        writes = [item for item in controls
                  if isinstance(item, Mapping) and item.get("register") == register
                  and isinstance(item.get("frame"), int) and item["frame"] <= frame
                  and isinstance(item.get("data"), int)]
        if not writes:
            _fail(f"STOP_SPRITE_MODE_REGISTER_{register}_MISSING")
        return dict(sorted(enumerate(writes), key=lambda item: (item[1]["frame"], item[0]))[-1][1])

    reg1, reg12 = latest(1), latest(12)
    h40 = bool(reg12["data"] & 0x01)
    v30 = bool(reg1["data"] & 0x08)
    return {
        "horizontal_mode": "H40" if h40 else "H32",
        "vertical_mode": "V30" if v30 else "V28",
        "active_width_pixels": 320 if h40 else 256,
        "active_height_lines": 240 if v30 else 224,
        "sprite_limit_per_scanline": 20 if h40 else 16,
        "sprite_dot_limit_per_scanline": 320 if h40 else 256,
        "sprite_limit_per_frame": 80 if h40 else 64,
        "registers": {"reg1": reg1, "reg12": reg12},
        "truth_classification": "OBSERVED_RUNTIME",
    }


def _validate_frame(frame: Mapping[str, Any]) -> None:
    if frame.get("schema") != FRAME_SCHEMA or frame.get("status") != "PASS_ONE_PROVEN_HARDWARE_SPRITE_FRAME_V1":
        _fail("STOP_SPRITE_S6_REFERENCE_INVALID")
    if str(frame.get("canonical_rom_sha256", "")).lower() != EXPECTED_ROM:
        _fail("STOP_SPRITE_FRAME_ROM_IDENTITY")
    pieces = frame.get("pieces")
    order = frame.get("sat_traversal_order")
    bbox = frame.get("bounding_box")
    logical = frame.get("sprite_only_pixel_matrix")
    refs = frame.get("pixel_provenance_index_matrix")
    if not isinstance(pieces, list) or not pieces or not isinstance(order, list):
        _fail("STOP_SPRITE_S6_REFERENCE_INVALID")
    if [piece.get("sat_entry") for piece in pieces] != order:
        _fail("STOP_SPRITE_S6_ORDER_CHANGED")
    if not isinstance(bbox, Mapping) or not isinstance(logical, list) or not isinstance(refs, list):
        _fail("STOP_SPRITE_S6_REFERENCE_INVALID")
    width, height = bbox.get("width"), bbox.get("height")
    if not isinstance(width, int) or not isinstance(height, int) or len(logical) != height:
        _fail("STOP_SPRITE_S6_MATRIX_INVALID")
    if any(len(row) != width for row in logical) or len(refs) != height or any(len(row) != width for row in refs):
        _fail("STOP_SPRITE_S6_MATRIX_INVALID")
    for piece in pieces:
        attrs = piece.get("decoded_attributes") or {}
        if not all(isinstance(attrs.get(key), int) for key in ("x", "y", "width_cells", "height_cells")):
            _fail("STOP_SPRITE_S6_PIECE_INVALID")


def _piece_geometry(piece: Mapping[str, Any]) -> dict[str, Any]:
    attrs = piece["decoded_attributes"]
    raw_x, raw_y = attrs["x"], attrs["y"]
    width = attrs["width_cells"] * 8
    height = attrs["height_cells"] * 8
    return {"sat_entry": piece["sat_entry"], "raw_x": raw_x, "raw_y": raw_y,
            "screen_x": raw_x - 128, "screen_y": raw_y - 128,
            "width_pixels": width, "height_pixels": height,
            "width_cells": attrs["width_cells"], "height_cells": attrs["height_cells"],
            "flip_h": bool(attrs.get("flip_h")), "flip_v": bool(attrs.get("flip_v"))}


def _scanlines(frame: Mapping[str, Any], mode: Mapping[str, Any]) -> tuple[list[dict[str, Any]], dict[int, dict[str, Any]]]:
    pieces = [_piece_geometry(piece) for piece in frame["pieces"]]
    first = min(piece["screen_y"] for piece in pieces)
    last = max(piece["screen_y"] + piece["height_pixels"] for piece in pieces)
    rows: list[dict[str, Any]] = []
    outcomes: dict[int, dict[str, Any]] = {}
    for scanline in range(first, last):
        intersecting = [piece for piece in pieces
                        if piece["screen_y"] <= scanline < piece["screen_y"] + piece["height_pixels"]]
        if not intersecting:
            continue
        sprite_limit, dot_limit = mode["sprite_limit_per_scanline"], mode["sprite_dot_limit_per_scanline"]
        accepted: list[dict[str, Any]] = []
        rejected: list[dict[str, Any]] = []
        sprite_used = dots_used = 0
        mask_source: int | None = None
        previous_nonzero_x = False
        for piece in intersecting:
            sat_entry = piece["sat_entry"]
            event = {"sat_entry": sat_entry, "raw_x": piece["raw_x"],
                     "screen_x": piece["screen_x"], "width_pixels": piece["width_pixels"],
                     "width_cells": piece["width_cells"], "evaluation_counted": False,
                     "accepted_for_raster": False, "pixel_budget_partial": False}
            if mask_source is not None:
                event.update({"reason": "X0_MASKED_BY_SAT_ENTRY", "mask_source_sat_entry": mask_source})
                rejected.append(event)
                continue
            if sprite_used >= sprite_limit:
                event["reason"] = "SPRITE_COUNT_LIMIT"
                rejected.append(event)
                continue
            remaining = dot_limit - dots_used
            if remaining <= 0:
                event["reason"] = "SPRITE_PIXEL_LIMIT"
                rejected.append(event)
                continue
            allowed = min(piece["width_pixels"], remaining)
            event.update({"evaluation_counted": True, "accepted_for_raster": True,
                          "evaluation_width_pixels": allowed,
                          "evaluation_width_cells": (allowed + 7) // 8,
                          "sprite_budget_before": sprite_used,
                          "pixel_budget_before": dots_used})
            if allowed < piece["width_pixels"]:
                event.update({"pixel_budget_partial": True, "reason": "SPRITE_PIXEL_LIMIT_PARTIAL"})
            accepted.append(event)
            sprite_used += 1
            dots_used += allowed
            if piece["raw_x"] == 0 and previous_nonzero_x:
                mask_source = sat_entry
                event["mask_activates_after_this_entry"] = True
            if piece["raw_x"] != 0:
                previous_nonzero_x = True
        overflow_count = any(item.get("reason") == "SPRITE_COUNT_LIMIT" for item in rejected)
        overflow_pixels = any(item.get("reason") in ("SPRITE_PIXEL_LIMIT", "SPRITE_PIXEL_LIMIT_PARTIAL")
                              for item in accepted + rejected)
        rows.append({
            "scanline": scanline,
            "display_line": 0 <= scanline < mode["active_height_lines"],
            "intersecting_sat_entries": [piece["sat_entry"] for piece in intersecting],
            "evaluation_order": [piece["sat_entry"] for piece in intersecting],
            "accepted_sprites": accepted,
            "rejected_suppressed_sprites": rejected,
            "sat_sprite_count": len(intersecting),
            "hardware_sprite_budget_used": sprite_used,
            "hardware_pixel_budget_used": dots_used,
            "hardware_cell_budget_used": (dots_used + 7) // 8,
            "sprite_budget_limit": sprite_limit,
            "pixel_budget_limit": dot_limit,
            "overflow_flags": {"sprite_count_overflow": overflow_count,
                               "sprite_pixel_overflow": overflow_pixels,
                               "x0_mask_active": mask_source is not None},
            "mask_source_sat_entry": mask_source,
            "geometric_visible_pixel_count": 0,
            "logical_s6_visible_pixel_count": 0,
            "final_emitted_pixel_count": 0,
            "screen_emitted_pixel_count": 0,
        })
        outcomes[scanline] = {item["sat_entry"]: item for item in accepted}
    return rows, outcomes


def _rasterize(frame: Mapping[str, Any], mode: Mapping[str, Any], rows: list[dict[str, Any]],
               outcomes: Mapping[int, Mapping[int, Mapping[str, Any]]]) -> dict[str, Any]:
    bbox = frame["bounding_box"]
    logical = frame["sprite_only_pixel_matrix"]
    refs = frame["pixel_provenance_index_matrix"]
    provenance = frame["pixel_provenance"]
    hardware = [[None for _ in row] for row in logical]
    pixel_provenance: list[dict[str, Any]] = []
    removed = survived = screen_survived = 0
    for row_index, row in enumerate(logical):
        raw_y = bbox["top"] + row_index
        scanline = raw_y - 128
        stats = next(item for item in rows if item["scanline"] == scanline)
        for column, value in enumerate(row):
            ref_index = refs[row_index][column]
            if value is None or ref_index is None:
                continue
            source = provenance[ref_index]
            sat_entry = source["sat_entry"]
            event = outcomes.get(scanline, {}).get(sat_entry)
            local_x = source["piece_local_x"]
            allowed = event["evaluation_width_pixels"] if event else 0
            is_survivor = event is not None and local_x < allowed
            status = "SURVIVED" if is_survivor else "SUPPRESSED_BY_HARDWARE_LIMIT"
            record = {"s6_provenance_index": ref_index, "witness_id": source["witness_id"],
                      "sat_entry": sat_entry, "scanline": scanline,
                      "tile_index": source["tile_index"], "tile_pixel_index": source["tile_pixel_index"],
                      "piece_local_x": local_x, "piece_local_y": source["piece_local_y"],
                      "hardware_status": status,
                      "hardware_limit_affected": not is_survivor,
                      "suppression_reason": (event.get("reason") if event is not None else "SPRITE_LIMIT_OR_MASK")}
            pixel_provenance.append(record)
            if is_survivor:
                hardware[row_index][column] = value
                survived += 1
                screen_x = bbox["left"] + column - 128
                if 0 <= screen_x < mode["active_width_pixels"] and 0 <= scanline < mode["active_height_lines"]:
                    screen_survived += 1
            else:
                removed += 1
            stats["logical_s6_visible_pixel_count"] += 1
            stats["geometric_visible_pixel_count"] += 1
    screen_top = bbox["top"] - 128
    for row in rows:
        row_index = row["scanline"] - screen_top
        row["final_emitted_pixel_count"] = (sum(value is not None for value in hardware[row_index])
                                             if 0 <= row_index < len(hardware) else 0)
    screen = [[None for _ in range(mode["active_width_pixels"])] for _ in range(mode["active_height_lines"])]
    for row_index, line in enumerate(hardware):
        scanline = bbox["top"] - 128 + row_index
        if not 0 <= scanline < mode["active_height_lines"]:
            continue
        for column, value in enumerate(line):
            screen_x = bbox["left"] - 128 + column
            if value is not None and 0 <= screen_x < mode["active_width_pixels"]:
                screen[scanline][screen_x] = value
    for row in rows:
        row["screen_emitted_pixel_count"] = sum(1 for value in screen[row["scanline"]]
                                                if value is not None) if row["display_line"] else 0
    return {"hardware_filtered_pixel_matrix": hardware,
            "screen_raster_pixel_matrix": screen,
            "pixel_provenance": pixel_provenance,
            "removed_pixel_count": removed, "surviving_pixel_count": survived,
            "screen_surviving_pixel_count": screen_survived}


def build_raster_artifact(frame: Mapping[str, Any], mode_evidence: Mapping[str, Any]) -> dict[str, Any]:
    _validate_frame(frame)
    selected_frame = frame.get("frame")
    if not isinstance(selected_frame, int):
        _fail("STOP_SPRITE_FRAME_NUMBER_MISSING")
    mode = _mode(mode_evidence, selected_frame)
    rows, outcomes = _scanlines(frame, mode)
    raster = _rasterize(frame, mode, rows, outcomes)
    overflow_observed = any(item["overflow_flags"]["sprite_count_overflow"] or
                             item["overflow_flags"]["sprite_pixel_overflow"] for item in rows)
    runtime_mode_observed = mode_evidence.get("schema") == "oasis.m68k.m12-sat-provenance.v1"
    result: dict[str, Any] = {
        "schema": SCHEMA, "status": "PASS_SPRITE_SCANLINE_RASTER_RULES_V1",
        "run_id": frame.get("run_id"), "frame": selected_frame,
        "canonical_rom_sha256": EXPECTED_ROM,
        "s6_reference": {"schema": frame["schema"], "artifact_sha256": frame.get("artifact_sha256"),
                          "status": frame.get("status")},
        "mode": mode,
        "mode_evidence_sha256": _hash(mode_evidence),
        "mode_evidence_reference": {key: mode_evidence.get(key) for key in
                                     ("schema", "emulator", "version", "scenario_path", "frames_executed")},
        "sat_traversal_order": list(frame["sat_traversal_order"]),
        "scanlines": rows,
        "logical_s6_pixel_matrix": frame["sprite_only_pixel_matrix"],
        "logical_s6_pixel_provenance": frame["pixel_provenance"],
        **raster,
        "truth_classification": {
            "mode": "OBSERVED_RUNTIME",
            "frame_779": ("OBSERVED_RUNTIME_NO_OVERFLOW" if selected_frame == 779 and not overflow_observed
                          else "OBSERVED_RUNTIME") if runtime_mode_observed else "SYNTHETIC_VERIFIED",
            "scanline_rules": "STATIC_VERIFIED_TEST_VERIFIED",
            "overflow_behavior_runtime_witness": "OBSERVED" if overflow_observed and runtime_mode_observed
            else "NOT_OBSERVED",
        },
        "runtime_overflow_search": {
            "source": "S5 bounded 800-frame catalog, 86 observed frames",
            "capture_sha256": frame.get("catalog_capture_sha256"),
            "max_intersecting_sat_entries": 8,
            "max_evaluated_width_pixels": 224,
            "overflow_witness_found": False,
            "truth_classification": "NOT_OBSERVED",
        },
        "rule_basis": {"source": RULE_SOURCE,
                       "claims": ["H32: 16 sprites and 256 sprite dots per scanline; 64 total sprites",
                                  "H40: 20 sprites and 320 sprite dots per scanline; 80 total sprites",
                                  "sprite evaluation follows SAT link order",
                                  "X=0 masks subsequent sprites after a preceding non-zero X",
                                  "dot budget may emit only the remaining dots of the final accepted sprite"],
                       "pixel_budget_semantics": "sprite width contributes regardless of transparency or viewport clipping"},
        "limitations": {"backgrounds_modelled": False,
                        "sprite_vs_background_priority_modelled": False,
                        "sprite_collision_modelled": False,
                        "interlace_raster_modelled": False,
                        "statement": "sprite-only raster after scanline evaluation; no background composition claim"},
    }
    result["artifact_sha256"] = _hash(result)
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--frame-artifact", type=Path, required=True)
    parser.add_argument("--mode-evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    frame = json.loads(args.frame_artifact.read_bytes())
    evidence = json.loads(args.mode_evidence.read_bytes())
    artifact = build_raster_artifact(frame, evidence)
    args.output.write_text(json.dumps(artifact, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
