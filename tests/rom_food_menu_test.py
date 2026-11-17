#!/usr/bin/env python3
"""Unit tests for M15.329 ROM 4x4 Food/Consumable Menu Use & Drop (0x00474E..0x00494A)."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_food_menu import ROM_PATH, extract_evidence


def step_cursor_4x4(cursor: int, dcol: int, drow: int) -> int:
    col = ((cursor & 0x03) + dcol) & 0x03
    row = (((cursor >> 2) & 0x03) + drow) & 0x03
    return (row << 2) | col


def try_drop_food_item(
    inventory: list[tuple[int, int]],
    cursor: int,
    item_flags_table: dict[int, int],
    terrain_blocked: bool,
) -> tuple[bool, int, int]:
    """Reference simulation of 0x004896..0x004904."""
    if terrain_blocked:
        return (False, 0, 0)
    item_id, qty = inventory[cursor]
    if item_id == 0:
        return (False, 0, 0)
    if item_flags_table.get(item_id, 0) & (1 << 5):
        return (False, 0, 0)
    inventory[cursor] = (0, qty)
    return (True, item_id | 0x80, qty)


class TestRomFoodMenu(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["consume_timer_frames"], 90)
        self.assertEqual(ev["constants"]["item_flags_table_rom"], "0x0005CE16")
        saved = json.loads(
            Path("data/thor-evidence/rom_food_menu_m15_329.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_4x4_cursor_and_undroppable_guard(self) -> None:
        self.assertEqual(step_cursor_4x4(0, -1, -1), 15)
        self.assertEqual(step_cursor_4x4(15, 1, 1), 0)
        inv = [(0x10, 1), (0x20, 1), (0, 0)]
        flags = {0x10: 0x20, 0x20: 0x00}  # 0x10 has bit 5 (undroppable)
        self.assertEqual(try_drop_food_item(inv, 0, flags, False), (False, 0, 0))
        self.assertEqual(try_drop_food_item(inv, 1, flags, True), (False, 0, 0))
        self.assertEqual(try_drop_food_item(inv, 1, flags, False), (True, 0xA0, 1))
        self.assertEqual(inv[1][0], 0)


if __name__ == "__main__":
    unittest.main()
