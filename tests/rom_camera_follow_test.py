#!/usr/bin/env python3
"""Unit tests for M15.319 ROM Camera Deadzone Follow, Screen Shake & HUD Slide Trigger."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_camera_follow import ROM_PATH, extract_evidence


def step_camera_follow(state: dict) -> None:
    """Reference simulation of 0x00CEEA..0x00D114."""
    px = state["player_x"]
    py = state["player_y"] - (0 if state["ignore_z"] else state["player_z"])
    cam_x = state["cam_x"]
    cam_y = state["cam_y"]
    snap = state["instant_snap"]
    state["instant_snap"] = False

    if state["locked_center_mode"]:
        cam_x = (px + state["lock_off_x"] - 0xA0)
        cam_y = (py + state["lock_off_y"] - 0x78)
    else:
        rel_x = px - cam_x
        if not state["hud_slide_rev"]:
            if rel_x <= 0x50:
                state["hud_slide_rev"] = True
                if snap:
                    state["hud_slide_offset"] = 0x138
        else:
            if rel_x >= 0xF0:
                state["hud_slide_rev"] = False
                if snap:
                    state["hud_slide_offset"] = 0x20

        old_cam_x = cam_x
        if rel_x < 0x78:
            d = rel_x - 0x78
            if not snap and d < -3:
                d = -3 if d >= -0x20 else d + 0x1E
            cam_x += d
        cam_x = max(0, cam_x)
        if rel_x > 0xC8:
            d = rel_x - 0xC8
            if not snap and d > 3:
                d = 3 if d <= 0x20 else d - 0x1E
            cam_x += d
        cam_x = min(state["cam_max_x"], cam_x)

        rel_y = py - cam_y
        if rel_y < 0x68:
            d = rel_y - 0x68
            if not snap and d < -3:
                if d >= -0x20:
                    d = -3
                else:
                    d = max(-6, d + 0x1E)
            cam_y += d
        cam_y = max(0, cam_y)
        if rel_y > 0x80:
            d = rel_y - 0x80
            if not snap and d > 3:
                d = 3 if d <= 0x20 else d - 0x1E
            cam_y += d
        cam_y = min(state["cam_max_y"], cam_y)

        if not snap:
            if cam_x < old_cam_x:
                state["hud_slide_rev"] = True
            elif cam_x > old_cam_x:
                state["hud_slide_rev"] = False

    if state["shake_timer"] > 0:
        state["shake_timer"] -= 1
        t = state["shake_timer"]
        cam_y += t if (t & 1) else -t
        cam_y = max(0, min(state["cam_max_y"], cam_y))

    state["cam_x"] = cam_x
    state["cam_y"] = cam_y


class TestRomCameraFollow(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["deadzone_x_min"], 0x78)
        self.assertEqual(ev["constants"]["deadzone_x_max"], 0xC8)
        saved = json.loads(
            Path("data/thor-evidence/rom_camera_follow_m15_319.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_deadzone_smooth_follow_hud_trigger_and_shake(self) -> None:
        state = {
            "player_x": 220,
            "player_y": 140,
            "player_z": 0,
            "ignore_z": False,
            "cam_x": 0,
            "cam_y": 0,
            "cam_max_x": 640,
            "cam_max_y": 480,
            "instant_snap": False,
            "locked_center_mode": False,
            "lock_off_x": 0,
            "lock_off_y": 0,
            "hud_slide_rev": True,
            "hud_slide_offset": 0x138,
            "shake_timer": 3,
        }
        step_camera_follow(state)
        # rel_x=220 > 200 -> d=20 <= 32 -> +3 -> cam_x=3, hud_slide_rev becomes False
        self.assertEqual(state["cam_x"], 3)
        self.assertFalse(state["hud_slide_rev"])
        # rel_y=140 > 128 -> d=12 <= 32 -> +3, plus shake_timer=2 (even -> -2) -> cam_y=1
        self.assertEqual(state["cam_y"], 1)
        self.assertEqual(state["shake_timer"], 2)


if __name__ == "__main__":
    unittest.main()
