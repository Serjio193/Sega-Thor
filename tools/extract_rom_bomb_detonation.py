#!/usr/bin/env python3
"""Extract and verify ROM bomb detonation blast spawner & screen-shake trigger (0x00ED04..0x00EDA8)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_bomb_detonation_m15_346.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x00ED26) == 0x003A      # Blast entity type (0x3A = 58)
    assert u32(rom, 0x00ED3C) == 0x00040000  # Z drop 4.0 px
    assert u16(rom, 0x00ED58) == 0x8007      # Blast damage 0x8007
    assert u16(rom, 0x00ED5E) == 0x0002      # Blast hit type 2
    assert u16(rom, 0x00ED82) == 0x0008      # Blast state/anim 8
    assert u32(rom, 0x00ED8E) == 0x00B00225  # Blast SFX 1
    assert u32(rom, 0x00ED98) == 0x00600382  # Blast SFX 2
    assert u16(rom, 0x00EDA2) == 0x0004      # Screen shake frames (4)
    assert u32(rom, 0x00EDA4) == 0x00FF17C4  # Screen shake timer RAM

    return {
        "milestone": "M15.346",
        "title": "ROM Bomb Detonation Blast Spawner & Screen-Shake Trigger",
        "routine": {"start": "0x00ED04", "end": "0x00EDA8"},
        "constants": {
            "blast_entity_type": "0x003A",
            "blast_z_drop_16_16": "0x00040000",
            "blast_damage_word": "0x8007",
            "blast_hit_type": 2,
            "blast_state": 8,
            "blast_sfx_1": "0x00B00225",
            "blast_sfx_2": "0x00600382",
            "screen_shake_frames": 4,
            "screen_shake_ram": "0x00FF17C4",
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
