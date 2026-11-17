#!/usr/bin/env python3
"""Extract and verify ROM horizontal tilemap wall collision, surface height step & ledge drop detector (0x00938E..0x0094D2)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_horizontal_wall_probe_m15_401.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x00938E: move.b $38(a6), d6
    assert u16(rom, 0x00938E) == 0x1C2E
    assert u16(rom, 0x00938E + 2) == 0x0038
    # 0x009392: btst.b #0, d6
    assert u16(rom, 0x009392) == 0x0806
    assert u16(rom, 0x009392 + 2) == 0x0000

    # 0x0093B0: lea.l $96E8(pc), a1
    assert u16(rom, 0x0093B0) == 0x43FA
    disp = u16(rom, 0x0093B0 + 2)
    assert (0x0093B2 + disp) & 0xFFFFFF == 0x0096E8

    # 0x00941C: lea.l $FF1706.l, a1 (elevation table)
    assert u16(rom, 0x00941C) == 0x43F9
    assert u32(rom, 0x00941C + 2) == 0x00FF1706

    # 0x009434: move.w d7, $FF196E.l (ledge drop height)
    assert u16(rom, 0x009434) == 0x33C7
    assert u32(rom, 0x009434 + 2) == 0x00FF196E
    # 0x00943E: bset.b #5, $40(a6) (ledge drop bit)
    assert u16(rom, 0x00943E) == 0x08EE
    assert u16(rom, 0x00943E + 2) == 0x0005
    assert u16(rom, 0x00943E + 4) == 0x0040

    # 0x009446: bset.b #6, $40(a6) (wall collision bit)
    assert u16(rom, 0x009446) == 0x08EE
    assert u16(rom, 0x009446 + 2) == 0x0006
    assert u16(rom, 0x009446 + 4) == 0x0040

    # 0x009450: ori.b #1, ccr (set carry - solid)
    assert u16(rom, 0x009450) == 0x003C
    assert u16(rom, 0x009450 + 2) == 0x0001
    assert u16(rom, 0x009454) == 0x4E75          # rts

    # 0x00945A: andi.b #$FE, ccr (clear carry - passable)
    assert u16(rom, 0x00945A) == 0x023C
    assert u16(rom, 0x00945A + 2) == 0x00FE
    assert u16(rom, 0x00945E) == 0x4E75          # rts

    # 0x009460: column scanner
    assert u16(rom, 0x009460) == 0x3004          # move.w d4, d0
    assert u16(rom, 0x009462) == 0x322E          # move.w $C(a6), d1
    assert u16(rom, 0x009462 + 2) == 0x000C
    assert u16(rom, 0x0094A0) == 0x4E75          # rts

    # 0x0094A2: row span sampler
    assert u16(rom, 0x0094A2) == 0x3A01          # move.w d1, d5
    assert u16(rom, 0x0094A4) == 0x3C01          # move.w d1, d6
    assert u16(rom, 0x0094D0) == 0x4E75          # rts

    return {
        "milestone": "M15.401",
        "title": "ROM Horizontal Tilemap Wall Collision, Surface Height Step & Ledge Drop Detector",
        "routines": {
            "tile_solid_probe_x": {
                "start": "0x00938E",
                "end": "0x009460",
                "bytes": 114,
                "intangible_flag_bit": 0,
                "flags_38_offset": "0x38",
                "surface_type_offset": "0x70",
                "collision_flags_offset": "0x40",
                "wall_collision_bit": 6,
                "ledge_drop_bit": 5,
                "elevation_lut_ram": "0x00FF1706",
                "ledge_drop_height_ram": "0x00FF196E",
            },
            "scan_tile_column_solidity": {
                "start": "0x009460",
                "end": "0x0094A2",
                "bytes": 66,
                "max_scan_extent": "0x0180",
                "step_size": 8,
            },
            "sample_tile_column_span": {
                "start": "0x0094A2",
                "end": "0x0094D2",
                "bytes": 48,
                "base_layer_offset": "0x00",
                "row_stride_offset": "0x04",
                "column_stride_shift_offset": "0x08",
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
