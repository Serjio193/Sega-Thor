#!/usr/bin/env python3
"""Extract and verify ROM death explosion burst, latch follow & projectile probe (0x00EA04..0x00EB40)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_projectile_latch_burst_m15_344.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x00EA38) == 0x001F      # Jitter mask (0..31)
    assert u16(rom, 0x00EA3C) == 0x0010      # Jitter bias (-16..+15)
    assert u32(rom, 0x00EA5C) == 0x0090021B  # Burst SFX 1
    assert u32(rom, 0x00EA66) == 0x00600382  # Burst SFX 2
    assert u32(rom, 0x00EAEA) == 0x00700378  # Projectile wall/shield bounce SFX
    assert u16(rom, 0x00EAFA) == 0x72FA      # moveq #-6, d1 (6px probe radius)

    return {
        "milestone": "M15.344",
        "title": "ROM Death Explosion Burst, Latch Follow & Projectile Probe",
        "routines": {
            "death_explosion_burst": {"start": "0x00EA04", "end": "0x00EA74"},
            "carried_latch_follow": {"start": "0x00EA76", "end": "0x00EABE"},
            "projectile_wall_and_hit_probe": {"start": "0x00EAC0", "end": "0x00EB40"},
        },
        "constants": {
            "burst_jitter_mask": "0x1F",
            "burst_jitter_bias": -16,
            "burst_sfx_1": "0x0090021B",
            "burst_sfx_2": "0x00600382",
            "latch_timer_offset": "0xB8",
            "latch_parent_offset": "0xB4",
            "latch_local_xyz_offset": "0xAE",
            "proj_bounce_sfx": "0x00700378",
            "proj_probe_half_extent": 6,
            "proj_payload_src_offset": "0xAA",
            "target_damage_dst_offset": "0x7E",
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
