#!/usr/bin/env python3
"""Extract and verify ROM plane scroll delta & VDP H/V-scroll buffer sync (0x00D188..0x00D342)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_plane_scroll_sync_m15_320.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u16(rom, 0x00D1CA) == 0x01FF  # HScroll 9-bit mask (512 px)
    assert u16(rom, 0x00D230) == 0x00FF  # VScroll 8-bit mask (256 px)
    assert s16(rom, 0x00D1E0) == -10000  # 0xD8F0
    assert s16(rom, 0x00D1EC) == 10000   # 0x2710
    assert u32(rom, 0x00D2E8) == 0x00FFFB5C  # Per-line/cell HScroll buffer
    assert u32(rom, 0x00D2EE) == 0x00FF1716  # Plane A state struct
    assert u32(rom, 0x00D332) == 0x00FF1344  # Full-screen HScroll pair

    return {
        "milestone": "M15.320",
        "title": "ROM Plane Scroll Delta & VDP H/V-Scroll Buffer Sync",
        "routines": {
            "update_plane_scroll_deltas": {"start": "0x00D188", "end": "0x00D264"},
            "wrap_camera_to_map_bounds": {"start": "0x00D266", "end": "0x00D2B2"},
            "flush_hscroll_to_vdp_buffer": {"start": "0x00D2B4", "end": "0x00D342"},
        },
        "constants": {
            "hscroll_mask": "0x01FF",
            "vscroll_mask": "0x00FF",
            "overflow_wrap_min": -10000,
            "overflow_wrap_max": 10000,
            "dirty_bit_col_stream": 0,
            "dirty_bit_row_stream": 1,
            "dirty_bit_hscroll": 2,
            "dirty_bit_vscroll": 3,
            "plane_a_struct_ram": "0x00FF1716",
            "hscroll_line_buf_ram": "0x00FFFB5C",
            "hscroll_full_ram": "0x00FF1344",
            "vdp_dirty_flags_ram": "0x00FF16F0",
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
