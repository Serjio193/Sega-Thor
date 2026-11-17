#!/usr/bin/env python3
"""Extract and verify ROM terrain elevation & ledge probe (0x00938E..0x0096F7)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_terrain_elevation_probe_m15_314.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def s8(val: int) -> int:
    return val - 256 if val >= 128 else val


def extract_evidence(rom: bytes) -> dict:
    # Verify entry points start with move.b $38(a6), d6 (0x1C2E 0x0038)
    for entry in (0x00938E, 0x0094D2, 0x0095AA):
        assert u16(rom, entry) == 0x1C2E
        assert u16(rom, entry + 2) == 0x0038

    # Verify height table & drop delta RAM addresses
    assert u32(rom, 0x00941E) == 0x00FF1706
    assert u32(rom, 0x009436) == 0x00FF196E
    assert u32(rom, 0x00956C) == 0x00FF1706
    assert u32(rom, 0x009584) == 0x00FF196E
    assert u32(rom, 0x00963A) == 0x00FF1706
    assert u32(rom, 0x009652) == 0x00FF196E

    # Verify max ledge scan height 0x180 (384 px) in 0x00948C and 0x0096A4
    assert u16(rom, 0x00948C) == 0x0C47
    assert u16(rom, 0x00948E) == 0x0180
    assert u16(rom, 0x0096A4) == 0x0C47
    assert u16(rom, 0x0096A6) == 0x0180

    # Verify 16-byte elevation nibble LUT at 0x0096E8..0x0096F7
    lut = [s8(b) for b in rom[0x0096E8:0x0096F8]]
    assert lut == [-1, 0, 2, 1, 4, -1, 3, -1, 6, 7, -1, -1, 5, -1, -1, -1]

    return {
        "milestone": "M15.314",
        "title": "ROM Terrain Elevation & Ledge Probe",
        "routines": {
            "probe_x": {"start": "0x00938E", "end": "0x00945E", "flag_byte_offset": "0x40"},
            "scan_col_with_ledge": {"start": "0x009460", "end": "0x0094A0"},
            "scan_col_span": {"start": "0x0094A2", "end": "0x0094D0"},
            "probe_y_up": {"start": "0x0094D2", "end": "0x0095A8", "flag_byte_offset": "0x41"},
            "probe_y_down": {"start": "0x0095AA", "end": "0x009676", "flag_byte_offset": "0x41"},
            "scan_row_with_ledge": {"start": "0x009678", "end": "0x0096B8"},
            "scan_row_span": {"start": "0x0096BA", "end": "0x0096E6"},
        },
        "elevation_nibble_lut": {
            "address": "0x0096E8",
            "size_bytes": 16,
            "values": lut,
        },
        "constants": {
            "tile_shift": 3,
            "max_ledge_scan_px": "0x0180",
            "ledge_scan_step_px": 8,
            "drop_delta_min": -2,
            "climb_delta_max": 2,
            "flag_ledge_bit": 5,
            "flag_blocked_bit": 6,
            "entity_flags_offset": "0x38",
            "entity_elev_offset": "0x70",
            "entity_z_vel_offset": "0x10",
            "height_table_ram": "0x00FF1706",
            "drop_height_ram": "0x00FF196E",
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
