#!/usr/bin/env python3
"""Extract and verify ROM integer sqrt & 256-step atan2 angle LUT (0x00E6BA..0x00E78C, 0x05D906)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_isqrt_atan2_m15_341.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00E6C6) == 0x00000400  # isqrt initial guess cap (1024)
    assert u32(rom, 0x00E70C) == 0x00FF19E8  # Ali entity base
    assert u32(rom, 0x00E770) == 0x0005D906  # 9-word tangent threshold LUT
    assert u16(rom, 0x00E78A) == 0x0040      # 90-degree angle (64)

    tan_lut = [u16(rom, 0x0005D906 + i * 2) for i in range(9)]
    assert tan_lut == [0x0019, 0x004E, 0x0089, 0x00D2, 0x0138, 0x01DF, 0x034C, 0x0A27, 0xFFFF]

    return {
        "milestone": "M15.341",
        "title": "ROM Integer Square Root & 256-Step atan2 Angle LUT",
        "routines": {
            "isqrt_u32": {"start": "0x00E6BA", "end": "0x00E6EE"},
            "atan2_to_point": {"start": "0x00E6F0", "end": "0x00E704"},
            "atan2_to_player": {"start": "0x00E706", "end": "0x00E766"},
            "atan_octant_lut": {"start": "0x00E768", "end": "0x00E78C"},
        },
        "tangent_lut_0x05D906": [f"0x{v:04X}" for v in tan_lut],
        "constants": {
            "isqrt_guess_cap": 0x0400,
            "angle_step": 8,
            "angle_90_deg": 64,
            "angle_180_deg": 128,
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
