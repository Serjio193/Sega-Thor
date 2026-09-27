"""Build one fail-closed, provenance-backed Genesis hardware-sprite witness."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Mapping, Sequence

from m12_sprite_reconstruction import (  # type: ignore[import-not-found]
    ReconstructionError,
    SpritePiece,
    compose_piece,
    decode_genesis_palette,
    decode_genesis_tile,
    decode_sat,
    reconstruction_metadata,
)


SCHEMA = "oasis.m68k.hardware-sprite-piece.v1"
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


def _same_frame(items: Sequence[Mapping[str, Any]], frame: int) -> list[dict[str, Any]]:
    return [dict(item) for item in items if item.get("frame") == frame]


def _pixel_matrix(pixels: Sequence[Sequence[Any]], palette: Sequence[Sequence[int]]) -> list[list[Any]]:
    result: list[list[Any]] = []
    for row in pixels:
        output_row: list[Any] = []
        for pixel in row:
            if pixel is None:
                output_row.append(None)
                continue
            rgb = list(palette[pixel.pixel_index])
            output_row.append({
                "pixel_index": pixel.pixel_index,
                "palette_line": pixel.palette_line,
                "cram_index": pixel.cram_index,
                "priority": pixel.priority,
                "rgb": rgb,
            })
        result.append(output_row)
    return result


def build_artifact(capture: Mapping[str, Any], capture_sha256: str,
                   run_id: str = "m12_m11_8_natural_v1", witness_index: int = 0) -> dict[str, Any]:
    if capture.get("schema") != CAPTURE_SCHEMA:
        _fail("unsupported runtime capture schema")
    if capture.get("canonical_rom_sha256", "").lower() != EXPECTED_ROM:
        _fail("runtime capture ROM identity is not canonical Beyond Oasis")
    if capture.get("state_writes_emitted") is not False:
        _fail("state-writing runtime capture is not accepted")
    witnesses = capture.get("witnesses")
    if not isinstance(witnesses, list) or not 0 <= witness_index < len(witnesses):
        _fail("requested hardware-sprite witness is missing")
    witness = witnesses[witness_index]
    frame = witness.get("frame")
    if not isinstance(frame, int):
        _fail("witness frame is missing")
    sat_raw = _bytes(witness.get("sat_bytes"), "SAT bytes")
    shadow_raw = _bytes(witness.get("shadow_bytes"), "shadow bytes")
    if len(sat_raw) != 8 or len(shadow_raw) != 8:
        _fail("one hardware-sprite piece requires one 8-byte SAT entry")
    dma = witness.get("dma")
    if not isinstance(dma, Mapping) or dma.get("frame") != frame:
        _fail("SAT publication is not in the witness frame")
    if _hex(dma.get("destination"), "DMA destination") != 0xD000:
        _fail("DMA destination is not the proven SAT base")
    if _hex(dma.get("source"), "DMA source") != _hex(witness.get("shadow_address"), "shadow address"):
        _fail("DMA source is not the witnessed shadow SAT")
    dma_source = _bytes(dma.get("source_bytes"), "DMA source bytes")
    if len(dma_source) < 8 or dma_source[:8] != shadow_raw or sat_raw != shadow_raw:
        _fail("shadow, DMA source, and VRAM SAT bytes disagree")

    entry = decode_sat(sat_raw, 0, 1)[0]
    if entry.width_cells != 1 or entry.height_cells != 1:
        _fail("S2 witness must be a single-cell hardware sprite piece")
    tile_raw = _bytes(witness.get("tile_bytes"), "VRAM tile bytes")
    cram_raw = _bytes(witness.get("cram_bytes"), "CRAM palette bytes")
    if len(tile_raw) != 32 or len(cram_raw) != 32:
        _fail("single-cell witness requires exactly 32 tile and 32 CRAM bytes")
    tile_address = entry.tile.tile_index * 32
    tile = decode_genesis_tile(tile_raw)
    palette = decode_genesis_palette(cram_raw)
    composed = compose_piece(entry, {entry.tile.tile_index: tile})
    producer_writes = _same_frame(witness.get("producer_writes", []), frame)
    producer_executions = _same_frame(witness.get("producer_executions", []), frame)
    shadow_address = _hex(witness.get("shadow_address"), "shadow address")
    if not producer_writes or not producer_executions:
        _fail("producer execution and shadow write evidence are required")
    if not any(_hex(item.get("address"), "producer write address") == shadow_address
               for item in producer_writes):
        _fail("producer evidence does not touch the shadow SAT")
    raw_hashes = {
        "sat": hashlib.sha256(sat_raw).hexdigest(),
        "shadow": hashlib.sha256(shadow_raw).hexdigest(),
        "tile": hashlib.sha256(tile_raw).hexdigest(),
        "cram": hashlib.sha256(cram_raw).hexdigest(),
    }
    return {
        "schema": SCHEMA,
        "run_id": run_id,
        "status": "PASS_ONE_HARDWARE_SPRITE_PIECE_V1",
        "canonical_rom_sha256": EXPECTED_ROM,
        "capture_sha256": capture_sha256,
        "capture_schema": CAPTURE_SCHEMA,
        "emulator": capture.get("emulator"),
        "scenario_path": capture.get("scenario_path"),
        "frame": frame,
        "sat_index": witness.get("sat_index"),
        "links": {
            "producer_to_shadow": "PASS_SAME_FRAME_RUNTIME_OBSERVATION",
            "shadow_to_vram_sat": "PASS_EXACT_8_BYTES",
            "sat_to_tile": "PASS_TILE_INDEX_TIMES_32",
            "sat_to_cram": "PASS_PALETTE_LINE",
            "decoder": "PASS_M12_GENESIS_4BPP_PACKED_NIBBLE",
            "composition": "PASS_ONE_CELL",
        },
        "observed_sequence": {
            "producer_executions": producer_executions,
            "producer_writes": producer_writes,
            "dma": dict(dma),
            "causal_order_claim": "same_frame_value_chain; callback sequence is retained and producer-to-DMA causality is not generalized",
        },
        "sat": {
            "address": witness.get("sat_address"),
            "raw_bytes": list(sat_raw),
            "decoded": {"x": entry.x, "y": entry.y, "width_cells": 1,
                        "height_cells": 1, "link": entry.link,
                        "tile_index": entry.tile.tile_index,
                        "palette_line": entry.tile.palette,
                        "priority": entry.tile.priority,
                        "flip_h": entry.tile.flip_h, "flip_v": entry.tile.flip_v},
        },
        "shadow_sat": {"address": witness.get("shadow_address"), "raw_bytes": list(shadow_raw)},
        "tile": {"vram_address": tile_address, "raw_bytes": list(tile_raw),
                 "sha256": raw_hashes["tile"]},
        "cram": {"base_address": entry.tile.palette * 32, "raw_bytes": list(cram_raw),
                 "sha256": raw_hashes["cram"]},
        "raw_sha256": raw_hashes,
        "pixels": _pixel_matrix(composed, palette),
        "reconstruction_metadata": reconstruction_metadata(),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--run-id", default="m12_m11_8_natural_v1")
    parser.add_argument("--witness-index", type=int, default=0)
    args = parser.parse_args()
    raw = args.capture.read_bytes()
    artifact = build_artifact(json.loads(raw), hashlib.sha256(raw).hexdigest(),
                              args.run_id, args.witness_index)
    args.output.write_text(json.dumps(artifact, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
