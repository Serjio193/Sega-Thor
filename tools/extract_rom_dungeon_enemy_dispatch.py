#!/usr/bin/env python3
"""
extract_rom_dungeon_enemy_dispatch.py — Milestone M15.276

Extracts ROM semantic knowledge for the 4 core Dungeon Enemy Families
(`0x006C`, `0x006E`, `0x0070`, `0x0072`) from `0x00DF54`:
1. `0x006C` (`0x01F5A4`): 3-substate `$66(A6)` loop + 4-entry variant table `[3, 6, 9, 12]` at `0x01F5C8`.
2. `0x006E` (`0x01EB54`): 10-substate `$66(A6)` loop (`0..9`).
3. `0x0070` (`0x01DF1C`): 6-substate `$66(A6)` loop (`0..5`, shared helper `0x01DFA4`).
4. `0x0072` (`0x01D140`): 10-substate `$66(A6)` self-relative jump table at `0x01D152`.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_dungeon_enemy_dispatch_m15_276.json"


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

    # Verify 0x00DF54 entries for 0x006C..0x0072
    expected_handlers = {
        0x006C: 0x01F5A4,
        0x006E: 0x01EB54,
        0x0070: 0x01DF1C,
        0x0072: 0x01D140,
    }
    for tid, pc in expected_handlers.items():
        entry = 0x00DF54 - 0x2C + tid * 2
        assert read_u32_be(rom, entry) == pc

    # Extract 0x006C 4-entry variant table at 0x01F5C8
    variants_006c = [read_u16_be(rom, 0x01F5C8 + i * 2) for i in range(4)]
    assert variants_006c == [3, 6, 9, 12]

    # Extract 0x0072 10-substate jump table at 0x01D152
    substates_0072 = {}
    for i in range(10):
        addr = 0x01D152 + i * 2
        tgt = addr + read_s16_be(rom, addr)
        substates_0072[str(i)] = f"0x{tgt:06X}"

    evidence = {
        "milestone": "M15.276",
        "description": "ROM Dungeon Enemy Families (0x006C, 0x006E, 0x0070, 0x0072) Substate Dispatch Contracts",
        "substate_field": "$66(A6)",
        "families": {
            "0x006C": {"handler": "0x01F5A4", "substate_count": 3, "variant_table": "0x01F5C8", "variants": variants_006c},
            "0x006E": {"handler": "0x01EB54", "substate_count": 10},
            "0x0070": {"handler": "0x01DF1C", "substate_count": 6, "shared_helper": "0x01DFA4"},
            "0x0072": {"handler": "0x01D140", "substate_count": 10, "jump_table": "0x01D152", "substates": substates_0072},
        },
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} (4 dungeon enemy families)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
