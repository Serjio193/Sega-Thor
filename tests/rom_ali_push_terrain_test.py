#!/usr/bin/env python3
"""Unit tests for M15.375 ROM Ali Push Physics, Jewel Socket Inserter & Isometric Velocity Scaler."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_ali_push_terrain import ROM_PATH, extract_evidence


def scale_isometric_terrain_velocity(
    vx: int,
    vy: int,
    slope_override_1985: bool,
    slowdown_1984: bool,
    slope_flag_16f1_bit4: bool,
    surface_dir: int,
) -> tuple[int, int]:
    """Reference simulation of 0x0064C4..0x006514."""
    if slope_override_1985:
        return (vx, vy)
    if slowdown_1984:
        return (vx >> 1, vy >> 1)
    if slope_flag_16f1_bit4:
        return (vx, vy)
    d6 = surface_dir & 0x0F
    if d6 in (1, 2, 4, 8):
        return (vx, vy)
    return (vx, vy >> 1)


def push_heavy_block(
    entity_type: int,
    z: int,
    vx: int,
    vy: int,
    frame: int,
) -> tuple[tuple[int, int], int | None] | None:
    """Reference simulation of 0x00638C..0x0063D8."""
    if z != 0:
        return None
    if entity_type not in (0x46, 0x7E):
        return None
    dx = vx >> 1
    dy = vy >> 1
    sfx = 0x00800390 if (frame & 3) == 0 else None
    return ((dx, dy), sfx)


class TestRomAliPushTerrain(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["footstep_sfx"], "0x01500218")
        self.assertEqual(ev["constants"]["push_block_sfx"], "0x00800390")
        self.assertEqual(ev["constants"]["jewel_socket_sfx"], "0x00B00385")
        self.assertEqual(ev["constants"]["jewel_toast_frames"], 60)
        saved = json.loads(
            Path("data/thor-evidence/rom_ali_push_terrain_m15_375.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_isometric_terrain_velocity_scaling(self) -> None:
        # Flat normal ground: Y halved for 2:1 isometric projection
        self.assertEqual(
            scale_isometric_terrain_velocity(0x20000, 0x20000, False, False, False, 0),
            (0x20000, 0x10000),
        )
        # Slowdown terrain (water / stairs / mud): both X and Y halved
        self.assertEqual(
            scale_isometric_terrain_velocity(0x20000, 0x20000, False, True, False, 0),
            (0x10000, 0x10000),
        )
        # Slope override: full speed
        self.assertEqual(
            scale_isometric_terrain_velocity(0x20000, 0x20000, True, False, False, 0),
            (0x20000, 0x20000),
        )
        # Cardinal slope surface (dir 2): full speed
        self.assertEqual(
            scale_isometric_terrain_velocity(0x20000, 0x20000, False, False, False, 2),
            (0x20000, 0x20000),
        )

    def test_heavy_block_push_physics(self) -> None:
        # Push block 0x46 on ground: moves at half speed
        res = push_heavy_block(0x46, 0, 0x1000, 0x2000, frame=4)
        self.assertIsNotNone(res)
        self.assertEqual(res[0], (0x800, 0x1000))
        self.assertEqual(res[1], 0x00800390)

        # Off-frame (frame 5): no SFX
        res_off = push_heavy_block(0x46, 0, 0x1000, 0x2000, frame=5)
        self.assertIsNotNone(res_off)
        self.assertEqual(res_off[0], (0x800, 0x1000))
        self.assertIsNone(res_off[1])

        # Pushable pillar 0x7E: works
        res_pillar = push_heavy_block(0x7E, 0, 0x1000, 0x2000, frame=8)
        self.assertIsNotNone(res_pillar)

        # Non-pushable entity or airborne: ignored
        self.assertIsNone(push_heavy_block(0x10, 0, 0x1000, 0x2000, frame=4))
        self.assertIsNone(push_heavy_block(0x46, 5, 0x1000, 0x2000, frame=4))


if __name__ == "__main__":
    unittest.main()
