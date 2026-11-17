#!/usr/bin/env python3
"""Unit tests for M15.402 ROM Vertical Tilemap Wall Collision, Row Sampler & Height Step Subsystem."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_vertical_wall_probe import ROM_PATH, extract_evidence


def simulate_vertical_solid_probe(
    flags_38: int,
    cur_surface_type: int,
    target_surface_type: int,
    target_wall_bit: bool,
    air_height: int,
    elevation_table: dict[int, int],
    scan_extent_a4: int,
) -> tuple[bool, int, int | None]:
    """Reference simulation of 0x0094D2..0x0095A8 / 0x0095AA..0x009676.

    Returns (is_solid, updated_flags_41, ledge_drop_height).
    """
    if flags_38 & 0x01:
        return False, 0, None

    if cur_surface_type < 0:
        return False, 0, None

    flags_41 = 0

    if target_surface_type < 0 or target_surface_type == 0xFF:
        flags_41 |= 0x40  # bit 6 on flags_41 (wall hit)
        return True, flags_41, None

    delta = target_surface_type - cur_surface_type

    if delta == 0:
        if target_wall_bit:
            flags_41 |= 0x40
            return True, flags_41, None
        return False, 0, None

    if delta >= 2:
        flags_41 |= 0x40
        return True, flags_41, None

    if delta == 1:
        if flags_38 & 0x20:  # climb disabled
            flags_41 |= 0x40
            return True, flags_41, None
        if target_wall_bit:
            flags_41 |= 0x40
            return True, flags_41, None
        return False, 0, None

    if delta <= -2:
        can_ledge_drop = bool(flags_38 & 0x10)
        is_on_ground = air_height == 0
        is_even_surface = (cur_surface_type & 0x01) == 0

        if can_ledge_drop and is_on_ground and is_even_surface:
            elev_cur = elevation_table.get(cur_surface_type, 0)
            elev_tgt = elevation_table.get(target_surface_type, 0)
            drop_height = elev_cur - elev_tgt
            if drop_height >= scan_extent_a4:
                flags_41 |= 0x20  # bit 5 on flags_41 (ledge drop trigger)
                return True, flags_41, drop_height

        flags_41 |= 0x40
        return True, flags_41, None

    return False, 0, None


class TestRomVerticalWallProbe(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.402")
        self.assertEqual(ev["routines"]["tile_solid_probe_neg_y"]["collision_flags_offset"], "0x41")
        self.assertEqual(ev["routines"]["tile_solid_probe_pos_y"]["collision_flags_offset"], "0x41")
        self.assertEqual(ev["routines"]["tile_solid_probe_neg_y"]["wall_collision_bit"], 6)
        saved = json.loads(
            Path("data/thor-evidence/rom_vertical_wall_probe_m15_402.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_steep_wall_collision_y(self) -> None:
        # delta = 3 >= 2 -> solid wall on flags_41
        is_solid, flags_41, _ = simulate_vertical_solid_probe(
            flags_38=0,
            cur_surface_type=1,
            target_surface_type=4,
            target_wall_bit=False,
            air_height=0,
            elevation_table={},
            scan_extent_a4=0,
        )
        self.assertTrue(is_solid)
        self.assertTrue(bool(flags_41 & 0x40))

    def test_climb_disabled_y(self) -> None:
        # delta = 1, bit 5 set on flags_38 -> cannot step up 1 unit
        is_solid, flags_41, _ = simulate_vertical_solid_probe(
            flags_38=0x20,
            cur_surface_type=1,
            target_surface_type=2,
            target_wall_bit=False,
            air_height=0,
            elevation_table={},
            scan_extent_a4=0,
        )
        self.assertTrue(is_solid)
        self.assertTrue(bool(flags_41 & 0x40))

    def test_ledge_drop_y(self) -> None:
        elev_table = {2: 48, 0: 16}  # drop of 32
        is_solid, flags_41, drop_h = simulate_vertical_solid_probe(
            flags_38=0x10,
            cur_surface_type=2,
            target_surface_type=0,
            target_wall_bit=False,
            air_height=0,
            elevation_table=elev_table,
            scan_extent_a4=24,
        )
        self.assertTrue(is_solid)
        self.assertTrue(bool(flags_41 & 0x20))  # bit 5 on flags_41
        self.assertEqual(drop_h, 32)


if __name__ == "__main__":
    unittest.main()
