#!/usr/bin/env python3
"""
extract_rom_decimal_string_formatter.py — ROM Fixed-Width & Space-Padded
Decimal String Formatters (0x00C0BE..0x00C17A, 188 bytes)
"""

import hashlib
import json
import os
import struct

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C0BE
END_ADDR = 0x00C17A
SIZE = END_ADDR - START_ADDR  # 188 bytes

BUFFER_ADDR = 0x00FF192A
BUFFER_LEN = 5

DIVISORS = [10000, 1000, 100, 10]

def format_dec_fixed5(val: int) -> str:
    """Emulates 0x00C0BE: 5-digit zero-padded decimal string."""
    val = val & 0xFFFF
    digits = []
    rem = val
    for d in DIVISORS:
        q, rem = divmod(rem, d)
        digits.append(chr(ord('0') + q))
    digits.append(chr(ord('0') + rem))
    return "".join(digits)

def format_dec_spaces(val: int) -> str:
    """Emulates 0x00C114: 5-character leading-space padded decimal string."""
    val = val & 0xFFFF
    digits = []
    rem = val
    suppress = True
    for d in DIVISORS:
        q, rem = divmod(rem, d)
        if q != 0 or not suppress:
            suppress = False
            digits.append(chr(ord('0') + q))
        else:
            digits.append(' ')
    digits.append(chr(ord('0') + rem))
    return "".join(digits)

def extract():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == SIZE, f"Expected {SIZE} bytes, got {len(routine_bytes)}"

    # Check key opcodes
    assert struct.unpack_from(">H", rom, 0x00C0BE)[0] == 0x48E7, "movem.l d0, -(a7)"
    assert struct.unpack_from(">I", rom, 0x00C0C4)[0] == BUFFER_ADDR
    assert struct.unpack_from(">H", rom, 0x00C0CE)[0] == 0x80FC, "divu.w #$2710, d0"
    assert struct.unpack_from(">H", rom, 0x00C0D0)[0] == 0x2710
    assert struct.unpack_from(">H", rom, 0x00C112)[0] == 0x4E75, "rts 1"
    assert struct.unpack_from(">H", rom, 0x00C114)[0] == 0x48E7, "movem.l d0, -(a7)"
    assert struct.unpack_from(">I", rom, 0x00C11A)[0] == BUFFER_ADDR
    assert struct.unpack_from(">H", rom, 0x00C12E)[0] == 0x10FC, "move.b #$20, (a0)+"
    assert struct.unpack_from(">H", rom, 0x00C130)[0] == 0x0020
    assert struct.unpack_from(">H", rom, 0x00C178)[0] == 0x4E75, "rts 2"

    data = {
        "routine": "Fixed-Width & Space-Padded Decimal String Formatters",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": SIZE,
        "buffer_address": f"0x{BUFFER_ADDR:08X}",
        "buffer_len": BUFFER_LEN,
        "entry_points": {
            "format_dec_fixed5": "0x00C0BE",
            "format_dec_spaces": "0x00C114",
        },
        "divisors": DIVISORS,
        "sha256": hashlib.sha256(routine_bytes).hexdigest(),
    }

    out_path = "data/thor-evidence/rom_decimal_string_formatter_m15_425.json"
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"Extracted {SIZE} bytes, saved evidence to {out_path}")

if __name__ == "__main__":
    extract()
