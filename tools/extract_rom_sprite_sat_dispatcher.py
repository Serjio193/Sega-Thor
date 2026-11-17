#!/usr/bin/env python3
"""Extract and verify ROM master sprite composition, Y-depth sorter & SAT dispatcher (0x00A196..0x00A336)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_sprite_sat_dispatcher_m15_410.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x00A196: Pre-composition clear
    assert u16(rom, 0x00A196) == 0x51F9          # sf.b $ff1651.l
    assert u32(rom, 0x00A196 + 2) == 0x00FF1651

    # 0x00A1AA: Group 1 scan (4 slots of 90 bytes at $FF2954)
    assert u16(rom, 0x00A1AA) == 0x4DF9          # lea.l $ff2954.l, a6
    assert u32(rom, 0x00A1AA + 2) == 0x00FF2954
    assert u16(rom, 0x00A1B0) == 0x7E03          # moveq #3, d7

    # 0x00A1C2: Special pool scan (4 slots of 8 bytes at $FF19C8)
    assert u16(rom, 0x00A1C2) == 0x4DF9          # lea.l $ff19c8.l, a6
    assert u32(rom, 0x00A1C2 + 2) == 0x00FF19C8
    assert u16(rom, 0x00A1C8) == 0x7E03          # moveq #3, d7

    # 0x00A1D6: Group 2 Depth Sorter (21 slots of 188 bytes at $FF19E8)
    assert u16(rom, 0x00A1D8) == 0x4DF9          # lea.l $ff19e8.l, a6
    assert u32(rom, 0x00A1D8 + 2) == 0x00FF19E8
    assert u16(rom, 0x00A1DE) == 0x7C14          # moveq #$14, d6
    assert u16(rom, 0x00A1F2) == 0x23C7          # move.l d7, $ff188e.l
    assert u32(rom, 0x00A1F2 + 2) == 0x00FF188E

    # 0x00A248: Group 4 scan (8 slots of 90 bytes at $FF2ABC)
    assert u16(rom, 0x00A248) == 0x4DF9          # lea.l $ff2abc.l, a6
    assert u32(rom, 0x00A248 + 2) == 0x00FF2ABC
    assert u16(rom, 0x00A24E) == 0x7E07          # moveq #7, d7

    # 0x00A260: Group 3 scan (6 slots of 90 bytes at $FF2D8C)
    assert u16(rom, 0x00A260) == 0x4DF9          # lea.l $ff2d8c.l, a6
    assert u32(rom, 0x00A260 + 2) == 0x00FF2D8C
    assert u16(rom, 0x00A266) == 0x7E05          # moveq #5, d7

    # 0x00A2A6: SAT link finalizer and VDP dirty flags
    assert u16(rom, 0x00A2A6) == 0x4DF9          # lea.l $ff13cc.l, a6 (SAT buffer)
    assert u32(rom, 0x00A2A6 + 2) == 0x00FF13CC
    assert u16(rom, 0x00A2BE) == 0x0039          # ori.b #3, $ff164e.l
    assert u16(rom, 0x00A2BE + 2) == 0x0003
    assert u32(rom, 0x00A2BE + 4) == 0x00FF164E
    assert u16(rom, 0x00A2C6) == 0x4E75          # rts

    return {
        "milestone": "M15.410",
        "title": "ROM Master Sprite Composition, Y-Depth Sorter & SAT Dispatcher",
        "address_range": {
            "start": "0x00A196",
            "end": "0x00A336",
            "bytes": 416,
        },
        "pools": {
            "group_1": {"address": "0x00FF2954", "slots": 4, "stride": 90},
            "special_pool": {"address": "0x00FF19C8", "slots": 4, "stride": 8},
            "group_2_depth_sorted": {"address": "0x00FF19E8", "slots": 21, "stride": 188},
            "group_4": {"address": "0x00FF2ABC", "slots": 8, "stride": 90},
            "group_3": {"address": "0x00FF2D8C", "slots": 6, "stride": 90},
        },
        "vdp_sat": {
            "buffer": "0x00FF13CC",
            "sync_flags": "0x00FF164E",
            "sync_mask": "0x03",
        },
        "render_targets": {
            "normal_sat": "0x00AD76",
            "custom_sat": "0x00A9A0",
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
