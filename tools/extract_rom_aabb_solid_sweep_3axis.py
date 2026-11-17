#!/usr/bin/env python3
"""Extract and verify ROM 3D AABB solid sweep (-X, +Y, -Y) (0x009EBA..0x00A194)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_aabb_solid_sweep_3axis_m15_332.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # Verify entity pool base, count (21 slots, dbra 0x14), and stride 0xBC
    for pool_addr, count_addr, stride_addr in (
        (0x009EE4, 0x009EE8, 0x009F94),  # -X sweep
        (0x009FC0, 0x009FC4, 0x00A08A),  # +Y sweep
        (0x00A0BA, 0x00A0BE, 0x00A186),  # -Y sweep
    ):
        assert u32(rom, pool_addr) == 0x00FF19E8
        assert u16(rom, count_addr) == 0x7E14  # moveq #$14, d7
        assert u16(rom, stride_addr) == 0x00BC

    # Verify push instructions: subq.w #1, $72(a5) / addq.w #1, $76(a5) / subq.w #1, $76(a5)
    assert u16(rom, 0x009F8E) == 0x536D
    assert u16(rom, 0x009F90) == 0x0072
    assert u16(rom, 0x00A084) == 0x526D
    assert u16(rom, 0x00A086) == 0x0076
    assert u16(rom, 0x00A180) == 0x536D
    assert u16(rom, 0x00A182) == 0x0076

    return {
        "milestone": "M15.332",
        "title": "ROM 3D AABB Solid Sweep (-X, +Y, -Y)",
        "routines": {
            "sweep_neg_x": {"start": "0x009EBA", "end": "0x009F9E", "flag_offset": "0x40", "push_offset": "0x72", "push_delta": -1},
            "sweep_pos_y": {"start": "0x009FA0", "end": "0x00A098", "flag_offset": "0x41", "push_offset": "0x76", "push_delta": 1},
            "sweep_neg_y": {"start": "0x00A09A", "end": "0x00A194", "flag_offset": "0x41", "push_offset": "0x76", "push_delta": -1},
        },
        "constants": {
            "entity_pool_ram": "0x00FF19E8",
            "entity_slot_count": 21,
            "entity_stride_bytes": "0x00BC",
            "hit_flag_bit": 7,
            "contact_ptr_offset": "0x68",
            "contact_type_offset": "0x6C",
            "contact_type_solid": 3,
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
