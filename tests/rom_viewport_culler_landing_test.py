#!/usr/bin/env python3
"""Unit tests for M15.339 ROM Entity Camera-Viewport Culler & Airborne Landing."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_viewport_culler_landing import ROM_PATH, extract_evidence


def is_offscreen(x: int, y: int, z: int, cam_x: int, cam_y: int, margin: int, use_z: bool) -> bool:
    """Reference simulation of 0x00E5AE / 0x00E610 (True = offscreen / carry=1)."""
    sy = y - z if use_z else y
    if x < cam_x - margin or sy < cam_y - margin:
        return True
    if x > cam_x + 320 + margin or sy > cam_y + 224 + margin:
        return True
    return False


class TestRomViewportCullerLanding(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["viewport_width_px"], 320)
        self.assertEqual(ev["constants"]["viewport_height_px"], 224)
        self.assertEqual(ev["constants"]["landing_stun_frames"], 15)
        saved = json.loads(
            Path("data/thor-evidence/rom_viewport_culler_landing_m15_339.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_viewport_culling_with_and_without_z(self) -> None:
        # At (x=100, y=260, z=40), cam=(0, 0), margin=16:
        # ground check: 260 > 240 -> offscreen; with Z: 260-40=220 <= 240 -> onscreen
        self.assertTrue(is_offscreen(100, 260, 40, 0, 0, 16, use_z=False))
        self.assertFalse(is_offscreen(100, 260, 40, 0, 0, 16, use_z=True))
        self.assertFalse(is_offscreen(336, 240, 0, 0, 0, 16, use_z=False))
        self.assertTrue(is_offscreen(337, 240, 0, 0, 0, 16, use_z=False))


if __name__ == "__main__":
    unittest.main()
