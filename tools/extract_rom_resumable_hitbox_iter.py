#!/usr/bin/env python3
"""Extract and verify ROM resumable 21-slot masked 3D hitbox iterator (0x00B852..0x00B920)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_resumable_hitbox_iter_m15_352.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00B874) == 0x00FF19E8  # 21-slot entity pool
    assert u16(rom, 0x00B878) == 0x7E14      # moveq #$14, d7 (21 slots)
    assert u16(rom, 0x00B8EE) == 0x00BC      # entity stride
    assert u32(rom, 0x00B902) == 0x00FF196A  # Iterator state save end (d0-d7)
    assert u32(rom, 0x00B914) == 0x00FF194A  # Iterator state restore base

    return {
        "milestone": "M15.352",
        "title": "ROM Resumable 21-Slot Masked 3D Hitbox Iterator",
        "routines": {
            "find_first_3d_point_hit": {"start": "0x00B852", "end": "0x00B910"},
            "find_next_3d_point_hit": {"start": "0x00B912", "end": "0x00B920"},
            "find_first_3d_box_hit": {"start": "0x00B922", "end": "0x00B99E"},
        },
        "constants": {
            "entity_pool_ram": "0x00FF19E8",
            "entity_slot_count": 21,
            "entity_stride_bytes": "0x00BC",
            "iter_save_ram_start": "0x00FF194A",
            "iter_save_ram_end": "0x00FF196A",
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
