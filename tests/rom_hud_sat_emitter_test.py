#!/usr/bin/env python3
"""Unit tests for M15.334 ROM HUD Frame SAT & Item-Toast DMA Emitter."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_hud_sat_emitter import ROM_PATH, extract_evidence


def compute_hud_slide_positions(slide_x: int, item_id: int) -> dict:
    """Reference simulation of HUD mirror & item-toast DMA math in 0x00A38A..0x00A41A."""
    return {
        "primary_x_offset": slide_x,
        "enemy_x_offset": 0x0158 - slide_x,
        "toast_x": slide_x + 0x0068,
        "toast_y": 0x0118,
        "toast_rom_addr": 0x000AAEBB + ((item_id & 0xFF) * 0x90),
    }


class TestRomHudSatEmitter(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["enemy_mirror_base_x"], "0x0158")
        self.assertEqual(ev["constants"]["toast_icon_vram_dst"], "0xB8E0")
        saved = json.loads(
            Path("data/thor-evidence/rom_hud_sat_emitter_m15_334.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_hud_mirror_and_toast_math(self) -> None:
        res = compute_hud_slide_positions(0x20, 0x1E)
        self.assertEqual(res["primary_x_offset"], 0x20)
        self.assertEqual(res["enemy_x_offset"], 0x138)
        self.assertEqual(res["toast_x"], 0x88)
        self.assertEqual(res["toast_y"], 0x118)
        self.assertEqual(res["toast_rom_addr"], 0x000AAEBB + 0x1E * 0x90)


if __name__ == "__main__":
    unittest.main()
