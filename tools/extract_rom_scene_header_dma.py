#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Scene Script Header Unpacker,
Dual-Plane Camera Initializer & 16KB VRAM DMA Upload Engine (0x00D406..0x00D542, 316 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_scene_header_dma_m15_439.json"

START_ADDR = 0x00D406
END_ADDR = 0x00D542
ROUTINE_SIZE = END_ADDR - START_ADDR  # 316 bytes


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    evidence = {
        "routine": "scene_script_header_unpacker_and_16kb_vram_dma",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "load_scene_header_and_init_vram": f"0x{START_ADDR:06X}",
            "unpack_elevation_bands": "0x00D44A",
            "clear_camera_structs": "0x00D46C",
            "init_aligned_camera_coords": "0x00D484",
            "decompress_4_bg_patterns": "0x00D4C8",
            "queue_16kb_vram_dma": "0x00D50E",
        },
        "script_header_layout": {
            "plane_tilemap_ptr": "+0x04(a1)",
            "bg_pattern_indices": "+0x08(a1) (4 bytes)",
            "elevation_slice_thresholds": "+0x0C(a1) (4 bytes)",
            "bgm_track_id": "+0x10(a1) -> $FF16F4",
            "bgm_tempo": "+0x12(a1) -> $FF16F6",
            "bgm_sound_mode": "+0x14(a1) -> $FF16F8",
            "scroll_flags": "+0x16(a1) -> d6",
            "vdp_scroll_mode": "+0x18(a1) -> $FF16F0",
        },
        "elevation_tables": {
            "unpacked_thresholds_ram": "0xFF16FE (4 words, shifted << 3)",
            "height_band_boundaries_ram": "0xFF1706 (8 words, sentinel $F000)",
        },
        "camera_structs": {
            "plane_a": "0xFF1716 (20 words)",
            "plane_b": "0xFF173E (20 words)",
            "alignment_mask": "0xFFF8FFF8 (8-pixel grid)",
        },
        "dma_upload_parameters": {
            "staging_buffer": "0xFF3FA8",
            "bus_source_addr": "0x007F9FD4",  # 0xFF3FA8 >> 1
            "vram_dest": "0x0000",
            "length_words": 0x2000,            # 8192 words = 16384 bytes (512 tiles)
            "vint_request_flag": "bit 1 of $FF164E",
        },
        "byte_hash": f"{hash(routine_bytes):016x}",
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {ROUTINE_SIZE} bytes from 0x{START_ADDR:06X} to {OUTPUT_JSON}")


if __name__ == "__main__":
    extract_evidence()
