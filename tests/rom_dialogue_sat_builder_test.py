#!/usr/bin/env python3
"""Unit tests for M15.317 ROM Dialogue Cursor & Border SAT Builder (0x00C756..0x00C84C)."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_dialogue_sat_builder import ROM_PATH, extract_evidence


def build_dialogue_sat_entries(
    sat_count: int,
    cursor_enabled: bool,
    cursor_alt_palette: bool,
    blink_counter: int,
    choice_row: int,
    bar_tiles: tuple[int, int],
    bar_ys: tuple[int, int, int, int],
) -> list[tuple[int, int, int, int]]:
    """Reference simulation of 0x00C756..0x00C84C returning (y, size_link, attr, x) tuples."""
    out: list[tuple[int, int, int, int]] = []

    def append(y: int, size_link: int, attr: int, x: int) -> None:
        nonlocal sat_count
        if sat_count < 0x50:
            sat_count += 1
            out.append((y, (size_link + sat_count) & 0xFFFF, attr, x))

    if cursor_enabled and (blink_counter & 0x10):
        attr = 0x86DC | (0x1000 if cursor_alt_palette else 0)
        y = 0x014C
        for threshold in (3, 2, 1):
            if choice_row < threshold:
                y -= 0x0C
        append(y, 0x0000, attr, 0x011C)

    for idx, y_raw in enumerate(bar_ys):
        tile = ((bar_tiles[idx // 2] >> 5) | 0x8000) & 0xFFFF
        x = 0x00A0
        y = (y_raw + 0x0110) & 0xFFFF
        for _ in range(8):
            append(y, 0x0C00, tile, x)
            x += 0x20
            tile = (tile + 4) & 0xFFFF

    return out


class TestRomDialogueSatBuilder(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["sat_max_sprites"], 80)
        self.assertEqual(ev["constants"]["cursor_tile_base"], "0x86DC")
        saved = json.loads(
            Path("data/thor-evidence/rom_dialogue_sat_builder_m15_317.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_sat_entries_cursor_and_bars(self) -> None:
        entries = build_dialogue_sat_entries(
            sat_count=10,
            cursor_enabled=True,
            cursor_alt_palette=True,
            blink_counter=0x10,
            choice_row=1,
            bar_tiles=(0x0400, 0x0800),
            bar_ys=(0x10, 0x20, 0x30, 0x40),
        )
        # 1 cursor + 4 * 8 = 33 sprites
        self.assertEqual(len(entries), 33)
        # Cursor at choice_row=1 -> 0x14C - 2*0x0C = 0x134, link=11, attr=0x96DC, x=0x11C
        self.assertEqual(entries[0], (0x0134, 11, 0x96DC, 0x011C))
        # First bar sprite: y=0x120, size_link=0x0C0C, attr=0x8020, x=0xA0
        self.assertEqual(entries[1], (0x0120, 0x0C0C, 0x8020, 0x00A0))
        # Second sprite in first bar: tile+4, x+0x20
        self.assertEqual(entries[2], (0x0120, 0x0C0D, 0x8024, 0x00C0))


if __name__ == "__main__":
    unittest.main()
