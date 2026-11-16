#!/usr/bin/env python3
"""
extract_rom_heavy_enemy_dispatch.py — Milestone M15.277

Extracts ROM semantic knowledge for 4 Heavy / Boss Enemy Families
(`0x0068`, `0x0074`, `0x0076`, `0x0080`) from `0x00DF54`:
1. `0x0068` (`0x025B32`): 5-substate `$66(A6)` dispatch (`0..4`).
2. `0x0074` (`0x027C8A`): 5-substate `$66(A6)` dispatch (`0..4`).
3. `0x0076` (`0x0283E6`): 4-substate (`$66(A6) & 0x7F`) dispatch (`0..3`),
   timer `0x14` (20f), Z `0x20`, `$90 = $96`.
4. `0x0080` (`0x028810`): 3-substate `$66(A6)` dispatch (`0..2`),
   timer `0x28` (40f), `State 6`, `Action 4`, `$90 = $96`.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_heavy_enemy_dispatch_m15_277.json"


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def decode_beq_w(rom: bytes, pc: int) -> int:
    assert read_u16_be(rom, pc) == 0x6700, f"Expected beq.w at 0x{pc:06X}"
    return pc + 2 + read_s16_be(rom, pc + 2)


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    expected_handlers = {
        0x0068: 0x025B32,
        0x0074: 0x027C8A,
        0x0076: 0x0283E6,
        0x0080: 0x028810,
    }
    for tid, pc in expected_handlers.items():
        entry = 0x00DF54 - 0x2C + tid * 2
        assert read_u32_be(rom, entry) == pc

    families = {
        "0x0068": {
            "handler": "0x025B32",
            "substate_count": 5,
            "substate_0_target": f"0x{decode_beq_w(rom, 0x025B36):06X}",
            "substate_1_target": f"0x{decode_beq_w(rom, 0x025B3C):06X}",
        },
        "0x0074": {
            "handler": "0x027C8A",
            "substate_count": 5,
            "substate_0_target": f"0x{decode_beq_w(rom, 0x027C8E):06X}",
            "substate_1_target": f"0x{decode_beq_w(rom, 0x027C94):06X}",
        },
        "0x0076": {
            "handler": "0x0283E6",
            "substate_mask": "0x7F",
            "substate_count": 4,
            "substate_0_target": f"0x{decode_beq_w(rom, 0x0283EE):06X}",
            "init_timer_frames": 0x14,
        },
        "0x0080": {
            "handler": "0x028810",
            "substate_count": 3,
            "substate_0_target": f"0x{decode_beq_w(rom, 0x028814):06X}",
            "init_timer_frames": 0x28,
        },
    }

    evidence = {
        "milestone": "M15.277",
        "description": "ROM Heavy / Boss Enemy Families (0x0068, 0x0074, 0x0076, 0x0080) Substate Dispatch",
        "substate_field": "$66(A6)",
        "families": families,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} (4 heavy enemy families)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
