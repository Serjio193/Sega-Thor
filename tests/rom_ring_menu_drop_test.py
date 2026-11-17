#!/usr/bin/env python3
"""Unit tests for M15.328 ROM Ring Menu 4x2 Cursor Wrap & Item Drop Handler."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ring_menu_drop import ROM_PATH, extract_evidence


def step_cursor_4x2(cursor: int, dcol: int, drow: int) -> int:
    col = ((cursor & 0x03) + dcol) & 0x03
    row = (((cursor >> 2) & 0x01) + drow) & 0x01
    return (row << 2) | col


def try_drop_item(
    inventory: list[tuple[int, int]],
    cursor: int,
    protected_slot: int,
    terrain_blocked: bool,
) -> tuple[bool, int, int]:
    """Reference simulation of 0x004674..0x0046E2."""
    if cursor == protected_slot or terrain_blocked:
        return (False, 0, 0)
    item_id, qty = inventory[cursor]
    if item_id <= 1:
        return (False, 0, 0)
    inventory[cursor] = (0, qty)
    return (True, item_id | 0x80, qty)


class TestRomRingMenuDrop(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["cursor_move_sfx"], "0x006003A6")
        self.assertEqual(ev["constants"]["min_droppable_item_id"], 2)
        saved = json.loads(
            Path("data/thor-evidence/rom_ring_menu_drop_m15_328.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_cursor_wrap_and_item_drop(self) -> None:
        self.assertEqual(step_cursor_4x2(0, -1, 0), 3)
        self.assertEqual(step_cursor_4x2(3, 1, 1), 4)
        inv = [(1, 1), (4, 15), (0, 0)]
        self.assertEqual(try_drop_item(inv, 0, 2, False), (False, 0, 0))  # id <= 1
        self.assertEqual(try_drop_item(inv, 1, 1, False), (False, 0, 0))  # protected
        self.assertEqual(try_drop_item(inv, 1, 0, True), (False, 0, 0))   # terrain blocked
        self.assertEqual(try_drop_item(inv, 1, 0, False), (True, 0x84, 15))
        self.assertEqual(inv[1][0], 0)


if __name__ == "__main__":
    unittest.main()
