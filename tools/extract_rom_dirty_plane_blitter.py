#!/usr/bin/env python3
"""
extract_rom_dirty_plane_blitter.py — Extract ROM Dynamic Partial Tilemap Plane
VRAM Dirty Block Blitter, Viewport Bounds Clipper & Toroidal Stride Streamer
(0x00DC26..0x00DDA2, 380 bytes).
"""

import json
import os
import sys
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_dirty_plane_blitter_m15_444.json")

START_ADDR = 0x00DC26
END_ADDR = 0x00DDA2


def extract_instructions():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom_data = f.read()

    chunk = rom_data[START_ADDR:END_ADDR]
    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(chunk, START_ADDR):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "routine": "vdp_sync_dirty_plane_rect",
        "description": (
            "ROM Dynamic Partial Tilemap Plane VRAM Dirty Block Blitter, "
            "Viewport Bounds Clipper & Toroidal Stride Streamer"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "vdp_sync_dirty_plane_rect_clipped": {
                "start": "0x00DC26",
                "end": "0x00DD14",
                "notes": "Checks bit 1 of +$27(a3). Clips left/right to [0, 41] and top/bottom to [0, 29]."
            },
            "vdp_sync_dirty_plane_rect_unclipped": {
                "start": "0x00DD14",
                "end": "0x00DDA2",
                "notes": "Direct toroidal sync without screen viewport boundary clipping."
            }
        },
        "registers": {
            "a1": "Source tile buffer pointer",
            "a2": "VDP data port ($C00000)",
            "a3": "Tilemap plane context ($FF1716 Plane A, $FF173E Plane B)",
            "a4": "VDP control port ($C00004)",
            "d1": "Target tile X / VDP command word low",
            "d2": "Target tile Y / VDP command word high",
            "d3": "Width in tiles minus 1 (column count)",
            "d4": "Height in tiles minus 1 (row count)",
            "d5": "VRAM toroidal X column (0..63)",
            "d6": "VRAM toroidal Y row (0..31)",
            "d7": "Source buffer row step in bytes (2 * stride)"
        },
        "vram_layout": {
            "plane_width_tiles": 64,
            "plane_height_tiles": 32,
            "screen_width_tiles": 40,
            "screen_height_tiles": 28,
            "viewport_clip_cols": 41,
            "viewport_clip_rows": 29
        },
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({END_ADDR - START_ADDR} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract_instructions()
