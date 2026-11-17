#!/usr/bin/env python3
"""Unit tests for M15.316 ROM Room & Area Transition Loader (0x00C8AA..0x00C96B)."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_room_area_loader import ROM_PATH, extract_evidence, resolve_room_header


def transition_room(state: dict, new_room_id: int) -> tuple[bool, bool]:
    """Reference simulation of 0x00C8AA..0x00C8EE flag-clearing logic.

    Returns (cleared_room_flags, cleared_area_flags).
    """
    old_room_id = state["current_room_id"]
    cleared_room = False
    cleared_area = False
    if new_room_id != old_room_id:
        state["room_flags"] = [0] * 16
        cleared_room = True
        if ((new_room_id >> 8) & 0xFF) != ((old_room_id >> 8) & 0xFF):
            state["area_flags"] = [0] * 16
            state["area_extra"] = [0] * 3
            cleared_area = True
    state["current_room_id"] = new_room_id
    return (cleared_room, cleared_area)


class TestRomRoomAreaLoader(unittest.TestCase):
    def test_rom_evidence_and_area_table(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["area_table"]["count"], 16)
        self.assertEqual(ev["area_table"]["pointers"][0], "0x0002C6B8")
        self.assertEqual(ev["area_table"]["pointers"][15], "0x0002C6F2")
        ptrs = [int(p, 16) for p in ev["area_table"]["pointers"]]
        hdr = resolve_room_header(rom, ptrs, 0x001B)
        self.assertEqual(f"0x{hdr:08X}", ev["constants"]["new_game_room_header"])
        saved = json.loads(
            Path("data/thor-evidence/rom_room_area_loader_m15_316.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_room_and_area_flag_clearing(self) -> None:
        state = {
            "current_room_id": 0x001B,
            "room_flags": [1] * 16,
            "area_flags": [2] * 16,
            "area_extra": [3] * 3,
        }
        # Same room -> nothing cleared
        self.assertEqual(transition_room(state, 0x001B), (False, False))
        self.assertEqual(state["room_flags"][0], 1)

        # Same area, different room -> only room flags cleared
        self.assertEqual(transition_room(state, 0x001C), (True, False))
        self.assertEqual(state["room_flags"], [0] * 16)
        self.assertEqual(state["area_flags"][0], 2)

        # Different area -> both room and area flags cleared
        state["room_flags"] = [4] * 16
        self.assertEqual(transition_room(state, 0x0100), (True, True))
        self.assertEqual(state["room_flags"], [0] * 16)
        self.assertEqual(state["area_flags"], [0] * 16)
        self.assertEqual(state["area_extra"], [0] * 3)


if __name__ == "__main__":
    unittest.main()
