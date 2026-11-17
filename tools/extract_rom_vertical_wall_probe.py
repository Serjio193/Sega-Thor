#!/usr/bin/env python3
"""Extract and verify ROM vertical tilemap wall collision, row sampler & height step subsystem (0x0094D2..0x0096E8)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_vertical_wall_probe_m15_402.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x0094D2: -Y solid probe
    assert u16(rom, 0x0094D2) == 0x1C2E          # move.b $38(a6), d6
    assert u16(rom, 0x0094D2 + 2) == 0x0038
    assert u16(rom, 0x0094D6) == 0x0806          # btst.b #0, d6
    assert u16(rom, 0x009588) == 0x08EE          # bset.b #5, $41(a6) (ledge drop)
    assert u16(rom, 0x009588 + 2) == 0x0005
    assert u16(rom, 0x009588 + 4) == 0x0041
    assert u16(rom, 0x009590) == 0x08EE          # bset.b #6, $41(a6) (wall hit)
    assert u16(rom, 0x009590 + 2) == 0x0006
    assert u16(rom, 0x009590 + 4) == 0x0041
    assert u16(rom, 0x00959A) == 0x003C          # ori.b #1, ccr
    assert u16(rom, 0x00959E) == 0x4E75          # rts

    # 0x0095AA: +Y solid probe
    assert u16(rom, 0x0095AA) == 0x1C2E          # move.b $38(a6), d6
    assert u16(rom, 0x0095AA + 2) == 0x0038
    assert u16(rom, 0x0095AE) == 0x0806          # btst.b #0, d6
    assert u16(rom, 0x009656) == 0x08EE          # bset.b #5, $41(a6) (ledge drop)
    assert u16(rom, 0x009656 + 2) == 0x0005
    assert u16(rom, 0x009656 + 4) == 0x0041
    assert u16(rom, 0x00965E) == 0x08EE          # bset.b #6, $41(a6) (wall hit)
    assert u16(rom, 0x00965E + 2) == 0x0006
    assert u16(rom, 0x00965E + 4) == 0x0041
    assert u16(rom, 0x009668) == 0x003C          # ori.b #1, ccr
    assert u16(rom, 0x00966C) == 0x4E75          # rts

    # 0x009678: row scanner
    assert u16(rom, 0x009678) == 0x3004          # move.w d4, d0
    assert u16(rom, 0x00967A) == 0x342E          # move.w $8(a6), d2
    assert u16(rom, 0x00967A + 2) == 0x0008
    assert u16(rom, 0x0096B8) == 0x4E75          # rts

    # 0x0096BA: row span sampler
    assert u16(rom, 0x0096BA) == 0x3A02          # move.w d2, d5
    assert u16(rom, 0x0096BC) == 0x3C02          # move.w d2, d6
    assert u16(rom, 0x0096E6) == 0x4E75          # rts

    # 0x0096E8: 16-byte terrain LUT
    lut = list(rom[0x0096E8:0x0096E8 + 16])
    assert len(lut) == 16

    return {
        "milestone": "M15.402",
        "title": "ROM Vertical Tilemap Wall Collision, Row Sampler & Height Step Subsystem",
        "routines": {
            "tile_solid_probe_neg_y": {
                "start": "0x0094D2",
                "end": "0x0095AA",
                "bytes": 216,
                "flags_38_offset": "0x38",
                "collision_flags_offset": "0x41",
                "wall_collision_bit": 6,
                "ledge_drop_bit": 5,
                "elevation_lut_ram": "0x00FF1706",
                "ledge_drop_height_ram": "0x00FF196E",
            },
            "tile_solid_probe_pos_y": {
                "start": "0x0095AA",
                "end": "0x009678",
                "bytes": 206,
                "flags_38_offset": "0x38",
                "collision_flags_offset": "0x41",
                "wall_collision_bit": 6,
                "ledge_drop_bit": 5,
                "elevation_lut_ram": "0x00FF1706",
                "ledge_drop_height_ram": "0x00FF196E",
            },
            "scan_tile_row_solidity": {
                "start": "0x009678",
                "end": "0x0096BA",
                "bytes": 66,
                "max_scan_extent": "0x0180",
                "step_size": 8,
            },
            "sample_tile_row_span": {
                "start": "0x0096BA",
                "end": "0x0096E8",
                "bytes": 46,
                "base_layer_offset": "0x00",
                "column_stride_shift_offset": "0x08",
            },
            "terrain_surface_lut": {
                "start": "0x0096E8",
                "end": "0x0096F8",
                "bytes": 16,
                "table": [f"0x{b:02X}" for b in lut],
            },
        },
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
