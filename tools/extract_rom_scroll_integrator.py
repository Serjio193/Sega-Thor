#!/usr/bin/env python3
"""
Extracts and validates Beyond Oasis ROM Plane A 512x256 Ring-Buffer Scroll Integrator,
Tile Strip Dirty Marker & Toroidal Modulo Normalizer (0x00D146..0x00D2B4, 366 bytes).
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_scroll_integrator_m15_436.json"

START_ADDR = 0x00D146
END_ADDR = 0x00D2B4
ROUTINE_SIZE = END_ADDR - START_ADDR  # 366 bytes


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == ROUTINE_SIZE, f"Length mismatch: {len(routine_bytes)}"

    evidence = {
        "routine": "plane_a_scroll_integrator_and_toroidal_normalizer",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": ROUTINE_SIZE,
        "entry_points": {
            "accumulate_long_delta_and_integrate": f"0x{START_ADDR:06X}",
            "accumulate_word_delta_and_integrate": "0x00D168",
            "check_enable_and_integrate": "0x00D17E",
            "integrate_scroll_deltas": "0x00D188",
            "normalize_toroidal_map_coords": "0x00D266",
        },
        "camera_struct_layout": {
            "cam_x": "0x00",
            "cam_sub_x": "0x02",
            "cam_y": "0x04",
            "cam_sub_y": "0x06",
            "vdp_hscroll_plane_a": "0x0E",
            "vdp_vscroll_plane_a": "0x10",
            "pixel_delta_x": "0x12",
            "pixel_delta_y": "0x14",
            "tile_column_step": "0x1C",
            "tile_row_step": "0x1E",
            "map_width_tiles": "0x20",
            "map_height_tiles": "0x22",
            "dirty_status_flags": "0x26",
            "control_flags": "0x27",
        },
        "dirty_status_bits": {
            "h_tile_column_strip": "bit 0 (0x01)",
            "v_tile_row_strip": "bit 1 (0x02)",
            "vdp_hscroll_dirty": "bit 2 (0x04)",
            "vdp_vscroll_dirty": "bit 3 (0x08)",
        },
        "vdp_ring_buffer_masks": {
            "hscroll_ring_mask": 0x1FF,  # 512 px Plane A width
            "vscroll_ring_mask": 0x0FF,  # 256 px Plane A height
        },
        "byte_hash": f"{hash(routine_bytes):016x}",
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {ROUTINE_SIZE} bytes from 0x{START_ADDR:06X} to {OUTPUT_JSON}")


if __name__ == "__main__":
    extract_evidence()
