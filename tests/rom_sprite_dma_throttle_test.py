#!/usr/bin/env python3
"""Unit tests for M15.335 ROM Entity Sprite-DMA Budget Throttle & Frame Lookup."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_sprite_dma_throttle import ROM_PATH, extract_evidence


def select_entity_frame_with_dma_budget(
    facing: int,
    anim_frame: int,
    cached_frame: int,
    dma_cost: int,
    dma_budget: int,
) -> tuple[int, bool, bool]:
    """Reference simulation of 0x00A9AE..0x00A9F8.

    Returns (selected_frame, needs_dma, throttled).
    """
    target = (1 if facing == 3 else facing) + anim_frame
    if target == cached_frame:
        return (target, False, False)
    if cached_frame >= 0 and dma_cost > dma_budget:
        return (cached_frame, False, True)
    return (target, True, False)


def should_draw_entity_shadow(z: int, flags_36: int) -> bool:
    """Reference simulation of 0x00AA78..0x00AAA5."""
    if flags_36 & 0x08:
        return False
    if z < 0:
        return False
    if z == 0 and not (flags_36 & 0x04):
        return False
    return True


class TestRomSpriteDmaThrottle(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["dma_budget_ram"], "0x00FF1652")
        self.assertEqual(ev["constants"]["fallback_size_large"], "0x0300")
        saved = json.loads(
            Path("data/thor-evidence/rom_sprite_dma_throttle_m15_335.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_dma_budget_throttle_and_shadow_rule(self) -> None:
        # Cached < 0 -> always updates & requests DMA even if cost > budget
        self.assertEqual(select_entity_frame_with_dma_budget(0, 4, -1, 128, 64), (4, True, False))
        # Cached >= 0 and cost > budget -> throttles to cached frame
        self.assertEqual(select_entity_frame_with_dma_budget(0, 8, 4, 128, 64), (4, False, True))
        # Cached >= 0 and cost <= budget -> updates & requests DMA
        self.assertEqual(select_entity_frame_with_dma_budget(3, 8, 4, 64, 64), (9, True, False))
        # Shadow visibility
        self.assertTrue(should_draw_entity_shadow(10, 0x00))
        self.assertFalse(should_draw_entity_shadow(0, 0x00))
        self.assertTrue(should_draw_entity_shadow(0, 0x04))
        self.assertFalse(should_draw_entity_shadow(10, 0x08))


if __name__ == "__main__":
    unittest.main()
