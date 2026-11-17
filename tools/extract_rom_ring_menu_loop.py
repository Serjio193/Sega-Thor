#!/usr/bin/env python3
"""Extract and verify ROM ring/spirit menu entry guard & input loop (0x004416..0x00455C)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ring_menu_loop_m15_327.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x004418) == 0x00FF1A6E  # Menu block flag
    assert u32(rom, 0x004422) == 0x00FF19EC  # Ali action state
    assert u16(rom, 0x004420) == 0x0002      # Max allowed state (<= 2)
    assert u32(rom, 0x00442A) == 0x00A00253  # Error SFX
    assert u32(rom, 0x004482) == 0x00FF0E50  # Packed ring cursor
    assert u32(rom, 0x0044A8) == 0x00FF1996  # Button A/BC swap flag
    assert u32(rom, 0x0044B2) == 0x00FF165E  # Joypad state
    assert u32(rom, 0x00453A) == 0x00A00347  # Menu close SFX

    return {
        "milestone": "M15.327",
        "title": "ROM Ring/Spirit Menu Entry Guard & Input Loop",
        "routines": {
            "try_open_ring_menu": {"start": "0x004416", "end": "0x004438"},
            "ring_menu_input_poll": {"start": "0x004480", "end": "0x00455C"},
        },
        "constants": {
            "menu_block_ram": "0x00FF1A6E",
            "player_state_ram": "0x00FF19EC",
            "max_open_player_state": 2,
            "error_sfx_cmd": "0x00A00253",
            "close_sfx_cmd": "0x00A00347",
            "cursor_ram": "0x00FF0E50",
            "button_swap_ram": "0x00FF1996",
            "joypad_ram": "0x00FF165E",
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
