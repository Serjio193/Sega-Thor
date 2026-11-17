#!/usr/bin/env python3
"""Extract and verify ROM 4-direction world boundary clamp subsystem (0x009D4E..0x009DD6)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_boundary_clamps_m15_407.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x009D4E: Right (+X) boundary clamp
    assert u16(rom, 0x009D4E) == 0x3E01          # move.w d1, d7
    assert u16(rom, 0x009D50) == 0xDE6E          # add.w $46(a6), d7
    assert u16(rom, 0x009D50 + 2) == 0x0046
    assert u16(rom, 0x009D54) == 0x3628          # move.w $4(a0), d3 (width in tiles)
    assert u16(rom, 0x009D54 + 2) == 0x0004
    assert u16(rom, 0x009D68) == 0x002E          # ori.b #$48, $40(a6)
    assert u16(rom, 0x009D68 + 2) == 0x0048
    assert u16(rom, 0x009D68 + 4) == 0x0040
    assert u16(rom, 0x009D6E) == 0x4E75          # rts

    # 0x009D70: Left (-X) boundary clamp
    assert u16(rom, 0x009D70) == 0x3601          # move.w d1, d3
    assert u16(rom, 0x009D72) == 0x966E          # sub.w $46(a6), d3
    assert u16(rom, 0x009D72 + 2) == 0x0046
    assert u16(rom, 0x009D8A) == 0x002E          # ori.b #$48, $40(a6)
    assert u16(rom, 0x009D8A + 2) == 0x0048
    assert u16(rom, 0x009D8A + 4) == 0x0040
    assert u16(rom, 0x009D90) == 0x4E75          # rts

    # 0x009D92: Bottom (+Y) boundary clamp
    assert u16(rom, 0x009D92) == 0x3E02          # move.w d2, d7
    assert u16(rom, 0x009D94) == 0xDE6E          # add.w $46(a6), d7
    assert u16(rom, 0x009D94 + 2) == 0x0046
    assert u16(rom, 0x009D98) == 0x3628          # move.w $6(a0), d3 (height in tiles)
    assert u16(rom, 0x009D98 + 2) == 0x0006
    assert u16(rom, 0x009DAC) == 0x002E          # ori.b #$48, $41(a6)
    assert u16(rom, 0x009DAC + 2) == 0x0048
    assert u16(rom, 0x009DAC + 4) == 0x0041
    assert u16(rom, 0x009DB2) == 0x4E75          # rts

    # 0x009DB4: Top (-Y) boundary clamp
    assert u16(rom, 0x009DB4) == 0x3602          # move.w d2, d3
    assert u16(rom, 0x009DB6) == 0x966E          # sub.w $46(a6), d3
    assert u16(rom, 0x009DB6 + 2) == 0x0046
    assert u16(rom, 0x009DCE) == 0x002E          # ori.b #$48, $41(a6)
    assert u16(rom, 0x009DCE + 2) == 0x0048
    assert u16(rom, 0x009DCE + 4) == 0x0041
    assert u16(rom, 0x009DD4) == 0x4E75          # rts

    return {
        "milestone": "M15.407",
        "title": "ROM 4-Direction World Boundary Clamp Subsystem",
        "routines": {
            "clamp_right_pos_x": {
                "start": "0x009D4E",
                "end": "0x009D70",
                "bytes": 34,
                "dimension_offset": "0x04",
                "radius_offset": "0x46",
                "collision_flags_offset": "0x40",
                "wall_mask": "0x48",
            },
            "clamp_left_neg_x": {
                "start": "0x009D70",
                "end": "0x009D92",
                "bytes": 34,
                "radius_offset": "0x46",
                "collision_flags_offset": "0x40",
                "wall_mask": "0x48",
            },
            "clamp_bottom_pos_y": {
                "start": "0x009D92",
                "end": "0x009DB4",
                "bytes": 34,
                "dimension_offset": "0x06",
                "radius_offset": "0x46",
                "collision_flags_offset": "0x41",
                "wall_mask": "0x48",
            },
            "clamp_top_neg_y": {
                "start": "0x009DB4",
                "end": "0x009DD6",
                "bytes": 34,
                "radius_offset": "0x46",
                "collision_flags_offset": "0x41",
                "wall_mask": "0x48",
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
