#!/usr/bin/env python3
"""Unit tests for M15.353 ROM Facing-Mirrored 3D Weapon Hitbox Builder."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_weapon_hitbox_builder import ROM_PATH, extract_evidence


def build_world_weapon_hitbox(
    ex: int,
    ey: int,
    ez: int,
    y_off: int,
    facing: int,
    local_box_6w: tuple[int, int, int, int, int, int],
) -> tuple[int, int, int, int, int, int]:
    """Reference simulation of 0x00BAC0..0x00BB16 returning (x1, y1, x2, y2, z1, z2)."""
    dz1, dz2, dx1, dy1, dx2, dy2 = local_box_6w
    if facing == 3:
        dx1, dx2 = -dx2, -dx1
    return (
        ex + dx1,
        ey + y_off + dy1,
        ex + dx2,
        ey + y_off + dy2,
        ez + y_off + dz1,
        ez + y_off + dz2,
    )


class TestRomWeaponHitboxBuilder(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["hitbox_entry_bytes"], 12)
        self.assertEqual(ev["constants"]["enemy_pool_slots"], 17)
        saved = json.loads(
            Path("data/thor-evidence/rom_weapon_hitbox_builder_m15_353.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_facing_3_x_mirroring(self) -> None:
        local = (0, 24, 4, -8, 28, 8)
        right = build_world_weapon_hitbox(100, 200, 10, 0, 1, local)
        left = build_world_weapon_hitbox(100, 200, 10, 0, 3, local)
        self.assertEqual(right, (104, 192, 128, 208, 10, 34))
        self.assertEqual(left, (72, 192, 96, 208, 10, 34))


if __name__ == "__main__":
    unittest.main()
