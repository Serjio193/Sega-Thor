#!/usr/bin/env python3
"""Extract and verify ROM in-game save prompt & save-allowed guard (0x005472..0x005578)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_save_prompt_m15_331.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00548A) == 0x00FF1996  # Button-swap / lock flag
    assert u32(rom, 0x005494) == 0x00FF17C1  # Cutscene/save-block flag
    assert u32(rom, 0x0054A0) == 0x00FF19EC  # Ali action state (<= 2)
    assert u32(rom, 0x0054C0) == 0x00FF16F1  # Room flags (bit 6 = save allowed)
    assert u32(rom, 0x0054CC) == 0x00FF1862  # Prompt choice (3=YES, 4=NO)
    assert u32(rom, 0x00552C) == 0x006003A5  # Toggle SFX
    assert u32(rom, 0x005544) == 0x00A003AA  # Save confirm SFX
    assert u32(rom, 0x005558) == 0x00A00347  # Cancel SFX
    assert u32(rom, 0x00556A) == 0x00A00253  # Error SFX

    return {
        "milestone": "M15.331",
        "title": "ROM In-Game Save Prompt & Save-Allowed Guard",
        "routine": {"start": "0x005472", "end": "0x005578"},
        "constants": {
            "choice_yes": 3,
            "choice_no": 4,
            "default_choice": 4,
            "room_save_allowed_bit": 6,
            "max_player_state": 2,
            "toggle_sfx": "0x006003A5",
            "confirm_sfx": "0x00A003AA",
            "cancel_sfx": "0x00A00347",
            "error_sfx": "0x00A00253",
            "sram_save_routine": "0x00001726",
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
