#!/usr/bin/env python3
"""Unit tests for M15.398 ROM Universal Entity Simulation Machine, Animation Frame Stepper & Terrain Pre-flight."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_entity_sim_anim_preflight import ROM_PATH, extract_evidence


def simulate_freeze_slowmo(
    entity_type: int,
    vel_x: int,
    vel_y: int,
    freeze_active: bool,
    slowmo_active: bool,
) -> tuple[int, int, bool]:
    """Reference simulation of 0x008F4C..0x008FA4 returning (vx, vy, frozen)."""
    if freeze_active and entity_type > 2:
        return 0, 0, True
    vx, vy = vel_x, vel_y
    if slowmo_active and entity_type > 2:
        vx >>= 1
        vy >>= 1
    return vx, vy, False


def simulate_anim_step(
    cur_state: int,
    prev_state: int,
    frame_idx: int,
    keyframe_timer: int,
    anim_frames: list[tuple[int, int]],  # (sprite_id, duration_and_loop)
) -> tuple[int, int, int, int]:
    """Reference simulation of 0x008FA4..0x00903E returning (new_prev_state, new_frame_idx, new_timer, sprite_id)."""
    if cur_state != prev_state:
        frame_idx = -1
        keyframe_timer = 0
        prev_state = cur_state

    keyframe_timer -= 1
    sprite_id = 0
    if keyframe_timer <= 0:
        frame_idx += 1
        if frame_idx >= len(anim_frames):
            frame_idx = 0
        sprite_id, raw_dur = anim_frames[frame_idx]
        loop_bit = bool(raw_dur & 0x8000)
        dur = raw_dur & 0x7FFF
        keyframe_timer = max(1, (dur >> 1) - 1)
    return prev_state, frame_idx, keyframe_timer, sprite_id


def simulate_preflight_layer(layer_select_bit7: bool) -> str:
    """Reference simulation of 0x009052..0x009062."""
    if layer_select_bit7:
        return "0x00FF1770"  # Layer 2
    return "0x00FF1766"  # Layer 1


class TestRomEntitySimAnimPreflight(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.398")
        self.assertEqual(ev["routines"]["terrain_collision_preflight"]["right_move_code"], "0x11")
        saved = json.loads(
            Path("data/thor-evidence/rom_entity_sim_anim_preflight_m15_398.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_freeze_and_slowmo(self) -> None:
        # Enemy (type 4) frozen -> vel zeroed
        vx, vy, frozen = simulate_freeze_slowmo(4, 0x20000, 0x10000, True, False)
        self.assertTrue(frozen)
        self.assertEqual(vx, 0)
        self.assertEqual(vy, 0)

        # Player (type 1) not frozen
        vx, vy, frozen = simulate_freeze_slowmo(1, 0x20000, 0x10000, True, False)
        self.assertFalse(frozen)
        self.assertEqual(vx, 0x20000)

        # Enemy in slow motion -> vel halved
        vx, vy, _ = simulate_freeze_slowmo(4, 0x20000, 0x10000, False, True)
        self.assertEqual(vx, 0x10000)
        self.assertEqual(vy, 0x08000)

    def test_anim_frame_step(self) -> None:
        frames = [(0x0100, 8), (0x0101, 8 | 0x8000)]
        # State transition initializes
        prev_st, f_idx, timer, sp = simulate_anim_step(2, 0, 0, 0, frames)
        self.assertEqual(prev_st, 2)
        self.assertEqual(f_idx, 0)
        self.assertEqual(sp, 0x0100)

    def test_preflight_layer_select(self) -> None:
        self.assertEqual(simulate_preflight_layer(False), "0x00FF1766")
        self.assertEqual(simulate_preflight_layer(True), "0x00FF1770")


if __name__ == "__main__":
    unittest.main()
