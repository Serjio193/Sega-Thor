#!/usr/bin/env python3
"""Extract and verify ROM ramp/bridge slope solver, fractional height interpolator & tile elevation engine (0x0099F0..0x009BC0)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_slope_elevation_engine_m15_405.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x0099F0: ramp search 1
    assert u16(rom, 0x0099F0) == 0x3802
    assert u16(rom, 0x0099F2) == 0xE644
    assert u16(rom, 0x0099F4) == 0x5244

    # 0x009A60: ramp search 2
    assert u16(rom, 0x009A60) == 0x3802
    assert u16(rom, 0x009A62) == 0xE644
    assert u16(rom, 0x009A64) == 0x5244

    # 0x009AD0: custom elevation probe entry
    assert u16(rom, 0x009AD0) == 0x48E7
    assert u16(rom, 0x009AD0 + 2) == 0x04C0
    assert u16(rom, 0x009AD4) == 0x6022          # bra.b $9AF8

    # 0x009AD6: entity elevation probe entry
    assert u16(rom, 0x009AD6) == 0x48E7
    assert u16(rom, 0x009AD6 + 2) == 0x04C0
    assert u16(rom, 0x009ADA) == 0x41F9
    assert u32(rom, 0x009ADA + 2) == 0x00FF1766
    assert u32(rom, 0x009AEA + 2) == 0x00FF1770

    # 0x009B12: lea.l $96F8(pc), a1
    assert u16(rom, 0x009B12) == 0x43FA
    disp = s16(rom, 0x009B12 + 2)
    assert (0x009B12 + 2 + disp) & 0xFFFFFF == 0x0096F8

    # 0x009B1E: odd surface test (slope)
    assert u16(rom, 0x009B1E) == 0x0805
    assert u16(rom, 0x009B1E + 2) == 0x0000

    # 0x009B84: elevation table $FF1706
    assert u16(rom, 0x009B84) == 0x45F9
    assert u32(rom, 0x009B84 + 2) == 0x00FF1706

    # 0x009B98: mulu.w d3, d1
    assert u16(rom, 0x009B98) == 0xC2C3
    # 0x009B9A: divu.w d2, d1
    assert u16(rom, 0x009B9A) == 0x82C2

    # 0x009BC0: rts
    assert u16(rom, 0x009BC0) == 0x4E75

    # 16-byte LUT 2 at 0x0096F8
    lut2 = list(rom[0x0096F8:0x0096F8 + 16])
    expected_lut2 = [0xFF, 0x00, 0x02, 0x01, 0x04, 0x04, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 0x05, 0x05, 0x05, 0x05]
    assert lut2 == expected_lut2

    return {
        "milestone": "M15.405",
        "title": "ROM Ramp/Bridge Slope Solver, Fractional Height Interpolator (mulu/divu) & Tile Elevation Engine",
        "routines": {
            "ramp_search_handler_1": {
                "start": "0x0099F0",
                "end": "0x009A60",
                "bytes": 112,
                "max_scan_tiles": 48,
                "bit4_mask": "0x10",
            },
            "ramp_search_handler_2": {
                "start": "0x009A60",
                "end": "0x009AD0",
                "bytes": 112,
                "max_scan_tiles": 48,
            },
            "tile_elevation_engine": {
                "start": "0x009AD0",
                "end": "0x009BC2",
                "bytes": 242,
                "layer1_ram": "0x00FF1766",
                "layer2_ram": "0x00FF1770",
                "lut2_addr": "0x0096F8",
                "elevation_table_ram": "0x00FF1706",
                "slope_math": "mulu.w d3, d1; divu.w d2, d1; add.w d1, d4",
                "lut2_table": [f"0x{b:02X}" for b in lut2],
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
