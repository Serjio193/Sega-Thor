#!/usr/bin/env python3
"""Extract and verify ROM explosion spawner, 7-frame SFX cooldown & Shade phase toggle (0x00E90E..0x00EA02)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_explosion_spawner_m15_343.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x00E962) == 0x8000      # Alt variant bit 15 flag
    assert u16(rom, 0x00E97C) == 0x0034      # Entity type 0x34 (52)
    assert u16(rom, 0x00E986) == 0x055E      # Base ID 0x055E
    assert u32(rom, 0x00E9AC) == 0x00FF199C  # Global frame counter
    assert u32(rom, 0x00E9B2) == 0x00FF1944  # Last explosion SFX frame
    assert u16(rom, 0x00E9BA) == 0x0007      # 7-frame SFX cooldown
    assert u32(rom, 0x00E9CA) == 0x0090021B  # Explosion SFX 1
    assert u32(rom, 0x00E9D4) == 0x00600382  # Explosion SFX 2
    assert u16(rom, 0x00E9E0) == 0x001E      # Shade spirit ID (0x1E)
    assert u32(rom, 0x00E9E2) == 0x00FF1AA4  # Active spirit ID RAM

    return {
        "milestone": "M15.343",
        "title": "ROM Explosion Spawner, 7-Frame SFX Cooldown & Shade Phase Toggle",
        "routines": {
            "spawn_explosion_normal": {"start": "0x00E90E", "end": "0x00E938"},
            "spawn_explosion_hi": {"start": "0x00E93A", "end": "0x00E96A"},
            "init_explosion_and_sfx": {"start": "0x00E96C", "end": "0x00E9DC"},
            "sync_shade_phase_flags": {"start": "0x00E9DE", "end": "0x00EA02"},
        },
        "constants": {
            "entity_type_id": "0x0034",
            "base_sprite_id": "0x055E",
            "hi_flag_mask": "0x8000",
            "sfx_cooldown_frames": 7,
            "explosion_sfx_1": "0x0090021B",
            "explosion_sfx_2": "0x00600382",
            "shade_spirit_id": "0x001E",
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
