#!/usr/bin/env python3
"""Extract and verify ROM room height-band table builder & tileset chunk loader (0x00D344..0x00D4FE)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_room_height_tileset_m15_321.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00D368) == 0x00FFFF5C  # Per-col VScroll buffer
    assert u32(rom, 0x00D386) == 0x00FF1348  # Full-screen VScroll pair
    assert u32(rom, 0x00D3BE) == 0x0005CE96  # Tileset/chunk pointer table
    assert u32(rom, 0x00D3CA) == 0x00003820  # Decompressor entry
    assert u32(rom, 0x00D440) == 0x00FF16FE  # Band step table (4 words)
    assert u32(rom, 0x00D446) == 0x00FF1706  # Elevation height LUT (8 words)
    assert u16(rom, 0x00D468) == 0xF000      # Sentinel at $FF1714
    assert u32(rom, 0x00D472) == 0x00FF1716  # Plane A state (20 words)
    assert u32(rom, 0x00D478) == 0x00FF173E  # Plane B state (20 words)
    assert u32(rom, 0x00D48C) == 0xFFF8FFF8  # 8px camera quantization mask
    assert u32(rom, 0x00D4CA) == 0x00FF3FA8  # Tileset decompression base
    assert u32(rom, 0x00D4D0) == 0x0005CE96  # Chunk pointer table

    return {
        "milestone": "M15.321",
        "title": "ROM Room Height-Band Table Builder & Tileset Chunk Loader",
        "routines": {
            "flush_vscroll_to_vdp_buffer": {"start": "0x00D344", "end": "0x00D396"},
            "sync_hv_dirty_to_vint": {"start": "0x00D398", "end": "0x00D3B0"},
            "load_single_chunk_dma": {"start": "0x00D3B2", "end": "0x00D404"},
            "init_room_bands_and_chunks": {"start": "0x00D406", "end": "0x00D4FE"},
        },
        "constants": {
            "band_count": 4,
            "height_lut_entries": 8,
            "height_sentinel": "0xF000",
            "plane_state_words": 20,
            "camera_quantize_mask": "0xFFF8FFF8",
            "chunk_table_rom": "0x0005CE96",
            "decompressor_rom": "0x00003820",
            "chunk_ram_base": "0x00FF3FA8",
            "chunk_ram_stride": "0x1000",
            "band_steps_ram": "0x00FF16FE",
            "height_lut_ram": "0x00FF1706",
            "vscroll_col_buf_ram": "0x00FFFF5C",
            "vscroll_full_ram": "0x00FF1348",
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
