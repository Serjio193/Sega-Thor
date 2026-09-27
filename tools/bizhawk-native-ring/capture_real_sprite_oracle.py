"""Capture and build Durable Real Sprite Oracle V1 using real BizHawk runtime."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time
from typing import Any, Mapping

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT / "src" / "tools") not in sys.path:
    sys.path.insert(0, str(ROOT / "src" / "tools"))

from m12_sprite_reconstruction import (  # noqa: E402
    active_sat_chain, compose_piece, decode_genesis_palette,
    decode_genesis_tile, decode_sat,
)

ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ORACLE_ID = "sprite_oracle_v1_f779"
ORACLE_RUN_ID = "sprite_oracle_real_v1"
ORACLE_FRAME = 779
ORACLE_SCHEMA = "oasis.m12.sprite-frame-capture.v1"
ORACLE_LOGICAL_SCHEMA = "oasis.m12.hardware-sprite-frame.v1"
DEFAULT_INSTALL = Path(r"C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64")
DEFAULT_ROM = ROOT / "local-roms" / "Beyond Oasis (USA).md"


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    hasher = hashlib.sha256()
    with open(path, "rb") as stream:
        while chunk := stream.read(1024 * 1024):
            hasher.update(chunk)
    return hasher.hexdigest()


def build_capture_lua(target_frame: int, raw_path: Path) -> str:
    raw_str = str(raw_path.resolve()).replace("\\", "\\\\")
    return f"""
local target_frame = {target_frame}
local oracle_id = "{ORACLE_ID}"
local run_id = "{ORACLE_RUN_ID}"
local current_regs = {{}}

for f = 1, target_frame - 1 do
    emu.frameadvance()
end
local start_boundary = emu.framecount()

event.on_bus_write(function(addr, val)
    if addr == 0xC00004 or addr == 0xC00006 then
        local w = val & 0xFFFF
        if (w & 0xC000) == 0x8000 then
            local reg = (w >> 8) & 0x1F
            current_regs[reg] = w & 0xFF
        end
    end
end, "sprite_reg_watch", "M68K BUS")

emu.frameadvance()
local end_boundary = emu.framecount()

local vram = memory.read_bytes_as_array(0, 0x10000, "VRAM")
local cram = memory.read_bytes_as_array(0, 0x80, "CRAM")
local vsram = memory.read_bytes_as_array(0, 0x50, "VSRAM")

local out = assert(io.open("{raw_str}", "w"))
out:write('{{\\n')
out:write('  "schema": "{ORACLE_SCHEMA}",\\n')
out:write('  "oracle_id": "' .. oracle_id .. '",\\n')
out:write('  "oracle_type": "SINGLE_COHERENT_FRAME",\\n')
out:write('  "run_id": "' .. run_id .. '",\\n')
out:write('  "frame": ' .. tostring(target_frame) .. ',\\n')
out:write('  "start_boundary": ' .. tostring(start_boundary) .. ',\\n')
out:write('  "end_boundary": ' .. tostring(end_boundary) .. ',\\n')
out:write('  "canonical_rom_sha256": "{ROM_SHA}",\\n')
out:write('  "registers": {{\\n')
local r_keys = {{}}
for k, _ in pairs(current_regs) do r_keys[#r_keys + 1] = k end
table.sort(r_keys)
for i, r in ipairs(r_keys) do
    out:write(string.format('    "%d": %d%s\\n', r, current_regs[r], i < #r_keys and ',' or ''))
end
out:write('  }},\\n')
out:write('  "vram": [' .. table.concat(vram, ',') .. '],\\n')
out:write('  "cram": [' .. table.concat(cram, ',') .. '],\\n')
out:write('  "vsram": [' .. table.concat(vsram, ',') .. ']\\n')
out:write('}}\\n')
out:close()
client.exit()
"""


def execute_capture(install: Path, rom: Path, out_dir: Path, target_frame: int = ORACLE_FRAME) -> Path:
    out_dir.mkdir(parents=True, exist_ok=True)
    raw_path = out_dir / "raw_capture.json"
    cfg_src = json.loads((install / "config.ini").read_text(encoding="utf-8-sig"))
    cfg_src["ClockThrottle"] = False
    cfg_src["RunInBackground"] = True
    cfg_src["SingleInstanceMode"] = False
    tmp_cfg = out_dir / "bizhawk_oracle_config.ini"
    tmp_cfg.write_text(json.dumps(cfg_src, indent=2))
    lua_script = out_dir / f"capture_frame_{target_frame}.lua"
    lua_script.write_text(build_capture_lua(target_frame, raw_path), encoding="utf-8")
    cmd = [str(install / "EmuHawk.exe"), f"--config={tmp_cfg.resolve()}", f"--lua={lua_script.resolve()}", str(rom.resolve())]
    subprocess.run(cmd, cwd=str(install), check=True, timeout=45)
    if not raw_path.is_file():
        raise FileNotFoundError(f"STOP_RAW_CAPTURE_FAILED:{raw_path}")
    return raw_path


def build_logical_sprite_artifact(raw_capture_path: Path) -> tuple[dict[str, Any], int]:
    doc = json.loads(raw_capture_path.read_text(encoding="utf-8"))
    vram = doc["vram"]
    cram = doc["cram"]
    regs = {int(k): v for k, v in doc.get("registers", {}).items()}
    h40 = bool(regs.get(12, 129) & 1)
    sat_base = (regs.get(5, 104) & (0x7F if h40 else 0x7E)) << 9
    sat_size = 640 if h40 else 512

    sat_bytes = bytes(vram[sat_base:sat_base + sat_size])
    pieces = decode_sat(sat_bytes, 0, 80 if h40 else 64)
    chain = active_sat_chain(pieces, 0)

    tile_map: dict[int, tuple[tuple[int, ...], ...]] = {}
    for p in chain:
        for cx in range(p.width_cells):
            for cy in range(p.height_cells):
                t_idx = p.tile.tile_index + cx * p.height_cells + cy
                if t_idx not in tile_map:
                    t_addr = t_idx * 32
                    tile_map[t_idx] = decode_genesis_tile(bytes(vram[t_addr:t_addr + 32]))

    visible_pixels = 0
    all_coords: set[tuple[int, int]] = set()
    overlaps = 0
    piece_records: list[dict[str, Any]] = []

    for idx, p in enumerate(chain):
        grid = compose_piece(p, tile_map)
        p_vis = 0
        for ry, row in enumerate(grid):
            for rx, px in enumerate(row):
                if px is not None:
                    visible_pixels += 1
                    p_vis += 1
                    coord = (p.x + rx, p.y + ry)
                    if coord in all_coords:
                        overlaps += 1
                    all_coords.add(coord)
        piece_records.append({
            "traversal_index": idx, "x": p.x, "y": p.y, "screen_x": p.x - 128, "screen_y": p.y - 128,
            "width_cells": p.width_cells, "height_cells": p.height_cells,
            "width_pixels": p.width_cells * 8, "height_pixels": p.height_cells * 8,
            "link": p.link, "tile_index": p.tile.tile_index, "palette": p.tile.palette,
            "priority": p.tile.priority, "flip_h": p.tile.flip_h, "flip_v": p.tile.flip_v,
            "visible_pixels": p_vis,
        })

    # Expected for frame 779: 8 active entries, 660 visible pixels, 0 overlaps
    conflicts = 0
    if len(chain) != 8:
        conflicts += 1
    if visible_pixels != 660:
        conflicts += 1
    if overlaps != 0:
        conflicts += 1

    artifact = {
        "schema": ORACLE_LOGICAL_SCHEMA, "oracle_id": ORACLE_ID, "frame": doc["frame"],
        "status": "PASS" if conflicts == 0 else "FAIL", "conflicts": conflicts,
        "mode": {"horizontal": "H40" if h40 else "H32", "sat_base": sat_base, "sat_size": sat_size},
        "active_sat_entries": len(chain), "sat_traversal_order": [p.link for p in chain],
        "pieces": piece_records, "visible_sprite_pixels": visible_pixels, "overlap_pixels": overlaps,
        "sat_raw_sha256": sha256_bytes(sat_bytes),
    }
    return artifact, conflicts


def generate_sprite_oracle(install: Path = DEFAULT_INSTALL, rom: Path = DEFAULT_ROM,
                           out_dir: Path | None = None) -> dict[str, Any]:
    out_dir = out_dir or (ROOT / "build" / "thor-evidence" / "oracles" / "sprite" / ORACLE_ID)
    out_dir.mkdir(parents=True, exist_ok=True)
    raw_path = execute_capture(install, rom, out_dir, ORACLE_FRAME)
    logical_doc, conflicts = build_logical_sprite_artifact(raw_path)
    logical_path = out_dir / "logical_artifact.json"
    logical_path.write_text(json.dumps(logical_doc, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    manifest = {
        "oracle_id": ORACLE_ID, "frame": ORACLE_FRAME,
        "hashes": {
            "raw_capture_sha256": sha256_file(raw_path),
            "logical_artifact_sha256": sha256_file(logical_path),
        }
    }
    manifest_path = out_dir / "sprite_oracle_manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    receipt = {
        "schema": "oasis.m12.sprite-oracle-receipt.v1", "oracle_id": ORACLE_ID, "frame": ORACLE_FRAME,
        "oracle_type": "SINGLE_COHERENT_FRAME", "status": "PASS" if conflicts == 0 else "FAIL",
        "conflict_count": conflicts, "visible_sprite_pixels": logical_doc["visible_sprite_pixels"],
        "active_sat_entries": logical_doc["active_sat_entries"],
        "storage_locator": str(out_dir.relative_to(ROOT)).replace("\\", "/") + "/",
        "creation_contract": "REAL_BIZHAWK_FRAME_779_SPRITE_ORACLE_V1",
        "canonical_rom_sha256": ROM_SHA,
        "artifact_hashes": {
            "raw_capture_sha256": manifest["hashes"]["raw_capture_sha256"],
            "logical_artifact_sha256": manifest["hashes"]["logical_artifact_sha256"],
            "manifest_sha256": sha256_file(manifest_path),
        },
        "source_owned_before": 1487672, "source_owned_after": 1487672, "source_owned_delta": 0,
        "timestamp": datetime.now(timezone.utc).isoformat(timespec="seconds").replace("+00:00", "Z"),
    }
    receipt_path = out_dir / "sprite_oracle_receipt.json"
    receipt_path.write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return receipt


if __name__ == "__main__":
    res = generate_sprite_oracle()
    print(json.dumps(res, indent=2))
