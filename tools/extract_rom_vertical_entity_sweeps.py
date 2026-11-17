#!/usr/bin/env python3
"""Extract and verify ROM vertical (+Y, -Y) solid dynamic entity collision sweeps (0x009FA0..0x00A196)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_vertical_entity_sweeps_m15_409.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x009FA0: Positive Y (+Y) sweep
    assert u16(rom, 0x009FA0) == 0xD46E          # add.w $14(a6), d2
    assert u16(rom, 0x009FA0 + 2) == 0x0014
    assert u16(rom, 0x009FA4) == 0xD46E          # add.w $44(a6), d2
    assert u16(rom, 0x009FA4 + 2) == 0x0044
    assert u16(rom, 0x009FBE) == 0x4BF9          # lea.l $ff19e8.l, a5
    assert u32(rom, 0x009FBE + 2) == 0x00FF19E8
    assert u16(rom, 0x009FC4) == 0x7E14          # moveq #$14, d7
    assert u16(rom, 0x00A05A) == 0x08EE          # bset.b #7, $41(a6)
    assert u16(rom, 0x00A05A + 2) == 0x0007
    assert u16(rom, 0x00A05A + 4) == 0x0041
    assert u16(rom, 0x00A064) == 0x1D7C          # move.b #3, $6c(a6)
    assert u16(rom, 0x00A064 + 2) == 0x0003
    assert u16(rom, 0x00A064 + 4) == 0x006C
    assert u16(rom, 0x00A084) == 0x526D          # addq.w #1, $76(a5)
    assert u16(rom, 0x00A084 + 2) == 0x0076
    assert u16(rom, 0x00A098) == 0x4E75          # rts

    # 0x00A09A: Negative Y (-Y) sweep
    assert u16(rom, 0x00A09A) == 0xD46E          # add.w $14(a6), d2
    assert u16(rom, 0x00A09A + 2) == 0x0014
    assert u16(rom, 0x00A09E) == 0x946E          # sub.w $44(a6), d2
    assert u16(rom, 0x00A09E + 2) == 0x0044
    assert u16(rom, 0x00A0B8) == 0x4BF9          # lea.l $ff19e8.l, a5
    assert u32(rom, 0x00A0B8 + 2) == 0x00FF19E8
    assert u16(rom, 0x00A0BE) == 0x7E14          # moveq #$14, d7
    assert u16(rom, 0x00A156) == 0x08EE          # bset.b #7, $41(a6)
    assert u16(rom, 0x00A156 + 2) == 0x0007
    assert u16(rom, 0x00A156 + 4) == 0x0041
    assert u16(rom, 0x00A160) == 0x1D7C          # move.b #3, $6c(a6)
    assert u16(rom, 0x00A160 + 2) == 0x0003
    assert u16(rom, 0x00A160 + 4) == 0x006C
    assert u16(rom, 0x00A180) == 0x536D          # subq.w #1, $76(a5)
    assert u16(rom, 0x00A180 + 2) == 0x0076
    assert u16(rom, 0x00A194) == 0x4E75          # rts

    return {
        "milestone": "M15.409",
        "title": "ROM Vertical (±Y) Solid Dynamic Entity Collision Sweeps",
        "pool": {
            "address": "0x00FF19E8",
            "slots": 21,
            "stride": 188,
        },
        "routines": {
            "sweep_pos_y": {
                "start": "0x009FA0",
                "end": "0x00A098",
                "bytes": 248,
                "push_opcode": "addq.w #1, $76(a5)",
                "collision_bit": 7,
                "reaction_code": 3,
            },
            "sweep_neg_y": {
                "start": "0x00A09A",
                "end": "0x00A196",
                "bytes": 252,
                "push_opcode": "subq.w #1, $76(a5)",
                "collision_bit": 7,
                "reaction_code": 3,
            },
        },
    }


def main() -> None:
    if not ROM_PATH.exists():
        raise FileNotFoundError(f"Missing ROM: {ROM_PATH}")
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
