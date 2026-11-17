#!/usr/bin/env python3
"""Unit tests for M15.406 ROM 2D Bounding Tile Box Sampler, Type 0x56 Pushable Block Scanner & Ground Latch."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_bounding_box_pushable_latch import ROM_PATH, extract_evidence


def compute_tile_box_spans(
    center_x: int,
    center_y: int,
    radius: int,
) -> tuple[int, int, int, int]:
    """Simulate 0x009BF6..0x009C14 tile box span bounds."""
    start_col = (center_x - radius) >> 3
    end_col = (center_x + radius) >> 3
    num_cols = end_col - start_col

    start_row = (center_y - radius) >> 3
    end_row = (center_y + radius) >> 3
    num_rows = end_row - start_row
    return start_col, num_cols, start_row, num_rows


def simulate_pushable_block_collision(
    entity_x: int,
    entity_y: int,
    entity_rad_x: int,
    entity_rad_y: int,
    pool_slots: list[dict],
) -> bool:
    """Simulate 0x009C7C..0x009CFE pushable block 6-slot pool scanner."""
    for slot in pool_slots[:6]:
        if slot.get("type", 0) != 0x56:
            continue
        bx = slot["x"]
        by = slot["y"]
        brad_x = slot["rad_x"]
        brad_y = slot["rad_y"]

        # X overlap check:
        # ent_x_min = entity_x - entity_rad_x
        # block_x_max = bx + brad_x
        # block_x_min = bx - brad_x
        # ent_x_max = entity_x + entity_rad_x
        if (entity_x - entity_rad_x) > (bx + brad_x):
            continue
        if (entity_x + entity_rad_x) < (bx - brad_x):
            continue

        # Y overlap check:
        if (entity_y - entity_rad_y) > (by + brad_y):
            continue
        if (entity_y + entity_rad_y) < (by - brad_y):
            continue

        return True  # Overlapping pushable block found!

    return False


class TestRomBoundingBoxPushableLatch(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.406")
        self.assertEqual(ev["routines"]["pushable_block_scanner"]["target_type"], "0x0056")
        self.assertEqual(ev["routines"]["pushable_block_scanner"]["slot_count"], 6)
        self.assertEqual(ev["routines"]["pushable_block_scanner"]["stride_bytes"], 90)
        saved = json.loads(
            Path("data/thor-evidence/rom_bounding_box_pushable_latch_m15_406.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_bounding_tile_box_intervals(self) -> None:
        # At (100, 100) with radius 8: span is [92..108, 92..108]
        # 92 >> 3 = 11, 108 >> 3 = 13. num_cols = 13 - 11 = 2 (dbra 2 = 3 tiles)
        start_c, num_c, start_r, num_r = compute_tile_box_spans(100, 100, 8)
        self.assertEqual(start_c, 11)
        self.assertEqual(num_c, 2)
        self.assertEqual(start_r, 11)
        self.assertEqual(num_r, 2)

    def test_pushable_block_hit(self) -> None:
        slots = [
            {"type": 0x56, "x": 100, "y": 100, "rad_x": 16, "rad_y": 16},
            {"type": 0x00, "x": 0, "y": 0, "rad_x": 0, "rad_y": 0},
        ]
        # Entity at (110, 105) with radius 8 overlaps block [84..116, 84..116]
        hit = simulate_pushable_block_collision(110, 105, 8, 8, slots)
        self.assertTrue(hit)

    def test_pushable_block_miss(self) -> None:
        slots = [
            {"type": 0x56, "x": 100, "y": 100, "rad_x": 16, "rad_y": 16},
            {"type": 0x40, "x": 150, "y": 150, "rad_x": 16, "rad_y": 16},  # not type 0x56
        ]
        # Entity at (150, 150) with radius 8 misses block at (100, 100)
        hit = simulate_pushable_block_collision(150, 150, 8, 8, slots)
        self.assertFalse(hit)


if __name__ == "__main__":
    unittest.main()
