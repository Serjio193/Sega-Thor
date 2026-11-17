#!/usr/bin/env python3
"""Extract and verify ROM 5-digit decimal-to-ASCII formatter (0x00C0BE..0x00C178)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_decimal_ascii_m15_326.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00C0C4) == 0x00FF192A
    assert u32(rom, 0x00C11A) == 0x00FF192A
    assert u16(rom, 0x00C0D0) == 10000
    assert u16(rom, 0x00C0DE) == 1000
    assert u16(rom, 0x00C0EC) == 100
    assert u16(rom, 0x00C0FA) == 10

    return {
        "milestone": "M15.326",
        "title": "ROM 5-Digit Decimal-to-ASCII Formatter",
        "routines": {
            "format_u16_dec_zero_padded": {"start": "0x00C0BE", "end": "0x00C112"},
            "format_u16_dec_space_padded": {"start": "0x00C114", "end": "0x00C178"},
        },
        "constants": {
            "output_buf_ram": "0x00FF192A",
            "digit_count": 5,
            "divisors": [10000, 1000, 100, 10, 1],
            "ascii_zero": "0x30",
            "ascii_space": "0x20",
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
