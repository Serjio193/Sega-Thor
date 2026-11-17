#!/usr/bin/env python3
"""Unit tests for M15.401 ROM Horizontal Tilemap Wall Collision, Surface Height Step & Ledge Drop Detector."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_horizontal_wall_probe import ROM_PATH, extract_evidence


def simulate_tile_solid_probe(
    flags_38: int,
    cur_surface_type: int,
    target_surface_type: int,
    target_wall_bit: bool,
    air_height: int,
    elevation_table: dict[int, int],
    scan_extent_a4: int,
) -> tuple[bool, int, int | None]:
    """Reference simulation of 0x00938E..0x00945E.

    Returns (is_solid, updated_flags_40, ledge_drop_height).
    """
    # 0x00938E: btst.b #0, flags_38 -> intangible
    if flags_38 & 0x01:
        return False, 0, None

    # 0x00939E: tst.b cur_surface_type -> if negative, passable
    if cur_surface_type < 0:
        return False, 0, None

    flags_40 = 0

    # 0x0093B8: target surface negative (void/pit)
    if target_surface_type < 0 or target_surface_type == 0xFF:
        flags_40 |= 0x40  # bit 6 set (wall hit)
        return True, flags_40, None

    delta = target_surface_type - cur_surface_type

    # 0x0093C4: same level
    if delta == 0:
        if target_wall_bit:
            flags_40 |= 0x40
            return True, flags_40, None
        return False, 0, None

    # 0x0093C8: step up delta >= 2 -> too high, wall hit
    if delta >= 2:
        flags_40 |= 0x40
        return True, flags_40, None

    # Step up delta == 1: check if climb disabled (bit 5 of flags_38)
    if delta == 1:
        if flags_38 & 0x20:
            flags_40 |= 0x40
            return True, flags_40, None
        if target_wall_bit:
            flags_40 |= 0x40
            return True, flags_40, None
        return False, 0, None

    # delta <= -2 (0x0093DA..0x009402): ledge drop evaluation
    if delta <= -2:
        can_ledge_drop = bool(flags_38 & 0x10)  # bit 4 of flags_38
        is_on_ground = air_height == 0
        is_even_surface = (cur_surface_type & 0x01) == 0

        if can_ledge_drop and is_on_ground and is_even_surface:
            elev_cur = elevation_table.get(cur_surface_type, 0)
            elev_tgt = elevation_table.get(target_surface_type, 0)
            drop_height = elev_cur - elev_tgt
            if drop_height >= scan_extent_a4:
                flags_40 |= 0x20  # bit 5 set (ledge drop trigger)
                return True, flags_40, drop_height

        # Otherwise treated as wall/barrier
        flags_40 |= 0x40
        return True, flags_40, None

    return False, 0, None


class TestRomHorizontalWallProbe(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.401")
        self.assertEqual(ev["routines"]["tile_solid_probe_x"]["wall_collision_bit"], 6)
        self.assertEqual(ev["routines"]["tile_solid_probe_x"]["ledge_drop_bit"], 5)
        saved = json.loads(
            Path("data/thor-evidence/rom_horizontal_wall_probe_m15_401.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_intangible_entity_passable(self) -> None:
        # flags_38 bit 0 set -> intangible
        is_solid, flags_40, _ = simulate_tile_solid_probe(
            flags_38=1,
            cur_surface_type=0,
            target_surface_type=4,
            target_wall_bit=True,
            air_height=0,
            elevation_table={},
            scan_extent_a4=0,
        )
        self.assertFalse(is_solid)
        self.assertEqual(flags_40, 0)

    def test_steep_wall_collision(self) -> None:
        # delta = 4 - 1 = 3 >= 2 -> solid wall
        is_solid, flags_40, _ = simulate_tile_solid_probe(
            flags_38=0,
            cur_surface_type=1,
            target_surface_type=4,
            target_wall_bit=False,
            air_height=0,
            elevation_table={},
            scan_extent_a4=0,
        )
        self.assertTrue(is_solid)
        self.assertTrue(bool(flags_40 & 0x40))

    def test_void_pit_collision(self) -> None:
        # target is void (0xFF)
        is_solid, flags_40, _ = simulate_tile_solid_probe(
            flags_38=0,
            cur_surface_type=0,
            target_surface_type=0xFF,
            target_wall_bit=False,
            air_height=0,
            elevation_table={},
            scan_extent_a4=0,
        )
        self.assertTrue(is_solid)
        self.assertTrue(bool(flags_40 & 0x40))

    def test_ledge_drop_trigger(self) -> None:
        elev_table = {2: 40, 0: 16}  # drop of 24
        # delta = 0 - 2 = -2
        is_solid, flags_40, drop_h = simulate_tile_solid_probe(
            flags_38=0x10,  # bit 4 set
            cur_surface_type=2,
            target_surface_type=0,
            target_wall_bit=False,
            air_height=0,
            elevation_table=elev_table,
            scan_extent_a4=20,
        )
        self.assertTrue(is_solid)
        self.assertTrue(bool(flags_40 & 0x20))  # bit 5 set
        self.assertEqual(drop_h, 24)


if __name__ == "__main__":
    unittest.main()
