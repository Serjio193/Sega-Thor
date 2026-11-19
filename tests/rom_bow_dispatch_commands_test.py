#!/usr/bin/env python3
"""
Unit tests for ROM Bow Spirit (0x1E) Master Tick, 9-Entry Jump Table, Command
Starters & States $0E/$0A/$0C (0x01387C..0x013B36, 698 bytes).
Milestone M15.497.
"""

from __future__ import annotations

from dataclasses import dataclass, field
import json
import math
from pathlib import Path
import unittest

EVIDENCE_PATH = (
    Path(__file__).resolve().parent.parent
    / "data"
    / "thor-evidence"
    / "rom_bow_dispatch_commands_m15_497.json"
)


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val


def helper_00f10c_vector3d(
    scale_d0: int, x1: int, y1: int, z1: int, x2: int, y2: int, z2: int
) -> tuple[int, int, int]:
    """Faithful 68K model of 0x00F10C 3D scaled vector projector."""
    dx = to_s16(x2 - x1)
    dy = to_s16(y2 - y1)
    dz = to_s16(z2 - z1)
    dist = math.isqrt(dx * dx + dy * dy + dz * dz)
    if dist == 0:
        return 0, 0, 0
    vx = to_s32(((int((dx * scale_d0) / dist) & 0xFFFF) << 16)) >> 4
    vy = to_s32(((int((dy * scale_d0) / dist) & 0xFFFF) << 16)) >> 4
    vz = to_s32(((int((dz * scale_d0) / dist) & 0xFFFF) << 16)) >> 4
    return vx, vy, vz


def helper_00e45e_homing_2d(x1: int, y1: int, x2: int, y2: int) -> tuple[int, int]:
    d1 = to_s16(x2 - x1)
    d2 = to_s16(y2 - y1)
    d4 = max(abs(d1), abs(d2)) >> 1
    if d4 == 0:
        return 0, 0
    vx = to_s32((to_s16(int((d1 << 8) / d4)) & 0xFFFF) << 8)
    vy = to_s32((to_s16(int((d2 << 8) / d4)) & 0xFFFF) << 8)
    return vx, vy


@dataclass
class BowEntity:
    type_00: int = 0x001E
    timer_02: int = 0
    state_04: int = 0
    x_08: int = 160
    y_0c: int = 120
    z_10_fixed: int = 0
    facing_16: int = 0
    priority_18: int = 0
    action_2a: int = 0
    anim_2e: int = 0
    anim_frame_30: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    flags_39: int = 0
    height_4a: int = 0
    vel_x_4e: int = 0
    vel_y_52: int = 0
    vel_z_56: int = 0
    hit_flag_82: int = 0
    hitstun_83: int = 0
    damage_86: int = 0
    sp_8e: int = 0x1E
    sub_mode_9e: int = 0
    sub_entity_a2: int = 0
    field_ac: int = 0
    latch_x_ae: int = 0
    latch_y_b2: int = 0

    @property
    def z_10_word(self) -> int:
        return to_s16((self.z_10_fixed >> 16) & 0xFFFF)

    @z_10_word.setter
    def z_10_word(self, val: int) -> None:
        self.z_10_fixed = ((val & 0xFFFF) << 16) | (self.z_10_fixed & 0xFFFF)


@dataclass
class BowWorld:
    ali: BowEntity = field(default_factory=lambda: BowEntity(type_00=0x0001, x_08=200, y_0c=150, sp_8e=40))
    anchor_ff1c1c: BowEntity = field(default_factory=lambda: BowEntity(type_00=0x0033, vel_x_4e=0x01400000, vel_y_52=0x00B00000))
    nearest_target: BowEntity | None = None
    nearest_dist: int = 0xFFFF
    terrain_d5: int = 0
    bow_rank_ff0db9: int = 0
    frame_ff1971: int = 0
    hold_counter_ff199e: int = 0
    ff17b8: int = 0
    hitstun_block: bool = False
    polled_input_count: int = 0
    tether_steps: list[int] = field(default_factory=list)
    surface_checks: int = 0
    sfx_log: list[int] = field(default_factory=list)


def bow_master_tick(bow: BowEntity, world: BowWorld, jump_map: dict[str, str]) -> str:
    if (bow.sub_entity_a2 & 0xFF) == 0:
        if world.hitstun_block:
            return "EARLY_RETURN_00E682"
        world.polled_input_count += 1
        if bow.sp_8e != 0:
            bow.sp_8e -= 1
        else:
            bow.sp_8e = 0x1E
            if world.ali.sp_8e == 0:
                return "DISMISS_013F22"
            world.ali.sp_8e -= 1
            if world.ali.sp_8e == 0:
                return "DISMISS_013F22"

        if world.ali.sp_8e <= 1:
            bow.flags_36 ^= 0x02
        else:
            bow.flags_36 &= ~0x02

        bow.damage_86 = 0
        bow.hitstun_83 = 0
        bow.hit_flag_82 = 0

    return jump_map[f"0x{bow.state_04:02X}"]


def bow_state0e_finish(bow: BowEntity, world: BowWorld) -> bool:
    if (bow.anim_flags_37 & 0x02) == 0:
        return False
    bow.priority_18 = 0x04F8
    bow.state_04 = 0x02
    bow.action_2a = 0x14
    bow.flags_36 |= 0x01
    world.sfx_log.append(0x0080037E)
    return True


def bow_command_reject_reset(bow: BowEntity, world: BowWorld) -> str:
    bow.action_2a = 6
    bow.state_04 = 0
    bow.sub_mode_9e = 4
    world.sfx_log.append(0x0090037D)
    return "REJECT_013CDC"


def bow_try_cmd1_lunge_bite(bow: BowEntity, world: BowWorld) -> str:
    target = world.nearest_target
    if target is None or world.nearest_dist > 0x00D8:
        return bow_command_reject_reset(bow, world)
    if to_s16(world.ali.sp_8e - 4) < 1:
        return bow_command_reject_reset(bow, world)
    world.ali.sp_8e -= 4

    tz = target.z_10_word + (target.height_4a >> 2)
    vx, vy, vz = helper_00f10c_vector3d(0x0080, bow.x_08, bow.y_0c, bow.z_10_word, target.x_08, target.y_0c, tz)
    bow.vel_x_4e, bow.vel_y_52, bow.vel_z_56 = vx, vy, vz
    bow.facing_16 = 1 if abs(vx) >= abs(vy) and vx >= 0 else (3 if abs(vx) >= abs(vy) else (2 if vy >= 0 else 0))
    bow.state_04 = 0x08
    bow.action_2a = 0x22
    bow.timer_02 = 0x0C
    world.sfx_log.append(0x00700221)
    return "STARTED_CMD1"


def bow_try_cmd2_underground_dive(bow: BowEntity, world: BowWorld) -> str:
    if world.ali.z_10_fixed != 0 or (world.ali.flags_39 & 0x02) or (world.terrain_d5 & 0x80):
        return bow_command_reject_reset(bow, world)
    if to_s16(world.ali.sp_8e - 9) < 1:
        return bow_command_reject_reset(bow, world)
    world.ali.sp_8e -= 9

    ax = (world.anchor_ff1c1c.vel_x_4e >> 16) & 0xFFFF
    ay = (world.anchor_ff1c1c.vel_y_52 >> 16) & 0xFFFF
    bow.state_04 = 0x04
    bow.action_2a = 0x00
    bow.vel_x_4e, bow.vel_y_52 = helper_00e45e_homing_2d(bow.x_08, bow.y_0c, ax, ay)
    bow.flags_38 |= 0x04
    bow.field_ac = 0x2710
    bow.timer_02 = 0x1E
    bow.latch_x_ae = world.ali.x_08
    bow.latch_y_b2 = world.ali.y_0c
    world.sfx_log.append(0x00800258)
    return "STARTED_CMD2"


def bow_try_cmd3_plant_bloom(bow: BowEntity, world: BowWorld) -> str:
    if to_s16(world.ali.sp_8e - 0x1C) < 1:
        return bow_command_reject_reset(bow, world)
    world.ali.sp_8e -= 0x1C
    bow.state_04 = 0x0A
    bow.action_2a = 0x10
    bow.facing_16 = 0
    bow.flags_38 |= 0x04
    return "STARTED_CMD3"


def bow_state0a_bloom_windup(bow: BowEntity, world: BowWorld) -> str:
    if bow.anim_frame_30 == 8 and bow.anim_2e == 0:
        world.sfx_log.append(0x00800226)
    if bow.anim_frame_30 < 0x0C:
        bow.z_10_word = to_s16(bow.z_10_word + 1)
        world.tether_steps.append(7)
    if (bow.anim_flags_37 & 0x02) == 0:
        return "WINDING_UP"
    bow.state_04 = 0x0C
    bow.action_2a = 0x26
    bow.flags_36 |= 0x02
    bow.timer_02 = ((world.bow_rank_ff0db9 & 0xFF) << 5) + 0x78
    world.ff17b8 |= 0x01
    bow.z_10_fixed = 0
    bow.x_08 = (world.anchor_ff1c1c.vel_x_4e >> 16) & 0xFFFF
    bow.y_0c = (world.anchor_ff1c1c.vel_y_52 >> 16) & 0xFFFF
    world.surface_checks += 1
    world.anchor_ff1c1c.type_00 = 0
    world.sfx_log.append(0x00800380)
    return "ENTERED_BLOOM_0C"


def bow_state0c_bloom_active(bow: BowEntity, world: BowWorld) -> str:
    world.hold_counter_ff199e = 0xFF00 | (world.frame_ff1971 & 0xFF)
    bow.flags_36 |= 0x02
    if world.ali.sp_8e != 0:
        bow.timer_02 = (bow.timer_02 - 1) & 0xFFFF
        if bow.timer_02 != 0:
            return "BLOOM_ACTIVE"
    bow.flags_36 &= ~0x02
    world.hold_counter_ff199e = 0
    world.ff17b8 &= ~0x01
    bow.state_04 = 0x02
    bow.action_2a = 0x14
    world.sfx_log.append(0x0080037E)
    return "BLOOM_FINISHED"


class TestROMBowDispatchCommands(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
        self.jump_map = {e["state"]: e["target"] for e in self.evidence["jump_table"]["entries"]}

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.497")
        self.assertEqual(self.evidence["range"], {"start": "0x01387C", "end": "0x013B36", "size_bytes": 698})

        jt = self.evidence["jump_table"]
        self.assertEqual((jt["address"], jt["end"], jt["size_bytes"]), ("0x0138E6", "0x0138F8", 18))
        expected_targets = {
            "0x00": "0x013CB4", "0x02": "0x013F6E", "0x04": "0x013BF2",
            "0x06": "0x013C7C", "0x08": "0x013B36", "0x0A": "0x013A5C",
            "0x0C": "0x013AE8", "0x0E": "0x0138F8", "0x10": "0x013F5A",
        }
        self.assertEqual(self.jump_map, expected_targets)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 8)
        self.assertEqual(sum(r["size_bytes"] for r in routines), 698)
        self.assertEqual(self.evidence["instruction_count"], 156)
        self.assertEqual(len(self.evidence["instructions"]), 156)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x01387C")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x013B34")

    def test_master_tick_sp_drain_damage_clear_and_dispatcher(self) -> None:
        # 1. Sub-entity +$A2 != 0 bypasses stun, SP drain, blink, and damage clear
        sub = BowEntity(state_04=0x0E, sub_entity_a2=0xFF, damage_86=5, hitstun_83=7, hit_flag_82=1, flags_36=0x82)
        w_sub = BowWorld(hitstun_block=True)
        self.assertEqual(bow_master_tick(sub, w_sub, self.jump_map), "0x0138F8")
        self.assertEqual((w_sub.polled_input_count, sub.damage_86, sub.hitstun_83, sub.flags_36), (0, 5, 7, 0x82))

        # 2. Normal tick: 30-frame ($1E) drain, low-SP blink, unconditional damage clear
        w = BowWorld(ali=BowEntity(sp_8e=2))
        b = BowEntity(sp_8e=0x1E, flags_36=0x82, damage_86=0x2010, hitstun_83=9, hit_flag_82=3)
        for rem in range(0x1D, -1, -1):
            self.assertEqual(bow_master_tick(b, w, self.jump_map), "0x013CB4")
            self.assertEqual((b.sp_8e, w.ali.sp_8e, b.flags_36, b.damage_86, b.hitstun_83, b.hit_flag_82), (rem, 2, 0x80, 0, 0, 0))
        self.assertEqual(bow_master_tick(b, w, self.jump_map), "0x013CB4")
        self.assertEqual((b.sp_8e, w.ali.sp_8e, b.flags_36), (0x1E, 1, 0x82))
        b.sp_8e = 0
        self.assertEqual(bow_master_tick(b, w, self.jump_map), "DISMISS_013F22")
        self.assertEqual(w.ali.sp_8e, 0)

        # 3. State $0E finish (0x0138F8)
        b_fin = BowEntity(state_04=0x0E, anim_flags_37=0)
        self.assertFalse(bow_state0e_finish(b_fin, w))
        b_fin.anim_flags_37 = 0x02
        self.assertTrue(bow_state0e_finish(b_fin, w))
        self.assertEqual((b_fin.priority_18, b_fin.state_04, b_fin.action_2a, b_fin.flags_36 & 1), (0x04F8, 0x02, 0x14, 1))
        self.assertEqual(w.sfx_log[-1], 0x0080037E)

    def test_command1_lunge_bite_and_reject_reset(self) -> None:
        # 1. Rejects when target missing, dist > $D8 (216), or SP <= 4
        b_rej = BowEntity()
        w_rej = BowWorld(nearest_target=None)
        self.assertEqual(bow_try_cmd1_lunge_bite(b_rej, w_rej), "REJECT_013CDC")
        self.assertEqual((b_rej.action_2a, b_rej.state_04, b_rej.sub_mode_9e, w_rej.sfx_log), (6, 0, 4, [0x0090037D]))

        target = BowEntity(type_00=0x0066, x_08=260, y_0c=120, height_4a=16)
        target.z_10_word = 0
        self.assertEqual(bow_try_cmd1_lunge_bite(BowEntity(), BowWorld(nearest_target=target, nearest_dist=217)), "REJECT_013CDC")
        self.assertEqual(bow_try_cmd1_lunge_bite(BowEntity(), BowWorld(ali=BowEntity(sp_8e=4), nearest_target=target, nearest_dist=216)), "REJECT_013CDC")

        # 2. Succeeds at dist=216, SP=5: consumes 4 SP, sets 3D launch vec, State $08, +$2A=$22, +$02=$0C, SFX #$700221
        w = BowWorld(ali=BowEntity(sp_8e=5), nearest_target=target, nearest_dist=216)
        b = BowEntity(x_08=160, y_0c=120)
        self.assertEqual(bow_try_cmd1_lunge_bite(b, w), "STARTED_CMD1")
        self.assertEqual((w.ali.sp_8e, b.state_04, b.action_2a, b.timer_02, b.facing_16), (1, 0x08, 0x22, 0x0C, 1))
        self.assertGreater(b.vel_x_4e, 0)
        self.assertGreater(b.vel_z_56, 0)  # height_4a=16 -> target_z += 4
        self.assertEqual(w.sfx_log, [0x00700221])

    def test_command2_underground_dive_and_command3_plant_bloom(self) -> None:
        # 1. Cmd 2 reject gates: Ali airborne, Ali +$39 bit 1, terrain bit 7, SP <= 9
        self.assertEqual(bow_try_cmd2_underground_dive(BowEntity(), BowWorld(ali=BowEntity(z_10_fixed=1, sp_8e=20))), "REJECT_013CDC")
        self.assertEqual(bow_try_cmd2_underground_dive(BowEntity(), BowWorld(ali=BowEntity(flags_39=2, sp_8e=20))), "REJECT_013CDC")
        self.assertEqual(bow_try_cmd2_underground_dive(BowEntity(), BowWorld(terrain_d5=0x80)), "REJECT_013CDC")
        self.assertEqual(bow_try_cmd2_underground_dive(BowEntity(), BowWorld(ali=BowEntity(sp_8e=9))), "REJECT_013CDC")

        # 2. Cmd 2 success (SP=10): State $04, +$2A=0, +$38|=4, +$AC=$2710, +$02=$1E, latches Ali (x,y), SFX #$800258
        w = BowWorld(ali=BowEntity(x_08=210, y_0c=160, sp_8e=10))
        b = BowEntity(x_08=100, y_0c=120)
        self.assertEqual(bow_try_cmd2_underground_dive(b, w), "STARTED_CMD2")
        self.assertEqual((w.ali.sp_8e, b.state_04, b.action_2a, b.flags_38 & 4), (1, 0x04, 0, 4))
        self.assertEqual((b.field_ac, b.timer_02, b.latch_x_ae, b.latch_y_b2, w.sfx_log), (0x2710, 0x1E, 210, 160, [0x00800258]))
        self.assertGreater(b.vel_x_4e, 0)

        # 3. Cmd 3: 28 ($1C) SP gate -> State $0A, +$2A=$10, +$16=0, +$38|=4
        self.assertEqual(bow_try_cmd3_plant_bloom(BowEntity(), BowWorld(ali=BowEntity(sp_8e=28))), "REJECT_013CDC")
        w3 = BowWorld(ali=BowEntity(sp_8e=29))
        b3 = BowEntity(facing_16=2)
        self.assertEqual(bow_try_cmd3_plant_bloom(b3, w3), "STARTED_CMD3")
        self.assertEqual((w3.ali.sp_8e, b3.state_04, b3.action_2a, b3.facing_16, b3.flags_38 & 4), (1, 0x0A, 0x10, 0, 4))

    def test_state0a_bloom_windup_and_state0c_bloom_active(self) -> None:
        # 1. State $0A windup: frame 8 + +$2E==0 plays #$800226; < 12 lifts +$10 and calls 0x014200 (d7=7)
        w = BowWorld(bow_rank_ff0db9=2, frame_ff1971=0x5A)
        b = BowEntity(state_04=0x0A, anim_frame_30=8, anim_2e=0)
        self.assertEqual(bow_state0a_bloom_windup(b, w), "WINDING_UP")
        self.assertEqual((w.sfx_log, b.z_10_word, w.tether_steps), ([0x00800226], 1, [7]))

        # Anim finish enters State $0C (+$2A=$26, +$36|=2, +$02=(rank<<5)+$78, $FF17B8|=1, clears $FF1C1C)
        b.anim_frame_30 = 12
        b.anim_flags_37 = 0x02
        self.assertEqual(bow_state0a_bloom_windup(b, w), "ENTERED_BLOOM_0C")
        self.assertEqual((b.state_04, b.action_2a, b.flags_36 & 2, b.timer_02), (0x0C, 0x26, 2, (2 << 5) + 0x78))
        self.assertEqual((w.ff17b8 & 1, b.z_10_fixed, b.x_08, b.y_0c), (1, 0, 0x0140, 0x00B0))
        self.assertEqual((w.surface_checks, w.anchor_ff1c1c.type_00, w.sfx_log[-1]), (1, 0, 0x00800380))

        # 2. State $0C active locks $FF199E = $FF00 | $FF1971 until timer==0 or SP==0
        b.timer_02 = 2
        self.assertEqual(bow_state0c_bloom_active(b, w), "BLOOM_ACTIVE")
        self.assertEqual((w.hold_counter_ff199e, b.timer_02, b.flags_36 & 2), (0xFF5A, 1, 2))
        self.assertEqual(bow_state0c_bloom_active(b, w), "BLOOM_FINISHED")
        self.assertEqual((b.flags_36 & 2, w.hold_counter_ff199e, w.ff17b8 & 1, b.state_04, b.action_2a), (0, 0, 0, 0x02, 0x14))
        self.assertEqual(w.sfx_log[-1], 0x0080037E)

        # SP==0 terminates State $0C immediately even with remaining timer
        b2 = BowEntity(state_04=0x0C, timer_02=50)
        w2 = BowWorld(ali=BowEntity(sp_8e=0), ff17b8=1)
        self.assertEqual(bow_state0c_bloom_active(b2, w2), "BLOOM_FINISHED")
        self.assertEqual((b2.state_04, w2.ff17b8 & 1), (0x02, 0))


if __name__ == "__main__":
    unittest.main()
