#!/usr/bin/env python3
"""Extract and verify ROM entity camera-viewport culler & airborne landing (0x00E522..0x00E67E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_viewport_culler_landing_m15_339.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert rom[0x00E57D] == 0x0F              # Landing stun frames (15)
    assert u32(rom, 0x00E590) == 0x00800270  # Landing thud SFX
    assert u32(rom, 0x00E5B0) == 0x00FF1716  # Plane A camera
    assert u32(rom, 0x00E5C2) == 0x00FF173E  # Plane B camera
    assert u16(rom, 0x00E5EE) == 0x0140      # Screen width 320
    assert u16(rom, 0x00E5F2) == 0x00E0      # Screen height 224
    assert u16(rom, 0x00E64C) == 0x0140      # Screen width 320
    assert u16(rom, 0x00E650) == 0x00E0      # Screen height 224
    assert u16(rom, 0x00E674) == 0x005D      # 94 words = 0xBC bytes struct copy

    return {
        "milestone": "M15.339",
        "title": "ROM Entity Camera-Viewport Culler & Airborne Landing Handler",
        "routines": {
            "airborne_move_and_land": {"start": "0x00E522", "end": "0x00E5AC"},
            "is_offscreen_with_z": {"start": "0x00E5AE", "end": "0x00E60E"},
            "is_offscreen_ground": {"start": "0x00E610", "end": "0x00E66C"},
            "copy_entity_struct_0xbc": {"start": "0x00E66E", "end": "0x00E67E"},
        },
        "constants": {
            "viewport_width_px": 320,
            "viewport_height_px": 224,
            "landing_stun_frames": 15,
            "landing_thud_sfx": "0x00800270",
            "entity_copy_words": 94,
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
