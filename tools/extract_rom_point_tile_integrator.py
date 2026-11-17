#!/usr/bin/env python3
"""Extract and verify ROM point tile solidity, air/elevation integrator & tile attribute sampler (0x0098EC..0x0099EE)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_point_tile_integrator_m15_404.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x0098EC: movem.l d0, -(a7)
    assert u16(rom, 0x0098EC) == 0x48E7
    assert u16(rom, 0x0098EC + 2) == 0x8000
    assert u16(rom, 0x0098F0) == 0x302E          # move.w $14(a6), d0
    assert u16(rom, 0x0098F0 + 2) == 0x0014

    # 0x009910: air height offset +0x10
    assert u16(rom, 0x009910) == 0x3C2E          # move.w $10(a6), d6
    assert u16(rom, 0x009910 + 2) == 0x0010
    assert u16(rom, 0x009918) == 0x003C          # ori.b #1, ccr (solid)
    assert u16(rom, 0x009918 + 2) == 0x0001

    # 0x009966: addq.w #1, $10(a6) (descending step)
    assert u16(rom, 0x009966) == 0x526E
    assert u16(rom, 0x009966 + 2) == 0x0010

    # 0x00998E: subq.w #1, $10(a6) (ascending step)
    assert u16(rom, 0x00998E) == 0x536E
    assert u16(rom, 0x00998E + 2) == 0x0010

    # 0x00999C: write updated Y pos +0x0C
    assert u16(rom, 0x00999C) == 0x3D42
    assert u16(rom, 0x00999C + 2) == 0x000C

    # 0x0099AA: write updated ground height +0x14
    assert u16(rom, 0x0099AA) == 0x3D4C
    assert u16(rom, 0x0099AA + 2) == 0x0014
    assert u16(rom, 0x0099AE) == 0x023C          # andi.b #$FE, ccr (passable)
    assert u16(rom, 0x0099AE + 2) == 0x00FE
    assert u16(rom, 0x0099B6) == 0x4E75          # rts

    # 0x0099B8: sample_point_tile_attr
    assert u16(rom, 0x0099B8) == 0x3802          # move.w d2, d4
    assert u16(rom, 0x0099D0) == 0x0203          # andi.b #$F, d3
    assert u16(rom, 0x0099D0 + 2) == 0x000F
    assert u16(rom, 0x0099D4) == 0x4E75          # rts

    # 0x0099D6: read_point_tile_raw
    assert u16(rom, 0x0099D6) == 0x3802          # move.w d2, d4
    assert u16(rom, 0x0099EE) == 0x4E75          # rts

    return {
        "milestone": "M15.404",
        "title": "ROM Point Tile Solidity, Air/Elevation Integrator & Tile Attribute Sampler",
        "routines": {
            "point_tile_solidity_probe": {
                "start": "0x0098EC",
                "end": "0x0099B8",
                "bytes": 204,
                "ground_height_offset": "0x14",
                "air_height_offset": "0x10",
                "y_pos_offset": "0x0C",
                "elevation_probe_call": "0x009AD0",
                "ramp_climb_flag_mask": "0x90",
            },
            "sample_point_tile_attr": {
                "start": "0x0099B8",
                "end": "0x0099D6",
                "bytes": 30,
                "attribute_mask": "0x0F",
            },
            "read_point_tile_raw": {
                "start": "0x0099D6",
                "end": "0x0099F0",
                "bytes": 26,
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
