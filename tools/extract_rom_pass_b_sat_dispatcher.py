#!/usr/bin/env python3
"""Extract and verify ROM Pass B master SAT dispatcher & cache mask reuse (0x00A524..0x00A69A)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_pass_b_sat_dispatcher_m15_412.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x00A524: Group 1 scan (4 slots of 90 bytes at $FF2954)
    assert u16(rom, 0x00A524) == 0x4DF9          # lea.l $ff2954.l, a6
    assert u32(rom, 0x00A524 + 2) == 0x00FF2954
    assert u16(rom, 0x00A52A) == 0x7E03          # moveq #3, d7

    # 0x00A53C: Special pool scan (4 slots of 8 bytes at $FF19C8)
    assert u16(rom, 0x00A53C) == 0x4DF9          # lea.l $ff19c8.l, a6
    assert u32(rom, 0x00A53C + 2) == 0x00FF19C8
    assert u16(rom, 0x00A542) == 0x7E03          # moveq #3, d7

    # 0x00A550: Cached active bitmask load and Group 2 depth sort
    assert u16(rom, 0x00A550) == 0x2E39          # move.l $ff188e.l, d7
    assert u32(rom, 0x00A550 + 2) == 0x00FF188E
    assert u16(rom, 0x00A556) == 0x4DF9          # lea.l $ff19e8.l, a6
    assert u32(rom, 0x00A556 + 2) == 0x00FF19E8
    assert u16(rom, 0x00A55C) == 0x7C14          # moveq #$14, d6

    # 0x00A5A6: Group 4 scan (8 slots of 90 bytes at $FF2ABC)
    assert u16(rom, 0x00A5A6) == 0x4DF9          # lea.l $ff2abc.l, a6
    assert u32(rom, 0x00A5A6 + 2) == 0x00FF2ABC
    assert u16(rom, 0x00A5AC) == 0x7E07          # moveq #7, d7

    # 0x00A5BE: Group 3 scan (6 slots of 90 bytes at $FF2D8C)
    assert u16(rom, 0x00A5BE) == 0x4DF9          # lea.l $ff2d8c.l, a6
    assert u32(rom, 0x00A5BE + 2) == 0x00FF2D8C
    assert u16(rom, 0x00A5C4) == 0x7E05          # moveq #5, d7

    # 0x00A604: SAT link terminator and VDP sync dirty flags
    assert u16(rom, 0x00A604) == 0x4DF9          # lea.l $ff13cc.l, a6
    assert u32(rom, 0x00A604 + 2) == 0x00FF13CC
    assert u16(rom, 0x00A61C) == 0x0039          # ori.b #3, $ff164e.l
    assert u16(rom, 0x00A61C + 2) == 0x0003
    assert u32(rom, 0x00A61C + 4) == 0x00FF164E
    assert u16(rom, 0x00A624) == 0x4E75          # rts

    return {
        "milestone": "M15.412",
        "title": "ROM Pass B Master SAT Dispatcher & Cache Mask Reuse",
        "address_range": {
            "start": "0x00A524",
            "end": "0x00A69A",
            "bytes": 374,
        },
        "mask_cache": "0x00FF188E",
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
