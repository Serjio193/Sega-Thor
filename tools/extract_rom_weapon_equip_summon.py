#!/usr/bin/env python3
"""Extract and verify ROM spirit summon trigger & weapon quick-equip mapper (0x007E9E..0x007F80)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_weapon_equip_summon_m15_338.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x007EAC) == 0x0014      # Ali summon state 0x14 (20)
    assert u32(rom, 0x007EB8) == 0x00900359  # Spirit summon SFX
    assert u32(rom, 0x007ED2) == 0x00FF0E50  # Ring cursor slot
    assert u32(rom, 0x007ED8) == 0x00FF0DB0  # Equipped slot
    assert u32(rom, 0x007F0C) == 0x00FF0DB1  # Equipped item ID
    assert u32(rom, 0x007F34) == 0x00FF0D7E  # Weapon class ID
    assert u16(rom, 0x007F50) == 0x003C      # 60-frame equip toast timer
    assert u32(rom, 0x007F5E) == 0x00C0025D  # Weapon equip SFX

    return {
        "milestone": "M15.338",
        "title": "ROM Spirit Summon Trigger & Weapon Quick-Equip Class Mapper",
        "routines": {
            "start_spirit_summon": {"start": "0x007E9E", "end": "0x007EC2"},
            "quick_equip_weapon_slot": {"start": "0x007EC4", "end": "0x007F80"},
        },
        "weapon_class_map": {
            "dagger_id_1": "0x0A",
            "sword_ids_2_to_5": "0x0C",
            "bow_ids_6_to_8": "0x0E",
            "bomb_ids_9_to_11": "0x10",
            "special_id_12": "0x0E",
            "default_gt_12": "0x0A",
        },
        "constants": {
            "summon_player_state": 20,
            "summon_sfx": "0x00900359",
            "equip_sfx": "0x00C0025D",
            "error_sfx": "0x00A00253",
            "equip_toast_frames": 60,
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
