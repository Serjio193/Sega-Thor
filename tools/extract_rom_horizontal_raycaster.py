#!/usr/bin/env python3
"""Extract and verify ROM horizontal (±X) tilemap raycaster, surface height probe & 0x0096E8 terrain LUT (0x009084..0x0091F6)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_horizontal_raycaster_m15_399.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x009084: +X raycaster
    assert u16(rom, 0x009084) == 0x1D7C          # move.b #$11, $40(a6)
    assert u16(rom, 0x009084 + 2) == 0x0011
    assert u16(rom, 0x00908C) == 0x3D40          # move.w d0, $A(a6) (subpixel)
    assert u16(rom, 0x009096) == 0x3D40          # move.w d0, $72(a6) (step)
    assert u16(rom, 0x009096 + 2) == 0x0072

    # 0x009128: -X raycaster
    assert u16(rom, 0x009128) == 0x1D7C          # move.b #$13, $40(a6)
    assert u16(rom, 0x009128 + 2) == 0x0013
    assert u16(rom, 0x009130) == 0x3D40          # move.w d0, $A(a6)
    assert u16(rom, 0x00913A) == 0x3D40          # move.w d0, $72(a6)
    assert u16(rom, 0x00913A + 2) == 0x0072

    # 0x0091C6: final X write and clear vel
    assert u16(rom, 0x0091C6) == 0x3D41          # move.w d1, $8(a6)
    assert u16(rom, 0x0091C6 + 2) == 0x0008
    assert u16(rom, 0x0091F6) == 0x42AE          # clr.l $72(a6)
    assert u16(rom, 0x0091F6 + 2) == 0x0072

    # 16-byte terrain surface type LUT at 0x0096E8
    lut = list(rom[0x0096E8:0x0096E8 + 16])
    expected_lut = [0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]
    assert lut == expected_lut

    return {
        "milestone": "M15.399",
        "title": "ROM Horizontal (±X) Tilemap Raycaster, Surface Height Probe & 0x0096E8 Terrain LUT",
        "routines": {
            "pos_x_raycaster": {
                "start": "0x009084",
                "end": "0x009128",
                "bytes": 164,
                "direction_code": "0x11",
                "boundary_clamp": "0x009D4E",
                "solid_entity_sweep": "0x009DD6",
                "tile_solid_probe": "0x00938E",
                "surface_height_probe": "0x009BF2",
                "surface_height_store": "0x009AD6",
                "terrain_lut_addr": "0x0096E8",
            },
            "neg_x_raycaster": {
                "start": "0x009128",
                "end": "0x0091C6",
                "bytes": 158,
                "direction_code": "0x13",
                "boundary_clamp": "0x009D70",
                "solid_entity_sweep": "0x009EBA",
                "tile_solid_probe": "0x00938E",
                "surface_height_probe": "0x009BF2",
                "surface_height_store": "0x009AD6",
                "terrain_lut_addr": "0x0096E8",
            },
            "final_x_write_and_cleanup": {
                "start": "0x0091C6",
                "end": "0x0091F6",
                "bytes": 48,
                "x_pos_offset": "0x08",
                "x_vel_offset": "0x72",
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
