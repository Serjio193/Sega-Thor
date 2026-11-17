#!/usr/bin/env python3
"""Unit tests for M15.376 ROM Ali Crawl Movement, Multi-Surface Footstep SFX & Skid Deceleration."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ali_crawl_skid import ROM_PATH, extract_evidence


def apply_crawl_nudge(is_horizontal_facing: bool, dpad_d0: int) -> tuple[int, int]:
    """Reference simulation of 0x006528..0x006598."""
    if not is_horizontal_facing:
        # Facing Up or Down: horizontal nudge
        dx = 0
        if dpad_d0 & 0x04:  # Left
            dx -= 0x2A000
        if dpad_d0 & 0x08:  # Right
            dx += 0x2A000
        return (dx, 0)
    else:
        # Facing Left or Right: vertical nudge
        dy = 0
        if dpad_d0 & 0x01:  # Up
            dy -= 0x25800
        if dpad_d0 & 0x02:  # Down
            dy += 0x25800
        return (0, dy)


def apply_friction_step(v: int, step: int) -> int:
    """Reference simulation of 0x00665C..0x006688."""
    if v >= 0:
        v -= step
        return max(0, v)
    else:
        v += step
        return min(0, v)


class TestRomAliCrawlSkid(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["crawl_x_nudge"], 0x2A000)
        self.assertEqual(ev["constants"]["crawl_y_nudge"], 0x25800)
        self.assertEqual(ev["constants"]["skid_scrape_sfx"], "0x1B400344")
        self.assertEqual(ev["constants"]["slide_impact_sfx"], "0x00600343")
        self.assertEqual(ev["constants"]["min_skid_run_frames"], 6)
        self.assertEqual(ev["constants"]["skid_decel_step"], 0xACCC)
        self.assertEqual(ev["constants"]["slide_decel_step"], 0x8000)
        saved = json.loads(
            Path("data/thor-evidence/rom_ali_crawl_skid_m15_376.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_crawl_perpendicular_nudge(self) -> None:
        # Vertical facing (Up/Down) -> Left dpad (bit 2) nudges -0x2A000 X
        self.assertEqual(apply_crawl_nudge(is_horizontal_facing=False, dpad_d0=0x04), (-0x2A000, 0))
        # Vertical facing -> Right dpad (bit 3) nudges +0x2A000 X
        self.assertEqual(apply_crawl_nudge(is_horizontal_facing=False, dpad_d0=0x08), (0x2A000, 0))
        # Horizontal facing (Left/Right) -> Up dpad (bit 0) nudges -0x25800 Y
        self.assertEqual(apply_crawl_nudge(is_horizontal_facing=True, dpad_d0=0x01), (0, -0x25800))
        # Horizontal facing -> Down dpad (bit 1) nudges +0x25800 Y
        self.assertEqual(apply_crawl_nudge(is_horizontal_facing=True, dpad_d0=0x02), (0, 0x25800))

    def test_friction_deceleration(self) -> None:
        # Skid friction (0xACCC)
        self.assertEqual(apply_friction_step(0x20000, 0xACCC), 0x20000 - 0xACCC)
        self.assertEqual(apply_friction_step(0x5000, 0xACCC), 0)
        self.assertEqual(apply_friction_step(-0x20000, 0xACCC), -0x20000 + 0xACCC)
        self.assertEqual(apply_friction_step(-0x5000, 0xACCC), 0)

        # Slide friction (0x8000)
        self.assertEqual(apply_friction_step(0x10000, 0x8000), 0x8000)
        self.assertEqual(apply_friction_step(0x8000, 0x8000), 0)
        self.assertEqual(apply_friction_step(-0x10000, 0x8000), -0x8000)


if __name__ == "__main__":
    unittest.main()
