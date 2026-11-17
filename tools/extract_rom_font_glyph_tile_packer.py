#!/usr/bin/env python3
"""
extract_rom_font_glyph_tile_packer.py — ROM Dynamic 1-Bit Font Glyph
Expansion, 4-Neighbor Outline Filter & 4bpp Tile Packer (0x00C222..0x00C2EC, 202 bytes)
"""

import hashlib
import json
import os
import struct

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C222
END_ADDR = 0x00C2EC
SIZE = END_ADDR - START_ADDR  # 202 bytes

FONT_TABLE_ADDR = 0x0016C1CA
CANVAS_RAM_ADDR = 0x00FF3028
TILES_OUT_RAM = 0x00FF2FA8
GLYPH_RECORD_SIZE = 28  # 14 words
CANVAS_PITCH = 18
CANVAS_ROWS = 18
CANVAS_TOTAL_BYTES = CANVAS_PITCH * CANVAS_ROWS  # 324 bytes
OUTLINE_COLOR = 0x0E

def expand_glyph_to_canvas(glyph_words: list[int]) -> list[int]:
    """Expands 14 glyph words into 18x16 canvas with 0-border."""
    canvas = [0] * CANVAS_TOTAL_BYTES
    # Row 0 is 18 zeroes.
    # Rows 1..14:
    for row_idx, w in enumerate(glyph_words):
        row_offset = (row_idx + 1) * CANVAS_PITCH
        canvas[row_offset] = 0  # leading border
        for bit_idx in range(16):
            bit = (w >> (15 - bit_idx)) & 1
            canvas[row_offset + 1 + bit_idx] = 0xFF if bit else 0x00
        canvas[row_offset + 17] = 0  # trailing border
    # Row 15 is 18 zeroes.
    return canvas

def apply_4neighbor_outline(canvas: list[int], outline_color: int = OUTLINE_COLOR) -> list[int]:
    """Applies 4-neighbor (top, bottom, left, right) outline filter."""
    result = list(canvas)
    pitch = CANVAS_PITCH
    for row in range(1, CANVAS_ROWS - 1):
        for col in range(1, CANVAS_PITCH - 1):
            idx = row * pitch + col
            if canvas[idx] == 0xFF:
                continue  # foreground pixel
            # Check 4 neighbors
            top = canvas[idx - pitch]
            bottom = canvas[idx + pitch]
            left = canvas[idx - 1]
            right = canvas[idx + 1]
            if (top | bottom | left | right) & 0x80:
                result[idx] = outline_color
    return result

def pack_quadrant_8x8(canvas: list[int], start_row: int, start_col: int) -> list[int]:
    """Packs 8x8 pixel quadrant into 32 bytes of 4bpp Genesis planar tile data."""
    tile_bytes = []
    pitch = CANVAS_PITCH
    for r in range(8):
        row_offset = (start_row + r) * pitch + start_col
        for c in range(0, 8, 2):
            p0 = canvas[row_offset + c] & 0x0F
            p1 = canvas[row_offset + c + 1] & 0x0F
            tile_bytes.append((p0 << 4) | p1)
    return tile_bytes

def render_font_glyph(glyph_words: list[int]) -> list[int]:
    """Full pipeline: expand -> outline -> pack 4 tiles (128 bytes)."""
    canvas = expand_glyph_to_canvas(glyph_words)
    outlined = apply_4neighbor_outline(canvas)
    # Tile 0: Top-Left (row 1, col 1)
    t0 = pack_quadrant_8x8(outlined, start_row=1, start_col=1)
    # Tile 1: Bottom-Left (row 9, col 1)
    t1 = pack_quadrant_8x8(outlined, start_row=9, start_col=1)
    # Tile 2: Top-Right (row 1, col 9)
    t2 = pack_quadrant_8x8(outlined, start_row=1, start_col=9)
    # Tile 3: Bottom-Right (row 9, col 9)
    t3 = pack_quadrant_8x8(outlined, start_row=9, start_col=9)
    return t0 + t1 + t2 + t3

def extract():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == SIZE, f"Expected {SIZE} bytes, got {len(routine_bytes)}"

    # Check key opcodes
    assert struct.unpack_from(">H", rom, 0x00C222)[0] == 0x41F9, "lea $16c1ca.l, a0"
    assert struct.unpack_from(">I", rom, 0x00C224)[0] == FONT_TABLE_ADDR
    assert struct.unpack_from(">H", rom, 0x00C228)[0] == 0xC0FC, "mulu.w #$1c, d0"
    assert struct.unpack_from(">H", rom, 0x00C22A)[0] == 0x001C
    assert struct.unpack_from(">I", rom, 0x00C230)[0] == CANVAS_RAM_ADDR
    assert struct.unpack_from(">I", rom, 0x00C27E)[0] == TILES_OUT_RAM
    assert struct.unpack_from(">H", rom, 0x00C24E)[0] == 0x123C, "move.b #$e, d1"
    assert struct.unpack_from(">H", rom, 0x00C250)[0] == 0x000E
    assert struct.unpack_from(">H", rom, 0x00C2A4)[0] == 0x4E75, "rts 1"
    assert struct.unpack_from(">H", rom, 0x00C2C6)[0] == 0x4E75, "rts 2"
    assert struct.unpack_from(">H", rom, 0x00C2D8)[0] == 0x4E75, "rts 3"
    assert struct.unpack_from(">H", rom, 0x00C2EA)[0] == 0x4E75, "rts 4"

    data = {
        "routine": "Dynamic 1-Bit Font Glyph Expansion, 4-Neighbor Outline Filter & 4bpp Tile Packer",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": SIZE,
        "font_table_rom": f"0x{FONT_TABLE_ADDR:08X}",
        "glyph_record_size": GLYPH_RECORD_SIZE,
        "canvas_ram": f"0x{CANVAS_RAM_ADDR:08X}",
        "tiles_out_ram": f"0x{TILES_OUT_RAM:08X}",
        "canvas_pitch": CANVAS_PITCH,
        "canvas_rows": CANVAS_ROWS,
        "outline_color": OUTLINE_COLOR,
        "sha256": hashlib.sha256(routine_bytes).hexdigest(),
    }

    out_path = "data/thor-evidence/rom_font_glyph_tile_packer_m15_427.json"
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"Extracted {SIZE} bytes, saved evidence to {out_path}")

if __name__ == "__main__":
    extract()
