#!/usr/bin/env python3
"""Unit tests for M15.333 ROM Master Y-Sorted SAT Render Pipeline."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ysort_sat_pipeline import ROM_PATH, extract_evidence


def y_sort_entity_indices_desc(entities: list[tuple[int, int]]) -> list[int]:
    """Reference simulation of 0x00A1D6..0x00A246 (active=state!=0, y=int16).

    Mega Drive SAT draws earlier entries in front, so largest Y is emitted first.
    """
    remaining = [i for i, (state, _) in enumerate(entities) if state != 0]
    order: list[int] = []
    while remaining:
        best_pos = -1
        best_y = -1  # 0x00A200: moveq #-1, d5
        for pos, idx in enumerate(remaining):
            y = entities[idx][1]
            if best_y < y:
                best_y = y
                best_pos = pos
        if best_pos < 0:
            break
        order.append(remaining.pop(best_pos))
    return order


class TestRomYsortSatPipeline(unittest.TestCase):
    def test_rom_evidence_matches_passes(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(len(ev["render_passes"]), 5)
        self.assertEqual(ev["render_passes"][2]["count"], 21)
        saved = json.loads(
            Path("data/thor-evidence/rom_ysort_sat_pipeline_m15_333.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_descending_y_sort_skips_inactive_and_neg_y(self) -> None:
        entities = [
            (0, 300),   # inactive
            (1, 100),   # active y=100
            (1, 240),   # active y=240
            (1, 180),   # active y=180
            (1, -5),    # negative Y (< -1 sentinel) skipped by 0x00A20A
        ]
        self.assertEqual(y_sort_entity_indices_desc(entities), [2, 3, 1])


if __name__ == "__main__":
    unittest.main()
