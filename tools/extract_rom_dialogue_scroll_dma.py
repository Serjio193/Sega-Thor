#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Dialogue Window 4-Line Circular Scroll Engine,
VDP Direct Clearer & Glyph DMA Streamer routines (0x00C566..0x00C756, 496 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_dialogue_scroll_dma_m15_429.json"

START_ADDR = 0x00C566
END_ADDR = 0x00C756
ROUTINE_SIZE = END_ADDR - START_ADDR  # 496 bytes


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    evidence = {
        "routine": "dialogue_scroll_vdp_and_glyph_dma_streamer",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "dialogue_advance_line_and_scroll": f"0x{START_ADDR:06X}",
            "dialogue_vdp_clear_line": "0x00C66A",
            "dialogue_render_glyph_dma": "0x00C6BE",
            "dialogue_queue_blank_dma": "0x00C734",
        },
        "line_offsets_init": [12, 24, 36, 48],
        "line_recycle_val": 48,  # 0x30
        "step_decrement": 2,
        "line_tile_row_stride": 0x400,
        "glyph_tile_size_bytes": 32,
        "dma_queue_entry_size_bytes": 8,  # 4B src, 2B dest, 2B len_words
        "blank_dma_words": 0x200,  # 512 words = 1024 bytes
        "blank_rom_source": "0x007F9DD4",
        "memory_map": {
            "FF1872": "plane_vram_top_base",
            "FF1874": "plane_vram_bottom_base",
            "FF1876": "line1_row_offset",
            "FF1878": "line2_row_offset",
            "FF187A": "line3_row_offset",
            "FF187C": "line4_row_offset",
            "FF187E": "cursor_tile_x",
            "FF1880": "line_counter",
            "FF1892": "dma_queue_pointer",
            "FF164E": "vint_flags",
            "FF2FA8": "glyph_tile_buffer_base",
        },
        "code_hex": routine_bytes.hex(),
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted evidence: {ROUTINE_SIZE} bytes -> {OUTPUT_JSON}")
    return evidence


if __name__ == "__main__":
    extract_evidence()
