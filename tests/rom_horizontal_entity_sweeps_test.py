#!/usr/bin/env python3
"""Unit tests for ROM horizontal (±X) solid dynamic entity collision sweeps (0x009DD6..0x009F9E)."""

import unittest
from dataclasses import dataclass
from typing import List, Optional


@dataclass
class DynamicEntity:
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
    flags_38: int = 0      # bit 2: intangible / pass-through
    flags_3a: int = 0      # bit 1: ignore collision
    collision_flags_x: int = 0
    collided_ptr: Optional[int] = None
    reaction_code: int = 0
    vel_x: int = 0


def sweep_pos_x(dx: int, self_ent: DynamicEntity, pool: List[DynamicEntity]) -> int:
    if self_ent.flags_38 & 0x04:
        return dx

    self_right = self_ent.x + self_ent.rad_x
    self_elevation = self_ent.z + self_ent.ground_h
    self_y_base = self_ent.y + self_ent.ground_h
    self_top_y = self_y_base - self_ent.rad_y
    self_bot_y = self_y_base + self_ent.rad_y

    for target in pool:
        if target.entity_type <= 0:
            continue
        if target.flags_38 & 0x04:
            continue
        if target.flags_3a & 0x02:
            continue
        if target.x <= self_ent.x:
            continue

        dist = (target.x - target.rad_x) - self_right
        if dist < 0:
            dist = 0
        if dist >= dx:
            continue

        # Y overlap check
        tgt_y_base = target.y + target.ground_h
        tgt_top_y = tgt_y_base - target.rad_y
        tgt_bot_y = tgt_y_base + target.rad_y
        if tgt_bot_y <= self_top_y or tgt_top_y >= self_bot_y:
            continue

        # Z (elevation) overlap check
        tgt_elevation = target.z + target.ground_h
        tgt_top_z = tgt_elevation + target.height
        self_top_z = self_elevation + self_ent.height
        if tgt_top_z <= self_elevation or tgt_elevation >= self_top_z:
            continue

        # 3D collision confirmed
        self_ent.collision_flags_x |= 0x80
        self_ent.collided_ptr = target.entity_id
        self_ent.reaction_code = 3
        dx = dist
        if dx != 0:
            continue

        # Push logic when flush contact (dist == 0)
        if not (self_ent.flags_37 & 0x20) and not (target.flags_37 & 0x40):
            target.vel_x += 1

    return dx


def sweep_neg_x(dx: int, self_ent: DynamicEntity, pool: List[DynamicEntity]) -> int:
    if self_ent.flags_38 & 0x04:
        return dx

    self_left = self_ent.x - self_ent.rad_x
    self_elevation = self_ent.z + self_ent.ground_h
    self_y_base = self_ent.y + self_ent.ground_h
    self_top_y = self_y_base - self_ent.rad_y
    self_bot_y = self_y_base + self_ent.rad_y

    for target in pool:
        if target.entity_type <= 0:
            continue
        if target.flags_38 & 0x04:
            continue
        if target.flags_3a & 0x02:
            continue
        if target.x >= self_ent.x:
            continue

        dist = self_left - (target.x + target.rad_x)
        if dist < 0:
            dist = 0
        if dist >= dx:
            continue

        # Y overlap check
        tgt_y_base = target.y + target.ground_h
        tgt_top_y = tgt_y_base - target.rad_y
        tgt_bot_y = tgt_y_base + target.rad_y
        if tgt_bot_y <= self_top_y or tgt_top_y >= self_bot_y:
            continue

        # Z (elevation) overlap check
        tgt_elevation = target.z + target.ground_h
        tgt_top_z = tgt_elevation + target.height
        self_top_z = self_elevation + self_ent.height
        if tgt_top_z <= self_elevation or tgt_elevation >= self_top_z:
            continue

        # 3D collision confirmed
        self_ent.collision_flags_x |= 0x80
        self_ent.collided_ptr = target.entity_id
        self_ent.reaction_code = 3
        dx = dist
        if dx != 0:
            continue

        # Push logic when flush contact (dist == 0)
        if not (self_ent.flags_37 & 0x20) and not (target.flags_37 & 0x40):
            target.vel_x -= 1

    return dx


class RomHorizontalEntitySweepsTest(unittest.TestCase):
    def test_intangible_self_skips_sweep(self):
        self_ent = DynamicEntity(1, 1, 100, 100, flags_38=0x04)
        target = DynamicEntity(2, 2, 110, 100)
        step = sweep_pos_x(10, self_ent, [target])
        self.assertEqual(step, 10)
        self.assertEqual(self_ent.collision_flags_x, 0)

    def test_pos_x_sweep_clear_when_target_far(self):
        self_ent = DynamicEntity(1, 1, 100, 100, rad_x=8)
        # target left edge = 130 - 8 = 122. self right = 108. dist = 14. step = 10 < 14
        target = DynamicEntity(2, 2, 130, 100, rad_x=8)
        step = sweep_pos_x(10, self_ent, [target])
        self.assertEqual(step, 10)
        self.assertEqual(self_ent.collision_flags_x, 0)

    def test_pos_x_sweep_clamps_step_on_overlap(self):
        self_ent = DynamicEntity(1, 1, 100, 100, rad_x=8)
        # target left edge = 120 - 8 = 112. self right = 108. dist = 4. step = 10 > 4 -> clamped to 4
        target = DynamicEntity(2, 2, 120, 100, rad_x=8)
        step = sweep_pos_x(10, self_ent, [target])
        self.assertEqual(step, 4)
        self.assertEqual(self_ent.collision_flags_x, 0x80)
        self.assertEqual(self_ent.collided_ptr, 2)
        self.assertEqual(self_ent.reaction_code, 3)
        self.assertEqual(target.vel_x, 0)

    def test_pos_x_sweep_flush_contact_pushes_target(self):
        self_ent = DynamicEntity(1, 1, 100, 100, rad_x=8)
        # target left edge = 116 - 8 = 108. self right = 108. dist = 0.
        target = DynamicEntity(2, 2, 116, 100, rad_x=8)
        step = sweep_pos_x(5, self_ent, [target])
        self.assertEqual(step, 0)
        self.assertEqual(self_ent.collision_flags_x, 0x80)
        self.assertEqual(target.vel_x, 1)  # pushed +1 in X!

    def test_pos_x_sweep_flush_contact_push_disabled(self):
        # self cannot push (flags_37 bit 5 = 0x20)
        self_ent = DynamicEntity(1, 1, 100, 100, rad_x=8, flags_37=0x20)
        target = DynamicEntity(2, 2, 116, 100, rad_x=8)
        step = sweep_pos_x(5, self_ent, [target])
        self.assertEqual(step, 0)
        self.assertEqual(target.vel_x, 0)

    def test_pos_x_sweep_flush_contact_target_resistant(self):
        self_ent = DynamicEntity(1, 1, 100, 100, rad_x=8)
        # target push resistant (flags_37 bit 6 = 0x40)
        target = DynamicEntity(2, 2, 116, 100, rad_x=8, flags_37=0x40)
        step = sweep_pos_x(5, self_ent, [target])
        self.assertEqual(step, 0)
        self.assertEqual(target.vel_x, 0)

    def test_pos_x_sweep_misses_when_y_disjoint(self):
        self_ent = DynamicEntity(1, 1, 100, 100, rad_x=8, rad_y=8)
        # target Y = 130 (top = 122 > self bot = 108)
        target = DynamicEntity(2, 2, 116, 130, rad_x=8, rad_y=8)
        step = sweep_pos_x(5, self_ent, [target])
        self.assertEqual(step, 5)
        self.assertEqual(self_ent.collision_flags_x, 0)

    def test_pos_x_sweep_misses_when_z_disjoint(self):
        self_ent = DynamicEntity(1, 1, 100, 100, z=0, height=16)
        # target Z = 30 (elevation 30 >= self top 16)
        target = DynamicEntity(2, 2, 116, 100, z=30, height=16)
        step = sweep_pos_x(5, self_ent, [target])
        self.assertEqual(step, 5)
        self.assertEqual(self_ent.collision_flags_x, 0)

    def test_neg_x_sweep_clamps_and_pushes_left(self):
        self_ent = DynamicEntity(1, 1, 100, 100, rad_x=8)
        # target right edge = 84 + 8 = 92. self left = 92. dist = 0.
        target = DynamicEntity(2, 2, 84, 100, rad_x=8)
        step = sweep_neg_x(5, self_ent, [target])
        self.assertEqual(step, 0)
        self.assertEqual(self_ent.collision_flags_x, 0x80)
        self.assertEqual(target.vel_x, -1)  # pushed -1 in X!


if __name__ == "__main__":
    unittest.main()
