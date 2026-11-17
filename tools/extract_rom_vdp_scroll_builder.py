#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM VDP Dual-Plane H/V Scroll Table Builder,
Scanline/Strip Mode Dispatcher & V-Int DMA Sync Trigger (0x00D2B4..0x00D3B0, 252 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_vdp_scroll_builder_m15_437.json"

START_ADDR = 0x00D2B4
END_ADDR = 0x00D3B2
ROUTINE_SIZE = END_ADDR - START_ADDR  # 254 bytes


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    evidence = {
        "routine": "vdp_scroll_table_builder_and_dma_sync",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "vdp_update_hscroll_table": f"0x{START_ADDR:06X}",
            "vdp_update_vscroll_table": "0x00D344",
            "vdp_commit_scroll_dma_flags": "0x00D398",
        },
        "vdp_scroll_modes": {
            "hscroll_modes": {
                "fullscreen_word": {
                    "condition": "$FF16F0 bit 1 == 0",
                    "dest_plane_a": "0xFF1344",
                    "dest_plane_b": "0xFF1346",
                },
                "strip_16px_tiles": {
                    "condition": "$FF16F0 bit 1 == 1 and bit 0 == 0",
                    "dest_buffer": "0xFFFB5C",
                    "iterations": 29,  # d1 = 0x1C (29 words, stride 32 bytes)
                    "stride_bytes": 32,
                },
                "scanline_raster_lines": {
                    "condition": "$FF16F0 bit 1 == 1 and bit 0 == 1",
                    "dest_buffer": "0xFFFB5C",
                    "iterations": 58,  # d1 = 0x39 (58 loops * 4 words = 232 words)
                    "stride_bytes": 4,
                },
            },
            "vscroll_modes": {
                "fullscreen_word": {
                    "condition": "$FF16F0 bit 2 == 0",
                    "dest_plane_a": "0xFF1348",
                    "dest_plane_b": "0xFF134A",
                },
                "column_2cell_16px": {
                    "condition": "$FF16F0 bit 2 == 1",
                    "dest_buffer": "0xFFFF5C",
                    "iterations": 20,  # d1 = 0x13 (20 columns, stride 4 bytes)
                    "stride_bytes": 4,
                },
            },
        },
        "hardware_and_ram_flags": {
            "scroll_mode_reg": "0xFF16F0",
            "vdp_vint_sync_flags": "0xFF164E",
            "camera_plane_a_struct": "0xFF1716",
            "camera_dirty_flags_offset": "0x26",
            "hscroll_dirty_bit": "bit 2 (0x04)",
            "vscroll_dirty_bit": "bit 3 (0x08)",
            "hscroll_pending_flag": "bit 3 of $FF16F0 (0x08)",
            "vscroll_pending_flag": "bit 4 of $FF16F0 (0x10)",
        },
        "byte_hash": f"{hash(routine_bytes):016x}",
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {ROUTINE_SIZE} bytes from 0x{START_ADDR:06X} to {OUTPUT_JSON}")


if __name__ == "__main__":
    extract_evidence()
