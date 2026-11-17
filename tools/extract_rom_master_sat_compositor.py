#!/usr/bin/env python3
"""
Extracts ROM evidence for M15.415: Master Entity Sprite Attribute Table (SAT)
Compositor, Viewport Clipping & Dynamic Shadow Blitter Subsystem
ROM Range: 0x00A9A0..0x00AD76 (982 bytes).
"""

import hashlib
import json
import os
import struct

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_JSON = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_master_sat_compositor_m15_415.json")

START_ADDR = 0x00A9A0
END_ADDR = 0x00AD76
BYTE_COUNT = END_ADDR - START_ADDR  # 982 bytes

def u8(data, off):
    return data[off]

def u16(data, off):
    return struct.unpack_from(">H", data, off)[0]

def s16(data, off):
    return struct.unpack_from(">h", data, off)[0]

def u32(data, off):
    return struct.unpack_from(">I", data, off)[0]

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    assert len(rom) >= END_ADDR, "ROM too small"

    block = rom[START_ADDR:END_ADDR]
    sha256 = hashlib.sha256(block).hexdigest()

    # Verify key entry point opcodes
    # 0x00A9A0: btst.b #1, $36(a6) -> 0x082E, 0x0001, 0x0036
    assert u16(rom, 0x00A9A0) == 0x082E
    assert u16(rom, 0x00A9A2) == 0x0001
    assert u16(rom, 0x00A9A4) == 0x0036
    # 0x00A9A6: bne.w $acfc -> 0x6600, 0x0354
    assert u16(rom, 0x00A9A6) == 0x6600
    assert u16(rom, 0x00A9A8) == 0x0354

    # 0x00A9AA: movem.l d7/a6, -(a7) -> 0x48E7, 0x0102
    assert u16(rom, 0x00A9AA) == 0x48E7
    assert u16(rom, 0x00A9AC) == 0x0102

    # 0x00A9B6: cmpi.b #3, d0 -> 0x0C00, 0x0003
    assert u16(rom, 0x00A9B6) == 0x0C00
    assert u16(rom, 0x00A9B8) == 0x0003

    # 0x00ACF8: movem.l (a7)+, d7/a6 -> 0x4CDF, 0x4080
    assert u16(rom, 0x00ACF8) == 0x4CDF
    assert u16(rom, 0x00ACFA) == 0x4080
    # 0x00ACFC: rts -> 0x4E75
    assert u16(rom, 0x00ACFC) == 0x4E75

    # 0x00AD1E: suba.l #$ff13cc, a1 -> 0x93FC, 0x00FF, 0x13CC
    assert u16(rom, 0x00AD1E) == 0x93FC
    assert u32(rom, 0x00AD20) == 0x00FF13CC
    # 0x00AD3A: move.w #$ffff, $34(a6) -> 0x3D7C, 0xFFFF, 0x0034
    assert u16(rom, 0x00AD3A) == 0x3D7C
    assert u16(rom, 0x00AD3C) == 0xFFFF
    assert u16(rom, 0x00AD3E) == 0x0034
    # 0x00AD40: rts -> 0x4E75
    assert u16(rom, 0x00AD40) == 0x4E75

    # Extract Shadow Table at 0x00ACFE (4 entries of 8 bytes)
    shadow_entries = []
    for i in range(4):
        off = 0x00ACFE + i * 8
        shadow_entries.append({
            "index": i,
            "rom_offset": f"0x{off:06X}",
            "y_offset": u16(rom, off),
            "sprite_size": f"0x{u16(rom, off + 2):04X}",
            "tile_index": f"0x{u16(rom, off + 4):04X}",
            "x_offset": u16(rom, off + 6)
        })

    # Extract 4x4 Sprite Tile Count Table at 0x00AD56 (16 words)
    tile_count_table = []
    for w in range(1, 5):
        row = []
        for h in range(1, 5):
            idx = (w - 1) * 4 + (h - 1)
            off = 0x00AD56 + idx * 2
            cnt = u16(rom, off)
            assert cnt == w * h, f"Mismatch at {w}x{h}: expected {w*h}, got {cnt}"
            row.append(cnt)
        tile_count_table.append(row)

    evidence = {
        "subsystem": "ROM Master Entity SAT Sprite Compositor, Screen Clipping & Dynamic Shadow Blitter",
        "milestone": "M15.415",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": BYTE_COUNT,
            "sha256": sha256
        },
        "embedded_tables": {
            "shadow_table": {
                "address": "0x00ACFE",
                "entries": shadow_entries
            },
            "tile_count_table_4x4": {
                "address": "0x00AD56",
                "grid_w_by_h": tile_count_table
            }
        },
        "registers_and_offsets": {
            "entity": {
                "frame_mode": "0x06",
                "world_x": "0x08",
                "world_y": "0x0C",
                "elevation_z": "0x10",
                "status_code": "0x16",
                "screen_mode_flags": "0x17",
                "vram_tile_base": "0x18",
                "frame_table_ptr": "0x1A",
                "graphic_src_ptr": "0x1E",
                "frame_counter": "0x32",
                "cached_frame": "0x34",
                "flags": "0x36",
                "dynamic_render_mode": "0x39",
                "shadow_preset": "0x3D",
                "flash_palette_counter": "0x3E",
                "priority_depth": "0x4C",
                "secondary_anim_ptr": "0x5E"
            },
            "global_ram": {
                "cache_invalidate_flag": "0xFF1651",
                "remaining_tile_quota": "0xFF1652",
                "camera_active_flag": "0xFF16F0",
                "camera_plane_a": "0xFF1716",
                "camera_plane_b": "0xFF173E",
                "sat_buffer_base": "0xFF13CC",
                "sat_buffer_offset": "0xFF188C",
                "sat_sprite_count": "0xFF188A",
                "dma_queue_ptr": "0xFF1892",
                "shadow_suppress_flag": "0xFF1927",
                "cached_secondary_header": "0xFF1946",
                "subpixel_jitter": "0xFF1971"
            }
        }
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.415 evidence: {BYTE_COUNT} bytes, SHA256: {sha256}")

if __name__ == "__main__":
    extract()
