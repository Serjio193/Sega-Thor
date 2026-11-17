#!/usr/bin/env python3
"""Extract and verify ROM Ali 5-frame input-buffer action & spirit-ball dispatcher (0x007D8A..0x007E9C)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_input_dispatch_m15_350.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x007D8E) == 0x00FF1983  # Input lock flag
    assert u32(rom, 0x007D98) == 0x00FF1668  # 64-frame input ring
    assert u16(rom, 0x007E06) == 0x0004      # 5-frame buffer window (dbra 4)
    assert u32(rom, 0x007E48) == 0x00FF1985  # Cutscene/spirit lock flag
    assert u32(rom, 0x007E68) == 0x00FF1AA4  # Active spirit ID
    assert u16(rom, 0x007E7A) == 0x0014      # Summon state 0x14 (20)
    assert u32(rom, 0x007E8C) == 0x00900359  # Summon SFX

    return {
        "milestone": "M15.350",
        "title": "ROM Ali 5-Frame Input-Buffer Action & Spirit-Ball Dispatcher",
        "routines": {
            "poll_buffered_input": {"start": "0x007D8A", "end": "0x007DC6"},
            "dispatch_ali_buffered_buttons": {"start": "0x007DC8", "end": "0x007E9C"},
        },
        "constants": {
            "input_lock_ram": "0x00FF1983",
            "action_buffer_frames": 5,
            "priority_order": ["C_summon_bit6", "A_attack_bit4", "B_jump_bit5"],
            "summon_state": 20,
            "summon_sfx": "0x00900359",
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
