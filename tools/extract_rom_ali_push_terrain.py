#!/usr/bin/env python3
"""Extract and verify ROM Ali push physics, jewel socket inserter & isometric velocity scaler (0x0062E4..0x006514)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_push_terrain_m15_375.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00632C) == 0x01500218  # Footstep SFX
    assert u32(rom, 0x0063CE) == 0x00800390  # Heavy block / stone push SFX
    assert u16(rom, 0x006446) == 0x003C      # Jewel toast timer (60 frames)
    assert u16(rom, 0x00644E) == 0x0036      # Jewel insert anim 0x36
    assert u16(rom, 0x006454) == 0x006A      # Jewel insert state 0x6A
    assert u32(rom, 0x0064B8) == 0x00B00385  # Jewel socket chime SFX
    assert u32(rom, 0x0064C6) == 0x00FF1985  # Slope override RAM flag
    assert u32(rom, 0x0064D0) == 0x00FF1984  # Terrain slowdown RAM flag

    return {
        "milestone": "M15.375",
        "title": "ROM Ali Push Physics, Jewel Socket Inserter & Isometric Velocity Scaler",
        "routines": {
            "ali_walk_step_and_footstep_sfx": {"start": "0x0062E4", "end": "0x006336"},
            "ali_push_trigger_and_state": {"start": "0x006338", "end": "0x00638A"},
            "ali_push_heavy_block": {"start": "0x00638C", "end": "0x0063D8"},
            "ali_jewel_socket_insert": {"start": "0x0063DA", "end": "0x0064C2"},
            "scale_isometric_terrain_velocity": {"start": "0x0064C4", "end": "0x006514"},
        },
        "constants": {
            "footstep_sfx": "0x01500218",
            "push_block_sfx": "0x00800390",
            "jewel_toast_frames": 60,
            "jewel_insert_anim": "0x0036",
            "jewel_insert_state": "0x006A",
            "jewel_socket_sfx": "0x00B00385",
            "pushable_block_type": "0x0046",
            "movable_stone_pillar_type": "0x007E",
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
