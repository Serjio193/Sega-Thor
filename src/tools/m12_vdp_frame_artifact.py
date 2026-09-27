"""Fail-closed logical Genesis VDP frame reconstruction for M12 S8.

Developer-only tooling.  It consumes a coherent BizHawk VDP snapshot and the
accepted S7 sprite raster; it never writes emulator state or ROM data.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any, Mapping, Sequence

from m12_sprite_reconstruction import (  # type: ignore[import-not-found]
    ReconstructionError, decode_genesis_tile, decode_tile_attributes,
)


SCHEMA = "oasis.m68k.hardware-vdp-frame.v1"
CAPTURE_SCHEMA = "oasis.m68k.m12-vdp-frame-capture.v1"
EXPECTED_ROM = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
RASTER_SCHEMA = "oasis.m68k.hardware-sprite-raster.v1"
S7_STATUS = "PASS_SPRITE_SCANLINE_RASTER_RULES_V1"
PRIORITY_SOURCE = "https://nemesis.hacking-cult.org/MegaDrive/Documentation/GenesisSoftwareManual.pdf"
PRIORITY_ORDER = {"BACKDROP": 0, "PLANE_B": 1, "PLANE_A": 2,
                  "SPRITE": 3, "WINDOW": 4}


def _priority_rank(item: Mapping[str, Any]) -> tuple[int, int]:
    """Genesis layer order: low-priority planes, sprites, then high planes."""
    if item["layer"] == "BACKDROP":
        return (0, 0)
    return (4 if item.get("priority") else 0, PRIORITY_ORDER[item["layer"]])


def _fail(message: str) -> None:
    raise ReconstructionError(message)


def _canon(value: Any) -> bytes:
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode()


def _hash(value: Any) -> str:
    return hashlib.sha256(_canon(value)).hexdigest()


def _bytes(value: Any, name: str, minimum: int) -> bytes:
    if not isinstance(value, list) or len(value) < minimum or any(
            not isinstance(item, int) or not 0 <= item <= 255 for item in value):
        _fail(f"STOP_VDP_{name}_MISSING")
    return bytes(value)


def _u16(data: bytes, address: int) -> int:
    if address < 0 or address + 2 > len(data):
        _fail("STOP_VDP_ADDRESS_OUT_OF_RANGE")
    return (data[address] << 8) | data[address + 1]


def _signed10(value: int) -> int:
    value &= 0x3FF
    return value - 0x400 if value & 0x200 else value


def _rgb(word: int) -> list[int]:
    return [(((word >> shift) & 7) * 255 + 3) // 7 for shift in (1, 5, 9)]


def _registers(capture: Mapping[str, Any], frame: int) -> dict[int, dict[str, Any]]:
    events = capture.get("register_events")
    if not isinstance(events, list):
        _fail("STOP_VDP_REGISTER_EVIDENCE_MISSING")
    result: dict[int, dict[str, Any]] = {}
    for event in events:
        if not isinstance(event, Mapping):
            continue
        reg, event_frame = event.get("register"), event.get("frame")
        if isinstance(reg, int) and isinstance(event_frame, int) and event_frame <= frame:
            if isinstance(event.get("value"), int):
                result[reg] = dict(event)
            elif isinstance(event.get("data"), int):
                item = dict(event)
                item["value"] = item["data"]
                result[reg] = item
    needed = (1, 2, 3, 4, 5, 7, 10, 11, 12, 13, 16, 17, 18)
    missing = [reg for reg in needed if reg not in result]
    if missing:
        _fail("STOP_VDP_REGISTER_" + "_".join(map(str, missing)) + "_MISSING")
    return result


def _plane_size(reg16: int) -> tuple[int, int]:
    sizes = {0: 32, 1: 64, 3: 128}
    width_code, height_code = reg16 & 3, (reg16 >> 4) & 3
    if width_code == 2 or height_code == 2:
        _fail("STOP_VDP_PLANE_SIZE_UNSUPPORTED")
    return sizes[width_code], sizes[height_code]


def _window_region(reg17: int, reg18: int, x: int, y: int) -> bool:
    hpos, vpos = (reg17 & 0x1F) * 16, reg18 & 0x1F
    h_active = x >= hpos if reg17 & 0x80 else x < hpos
    v_active = y >= vpos if reg18 & 0x80 else y < vpos
    return h_active and v_active


def _scroll(vram: bytes, vsram: bytes, regs: Mapping[int, dict[str, Any]],
            layer: str, x: int, y: int) -> tuple[int, int, dict[str, Any]]:
    r11, r13 = regs[11]["value"], regs[13]["value"]
    hmode, vmode = r11 & 3, (r11 >> 2) & 1
    plane_offset = 0 if layer == "PLANE_A" else 2
    h_entry = 0 if hmode == 0 else (y // (8 if hmode == 2 else 1))
    h_address = ((r13 & 0x3F) << 10) + h_entry * 4 + plane_offset
    h_value = _signed10(_u16(vram, h_address))
    if vmode == 0:
        v_address = plane_offset
    else:
        v_address = (x // 16) * 4 + plane_offset
    v_value = _signed10(_u16(vsram, v_address))
    return h_value, v_value, {"hscroll_address": h_address, "hscroll_raw": _u16(vram, h_address),
                              "vscroll_address": v_address, "vscroll_raw": _u16(vsram, v_address),
                              "hscroll_mode": hmode, "vscroll_mode": vmode}


def _plane_base(regs: Mapping[int, dict[str, Any]], layer: str) -> int:
    if layer == "PLANE_A":
        return (regs[2]["value"] & 0x38) << 10
    if layer == "WINDOW":
        return (regs[3]["value"] & 0x3E) << 10
    return (regs[4]["value"] & 7) << 13


def _plane_pixel(vram: bytes, cram: bytes, regs: Mapping[int, dict[str, Any]], layer: str,
                 x: int, y: int, size: tuple[int, int], use_scroll: bool) -> dict[str, Any]:
    width, height = size
    if use_scroll:
        scroll_x, scroll_y, scroll_meta = _scroll(vram, _CURRENT_VSRAM, regs, layer, x, y)
    else:
        scroll_x = scroll_y = 0
        scroll_meta = {"hscroll_mode": 0, "vscroll_mode": 0}
    source_x, source_y = x + scroll_x, y + scroll_y
    cell_x, cell_y = (source_x // 8) % width, (source_y // 8) % height
    local_x, local_y = source_x % 8, source_y % 8
    table_address = (_plane_base(regs, layer) + (cell_y * width + cell_x) * 2) & 0xFFFF
    raw_word = _u16(vram, table_address)
    attrs = decode_tile_attributes(raw_word)
    tile_address = attrs.tile_index * 32
    tile_bytes = vram[tile_address:tile_address + 32]
    if len(tile_bytes) != 32:
        _fail("STOP_VDP_TILE_EVIDENCE_MISSING")
    tile = decode_genesis_tile(tile_bytes)
    pixel_x = 7 - local_x if attrs.flip_h else local_x
    pixel_y = 7 - local_y if attrs.flip_v else local_y
    pixel_index = tile[pixel_y][pixel_x]
    cram_index = attrs.palette * 16 + pixel_index
    cram_word = _u16(cram, cram_index * 2)
    return {"layer": layer, "screen_x": x, "screen_y": y, "table_address": table_address,
            "name_table_word": raw_word, "tile_index": attrs.tile_index,
            "tile_address": tile_address, "tile_bytes": list(tile_bytes),
            "tile_local_x": local_x, "tile_local_y": local_y, "pixel_index": pixel_index,
            "palette_line": attrs.palette, "cram_index": cram_index,
            "cram_word": cram_word, "rgb": _rgb(cram_word), "priority": attrs.priority,
            "transparent": pixel_index == 0, "scroll": scroll_meta}


def _sprite_candidates(s7: Mapping[str, Any], s6: Mapping[str, Any], frame: int) -> dict[tuple[int, int], dict[str, Any]]:
    if s7.get("schema") != RASTER_SCHEMA or s7.get("status") != S7_STATUS:
        _fail("STOP_VDP_S7_REFERENCE_INVALID")
    if s7.get("frame") != frame or s6.get("frame") != frame:
        _fail("STOP_VDP_FRAME_MISMATCH")
    records = s7.get("pixel_provenance")
    provenance = s6.get("pixel_provenance")
    if not isinstance(records, list) or not isinstance(provenance, list):
        _fail("STOP_VDP_S7_PROVENANCE_MISSING")
    result: dict[tuple[int, int], dict[str, Any]] = {}
    for record in records:
        if record.get("hardware_status") != "SURVIVED":
            continue
        index = record.get("s6_provenance_index")
        if not isinstance(index, int) or index < 0 or index >= len(provenance):
            _fail("STOP_VDP_S7_PROVENANCE_INVALID")
        source = dict(provenance[index])
        x, y = source.get("screen_x"), source.get("screen_y")
        if not isinstance(x, int) or not isinstance(y, int):
            _fail("STOP_VDP_S7_COORDINATE_MISSING")
        result[(x, y)] = {"layer": "SPRITE", "screen_x": x, "screen_y": y,
                          "sat_entry": source["sat_entry"], "tile_index": source["tile_index"],
                          "pixel_index": source["tile_pixel_index"],
                          "palette_line": source["palette_line"], "cram_index": source["cram_index"],
                          "rgb": source["rgb"], "priority": source["priority"],
                          "transparent": False, "s7_provenance_index": index,
                          "witness_id": source["witness_id"]}
    return result


def _scroll_evidence(vram: bytes, vsram: bytes, regs: Mapping[int, dict[str, Any]],
                    width: int, height: int) -> dict[str, Any]:
    hmode, vmode = regs[11]["value"] & 3, (regs[11]["value"] >> 2) & 1
    base = (regs[13]["value"] & 0x3F) << 10
    h_count = 1 if hmode == 0 else (height // (8 if hmode == 2 else 1))
    h_words = []
    for entry in range(h_count):
        for plane, offset in (("PLANE_A", 0), ("PLANE_B", 2)):
            address = base + entry * 4 + offset
            h_words.append({"layer": plane, "entry": entry, "address": address,
                            "raw": _u16(vram, address), "signed": _signed10(_u16(vram, address))})
    v_count = 1 if vmode == 0 else (width // 16)
    v_words = []
    for entry in range(v_count):
        for plane, offset in (("PLANE_A", 0), ("PLANE_B", 2)):
            address = entry * 4 + offset
            v_words.append({"layer": plane, "entry": entry, "address": address,
                            "raw": _u16(vsram, address), "signed": _signed10(_u16(vsram, address))})
    return {"reg11": regs[11]["value"], "reg13": regs[13]["value"],
            "hscroll_base": base, "hscroll_words": h_words,
            "vsram_words": v_words, "vertical_mode": vmode}


def build_artifact(capture: Mapping[str, Any], s7: Mapping[str, Any], s6: Mapping[str, Any],
                   frame: int | None = None) -> dict[str, Any]:
    if capture.get("schema") != CAPTURE_SCHEMA or capture.get("state_writes_emitted") is not False:
        _fail("STOP_VDP_CAPTURE_INVALID")
    if str(capture.get("canonical_rom_sha256", "")).lower() != EXPECTED_ROM:
        _fail("STOP_VDP_ROM_IDENTITY")
    selected = capture.get("frame") if frame is None else frame
    if not isinstance(selected, int):
        _fail("STOP_VDP_FRAME_MISSING")
    if capture.get("frame") != selected:
        _fail("STOP_VDP_FRAME_MISMATCH")
    for field in ("vram_frame", "cram_frame", "vsram_frame"):
        if capture.get(field, selected) != selected:
            _fail("STOP_VDP_" + field.upper() + "_MISMATCH")
    vram = _bytes(capture.get("vram"), "VRAM", 0x10000)
    cram = _bytes(capture.get("cram"), "CRAM", 0x80)
    global _CURRENT_VSRAM
    _CURRENT_VSRAM = _bytes(capture.get("vsram"), "VSRAM", 0x50)
    regs = _registers(capture, selected)
    width = 320 if regs[12]["value"] & 1 else 256
    height = 240 if regs[1]["value"] & 8 else 224
    dimensions = _plane_size(regs[16]["value"])
    sprites = _sprite_candidates(s7, s6, selected)
    visible_cells: dict[str, dict[int, dict[str, Any]]] = {"PLANE_A": {}, "PLANE_B": {}, "WINDOW": {}}
    composition: list[dict[str, Any]] = []
    for y in range(height):
        for x in range(width):
            window = _window_region(regs[17]["value"], regs[18]["value"], x, y)
            a = _plane_pixel(vram, cram, regs, "PLANE_A", x, y, dimensions, True)
            b = _plane_pixel(vram, cram, regs, "PLANE_B", x, y, dimensions, True)
            w = _plane_pixel(vram, cram, regs, "WINDOW", x, y, dimensions, False) if window else None
            candidates = [b, a, sprites.get((x, y))]
            if w is not None:
                candidates.append(w)
            for item in candidates:
                if item is not None and item["layer"] != "SPRITE":
                    visible_cells[item["layer"]][item["table_address"]] = {
                        "table_address": item["table_address"], "name_table_word": item["name_table_word"],
                        "tile_index": item["tile_index"], "tile_address": item["tile_address"],
                        "tile_bytes": item["tile_bytes"], "palette_line": item["palette_line"],
                        "priority": item["priority"], "flip_h": bool(decode_tile_attributes(item["name_table_word"]).flip_h),
                        "flip_v": bool(decode_tile_attributes(item["name_table_word"]).flip_v)}
            active = [item for item in candidates if item is not None and not item["transparent"]]
            backdrop_index = regs[7]["value"] & 0x3F
            backdrop_word = _u16(cram, backdrop_index * 2)
            backdrop = {"layer": "BACKDROP", "priority": False, "transparent": False,
                        "palette_line": backdrop_index // 16, "cram_index": backdrop_index,
                        "cram_word": backdrop_word, "rgb": _rgb(backdrop_word), "screen_x": x, "screen_y": y}
            active.append(backdrop)
            winner = max(active, key=_priority_rank)
            composition.append({"x": x, "y": y, "window_region": window,
                                "winner": {key: winner.get(key) for key in
                                            ("layer", "priority", "palette_line", "cram_index", "rgb", "screen_x", "screen_y", "sat_entry", "table_address", "name_table_word", "tile_index", "tile_local_x", "tile_local_y", "pixel_index") if key in winner},
                                "candidates": [{key: item.get(key) for key in
                                                ("layer", "transparent", "priority", "palette_line", "cram_index", "rgb", "sat_entry", "table_address", "name_table_word", "tile_index", "pixel_index") if key in item}
                                               for item in candidates if item is not None]})
    reg_output = {str(reg): {"raw_value": item["value"], "frame": item["frame"],
                             "sequence": item.get("sequence"), "pc": item.get("pc"),
                             "decoded": {}} for reg, item in regs.items()}
    reg_output["1"]["decoded"] = {"display_enabled": bool(regs[1]["value"] & 0x40), "v30": bool(regs[1]["value"] & 8)}
    reg_output["2"]["decoded"] = {"plane_a_table_base": _plane_base(regs, "PLANE_A")}
    reg_output["3"]["decoded"] = {"window_table_base": _plane_base(regs, "WINDOW")}
    reg_output["4"]["decoded"] = {"plane_b_table_base": _plane_base(regs, "PLANE_B")}
    reg_output["7"]["decoded"] = {"backdrop_cram_index": regs[7]["value"] & 0x3F}
    reg_output["11"]["decoded"] = {"horizontal_mode": regs[11]["value"] & 3,
                                     "vertical_mode": (regs[11]["value"] >> 2) & 1}
    reg_output["13"]["decoded"] = {"hscroll_table_base": (regs[13]["value"] & 0x3F) << 10}
    reg_output["12"]["decoded"] = {"h40": bool(regs[12]["value"] & 1)}
    reg_output["16"]["decoded"] = {"width_cells": dimensions[0], "height_cells": dimensions[1]}
    reg_output["17"]["decoded"] = {"right": bool(regs[17]["value"] & 0x80),
                                     "h_division_2cell_units": regs[17]["value"] & 0x1F}
    reg_output["18"]["decoded"] = {"down": bool(regs[18]["value"] & 0x80),
                                     "v_division_cell_units": regs[18]["value"] & 0x1F}
    return {"schema": SCHEMA, "status": "PASS_ONE_PROVEN_LOGICAL_VDP_FRAME_V1",
            "run_id": capture["run_id"], "frame": selected, "canonical_rom_sha256": EXPECTED_ROM,
            "registers": reg_output, "resolution": {"width": width, "height": height},
            "plane_dimensions_cells": {"width": dimensions[0], "height": dimensions[1]},
            "planes": {name: {"table_base": _plane_base(regs, name), "visible_cells": list(cells.values())}
                       for name, cells in visible_cells.items()},
            "window": {"reg17": regs[17]["value"], "reg18": regs[18]["value"]},
            "scroll_state": _scroll_evidence(vram, _CURRENT_VSRAM, regs, width, height),
            "cram": {"raw": list(cram), "sha256": hashlib.sha256(cram).hexdigest()},
            "sprites": {"s7_schema": s7["schema"], "s7_artifact_sha256": s7.get("artifact_sha256"),
                        "pixel_count": len(sprites)},
            "composition": composition,
            "priority_rule_basis": {"source": PRIORITY_SOURCE,
                                     "order_low_to_high": ["BACKDROP", "PLANE_B", "PLANE_A",
                                                            "SPRITE", "WINDOW", "PLANE_B_HIGH",
                                                            "PLANE_A_HIGH", "SPRITE_HIGH", "WINDOW_HIGH"]},
            "truth_classification": {"registers_tables_tiles": "OBSERVED",
                                      "rendering": "DERIVED_EXACT",
                                      "priority": "STATIC_VERIFIED"},
            "limitations": {"full_screen_vdp_equivalence": False, "vdp_timing": False,
                            "mid_scanline_register_effects": False, "conflict_count": 0}}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", type=Path, required=True)
    parser.add_argument("--sprites", type=Path, required=True)
    parser.add_argument("--sprite-frame", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    capture, s7, s6 = (json.loads(path.read_text()) for path in
                        (args.capture, args.sprites, args.sprite_frame))
    artifact = build_artifact(capture, s7, s6)
    unsigned = dict(artifact)
    unsigned.pop("artifact_sha256", None)
    artifact["artifact_sha256"] = _hash(unsigned)
    args.output.write_text(json.dumps(artifact, indent=2) + "\n")
    return 0


_CURRENT_VSRAM = b""

if __name__ == "__main__":
    raise SystemExit(main())
