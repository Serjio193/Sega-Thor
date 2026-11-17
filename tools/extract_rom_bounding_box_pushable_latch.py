#!/usr/bin/env python3
"""Extract and verify ROM 2D bounding tile box sampler, type 0x56 pushable block scanner & ground latch (0x009BC2..0x009D4E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_bounding_box_pushable_latch_m15_406.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x009BC2: layer selection & bounding box dispatch
    assert u16(rom, 0x009BC2) == 0x48E7
    assert u16(rom, 0x009BC6) == 0x41F9
    assert u32(rom, 0x009BC6 + 2) == 0x00FF1766
    assert u32(rom, 0x009BD6 + 2) == 0x00FF1770
    assert u16(rom, 0x009BF0) == 0x4E75          # rts

    # 0x009BF2: 2D bounding tile box sampler
    assert u16(rom, 0x009BF2) == 0x48E7
    assert u16(rom, 0x009BF6) == 0x2268          # movea.l 0(a0), a1
    assert u16(rom, 0x009BF6 + 2) == 0x0000
    assert u16(rom, 0x009C3E) == 0x4E75          # rts

    # 0x009C7C: pushable block scanner
    assert u16(rom, 0x009C7C) == 0x48E7
    assert u16(rom, 0x009C80) == 0x43F9          # lea.l $FF2D8C.l, a1
    assert u32(rom, 0x009C80 + 2) == 0x00FF2D8C
    assert u16(rom, 0x009C86) == 0x7E05          # moveq #5, d7 (6 slots)
    assert u16(rom, 0x009C88) == 0x0C69          # cmpi.w #$56, $0(a1)
    assert u16(rom, 0x009C88 + 2) == 0x0056
    assert u16(rom, 0x009CEE) == 0x43E9          # lea.l $5A(a1), a1 (stride 0x5A)
    assert u16(rom, 0x009CEE + 2) == 0x005A
    assert u16(rom, 0x009CFE) == 0x4E75          # rts

    # 0x009D00: ground surface latch
    assert u16(rom, 0x009D00) == 0x48E7
    assert u16(rom, 0x009D32) == 0x43FA          # lea.l $96E8(pc), a1
    disp = s16(rom, 0x009D32 + 2)
    assert (0x009D32 + 2 + disp) & 0xFFFFFF == 0x0096E8
    assert u16(rom, 0x009D44) == 0x3D44          # move.w d4, $14(a6)
    assert u16(rom, 0x009D4C) == 0x4E75          # rts

    return {
        "milestone": "M15.406",
        "title": "ROM 2D Bounding Tile Box Sampler, Type 0x56 Pushable Block Scanner & Ground Latch",
        "routines": {
            "bounding_tile_box_sampler": {
                "start": "0x009BF2",
                "end": "0x009C40",
                "bytes": 78,
                "layer1_ram": "0x00FF1766",
                "layer2_ram": "0x00FF1770",
                "stride_shift_offset": "0x08",
                "row_stride_offset": "0x04",
            },
            "pushable_block_scanner": {
                "start": "0x009C7C",
                "end": "0x009D00",
                "bytes": 132,
                "pool_ram": "0x00FF2D8C",
                "slot_count": 6,
                "stride_bytes": 90,
                "target_type": "0x0056",
                "x_radius_offset": "0x42",
                "y_radius_offset": "0x44",
            },
            "ground_surface_latch": {
                "start": "0x009D00",
                "end": "0x009D4E",
                "bytes": 78,
                "lut_addr": "0x0096E8",
                "surface_type_offset": "0x70",
                "ground_height_offset": "0x14",
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
