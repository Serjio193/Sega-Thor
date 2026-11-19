#!/usr/bin/env python3
"""
Unit tests for ROM Shade Spirit Master Tick, 40-Frame SP Drain, Damage-to-SP
Absorption, 10-State Jump Table, Shadow Grapple Starter & Dark Hole / Anchor
Warp Starter (0x012F1E..0x01319E, 640 bytes).
Milestone M15.493.
"""

from __future__ import annotations

from dataclasses import dataclass, field
import json
from pathlib import Path
import unittest

EVIDENCE_PATH = (
    Path(__file__).resolve().parent.parent
    / "data"
    / "thor-evidence"
    / "rom_shade_dispatch_grapple_m15_493.json"
)


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val


def helper_00e2d4_velocity(vx_q8: int, vy_q8: int, facing: int) -> tuple[int, int]:
    d1 = ((vx_q8 & 0xFFFF) << 8) & 0xFFFFFFFF
    d2 = ((vy_q8 & 0xFFFF) << 8) & 0xFFFFFFFF
    f = facing & 7
    if f == 0:
        return 0, to_s32(-d2)
    if f == 1:
        return to_s32(d1), 0
    if f == 2:
        return 0, to_s32(d2)
    if f == 3:
        return to_s32(-d1), 0
    return to_s32(d1), to_s32(d2)


@dataclass
class ShadeEntity:
    type_00: int = 0x001C
    timer_02: int = 0
    state_04: int = 0
    sprite_ov_06: int = 0x10
    x_08: int = 100
    y_0c: int = 120
    z_10: int = 0
    facing_16: int = 0
    priority_18: int = 0x8200
    sprite_1a: int = 0
    palette_1e: int = 0
    callback_22: int = 0
    extra_26: int = 0
    action_2a: int = 0
    anim_2c: int = 0
    anim_2e: int = 0
    anim_frame_30: int = 0
    subtype_32: int = 0
    param_34: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    flags_39: int = 0
    vel_x_4e: int = 0
    vel_y_52: int = 0
    pos_x_72: int = 0
    pos_y_76: int = 0
    attacker_ptr_7e: object | None = None
    hit_flag_82: int = 0
    hitstun_83: int = 0
    hit_dir_84: int = 0
    damage_86: int = 0
    max_hp_88: int = 100
    hp_8a: int = 100
    sp_8e: int = 0x28
    sub_entity_a2: int = 0
    skip_flag_a4: int = 0
    tether_link_a6: object | None = None
    field_aa: int = 0


@dataclass
class ShadeWorld:
    ali: ShadeEntity = field(default_factory=lambda: ShadeEntity(type_00=0x0001, x_08=200, y_0c=150, sp_8e=40))
    pool1: list[ShadeEntity] = field(default_factory=lambda: [ShadeEntity(type_00=0) for _ in range(18)])
    ff17b8: int = 0
    ff1982: int = 0
    ff1985: int = 0
    ff1986: int = 0
    ff1989: int = 0xFF
    ff198e: int = 0xFF
    terrain_d5: int = 0
    hitstun_block: bool = False
    polled_input_count: int = 0
    audio_60004_log: list[int] = field(default_factory=list)

    def alloc_pool1(self) -> ShadeEntity | None:
        for slot in self.pool1:
            if slot.type_00 == 0:
                return slot
        return None


def helper_013764_apply_sprite(ent: ShadeEntity) -> None:
    ent.sprite_1a = 0x00142B6A
    ent.palette_1e = 0x00142C00
    ent.extra_26 = 0x00142D00
    ent.flags_38 = 0x0E
    ent.anim_2c = 0xFFFF
    ent.param_34 = 0xFFFF


def shade_master_tick(shade: ShadeEntity, world: ShadeWorld, jump_map: dict[str, str]) -> str:
    if (shade.skip_flag_a4 & 0xFF) != 0:
        return "EARLY_RETURN_A4"

    if (shade.sub_entity_a2 & 0xFF) == 0:
        world.ff1989 = 0
        if world.hitstun_block:
            return "EARLY_RETURN_00E682"
        world.polled_input_count += 1

        if shade.sp_8e != 0:
            shade.sp_8e -= 1
        else:
            shade.sp_8e = 0x28
            if world.ali.sp_8e == 0:
                return "DISMISS_0136CC"
            world.ali.sp_8e -= 1
            if world.ali.sp_8e == 0:
                return "DISMISS_0136CC"

        if world.ali.sp_8e <= 1:
            shade.flags_36 ^= 0x02
        else:
            shade.flags_36 &= ~0x02

        if shade.damage_86 != 0:
            d0 = shade.damage_86 & 0xFFFF
            shade.damage_86 = 0
            d1 = ((shade.hitstun_83 + shade.hit_flag_82) * 2) & 0xFF
            shade.hit_flag_82 = 0
            if d1 < 8:
                d1 = 8
            shade.hitstun_83 = d1
            d7 = d0
            sp_loss = d0 & 0x0FFF
            new_sp = to_s16(world.ali.sp_8e - sp_loss)
            world.ali.sp_8e = max(0, new_sp)
            _a0 = shade.attacker_ptr_7e
            if d7 & (1 << 13):
                shade.flags_39 |= 0x08
            shade.state_04 = 0x04
            facing = shade.hit_dir_84 & 0xFFFF
            if facing & 0x01:
                facing ^= 0x02
            shade.facing_16 = facing
            if facing != 0:
                shade.y_0c = to_s16(shade.y_0c + 1)
            shade.z_10 = to_s16(shade.z_10 + 3)
            shade.action_2a = 0x0C
            shade.anim_2c = 0xFFFF
            shade.anim_flags_37 &= 0xF9
            shade.anim_frame_30 = 0
            shade.anim_2e = 0
            helper_013764_apply_sprite(shade)

    return jump_map[f"0x{shade.state_04:02X}"]


def try_start_shadow_grapple(shade: ShadeEntity, world: ShadeWorld) -> str:
    if world.ali.state_04 != 0 or (world.terrain_d5 & 0x80) != 0:
        world.audio_60004_log.append(0x00A00253)
        return "REJECTED_01319E"
    if to_s16(world.ali.sp_8e - 7) < 1:
        world.audio_60004_log.append(0x00A00253)
        return "REJECTED_01319E"

    world.ali.sp_8e -= 7
    world.ff1982 = 0xFF
    world.ali.flags_38 |= 0x08
    world.ff17b8 |= 0x04
    shade.state_04 = 0x06
    shade.action_2a = 0x02
    helper_013764_apply_sprite(shade)

    world.ali.pos_x_72 = 0
    world.ali.pos_y_76 = 0
    shade.pos_x_72 = 0
    shade.pos_y_76 = 0
    shade.x_08 = world.ali.x_08
    shade.y_0c = world.ali.y_0c
    shade.facing_16 = world.ali.facing_16

    off_x, off_y = helper_00e2d4_velocity(0x1000, 0x1000, shade.facing_16)
    shade.pos_x_72 = to_s32(shade.pos_x_72 + off_x)
    shade.pos_y_76 = to_s32(shade.pos_y_76 + off_y)
    shade.vel_x_4e, shade.vel_y_52 = helper_00e2d4_velocity(0x0100, 0x0100, shade.facing_16)
    shade.z_10 = 1
    shade.timer_02 = 0x12
    shade.flags_38 &= ~0x04
    shade.field_aa = 0

    tether = world.alloc_pool1()
    if tether is None:
        return "STARTED_NO_TETHER"
    tether.type_00 = 0xFFFF
    tether.callback_22 = 0x00013576
    tether.facing_16 = shade.facing_16
    tether.subtype_32 = 0x0060
    tether.param_34 = 0xFFFF
    tether.flags_36 = 0
    tether.flags_38 = 0x0D
    tether.y_0c = 0
    tether.z_10 = 0
    tether.priority_18 = (shade.priority_18 & 0x8000) | 0x07A8
    tether.sprite_1a = 0x00142B6A
    tether.tether_link_a6 = shade
    shade.tether_link_a6 = tether
    world.audio_60004_log.append(0x0080037B)
    return "STARTED_GRAPPLE"


def try_start_shadow_anchor(shade: ShadeEntity, world: ShadeWorld) -> str:
    if (world.ff1986 & 0xFF) == 0:
        world.audio_60004_log.append(0x00A00253)
        return "REJECTED_01319E"
    world.ali.flags_38 |= 0x4C
    world.ff198e = 0
    world.ff17b8 |= 0x04
    world.ali.sprite_ov_06 = 0
    shade.x_08 = world.ali.x_08
    shade.y_0c = world.ali.y_0c
    shade.pos_x_72 = 0
    shade.pos_y_76 = 0
    world.ff1985 = 0xFF
    shade.facing_16 = 0
    shade.state_04 = 0x0C
    shade.action_2a = 0x0E
    helper_013764_apply_sprite(shade)
    world.audio_60004_log.append(0x0080037C)
    return "STARTED_ANCHOR"


class TestROMShadeDispatchGrapple(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
        self.jump_map = {e["state"]: e["target"] for e in self.evidence["jump_table"]["entries"]}

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.493")
        self.assertEqual(self.evidence["range"], {"start": "0x012F1E", "end": "0x01319E", "size_bytes": 640})

        jt = self.evidence["jump_table"]
        self.assertEqual((jt["address"], jt["end"], jt["size_bytes"]), ("0x013008", "0x01301C", 20))
        expected_targets = {
            "0x00": "0x01366A", "0x02": "0x013716", "0x04": "0x0136AC", "0x06": "0x0132AA",
            "0x08": "0x0133D0", "0x0A": "0x01341A", "0x0C": "0x0131AE", "0x0E": "0x01325E",
            "0x10": "0x0136F8", "0x12": "0x01301C",
        }
        self.assertEqual(self.jump_map, expected_targets)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 3)
        self.assertEqual(sum(r["size_bytes"] for r in routines), 640)
        self.assertEqual(self.evidence["instruction_count"], 144)
        self.assertEqual(len(self.evidence["instructions"]), 144)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x012F1E")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x01319C")

    def test_master_tick_sp_drain_and_dispatcher(self) -> None:
        # 1. +$A4 != 0 returns before clearing $FF1989 or touching SP
        w_skip = ShadeWorld(ff1989=0xAA)
        self.assertEqual(shade_master_tick(ShadeEntity(skip_flag_a4=1), w_skip, self.jump_map), "EARLY_RETURN_A4")
        self.assertEqual(w_skip.ff1989, 0xAA)

        # 2. +$A2 != 0 jumps straight to dispatcher (e.g. State $12 -> 0x01301C rts)
        w_sub = ShadeWorld(ff1989=0xAA)
        self.assertEqual(shade_master_tick(ShadeEntity(state_04=0x12, sub_entity_a2=0xFF), w_sub, self.jump_map), "0x01301C")
        self.assertEqual((w_sub.ff1989, w_sub.polled_input_count), (0xAA, 0))

        # 3. 40-frame ($28) SP drain cadence & low-SP blink bit 1
        w = ShadeWorld(ali=ShadeEntity(sp_8e=2))
        s = ShadeEntity(sp_8e=0x28, flags_36=0x82)
        for rem in range(0x27, -1, -1):
            self.assertEqual(shade_master_tick(s, w, self.jump_map), "0x01366A")
            self.assertEqual((w.ff1989, s.sp_8e, w.ali.sp_8e, s.flags_36), (0, rem, 2, 0x80))
        # Drain 2 -> 1 reloads 0x28 and toggles bit 1 on
        self.assertEqual(shade_master_tick(s, w, self.jump_map), "0x01366A")
        self.assertEqual((s.sp_8e, w.ali.sp_8e, s.flags_36), (0x28, 1, 0x82))
        self.assertEqual(shade_master_tick(s, w, self.jump_map), "0x01366A")
        self.assertEqual(s.flags_36, 0x80)
        # Drain 1 -> 0 dismisses via 0x0136CC
        s.sp_8e = 0
        self.assertEqual(shade_master_tick(s, w, self.jump_map), "DISMISS_0136CC")
        self.assertEqual(w.ali.sp_8e, 0)

    def test_damage_to_sp_absorption(self) -> None:
        # 1. Bit 13 set (0x2000), facing bit 0 set (1 -> 3), non-zero facing increments +$0C
        w = ShadeWorld(ali=ShadeEntity(sp_8e=15))
        s = ShadeEntity(damage_86=0x2005, hitstun_83=1, hit_flag_82=1, hit_dir_84=1, y_0c=100, z_10=4)
        self.assertEqual(shade_master_tick(s, w, self.jump_map), "0x0136AC")
        self.assertEqual((s.damage_86, s.hit_flag_82, s.hitstun_83), (0, 0, 8))  # (1+1)*2=4 clamped to 8
        self.assertEqual((w.ali.sp_8e, s.flags_39 & 0x08, s.state_04, s.action_2a), (10, 0x08, 0x04, 0x0C))
        self.assertEqual((s.facing_16, s.y_0c, s.z_10, s.sprite_1a), (3, 101, 7, 0x00142B6A))

        # 2. Bit 13 clear, SP clamps at 0, hit_dir_84 == 0 skips +$0C increment, hitstun >= 8 un-clamped
        w2 = ShadeWorld(ali=ShadeEntity(sp_8e=4))
        s2 = ShadeEntity(damage_86=0x0010, hitstun_83=3, hit_flag_82=2, hit_dir_84=0, y_0c=100, z_10=4)
        shade_master_tick(s2, w2, self.jump_map)
        self.assertEqual((w2.ali.sp_8e, s2.hitstun_83, s2.flags_39 & 0x08), (0, 10, 0))
        self.assertEqual((s2.facing_16, s2.y_0c, s2.z_10), (0, 100, 7))

    def test_try_start_shadow_grapple(self) -> None:
        # 1. Rejection paths: Ali state != 0, terrain bit 7 set, SP <= 7
        self.assertEqual(try_start_shadow_grapple(ShadeEntity(), ShadeWorld(ali=ShadeEntity(state_04=2))), "REJECTED_01319E")
        self.assertEqual(try_start_shadow_grapple(ShadeEntity(), ShadeWorld(terrain_d5=0x80)), "REJECTED_01319E")
        w_sp = ShadeWorld(ali=ShadeEntity(sp_8e=7))
        self.assertEqual(try_start_shadow_grapple(ShadeEntity(), w_sp), "REJECTED_01319E")
        self.assertEqual(w_sp.audio_60004_log, [0x00A00253])

        # 2. Success path across facing=1 (Right): +$72/+$76 = +0x100000, +$4E/+$52 = (+0x10000, 0)
        w = ShadeWorld(ali=ShadeEntity(x_08=240, y_0c=180, facing_16=1, sp_8e=20, pos_x_72=99, pos_y_76=88))
        s = ShadeEntity(priority_18=0x8200, flags_38=0x04, field_aa=5)
        self.assertEqual(try_start_shadow_grapple(s, w), "STARTED_GRAPPLE")
        self.assertEqual((w.ali.sp_8e, w.ff1982, w.ali.flags_38 & 0x08, w.ff17b8 & 0x04), (13, 0xFF, 0x08, 0x04))
        self.assertEqual((w.ali.pos_x_72, w.ali.pos_y_76, s.x_08, s.y_0c, s.facing_16), (0, 0, 240, 180, 1))
        self.assertEqual((s.state_04, s.action_2a, s.z_10, s.timer_02, s.flags_38 & 0x04, s.field_aa), (6, 2, 1, 0x12, 0, 0))
        self.assertEqual((s.pos_x_72, s.pos_y_76, s.vel_x_4e, s.vel_y_52), (0x00100000, 0, 0x00010000, 0))

        tether = w.pool1[0]
        self.assertIs(s.tether_link_a6, tether)
        self.assertIs(tether.tether_link_a6, s)
        self.assertEqual((tether.type_00, tether.callback_22, tether.subtype_32, tether.param_34), (0xFFFF, 0x00013576, 0x60, 0xFFFF))
        self.assertEqual((tether.flags_36, tether.flags_38, tether.priority_18, tether.sprite_1a), (0, 0x0D, 0x87A8, 0x00142B6A))
        self.assertEqual(w.audio_60004_log, [0x0080037B])

        # 3. Pool full skips tether & SFX cleanly
        w_full = ShadeWorld(pool1=[ShadeEntity(type_00=1) for _ in range(18)])
        self.assertEqual(try_start_shadow_grapple(ShadeEntity(), w_full), "STARTED_NO_TETHER")
        self.assertEqual(w_full.audio_60004_log, [])

    def test_try_start_shadow_anchor_or_rescue(self) -> None:
        # 1. Requires $FF1986 != 0 else buzzer 0x01319E
        w_fail = ShadeWorld(ff1986=0)
        self.assertEqual(try_start_shadow_anchor(ShadeEntity(), w_fail), "REJECTED_01319E")
        self.assertEqual(w_fail.audio_60004_log, [0x00A00253])

        # 2. Success: sets +$38(a5)|=$4C, clears $FF198E, sets $FF17B8|=4, clears +$06(a5), State $0C
        w = ShadeWorld(
            ali=ShadeEntity(x_08=310, y_0c=210, sprite_ov_06=0x16, flags_38=0x01),
            ff1986=1,
            ff198e=0xFF,
            ff1985=0,
        )
        s = ShadeEntity(x_08=50, y_0c=60, pos_x_72=123, pos_y_76=456, facing_16=3)
        self.assertEqual(try_start_shadow_anchor(s, w), "STARTED_ANCHOR")
        self.assertEqual((w.ali.flags_38, w.ff198e, w.ff17b8 & 0x04, w.ali.sprite_ov_06), (0x4D, 0, 0x04, 0))
        self.assertEqual((s.x_08, s.y_0c, s.pos_x_72, s.pos_y_76, w.ff1985), (310, 210, 0, 0, 0xFF))
        self.assertEqual((s.facing_16, s.state_04, s.action_2a, s.sprite_1a), (0, 0x0C, 0x0E, 0x00142B6A))
        self.assertEqual(w.audio_60004_log, [0x0080037C])


if __name__ == "__main__":
    unittest.main()
