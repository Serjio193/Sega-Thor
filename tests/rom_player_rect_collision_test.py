#!/usr/bin/env python3
"""
rom_player_rect_collision_test.py — Unit tests for M15.426:
ROM Tile Rect vs Player Ali AABB Containment & Overlap Collision Testers (0x00C17A..0x00C222)
"""

import json
import os
import unittest
from tools.extract_rom_player_rect_collision import (
    unpack_tile_rect,
    player_inside_rect,
    player_overlaps_rect,
    START_ADDR,
    END_ADDR,
    SIZE,
    PLAYER_ADDR,
)

class TestRomPlayerRectCollision(unittest.TestCase):
    def setUp(self):
        self.evidence_path = "data/thor-evidence/rom_player_rect_collision_m15_426.json"

    def test_evidence_json_validity(self):
        self.assertTrue(os.path.exists(self.evidence_path))
        with open(self.evidence_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.assertEqual(data["start_address"], f"0x{START_ADDR:06X}")
        self.assertEqual(data["end_address"], f"0x{END_ADDR:06X}")
        self.assertEqual(data["size_bytes"], SIZE)
        self.assertEqual(data["player_address"], f"0x{PLAYER_ADDR:08X}")
        self.assertEqual(data["entry_points"]["player_inside_rect"], "0x00C17A")
        self.assertEqual(data["entry_points"]["player_overlaps_rect"], "0x00C1CE")

    def test_tile_rect_unpacking(self):
        # Tile min: (10, 20) -> X=10, Y=20
        # Tile max: (15, 30) -> X=15, Y=30
        d1 = (10 << 16) | 20
        d2 = (15 << 16) | 30
        (rx0, rx1), (ry0, ry1) = unpack_tile_rect(d1, d2)
        self.assertEqual(rx0, 10 * 8)
        self.assertEqual(rx1, 15 * 8 + 7)
        self.assertEqual(ry0, 20 * 8)
        self.assertEqual(ry1, 30 * 8 + 7)

    def test_player_inside_rect(self):
        # Rect X: [80, 127], Y: [160, 247]
        d1 = (10 << 16) | 20
        d2 = (15 << 16) | 30

        # Fully contained player at (100, 200), radius 10 -> X:[90, 110], Y:[190, 210]
        self.assertTrue(player_inside_rect(d1, d2, player_x=100, player_y=200, player_rad=10))

        # Partially outside on left -> X:[75, 95]
        self.assertFalse(player_inside_rect(d1, d2, player_x=85, player_y=200, player_rad=10))

        # Partially outside on right -> X:[120, 140]
        self.assertFalse(player_inside_rect(d1, d2, player_x=130, player_y=200, player_rad=10))

        # Partially outside on top -> Y:[155, 175]
        self.assertFalse(player_inside_rect(d1, d2, player_x=100, player_y=165, player_rad=10))

        # Partially outside on bottom -> Y:[240, 260]
        self.assertFalse(player_inside_rect(d1, d2, player_x=100, player_y=250, player_rad=10))

    def test_player_overlaps_rect(self):
        # Rect X: [80, 127], Y: [160, 247]
        d1 = (10 << 16) | 20
        d2 = (15 << 16) | 30

        # Fully inside overlaps
        self.assertTrue(player_overlaps_rect(d1, d2, player_x=100, player_y=200, player_rad=10))

        # Straddling left edge: X:[75, 95]
        self.assertTrue(player_overlaps_rect(d1, d2, player_x=85, player_y=200, player_rad=10))

        # Completely outside to the left: X:[50, 70]
        self.assertFalse(player_overlaps_rect(d1, d2, player_x=60, player_y=200, player_rad=10))

        # Straddling bottom edge: Y:[240, 260] overlaps [160, 247]
        self.assertTrue(player_overlaps_rect(d1, d2, player_x=100, player_y=250, player_rad=10))

        # Completely outside below: Y:[250, 270] misses [160, 247]
        self.assertFalse(player_overlaps_rect(d1, d2, player_x=100, player_y=260, player_rad=10))

    def test_exact_edge_alignment(self):
        d1 = (10 << 16) | 20
        d2 = (15 << 16) | 30
        # Player touching exact left edge: rx0=80. If player_rad=10, player_x=90 -> px0=80
        self.assertTrue(player_inside_rect(d1, d2, player_x=90, player_y=200, player_rad=10))
        # Player 1 pixel to the left: px0=79 -> fails inside test, passes overlap test
        self.assertFalse(player_inside_rect(d1, d2, player_x=89, player_y=200, player_rad=10))
        self.assertTrue(player_overlaps_rect(d1, d2, player_x=89, player_y=200, player_rad=10))

if __name__ == "__main__":
    unittest.main()
