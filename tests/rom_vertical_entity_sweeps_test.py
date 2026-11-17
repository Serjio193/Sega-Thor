#!/usr/bin/env python3
"""Unit tests for ROM vertical (±Y) solid dynamic entity collision sweeps (0x009FA0..0x00A196)."""

import unittest
from dataclasses import dataclass
from typing import List, Optional


@dataclass
class DynamicEntityY:
    entity_id: int
    entity_type: int
    x: int
    y: int
    z: int = 0
    ground_h: int = 0
    rad_x: int = 8
    rad_y: int = 8
    height: int = 16
    flags_37: int = 0      # bit 5: cannot push, bit 6: cannot be pushed
    flags_38: int = 0      # bit 2: ghost / intangible
    flags_3a: int = 0      # bit 1: bypass ghost check
    collision_flags_y: int = 0
    collided_ptr: Optional[int] = None
    reaction_code: int = 0
    vel_y: int = 0


def sweep_pos_y(dy: int, self_ent: DynamicEntityY, pool: List[DynamicEntityY]) -> int:
    self_bot_y = self_ent.y + self_ent.ground_h + self_ent.rad_y
    self_elev = self_ent.z + self_ent.ground_h
    self_left_x = self_ent.x - self_ent.rad_x
    self_right_x = self_ent.x + self_ent.rad_x
    self_top_z = self_elev + self_ent.height
    self_is_ghost = bool(self_ent.flags_38 & 0x04)

    for target in pool:
        if target.entity_type <= 0:
            continue
        if not (target.flags_3a & 0x02):
            if self_is_ghost or (target.flags_38 & 0x04):
                continue

        tgt_y_base = target.y + target.ground_h
        self_y_base = self_ent.y + self_ent.ground_h
        if tgt_y_base <= self_y_base:
            continue

        dist = (tgt_y_base - target.rad_y) - self_bot_y
        if dist < 0:
            dist = 0
        if dist >= dy:
            continue

        # X overlap check
        tgt_left_x = target.x - target.rad_x
        tgt_right_x = target.x + target.rad_x
        if tgt_right_x <= self_left_x or tgt_left_x >= self_right_x:
            continue

        # Z (elevation) overlap check
        tgt_elev = target.z + target.ground_h
        tgt_top_z = tgt_elev + target.height
        if tgt_top_z <= self_elev or tgt_elev >= self_top_z:
            continue

        # 3D collision confirmed
        self_ent.collision_flags_y |= 0x80
        self_ent.collided_ptr = target.entity_id
        self_ent.reaction_code = 3
        dy = dist
        if dy != 0:
            continue

        # Push logic when flush contact (dist == 0)
        if not (self_ent.flags_37 & 0x20) and not (target.flags_37 & 0x40):
            target.vel_y += 1

    return dy


def sweep_neg_y(dy: int, self_ent: DynamicEntityY, pool: List[DynamicEntityY]) -> int:
    self_top_y = self_ent.y + self_ent.ground_h - self_ent.rad_y
    self_elev = self_ent.z + self_ent.ground_h
    self_left_x = self_ent.x - self_ent.rad_x
    self_right_x = self_ent.x + self_ent.rad_x
    self_top_z = self_elev + self_ent.height
    self_is_ghost = bool(self_ent.flags_38 & 0x04)

    for target in pool:
        if target.entity_type <= 0:
            continue
        if not (target.flags_3a & 0x02):
            if self_is_ghost or (target.flags_38 & 0x04):
                continue

        tgt_y_base = target.y + target.ground_h
        self_y_base = self_ent.y + self_ent.ground_h
        if tgt_y_base >= self_y_base:
            continue

        dist = self_top_y - (tgt_y_base + target.rad_y)
        if dist < 0:
            dist = 0
        if dist >= dy:
            continue

        # X overlap check
        tgt_left_x = target.x - target.rad_x
        tgt_right_x = target.x + target.rad_x
        if tgt_right_x <= self_left_x or tgt_left_x >= self_right_x:
            continue

        # Z (elevation) overlap check
        tgt_elev = target.z + target.ground_h
        tgt_top_z = tgt_elev + target.height
        if tgt_top_z <= self_elev or tgt_elev >= self_top_z:
            continue

        # 3D collision confirmed
        self_ent.collision_flags_y |= 0x80
        self_ent.collided_ptr = target.entity_id
        self_ent.reaction_code = 3
        dy = dist
        if dy != 0:
            continue

        # Push logic when flush contact (dist == 0)
        if not (self_ent.flags_37 & 0x20) and not (target.flags_37 & 0x40):
            target.vel_y -= 1

    return dy


class RomVerticalEntitySweepsTest(unittest.TestCase):
    def test_pos_y_sweep_clamps_step(self):
        self_ent = DynamicEntityY(1, 1, 100, 100, rad_y=8)
        # target top Y = 120 - 8 = 112. self bot = 108. dist = 4. step = 10 > 4 -> clamped to 4
        target = DynamicEntityY(2, 2, 100, 120, rad_y=8)
        step = sweep_pos_y(10, self_ent, [target])
        self.assertEqual(step, 4)
        self.assertEqual(self_ent.collision_flags_y, 0x80)
        self.assertEqual(self_ent.collided_ptr, 2)
        self.assertEqual(self_ent.reaction_code, 3)
        self.assertEqual(target.vel_y, 0)

    def test_pos_y_sweep_flush_contact_pushes_down(self):
        self_ent = DynamicEntityY(1, 1, 100, 100, rad_y=8)
        # target top Y = 116 - 8 = 108. self bot = 108. dist = 0.
        target = DynamicEntityY(2, 2, 100, 116, rad_y=8)
        step = sweep_pos_y(5, self_ent, [target])
        self.assertEqual(step, 0)
        self.assertEqual(self_ent.collision_flags_y, 0x80)
        self.assertEqual(target.vel_y, 1)  # pushed +1 in Y!

    def test_pos_y_sweep_target_above_ignored(self):
        self_ent = DynamicEntityY(1, 1, 100, 100)
        target = DynamicEntityY(2, 2, 100, 80)
        step = sweep_pos_y(5, self_ent, [target])
        self.assertEqual(step, 5)
        self.assertEqual(self_ent.collision_flags_y, 0)

    def test_pos_y_sweep_x_disjoint_ignored(self):
        self_ent = DynamicEntityY(1, 1, 100, 100, rad_x=8, rad_y=8)
        # target X = 130 (left = 122 > self right = 108)
        target = DynamicEntityY(2, 2, 130, 116, rad_x=8, rad_y=8)
        step = sweep_pos_y(5, self_ent, [target])
        self.assertEqual(step, 5)
        self.assertEqual(self_ent.collision_flags_y, 0)

    def test_pos_y_sweep_z_disjoint_ignored(self):
        self_ent = DynamicEntityY(1, 1, 100, 100, z=0, height=16)
        # target Z = 40 (elev = 40 > self top = 16)
        target = DynamicEntityY(2, 2, 100, 116, z=40, height=16)
        step = sweep_pos_y(5, self_ent, [target])
        self.assertEqual(step, 5)
        self.assertEqual(self_ent.collision_flags_y, 0)

    def test_neg_y_sweep_clamps_and_pushes_up(self):
        self_ent = DynamicEntityY(1, 1, 100, 100, rad_y=8)
        # target bot Y = 84 + 8 = 92. self top = 92. dist = 0.
        target = DynamicEntityY(2, 2, 100, 84, rad_y=8)
        step = sweep_neg_y(5, self_ent, [target])
        self.assertEqual(step, 0)
        self.assertEqual(self_ent.collision_flags_y, 0x80)
        self.assertEqual(target.vel_y, -1)  # pushed -1 in Y!

    def test_ghost_self_skips_sweep_unless_target_flag_3a(self):
        self_ent = DynamicEntityY(1, 1, 100, 100, flags_38=0x04)
        target = DynamicEntityY(2, 2, 100, 116)
        step = sweep_pos_y(5, self_ent, [target])
        self.assertEqual(step, 5)
        self.assertEqual(self_ent.collision_flags_y, 0)

    def test_ghost_self_hits_target_with_flag_3a(self):
        self_ent = DynamicEntityY(1, 1, 100, 100, flags_38=0x04)
        target = DynamicEntityY(2, 2, 100, 116, flags_3a=0x02)
        step = sweep_pos_y(5, self_ent, [target])
        self.assertEqual(step, 0)
        self.assertEqual(self_ent.collision_flags_y, 0x80)


if __name__ == "__main__":
    unittest.main()
