#!/usr/bin/env python3
"""Extract and verify ROM horizontal (+X, -X) solid dynamic entity collision sweeps (0x009DD6..0x009F9E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_horizontal_entity_sweeps_m15_408.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x009DD6: Positive X (+X) sweep
    assert u16(rom, 0x009DD6) == 0x082E          # btst.b #2, $38(a6)
    assert u16(rom, 0x009DD6 + 2) == 0x0002
    assert u16(rom, 0x009DD6 + 4) == 0x0038
    assert u16(rom, 0x009DDC) == 0x6600          # bne.w $9eb8
    assert u16(rom, 0x009DDC + 2) == 0x00DA
    assert u16(rom, 0x009DFE) == 0x4BF9          # lea.l $ff19e8.l, a5
    assert u32(rom, 0x009DFE + 2) == 0x00FF19E8
    assert u16(rom, 0x009E04) == 0x7E14          # moveq #$14, d7
    assert u16(rom, 0x009E7E) == 0x08EE          # bset.b #7, $40(a6)
    assert u16(rom, 0x009E7E + 2) == 0x0007
    assert u16(rom, 0x009E7E + 4) == 0x0040
    assert u16(rom, 0x009E88) == 0x1D7C          # move.b #3, $6c(a6)
    assert u16(rom, 0x009E88 + 2) == 0x0003
    assert u16(rom, 0x009E88 + 4) == 0x006C
    assert u16(rom, 0x009EA8) == 0x526D          # addq.w #1, $72(a5)
    assert u16(rom, 0x009EA8 + 2) == 0x0072
    assert u16(rom, 0x009EB8) == 0x4E75          # rts

    # 0x009EBA: Negative X (-X) sweep
    assert u16(rom, 0x009EBA) == 0x082E          # btst.b #2, $38(a6)
    assert u16(rom, 0x009EBA + 2) == 0x0002
    assert u16(rom, 0x009EBA + 4) == 0x0038
    assert u16(rom, 0x009EC0) == 0x6600          # bne.w $9f9e
    assert u16(rom, 0x009EC0 + 2) == 0x00DC
    assert u16(rom, 0x009EE2) == 0x4BF9          # lea.l $ff19e8.l, a5
    assert u32(rom, 0x009EE2 + 2) == 0x00FF19E8
    assert u16(rom, 0x009EE8) == 0x7E14          # moveq #$14, d7
    assert u16(rom, 0x009F64) == 0x08EE          # bset.b #7, $40(a6)
    assert u16(rom, 0x009F64 + 2) == 0x0007
    assert u16(rom, 0x009F64 + 4) == 0x0040
    assert u16(rom, 0x009F6E) == 0x1D7C          # move.b #3, $6c(a6)
    assert u16(rom, 0x009F6E + 2) == 0x0003
    assert u16(rom, 0x009F6E + 4) == 0x006C
    assert u16(rom, 0x009F8E) == 0x536D          # subq.w #1, $72(a5)
    assert u16(rom, 0x009F8E + 2) == 0x0072
    assert u16(rom, 0x009F9E) == 0x4E75          # rts

    return {
        "milestone": "M15.408",
        "title": "ROM Horizontal (±X) Solid Dynamic Entity Collision Sweeps",
        "pool": {
            "address": "0x00FF19E8",
            "slots": 21,
            "stride": 188,
        },
        "routines": {
            "sweep_pos_x": {
                "start": "0x009DD6",
                "end": "0x009EB8",
                "bytes": 226,
                "push_opcode": "addq.w #1, $72(a5)",
                "collision_bit": 7,
                "reaction_code": 3,
            },
            "sweep_neg_x": {
                "start": "0x009EBA",
                "end": "0x009F9E",
                "bytes": 228,
                "push_opcode": "subq.w #1, $72(a5)",
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
