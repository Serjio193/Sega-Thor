#!/usr/bin/env python3
"""
extract_rom_efreet_abilities.py — Milestone M15.273

Extracts ROM semantic knowledge for Efreet (Entity 0x001A) Abilities,
State Table (0x0125AE), and Shared Spirit SP-Cost Guard (0x011B28):
1. Shared Spirit Ability SP-Cost Guard (`0x011B28`):
   - Subtracts `d0` (SP cost) from Ali's `$8E($FF19E8)`.
   - Requires `remaining_sp >= 1` (`cmpi.w #$1, d1; blt`); otherwise fails (`CCR.C=1`).
2. Efreet State Jump Table (`0x0125AE`, 12 states `0x00..0x16`).
3. Efreet Player-Commanded Abilities (`0x0125CA..0x01267E`):
   - Fire Dash (`0x0125CA`): SP cost `8`, State `0x08`, Action `0x0C`, speed `0x0100`, SFX `0x800258`.
   - Flame Pillar (`0x012608`): SP cost `6`, reserves `$FF1C1C`, calls `0x012C4E`, State `0x0A`, Action `0x02`.
   - Meltdown (`0x01263C`): SP cost `25` (`0x19`), State `0x0C`, Action `0x08`, base duration `0x60` (`96` frames) + `$FF0DB7` level bonus.
   - Dismiss (`0x012672`): State `0x10`, Action `0x0A`.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_efreet_abilities_m15_273.json"


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    # 1. Verify shared SP guard minimum remaining SP = 1 at 0x011B30 (0C41 0001)
    assert read_u16_be(rom, 0x011B30) == 0x0C41
    min_remaining_sp = read_u16_be(rom, 0x011B32)
    assert min_remaining_sp == 1

    # 2. Verify Efreet ability SP costs (moveq #imm, d0 -> 70xx)
    dash_cost = rom[0x0125CB]       # 8
    pillar_cost = rom[0x012609]     # 6
    meltdown_cost = rom[0x01263D]   # 25 (0x19)
    assert (dash_cost, pillar_cost, meltdown_cost) == (8, 6, 25)

    # 3. Verify Fire Dash SFX 0x800258 at 0x0125FC (203C 0080 0258)
    assert read_u16_be(rom, 0x0125FC) == 0x203C
    dash_sfx = read_u32_be(rom, 0x0125FE)
    assert dash_sfx == 0x00800258

    # 4. Decode 12-entry state table at 0x0125AE
    tbl = 0x0125AE
    states = {}
    for i in range(12):
        addr = tbl + i * 2
        tgt = addr + read_s16_be(rom, addr)
        states[f"0x{i*2:02X}"] = f"0x{tgt:06X}"

    evidence = {
        "milestone": "M15.273",
        "description": "ROM Efreet (0x001A) Abilities, State Table (0x0125AE), and Shared Spirit SP-Cost Guard (0x011B28)",
        "shared_sp_guard": {
            "routine": "0x011B28",
            "player_sp_field": "$8E($FF19E8)",
            "min_remaining_sp": min_remaining_sp,
        },
        "efreet_state_table": {
            "address": "0x0125AE",
            "states": states,
        },
        "efreet_abilities": {
            "fire_dash": {"pc": "0x0125CA", "sp_cost": dash_cost, "next_state": "0x08", "action": "0x0C", "sfx": f"0x{dash_sfx:06X}"},
            "flame_pillar": {"pc": "0x012608", "sp_cost": pillar_cost, "next_state": "0x0A", "action": "0x02", "helper_slot": "0x00FF1C1C"},
            "meltdown": {"pc": "0x01263C", "sp_cost": meltdown_cost, "next_state": "0x0C", "action": "0x08", "base_duration": 0x60, "level_ram": "0x00FF0DB7"},
            "dismiss": {"pc": "0x012672", "sp_cost": 0, "next_state": "0x10", "action": "0x0A"},
        },
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(states)} states)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
