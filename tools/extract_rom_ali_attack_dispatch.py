#!/usr/bin/env python3
"""Extract and verify ROM Ali attack action & weapon-class state dispatcher (0x008002..0x00814C)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_attack_dispatch_m15_340.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x008004) == 0x00FF0D7E  # Weapon class ID
    assert u16(rom, 0x00801E) == 0x003A      # Dagger ground anim
    assert u16(rom, 0x008022) == 0x000A      # Dagger ground state
    assert u16(rom, 0x00806E) == 0x0060      # Sword thrust anim
    assert u16(rom, 0x008072) == 0x0054      # Sword thrust state
    assert u16(rom, 0x00807A) == 0x004E      # Sword slash anim
    assert u16(rom, 0x00807E) == 0x0046      # Sword slash state
    assert u16(rom, 0x008086) == 0x0066      # Sword spin anim
    assert u16(rom, 0x00808A) == 0x005C      # Sword spin state
    assert u32(rom, 0x00809C) == 0x00600377  # Bow fire SFX
    assert u16(rom, 0x0080A8) == 0x0054      # Normal bow anim
    assert u16(rom, 0x0080AC) == 0x004C      # Bow state
    assert u16(rom, 0x0080BC) == 0x005C      # Hyper bow anim (item 8)
    assert u16(rom, 0x0080CE) == 0x0058      # Bomb throw anim
    assert u16(rom, 0x0080D2) == 0x0050      # Bomb throw state
    assert u16(rom, 0x008110) == 0x0040      # Air kick anim
    assert u16(rom, 0x008114) == 0x0038      # Air kick state

    return {
        "milestone": "M15.340",
        "title": "ROM Ali Attack Action & Weapon-Class State Dispatcher",
        "routine": {"start": "0x008002", "end": "0x00814C"},
        "dispatch_states": {
            "dagger_ground": {"weapon_class": "0x0A", "anim": "0x3A", "state": "0x0A"},
            "dagger_air_kick": {"weapon_class": "0x0A", "anim": "0x40", "state": "0x38", "air_stun": "0x15"},
            "sword_slash": {"weapon_class": "0x0C", "anim": "0x4E", "state": "0x46"},
            "sword_thrust": {"weapon_class": "0x0C", "anim": "0x60", "state": "0x54"},
            "sword_spin": {"weapon_class": "0x0C", "anim": "0x66", "state": "0x5C"},
            "bow_fire": {"weapon_class": "0x0E", "anim": "0x54", "hyper_anim": "0x5C", "state": "0x4C", "sfx": "0x00600377"},
            "bomb_throw": {"weapon_class": "0x10", "anim": "0x58", "state": "0x50"},
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
