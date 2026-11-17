#!/usr/bin/env python3
"""Unit tests for M15.313 ROM Incremental Row/Col VDP Plane Scroller (0x0023CC..0x002630)."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_vdp_plane_scroll import ROM_PATH, extract_evidence


def stream_column(state: dict, tilemap: list[int], nametable: list[int]) -> None:
    """Reference simulation of 0x0023CC..0x002510."""
    w = state["map_width_tiles"]
    h = state["map_height_tiles"]
    row_shift = state["row_shift"]
    d6 = state["pending_dx_tiles"]
    vram_off = state["vram_offset"]

    if d6 >= 0:
        state["pending_dx_tiles"] = d6 - 1
        col = state["src_col"] + 1
        if col >= w:
            col -= w
        state["src_col"] = col
        read_col = col + 0x28
        while read_col >= w:
            read_col -= w
        col_byte = (vram_off & 0x7E) + 2
        col_byte &= 0x7E
        row_byte = vram_off & 0xF80
        state["vram_offset"] = col_byte | row_byte
        write_off = ((col_byte + 0x50) & 0x7E) | row_byte
    else:
        state["pending_dx_tiles"] = d6 + 1
        col = state["src_col"] - 1
        if col < 0:
            col += w
        state["src_col"] = col
        read_col = col
        col_byte = ((vram_off & 0x7E) - 2) & 0x7E
        row_byte = vram_off & 0xF80
        write_off = col_byte | row_byte
        state["vram_offset"] = write_off

    src_row = state["src_row"]
    src_idx = ((src_row << (row_shift - 1)) + read_col)
    vdp_row = (write_off & 0xF80) >> 7
    vram_cursor = write_off
    cur_row = src_row
    h_guard = h

    for _ in range(29):
        nametable[vram_cursor >> 1] = tilemap[src_idx]
        src_idx += w
        cur_row += 1
        if cur_row >= h_guard:
            src_idx -= h << (row_shift - 1)
            h_guard = 0x3E8
        vram_cursor += 0x80
        vdp_row += 1
        if vdp_row >= 0x20:
            vdp_row -= 0x20
            vram_cursor -= 0x1000


def stream_row(state: dict, tilemap: list[int], nametable: list[int]) -> None:
    """Reference simulation of 0x002512..0x002630."""
    w = state["map_width_tiles"]
    h = state["map_height_tiles"]
    row_shift = state["row_shift"]
    d6 = state["pending_dy_tiles"]
    vram_off = state["vram_offset"]

    if d6 >= 0:
        state["pending_dy_tiles"] = d6 - 1
        row = state["src_row"] + 1
        if row >= h:
            row -= h
        state["src_row"] = row
        read_row = row + 0x1C
        if read_row >= h:
            read_row -= h
        col_byte = vram_off & 0x7E
        row_byte = ((vram_off & 0xF80) + 0x80) & 0xF80
        state["vram_offset"] = row_byte | col_byte
        write_off = (((row_byte + 0xE00) & 0xF80) | col_byte)
    else:
        state["pending_dy_tiles"] = d6 + 1
        row = state["src_row"] - 1
        if row < 0:
            row += h
        state["src_row"] = row
        read_row = row
        col_byte = vram_off & 0x7E
        row_byte = ((vram_off & 0xF80) - 0x80) & 0xF80
        write_off = row_byte | col_byte
        state["vram_offset"] = write_off

    src_col = state["src_col"]
    src_idx = (read_row << (row_shift - 1)) + src_col
    vdp_col = (write_off & 0x7E) >> 1
    vram_cursor = write_off
    cur_col = src_col

    for _ in range(41):
        nametable[vram_cursor >> 1] = tilemap[src_idx]
        src_idx += 1
        cur_col += 1
        if cur_col >= w:
            cur_col -= w
            src_idx -= w
        vram_cursor += 2
        vdp_col += 1
        if vdp_col >= 0x40:
            vdp_col -= 0x40
            vram_cursor -= 0x80


class TestRomVdpPlaneScroll(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["routines"]["stream_column"]["tiles_per_column"], 29)
        self.assertEqual(ev["routines"]["stream_row"]["tiles_per_row"], 41)
        self.assertEqual(ev["routines"]["stream_column"]["vdp_auto_inc_setup"], "0x8F80")
        self.assertEqual(ev["routines"]["stream_column"]["vdp_auto_inc_restore"], "0x8F02")
        saved = json.loads(Path("data/thor-evidence/rom_vdp_plane_scroll_m15_313.json").read_text(encoding="utf-8"))
        self.assertEqual(ev, saved)

    def test_column_and_row_streaming_wrap(self) -> None:
        w, h, row_shift = 64, 32, 7  # 64 words/row = 128 bytes/row (1 << 7)
        tilemap = [(r << 8) | c for r in range(h) for c in range(w)]
        nametable = [0] * (64 * 32)
        state = {
            "src_col": 0,
            "src_row": 20,
            "vram_offset": 0x0A00,  # row 20, col 0
            "pending_dx_tiles": 2,
            "pending_dy_tiles": 1,
            "map_width_tiles": w,
            "map_height_tiles": h,
            "row_shift": row_shift,
        }
        stream_column(state, tilemap, nametable)
        self.assertEqual(state["pending_dx_tiles"], 1)
        self.assertEqual(state["src_col"], 1)
        self.assertEqual(state["vram_offset"], 0x0A02)
        # Written at VDP col 41 (0x52 >> 1), row 20..48 wrapped mod 32, reading map col 41
        self.assertEqual(nametable[(20 * 64) + 41], (20 << 8) | 41)
        self.assertEqual(nametable[(0 * 64) + 41], (0 << 8) | 41)

        stream_row(state, tilemap, nametable)
        self.assertEqual(state["pending_dy_tiles"], 0)
        self.assertEqual(state["src_row"], 21)
        self.assertEqual(state["vram_offset"], 0x0A82)
        # Written at VDP row (21 + 28) % 32 = 17, cols 1..41
        self.assertEqual(nametable[(17 * 64) + 1], (17 << 8) | 1)
        self.assertEqual(nametable[(17 * 64) + 41], (17 << 8) | 41)


if __name__ == "__main__":
    unittest.main()
