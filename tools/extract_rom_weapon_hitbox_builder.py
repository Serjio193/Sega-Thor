#!/usr/bin/env python3
"""Extract and verify ROM facing-mirrored 3D weapon hitbox builder & FX-pool scanner (0x00B9EC..0x00BB1E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_weapon_hitbox_builder_m15_353.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00BA0A) == 0x00FF2ABC  # 8-slot pickup/FX pool
    assert u16(rom, 0x00BA0E) == 0x7E07      # moveq #7, d7 (8 slots)
    assert u16(rom, 0x00BA78) == 0x005A      # 0x5A stride
    assert u32(rom, 0x00BAAE) == 0x00FF19E8  # Full 21-slot pool base
    assert u32(rom, 0x00BABA) == 0x00FF1CD8  # Enemy-only pool base (0xFF19E8 + 4*0xBC)
    assert u16(rom, 0x00BABE) == 0x7E10      # moveq #$10, d7 (17 enemy slots)

    return {
        "milestone": "M15.353",
        "title": "ROM Facing-Mirrored 3D Weapon Hitbox Builder & FX-Pool Scanner",
        "routines": {
            "find_fx_pool_hit": {"start": "0x00B9EC", "end": "0x00BAA8"},
            "build_weapon_hitbox_all": {"start": "0x00BAAC", "end": "0x00BB1E"},
            "build_weapon_hitbox_enemies": {"start": "0x00BAB8", "end": "0x00BB1E"},
        },
        "constants": {
            "fx_pool_ram": "0x00FF2ABC",
            "fx_pool_slots": 8,
            "fx_pool_stride": "0x005A",
            "enemy_pool_ram": "0x00FF1CD8",
            "enemy_pool_slots": 17,
            "hitbox_entry_bytes": 12,
            "mirror_facing_id": 3,
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
