#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Background Tile Pattern Decompressor
& 4KB VRAM DMA Streamer (0x00D3B2..0x00D406, 84 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_bg_tile_dma_m15_438.json"

START_ADDR = 0x00D3B2
END_ADDR = 0x00D406
ROUTINE_SIZE = END_ADDR - START_ADDR  # 84 bytes

BG_TILE_TABLE_ADDR = 0x05CE96


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    # Read tile pattern pointers from 0x05CE96
    tile_ptrs = []
    for i in range(20):
        ptr = struct.unpack_from(">I", rom, BG_TILE_TABLE_ADDR + i * 4)[0]
        if ptr < len(rom):
            tile_ptrs.append({
                "index": i,
                "address": f"0x{ptr:06X}",
                "valid": ptr != 0,
            })

    evidence = {
        "routine": "bg_tile_pattern_decompressor_and_dma_streamer",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "decompress_and_dma_bg_tiles": f"0x{START_ADDR:06X}",
        },
        "tile_pattern_table": {
            "address": f"0x{BG_TILE_TABLE_ADDR:06X}",
            "sample_entries": tile_ptrs,
        },
        "dma_parameters": {
            "source_ram_buffer": "0xFF2FA8",
            "bus_source_address": "0x007F97D4",  # 0xFF2FA8 >> 1 in word addressing
            "transfer_length_words": 0x800,       # 2048 words = 4096 bytes (128 tiles)
            "dma_queue_ptr_ram": "0xFF1892",
            "vint_sync_flags_ram": "0xFF164E",
            "dma_request_bit": "bit 1 (0x02)",
        },
        "byte_hash": f"{hash(routine_bytes):016x}",
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {ROUTINE_SIZE} bytes from 0x{START_ADDR:06X} to {OUTPUT_JSON}")


if __name__ == "__main__":
    extract_evidence()
