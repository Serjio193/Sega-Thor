#!/usr/bin/env python3
"""
extract_rom_shade_bow_abilities.py — Milestone M15.274

Extracts ROM semantic knowledge for Shade (0x001C) and Bow (0x001E)
abilities, SP costs (via shared guard 0x011B28), and state transitions:
1. Shade (0x001C, state table 0x013008):
   - Shadow Tether / Claw (`0x013036`): SP cost `7`, State `0x06`, Action `0x02`, locks `$FF1982`.
   - Shadow Surge (`0x013180`): State `0x0C`, Action `0x0E`.
2. Bow (0x001E, state table 0x0138E6):
   - Bite (`0x013934`): SP cost `4`.
   - Pollen / Vine (`0x0139BA`): SP cost `9`, calls `0x013FDC`, State `0x04`, Action `0x00`.
   - Plant Storm (`0x013A18`): SP cost `28` (`0x1C`), State `0x0A`, Action `0x10`.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_shade_bow_abilities_m15_274.json"


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    # Verify Shade SP cost 7 at 0x013036 (7007 6100 ...)
    assert read_u16_be(rom, 0x013036) == 0x7007
    shade_cost_1 = rom[0x013037]

    # Verify Bow SP costs 4, 9, 28 (0x1C)
    assert read_u16_be(rom, 0x013934) == 0x7004
    bow_cost_1 = rom[0x013935]
    assert read_u16_be(rom, 0x0139BA) == 0x7009
    bow_cost_2 = rom[0x0139BB]
    assert read_u16_be(rom, 0x013A18) == 0x701C
    bow_cost_3 = rom[0x013A19]

    evidence = {
        "milestone": "M15.274",
        "description": "ROM Shade (0x001C) and Bow (0x001E) Abilities, SP Costs (0x011B28), and State Transitions",
        "shared_sp_guard": "0x011B28",
        "shade_0x001C": {
            "state_table": "0x013008",
            "abilities": {
                "shadow_tether": {"pc": "0x013036", "sp_cost": shade_cost_1, "next_state": "0x06", "action": "0x02"},
                "shadow_surge": {"pc": "0x013180", "next_state": "0x0C", "action": "0x0E"},
            },
        },
        "bow_0x001E": {
            "state_table": "0x0138E6",
            "abilities": {
                "bite": {"pc": "0x013934", "sp_cost": bow_cost_1},
                "pollen_vine": {"pc": "0x0139BA", "sp_cost": bow_cost_2, "helper": "0x013FDC", "next_state": "0x04", "action": "0x00"},
                "plant_storm": {"pc": "0x013A18", "sp_cost": bow_cost_3, "next_state": "0x0A", "action": "0x10"},
            },
        },
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} (Shade + Bow abilities)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
