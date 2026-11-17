#!/usr/bin/env python3
"""Unit tests for M15.318 ROM Room Update Hooks & Warp Transition Sequence."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_room_warp_sequence import ROM_PATH, extract_evidence


def can_trigger_warp(
    f_17b9: int,
    f_1987: int,
    f_1985: int,
    f_17b8: int,
    f_1983: int,
    f_1982: int,
) -> bool:
    """Reference simulation of 0x00C9B2..0x00C9EA."""
    return (
        f_17b9 == 0
        and f_1987 == 0
        and f_1985 == 0
        and (f_17b8 & 0x04) == 0
        and f_1983 == 0
        and f_1982 == 0
    )


def compute_warp_targets(room_id: int, spawn_x: int, spawn_y: int) -> tuple[int, int, int]:
    """Reference simulation of 0x00C9F8..0x00CA46: returns (resolved_room, cam_x, cam_y)."""
    resolved = 0x0611 if room_id == 0x0004 else room_id
    cam_x = (spawn_x - 0x00A0) & 0xFFFF
    cam_y = (spawn_y - 0x0070) & 0xFFFF
    return (resolved, cam_x, cam_y)


class TestRomRoomWarpSequence(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["redirect_src_room"], "0x0004")
        self.assertEqual(ev["constants"]["redirect_dst_room"], "0x0611")
        self.assertEqual(ev["constants"]["camera_half_w"], 160)
        self.assertEqual(ev["constants"]["camera_half_h"], 112)
        saved = json.loads(
            Path("data/thor-evidence/rom_room_warp_sequence_m15_318.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_warp_guard_and_camera_targets(self) -> None:
        self.assertTrue(can_trigger_warp(0, 0, 0, 0x02, 0, 0))
        self.assertFalse(can_trigger_warp(0, 0, 0, 0x04, 0, 0))
        self.assertFalse(can_trigger_warp(1, 0, 0, 0x00, 0, 0))
        self.assertEqual(compute_warp_targets(0x0004, 320, 224), (0x0611, 160, 112))
        self.assertEqual(compute_warp_targets(0x001B, 256, 180), (0x001B, 96, 68))


if __name__ == "__main__":
    unittest.main()
