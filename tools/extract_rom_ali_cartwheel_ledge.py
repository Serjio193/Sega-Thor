#!/usr/bin/env python3
"""Extract and verify ROM Ali cartwheel evade hitbox, ledge-catch probe & standup sync (0x006738..0x00693A)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_cartwheel_ledge_m15_377.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert rom[0x006743] == 0x0E             # Cartwheel base damage code: 14
    assert u16(rom, 0x006746) == 0x0018      # Keyframe 24 threshold for damage bonus
    assert u32(rom, 0x006770) == 0x1B400344  # Cartwheel spin whoosh SFX
    assert u32(rom, 0x00679C) == 0x00008000  # Cartwheel linear friction decel step
    assert u32(rom, 0x0067FA) == 0x0000CA24  # Ledge hang pull-up target subroutine
    assert u32(rom, 0x006816) == 0x0000C000  # Pit edge gravity delta
    assert u16(rom, 0x006834) == 0x000E      # Foot probe offset (+14)
    assert u16(rom, 0x006870) == 0x0032      # Ledge catch state 0x32
    assert u16(rom, 0x006898) == 0x0014      # Standup recovery timer (20 frames)
    assert u16(rom, 0x00689E) == 0x0034      # Standup state 0x34
    assert u32(rom, 0x0068FC) == 0x0003F4A6  # Ali anim asset descriptor table
    assert u32(rom, 0x006930) == 0x0080037A  # Standup recovery SFX

    return {
        "milestone": "M15.377",
        "title": "ROM Ali Cartwheel Evade Hitbox, Ledge-Catch Probe & Standup Sync",
        "routines": {
            "ali_cartwheel_evade_hitbox": {"start": "0x006738", "end": "0x0067DC"},
            "ali_ledge_hang_pullup_jump": {"start": "0x0067DE", "end": "0x0067FE"},
            "ali_pit_edge_catch_hold": {"start": "0x006800", "end": "0x006874"},
            "ali_ledge_standup_spirit_sync": {"start": "0x006876", "end": "0x00693A"},
        },
        "constants": {
            "cartwheel_base_damage": 14,
            "cartwheel_keyframe_bonus": 24,
            "cartwheel_spin_sfx": "0x1B400344",
            "cartwheel_friction_step": 0x8000,
            "ledge_pullup_jump_target": "0x0000CA24",
            "pit_edge_gravity_step": 0xC000,
            "ledge_foot_probe_offset": 14,
            "ledge_catch_state": "0x0032",
            "standup_recovery_timer": 20,
            "standup_state": "0x0034",
            "anim_asset_table": "0x0003F4A6",
            "standup_recovery_sfx": "0x0080037A",
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
