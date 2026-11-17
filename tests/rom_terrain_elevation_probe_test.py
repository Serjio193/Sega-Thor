#!/usr/bin/env python3
"""Unit tests for M15.314 ROM Terrain Elevation & Ledge Probe (0x00938E..0x0096F7)."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_terrain_elevation_probe import ROM_PATH, extract_evidence

LUT = [-1, 0, 2, 1, 4, -1, 3, -1, 6, 7, -1, -1, 5, -1, -1, -1]


def eval_terrain_probe(
    or_bits: int,
    and_bits: int,
    entity_flags: int,
    entity_elev: int,
    z_vel: int,
    scan_depth: int,
    height_table: list[int],
) -> tuple[bool, int, int]:
    """Reference simulation of the shared decision core in 0x00938E/0x0094D2/0x0095AA.

    Returns (collided, set_flag_bit, drop_height).
    """
    if (entity_flags & 0x01) or entity_elev < 0:
        return (False, 0, 0)

    tile_elev = LUT[and_bits & 0x0F]
    if tile_elev < 0:
        return (True, 6, 0)

    delta = (tile_elev - entity_elev) & 0xFF
    if delta >= 0x80:
        delta -= 0x100

    if delta == 0:
        if (or_bits & 0x80) and not (entity_flags & 0x40):
            return (True, 6, 0)
        return ((True, 6, 0) if (or_bits & 0x10) else (False, 0, 0))

    if delta >= 2:
        return (True, 6, 0)

    if delta > -2:
        # delta == 1 or delta == -1
        if delta == 1 and (entity_flags & 0x20):
            return (True, 6, 0)
        return ((True, 6, 0) if (or_bits & 0x10) else (False, 0, 0))

    # delta <= -2: ledge drop check
    if not (entity_flags & 0x10) or z_vel != 0 or (entity_elev & 1):
        return (True, 6, 0)

    drop_h = height_table[entity_elev] - height_table[tile_elev]
    if drop_h < scan_depth:
        return (True, 6, 0)
    return (True, 5, drop_h)


class TestRomTerrainElevationProbe(unittest.TestCase):
    def test_rom_evidence_matches_lut_and_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["elevation_nibble_lut"]["values"], LUT)
        saved = json.loads(
            Path("data/thor-evidence/rom_terrain_elevation_probe_m15_314.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_probe_decision_cases(self) -> None:
        heights = [0, 8, 16, 24, 32, 40, 48, 56]
        # Same elevation, no wall bit -> passable
        self.assertEqual(eval_terrain_probe(0x01, 0x01, 0x00, 0, 0, 0, heights), (False, 0, 0))
        # Mixed non-adjacent elevations -> LUT -1 -> blocked (bit 6)
        self.assertEqual(eval_terrain_probe(0x05, 0x05, 0x00, 0, 0, 0, heights), (True, 6, 0))
        # Step up +1 blocked when bit 5 of entity_flags is set
        self.assertEqual(eval_terrain_probe(0x03, 0x03, 0x20, 0, 0, 0, heights), (True, 6, 0))
        # Drop >= 2 levels with bit 4 set, grounded, even elev -> ledge drop (bit 5)
        self.assertEqual(eval_terrain_probe(0x01, 0x01, 0x10, 4, 0, 16, heights), (True, 5, 32))


if __name__ == "__main__":
    unittest.main()
