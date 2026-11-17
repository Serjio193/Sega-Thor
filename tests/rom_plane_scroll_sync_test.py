#!/usr/bin/env python3
"""Unit tests for M15.320 ROM Plane Scroll Delta & VDP H/V-Scroll Buffer Sync."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_plane_scroll_sync import ROM_PATH, extract_evidence


def update_plane_scroll_deltas(state: dict, target_x: int, target_y: int) -> None:
    """Reference simulation of 0x00D188..0x00D264."""
    dirty = 0
    state["dx_px"] = 0
    state["dy_px"] = 0

    old_x = state["cam_x"]
    if target_x != old_x:
        dx = target_x - old_x
        state["cam_x"] = target_x
        state["dx_px"] = dx
        if not state["disable_tile_stream"]:
            tile_dx = (target_x >> 3) - (old_x >> 3)
            if tile_dx != 0:
                state["pending_dx_tiles"] = tile_dx
                dirty |= 1 << 0
        state["vdp_hscroll"] = (state["vdp_hscroll"] - dx) & 0x1FF
        dirty |= 1 << 2
        px_w = state["map_width_tiles"] << 3
        if state["cam_x"] <= -10000:
            state["cam_x"] += 2 * px_w
        if state["cam_x"] >= 10000:
            state["cam_x"] -= 2 * px_w

    old_y = state["cam_y"]
    if target_y != old_y:
        dy = target_y - old_y
        state["cam_y"] = target_y
        state["dy_px"] = dy
        if not state["disable_tile_stream"]:
            tile_dy = (target_y >> 3) - (old_y >> 3)
            if tile_dy != 0:
                state["pending_dy_tiles"] = tile_dy
                dirty |= 1 << 1
        state["vdp_vscroll"] = (state["vdp_vscroll"] + dy) & 0xFF
        dirty |= 1 << 3
        px_h = state["map_height_tiles"] << 3
        if state["cam_y"] <= -10000:
            state["cam_y"] += 2 * px_h
        if state["cam_y"] >= 10000:
            state["cam_y"] -= 2 * px_h

    state["dirty_flags"] = dirty


class TestRomPlaneScrollSync(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["hscroll_mask"], "0x01FF")
        self.assertEqual(ev["constants"]["vscroll_mask"], "0x00FF")
        saved = json.loads(
            Path("data/thor-evidence/rom_plane_scroll_sync_m15_320.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_scroll_deltas_tile_crossing_and_hv_masks(self) -> None:
        state = {
            "cam_x": 6,
            "cam_y": 14,
            "dx_px": 0,
            "dy_px": 0,
            "vdp_hscroll": 0,
            "vdp_vscroll": 0,
            "pending_dx_tiles": 0,
            "pending_dy_tiles": 0,
            "map_width_tiles": 64,
            "map_height_tiles": 32,
            "disable_tile_stream": False,
            "dirty_flags": 0,
        }
        # Cross 8px boundary on X (6 -> 10) and Y (14 -> 18)
        update_plane_scroll_deltas(state, 10, 18)
        self.assertEqual(state["dx_px"], 4)
        self.assertEqual(state["dy_px"], 4)
        self.assertEqual(state["pending_dx_tiles"], 1)
        self.assertEqual(state["pending_dy_tiles"], 1)
        self.assertEqual(state["vdp_hscroll"], (-4) & 0x1FF)
        self.assertEqual(state["vdp_vscroll"], 4)
        self.assertEqual(state["dirty_flags"], 0x0F)


if __name__ == "__main__":
    unittest.main()
