#!/usr/bin/env python3
"""Extract and verify ROM shadow & special effects SAT composition pass (0x00A6A0..0x00A7E6)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_shadow_sat_pass_m15_413.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x00A6A0: Pre-composition helper
    assert u16(rom, 0x00A6A0) == 0x6100          # bsr.w $a342
    assert u16(rom, 0x00A6A0 + 2) == 0xFCA0

    # 0x00A6A4: Group 1 shadow scan (4 slots of 90 bytes at $FF2954)
    assert u16(rom, 0x00A6A4) == 0x4DF9          # lea.l $ff2954.l, a6
    assert u32(rom, 0x00A6A4 + 2) == 0x00FF2954
    assert u16(rom, 0x00A6B4) == 0x082E          # btst.b #2, $3a(a6)
    assert u16(rom, 0x00A6B4 + 2) == 0x0002
    assert u16(rom, 0x00A6B4 + 4) == 0x003A

    # 0x00A6C8: Group 2 shadow scan (21 slots of 188 bytes at $FF19E8)
    assert u16(rom, 0x00A6C8) == 0x4DF9          # lea.l $ff19e8.l, a6
    assert u32(rom, 0x00A6C8 + 2) == 0x00FF19E8
    assert u16(rom, 0x00A6D8) == 0x082E          # btst.b #2, $3a(a6)
    assert u16(rom, 0x00A6D8 + 2) == 0x0002
    assert u16(rom, 0x00A6D8 + 4) == 0x003A

    # 0x00A73C: Group 4 scan (8 slots of 90 bytes at $FF2ABC)
    assert u16(rom, 0x00A73C) == 0x4DF9          # lea.l $ff2abc.l, a6
    assert u32(rom, 0x00A73C + 2) == 0x00FF2ABC

    # 0x00A754: Group 3 scan (6 slots of 90 bytes at $FF2D8C)
    assert u16(rom, 0x00A754) == 0x4DF9          # lea.l $ff2d8c.l, a6
    assert u32(rom, 0x00A754 + 2) == 0x00FF2D8C

    # 0x00A76C: SAT link terminator
    assert u16(rom, 0x00A76C) == 0x4DF9          # lea.l $ff13cc.l, a6
    assert u32(rom, 0x00A76C + 2) == 0x00FF13CC
    assert u16(rom, 0x00A784) == 0x4E75          # rts

    return {
        "milestone": "M15.413",
        "title": "ROM Shadow & Special Effects SAT Composition Pass",
        "address_range": {
            "start": "0x00A6A0",
            "end": "0x00A7E6",
            "bytes": 326,
        },
        "shadow_flag": {
            "offset": "0x3A",
            "bit": 2,
            "mask": "0x04",
        },
        "pools": {
            "group_1": {"address": "0x00FF2954", "slots": 4, "stride": 90},
            "group_2_depth_sorted": {"address": "0x00FF19E8", "slots": 21, "stride": 188},
            "group_4": {"address": "0x00FF2ABC", "slots": 8, "stride": 90},
            "group_3": {"address": "0x00FF2D8C", "slots": 6, "stride": 90},
        },
        "vdp_sat": {
            "buffer": "0x00FF13CC",
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
