#!/usr/bin/env python3
"""Extract and verify ROM Ali crawl movement, multi-surface footstep SFX & skid deceleration (0x006516..0x006700)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_crawl_skid_m15_376.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00654E) == 0xFFFD6000  # Left crawl X nudge: -0x2A000
    assert u32(rom, 0x00655C) == 0x0002A000  # Right crawl X nudge: +0x2A000
    assert u32(rom, 0x006586) == 0xFFFDA800  # Up crawl Y nudge: -0x25800
    assert u32(rom, 0x006594) == 0x00025800  # Down crawl Y nudge: +0x25800
    assert u32(rom, 0x0065AC) == 0x02400357  # Water footstep SFX 1
    assert u32(rom, 0x0065BC) == 0x0040038E  # Dust footstep SFX 1
    assert u32(rom, 0x0065CA) == 0x02400358  # Water footstep SFX 2
    assert u32(rom, 0x0065DA) == 0x0040038F  # Dust footstep SFX 2
    assert u32(rom, 0x00660C) == 0x01500218  # Creep footstep SFX
    assert u16(rom, 0x00661A) == 0x0006      # Min skid run frames: 6
    assert rom[0x00663B] == 0x15             # Skid dust timer: 21 frames
    assert u32(rom, 0x006652) == 0x1B400344  # Skid scrape SFX
    assert u32(rom, 0x00665E) == 0x0000ACCC  # Skid decel step
    assert u32(rom, 0x0066E0) == 0x00600343  # Slide impact SFX
    assert u32(rom, 0x0066EC) == 0x00008000  # Slide decel step

    return {
        "milestone": "M15.376",
        "title": "ROM Ali Crawl Movement, Multi-Surface Footstep SFX & Skid Deceleration",
        "routines": {
            "ali_crawl_walk_and_surface_sfx": {"start": "0x006516", "end": "0x006616"},
            "ali_skid_stop_friction": {"start": "0x006618", "end": "0x0066A8"},
            "ali_combat_slide_decel": {"start": "0x0066AA", "end": "0x006700"},
        },
        "constants": {
            "crawl_x_nudge": 0x2A000,
            "crawl_y_nudge": 0x25800,
            "water_footstep_1": "0x02400357",
            "dust_footstep_1": "0x0040038E",
            "water_footstep_2": "0x02400358",
            "dust_footstep_2": "0x0040038F",
            "creep_footstep": "0x01500218",
            "min_skid_run_frames": 6,
            "skid_dust_frames": 21,
            "skid_scrape_sfx": "0x1B400344",
            "skid_decel_step": 0xACCC,
            "slide_impact_sfx": "0x00600343",
            "slide_decel_step": 0x8000,
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
