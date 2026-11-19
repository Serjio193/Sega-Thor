#!/usr/bin/env python3
"""
Unit tests for ROM Shade Spirit Idle Command Dispatcher, Hitstun Recovery,
Ascend Dismissal, Approach Homing & 2-Slot Ali Pose History Ring Buffer
(0x01366A..0x01387C, 530 bytes). Milestone M15.496.
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
    / "rom_shade_follow_history_m15_496.json"
)

SHADE_BODY_DESC = (0x00142B6A, 0x0010F100, 0x000409A8)
ALI_DOPPELGANGER_DESC = (0x0013CDFC, 0x000C0000, 0x0003FF7A)


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val


def m68k_divs_w(num: int, den: int) -> int:
    return to_s16(int(num / den))


def helper_00e45e_homing_velocity(
    self_x: int, self_y: int, target_x: int, target_y: int, facing_16: int = 0
) -> tuple[int, int, int, int]:
    """Model of 0x00E45E returning (vx, vy, facing_dir, half_chebyshev_d4)."""
    d1 = to_s16(target_x - self_x)
    d4 = abs(d1)
    d2 = to_s16(target_y - self_y)
    d5 = abs(d2)
    d3 = 1
    if d4 <= d5:
        d4 = d5
        d3 = 0
    d4 >>= 1
    if d4 == 0:
        return 0, 0, facing_16, 0
    vx = to_s32(m68k_divs_w(d1 << 8, d4) << 8)
    vy = to_s32(m68k_divs_w(d2 << 8, d4) << 8)
    if (vx | vy) == 0:
        return 0, 0, facing_16, 0
    return vx, vy, d3, d4


def helper_0123b8_track_elevation(shade: "ShadeEntity", ali: "ShadeEntity", d4: int) -> None:
    """Model of 0x0123B8 (AI_Dytto_TrackTargetElevation custom d4 entry)."""
    d5 = to_s16(ali.ground_z_14)
    d0 = to_s16(shade.ground_z_14)
    if d0 < 0:
        d0 = d5
    d5 = to_s16(d5 + d4)
    z_hi = to_s16(shade.z_pos_10)
    diff = to_s16(d0 + z_hi - d5)
    if diff == 0:
        return
    if diff < 0:
        z_hi = to_s16(z_hi + 1)
    elif z_hi > 0:
        z_hi = to_s16(z_hi - 1)
    shade.z_pos_10 = z_hi


@dataclass
class ShadeEntity:
    type_00: int = 0x001C
    state_04: int = 0
    piece_06: int = 0
    x_pos_08: int = 160
    y_pos_0c: int = 120
    z_pos_10: int = 0
    ground_z_14: int = 0
    facing_16: int = 2
    ptr_1a: int = 0
    ptr_1e: int = 0
    ptr_26: int = 0
    action_2a: int = 0
    prev_action_2c: int = 0
    anim_timer_2e: int = 0
    sprite_frame_32: int = 0
    prev_frame_34: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    flags_39: int = 0
    flags_6e: int = 0
    flags_6f: int = 0
    delta_x_72: int = 0
    delta_y_76: int = 0
    hit_guard_82: int = 0
    hitstun_83: int = 0
    damage_86: int = 0


@dataclass
class ShadeSimCtx:
    ali: ShadeEntity = field(default_factory=lambda: ShadeEntity(type_00=0x0001))
    mimic_active_1989: int = 0
    command_19a2: int = 0
    dismiss_19a7: int = 0
    ring_slot0_19a8: tuple[int, int, int, int, int, int] = (0, 0, 0, 0, 0, 0)
    ring_slot1_19b4: tuple[int, int, int, int, int, int] = (0, 0, 0, 0, 0, 0)
    sfx_log: list[int] = field(default_factory=list)
    calls: list[str] = field(default_factory=list)


def capture_ali_pose(ali: ShadeEntity) -> tuple[int, int, int, int, int, int]:
    """Builds the 6-word snapshot written by 0x013848..0x01387A."""
    dx_hi = to_s16(ali.delta_x_72 >> 16)
    dy_hi = to_s16(ali.delta_y_76 >> 16)
    return (
        to_s16(ali.x_pos_08 + dx_hi),
        to_s16(ali.y_pos_0c + dy_hi),
        to_s16(ali.z_pos_10),
        to_s16(ali.ground_z_14),
        ali.facing_16 & 0xFFFF,
        ali.sprite_frame_32 & 0xFFFF,
    )


def switch_to_shade_body_descriptor(shade: ShadeEntity) -> None:
    """0x013764..0x01378A."""
    shade.ptr_1a, shade.ptr_1e, shade.ptr_26 = SHADE_BODY_DESC
    shade.flags_38 = 0x0E
    shade.prev_action_2c = 0xFFFF
    shade.prev_frame_34 = 0xFFFF


def step_pose_history_ring(shade: ShadeEntity, ctx: ShadeSimCtx) -> None:
    """0x0137F0..0x01387C."""
    shade.delta_x_72 = 0
    shade.delta_y_76 = 0
    x, y, z, gz, facing, anim = ctx.ring_slot1_19b4
    shade.x_pos_08 = x
    shade.y_pos_0c = y
    shade.z_pos_10 = z
    shade.ground_z_14 = gz
    shade.facing_16 = facing
    shade.sprite_frame_32 = anim
    shade.anim_timer_2e = 0x03DE
    shade.flags_39 = 0
    shade.flags_6e = 0
    shade.flags_6f = 0
    ctx.ring_slot1_19b4 = ctx.ring_slot0_19a8
    ctx.ring_slot0_19a8 = capture_ali_pose(ctx.ali)


def enter_doppelganger_mimic_mode(shade: ShadeEntity, ctx: ShadeSimCtx) -> None:
    """0x01378A..0x0137F0 (falls through into 0x0137F0)."""
    shade.state_04 = 0
    shade.ptr_1a, shade.ptr_1e, shade.ptr_26 = ALI_DOPPELGANGER_DESC
    shade.flags_38 = 0x0D
    shade.piece_06 = 0
    shade.flags_39 = 0
    shade.hit_guard_82 = 0
    shade.hitstun_83 = 0
    shade.damage_86 = 0
    shade.prev_frame_34 = 0xFFFF
    snap = capture_ali_pose(ctx.ali)
    ctx.ring_slot0_19a8 = snap
    ctx.ring_slot1_19b4 = snap
    step_pose_history_ring(shade, ctx)


def start_dismissal(shade: ShadeEntity, ctx: ShadeSimCtx) -> None:
    """0x0136D4..0x0136F8."""
    ctx.sfx_log.append(0x00A0039E)
    switch_to_shade_body_descriptor(shade)
    shade.facing_16 = 0
    shade.state_04 = 0x10
    ctx.calls.append("0x008EF8(d0=0x14)")


def sp_drain_dismissal_guard(shade: ShadeEntity, ctx: ShadeSimCtx) -> str:
    """0x0136CC..0x0136D4."""
    if shade.state_04 != 0:
        ctx.calls.append("0x012F6A")
        return "GUARD_012F6A"
    start_dismissal(shade, ctx)
    return "DISMISS_STARTED"


def state00_idle_command_and_follow(shade: ShadeEntity, ctx: ShadeSimCtx) -> str:
    """0x01366A..0x0136AC."""
    if (ctx.ali.flags_36 & 0x02) != 0:
        shade.flags_36 |= 0x02
    if (ctx.dismiss_19a7 & 0xFF) != 0:
        start_dismissal(shade, ctx)
        return "DISMISS_STARTED"
    cmd = ctx.command_19a2 & 0xFF
    if cmd == 1:
        ctx.calls.append("0x01301E")
        return "CMD1_01301E"
    if cmd == 2:
        ctx.calls.append("0x01319E")
        return "CMD2_01319E"
    if cmd == 3:
        ctx.calls.append("0x013134")
        return "CMD3_013134"
    ctx.mimic_active_1989 = 0xFF
    step_pose_history_ring(shade, ctx)
    return "MIMIC_STEP"


def state04_hitstun_blink_recovery(shade: ShadeEntity, ctx: ShadeSimCtx) -> str:
    """0x0136AC..0x0136CC."""
    shade.hitstun_83 = (shade.hitstun_83 - 1) & 0xFF
    if shade.hitstun_83 != 0:
        shade.flags_36 |= 0x40
        return "BLINKING"
    shade.flags_36 &= ~0x40
    shade.flags_39 &= ~0x08
    enter_doppelganger_mimic_mode(shade, ctx)
    return "RECOVERED_TO_MIMIC"


def state10_ascend_despawn(shade: ShadeEntity) -> str:
    """0x0136F8..0x013716."""
    shade.flags_36 &= ~0x02
    z = to_s16(shade.z_pos_10)
    if z > 0x0100:
        shade.type_00 = 0
        return "DESPAWNED"
    shade.z_pos_10 = to_s16(z + 8)
    return "ASCENDING"


def state02_summon_approach_homing(shade: ShadeEntity, ctx: ShadeSimCtx) -> str:
    """0x013716..0x013764."""
    if shade.action_2a != 0x14 and (shade.anim_flags_37 & 0x02) != 0:
        shade.action_2a = 0x14
    helper_0123b8_track_elevation(shade, ctx.ali, 1)
    tx = ctx.ali.x_pos_08
    ty = ctx.ali.y_pos_0c
    d5 = to_s16(shade.ground_z_14)
    if d5 >= 0:
        d6 = to_s16(ctx.ali.ground_z_14)
        if d6 >= 0:
            ty = to_s16(ty + (d6 - d5))
    vx, vy, _, d4 = helper_00e45e_homing_velocity(
        shade.x_pos_08, shade.y_pos_0c, tx, ty, shade.facing_16
    )
    if d4 <= 7:
        enter_doppelganger_mimic_mode(shade, ctx)
        return "ARRIVED_MIMIC"
    shade.delta_x_72 = to_s32(vx << 2)
    shade.delta_y_76 = to_s32(vy << 2)
    return "APPROACHING"


class TestROMShadeFollowHistory(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure_and_disassembly(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.496")
        self.assertEqual(self.evidence["range"]["start"], "0x01366A")
        self.assertEqual(self.evidence["range"]["end"], "0x01387C")
        self.assertEqual(self.evidence["range"]["size_bytes"], 530)
        self.assertEqual(self.evidence["instruction_count"], 135)
        self.assertEqual(len(self.evidence["instructions"]), 135)
        self.assertEqual(sum(i["size"] for i in self.evidence["instructions"]), 530)

        cursor = 0x01366A
        for insn in self.evidence["instructions"]:
            self.assertEqual(int(insn["address"], 16), cursor)
            cursor += insn["size"]
        self.assertEqual(cursor, 0x01387C)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertEqual(
            list(routines.keys()),
            [
                "AI_Shade_State00_IdleCommandAndHistoryFollow",
                "AI_Shade_State04_HitstunBlinkRecovery",
                "AI_Shade_DismissalAndState10_AscendDespawn",
                "AI_Shade_State02_SummonApproachHoming",
                "AI_Shade_DescriptorSwitchAndPoseHistoryRing",
            ],
        )
        self.assertEqual(routines["AI_Shade_State00_IdleCommandAndHistoryFollow"]["size_bytes"], 66)
        self.assertEqual(routines["AI_Shade_State00_IdleCommandAndHistoryFollow"]["instruction_count"], 15)
        self.assertEqual(routines["AI_Shade_State04_HitstunBlinkRecovery"]["size_bytes"], 32)
        self.assertEqual(routines["AI_Shade_State04_HitstunBlinkRecovery"]["instruction_count"], 7)
        self.assertEqual(routines["AI_Shade_DismissalAndState10_AscendDespawn"]["size_bytes"], 74)
        self.assertEqual(routines["AI_Shade_DismissalAndState10_AscendDespawn"]["instruction_count"], 19)
        self.assertEqual(routines["AI_Shade_State02_SummonApproachHoming"]["size_bytes"], 78)
        self.assertEqual(routines["AI_Shade_State02_SummonApproachHoming"]["instruction_count"], 23)
        self.assertEqual(routines["AI_Shade_DescriptorSwitchAndPoseHistoryRing"]["size_bytes"], 280)
        self.assertEqual(routines["AI_Shade_DescriptorSwitchAndPoseHistoryRing"]["instruction_count"], 71)

        by_addr = {int(i["address"], 16): i for i in self.evidence["instructions"]}
        self.assertEqual(by_addr[0x0136A0]["op_str"], "$ff1989.l")
        self.assertEqual(by_addr[0x0136D4]["op_str"], "#$a0039e, d0")
        self.assertEqual(by_addr[0x013702]["op_str"], "#$100, d0")
        self.assertEqual(by_addr[0x013764]["op_str"], "$3f4a6.l, a0")
        self.assertEqual(by_addr[0x013790]["op_str"], "$3f306.l, a0")
        self.assertEqual(by_addr[0x013822]["op_str"], "#$3de, $2e(a6)")

    def test_state00_idle_command_dispatch_and_dismissal_guard(self) -> None:
        # 1. Bit 1 propagation from Ali +$36(a5) and neutral mimic step ($FF1989=$FF)
        ctx = ShadeSimCtx(
            ali=ShadeEntity(x_pos_08=200, y_pos_0c=150, flags_36=0x02),
            ring_slot0_19a8=(180, 140, 2, 0, 1, 0x22),
            ring_slot1_19b4=(170, 130, 0, 0, 3, 0x11),
        )
        shade = ShadeEntity(state_04=0, flags_36=0x00)
        self.assertEqual(state00_idle_command_and_follow(shade, ctx), "MIMIC_STEP")
        self.assertEqual(shade.flags_36 & 0x02, 0x02)
        self.assertEqual(ctx.mimic_active_1989, 0xFF)
        self.assertEqual((shade.x_pos_08, shade.y_pos_0c, shade.facing_16, shade.sprite_frame_32),
                         (170, 130, 3, 0x11))

        # Clearing Ali bit 1 does NOT clear Shade bit 1 in State 00
        ctx.ali.flags_36 = 0x00
        state00_idle_command_and_follow(shade, ctx)
        self.assertEqual(shade.flags_36 & 0x02, 0x02)

        # 2. Command dispatch 1/2/3 on $FF19A2 skips $FF1989 & ring step
        for cmd, exp_target in ((1, "CMD1_01301E"), (2, "CMD2_01319E"), (3, "CMD3_013134")):
            c = ShadeSimCtx(command_19a2=cmd)
            s = ShadeEntity(state_04=0)
            self.assertEqual(state00_idle_command_and_follow(s, c), exp_target)
            self.assertEqual(c.mimic_active_1989, 0)

        # 3. $FF19A7 dismissal flag & 0x0136CC SP-drain guard
        s_busy = ShadeEntity(state_04=0x04)
        c_busy = ShadeSimCtx()
        self.assertEqual(sp_drain_dismissal_guard(s_busy, c_busy), "GUARD_012F6A")
        self.assertEqual(s_busy.state_04, 0x04)

        s_idle = ShadeEntity(state_04=0x00, facing_16=2)
        c_dis = ShadeSimCtx(dismiss_19a7=1, command_19a2=2)
        self.assertEqual(state00_idle_command_and_follow(s_idle, c_dis), "DISMISS_STARTED")
        self.assertEqual(c_dis.sfx_log, [0x00A0039E])
        self.assertEqual((s_idle.state_04, s_idle.facing_16, s_idle.flags_38), (0x10, 0, 0x0E))
        self.assertEqual((s_idle.ptr_1a, s_idle.ptr_1e, s_idle.ptr_26), SHADE_BODY_DESC)
        self.assertEqual((s_idle.prev_action_2c, s_idle.prev_frame_34), (0xFFFF, 0xFFFF))
        self.assertEqual(c_dis.calls[-1], "0x008EF8(d0=0x14)")

    def test_state04_hitstun_recovery_and_state10_ascend_despawn(self) -> None:
        # 1. State $04 hitstun countdown +$83 -> blink bit 6 -> mimic recovery at 0
        ctx = ShadeSimCtx(ali=ShadeEntity(x_pos_08=210, y_pos_0c=160, facing_16=1, sprite_frame_32=0x05))
        shade = ShadeEntity(state_04=0x04, hitstun_83=3, flags_36=0x00, flags_39=0x08)
        self.assertEqual(state04_hitstun_blink_recovery(shade, ctx), "BLINKING")
        self.assertEqual((shade.hitstun_83, shade.flags_36 & 0x40), (2, 0x40))
        self.assertEqual(state04_hitstun_blink_recovery(shade, ctx), "BLINKING")
        self.assertEqual((shade.hitstun_83, shade.flags_36 & 0x40), (1, 0x40))
        self.assertEqual(state04_hitstun_blink_recovery(shade, ctx), "RECOVERED_TO_MIMIC")
        self.assertEqual((shade.hitstun_83, shade.flags_36 & 0x40, shade.flags_39), (0, 0, 0))
        self.assertEqual((shade.state_04, shade.flags_38, shade.ptr_1a), (0, 0x0D, ALI_DOPPELGANGER_DESC[0]))

        # 2. State $10 ascend (+8 per frame until > 256 -> despawn +$00=0)
        asc = ShadeEntity(type_00=0x001C, state_04=0x10, z_pos_10=248, flags_36=0x02)
        self.assertEqual(state10_ascend_despawn(asc), "ASCENDING")  # 248 <= 256 -> 256
        self.assertEqual((asc.z_pos_10, asc.flags_36 & 0x02, asc.type_00), (256, 0, 0x001C))
        self.assertEqual(state10_ascend_despawn(asc), "ASCENDING")  # 256 is NOT > 256 -> 264
        self.assertEqual((asc.z_pos_10, asc.type_00), (264, 0x001C))
        self.assertEqual(state10_ascend_despawn(asc), "DESPAWNED")  # 264 > 256 -> clr.w $0(a6)
        self.assertEqual(asc.type_00, 0)

    def test_state02_summon_approach_homing_and_elevation(self) -> None:
        # 1. +$2A promotion on +$37 bit 1, elevation +1 step, Z-compensated Y, and 4x velocity
        ali = ShadeEntity(x_pos_08=200, y_pos_0c=100, ground_z_14=20, facing_16=1, sprite_frame_32=0x09)
        ctx = ShadeSimCtx(ali=ali)
        shade = ShadeEntity(
            state_04=0x02, action_2a=0x00, anim_flags_37=0x00,
            x_pos_08=160, y_pos_0c=100, z_pos_10=10, ground_z_14=10,
        )
        self.assertEqual(state02_summon_approach_homing(shade, ctx), "APPROACHING")
        self.assertEqual((shade.action_2a, shade.z_pos_10), (0x00, 11))
        # Target Y = 100 + (20 - 10) = 110 -> dx=40, dy=10 -> d4=20, vx=0x200<<8, vy=0x80<<8 -> 4x
        exp_vx, exp_vy, _, _ = helper_00e45e_homing_velocity(160, 100, 200, 110)
        self.assertEqual((shade.delta_x_72, shade.delta_y_76), (exp_vx << 2, exp_vy << 2))

        # 2. +$37 bit 1 promotes +$2A=$14; negative ground_z skips Z compensation and falls back in 0x0123B8 (20+15=35 > 21 -> 14)
        shade.anim_flags_37 = 0x02
        shade.ground_z_14 = -1
        shade.z_pos_10 = 15
        self.assertEqual(state02_summon_approach_homing(shade, ctx), "APPROACHING")
        self.assertEqual((shade.action_2a, shade.z_pos_10), (0x14, 14))
        exp_vx2, exp_vy2, _, _ = helper_00e45e_homing_velocity(160, 100, 200, 100)
        self.assertEqual((shade.delta_x_72, shade.delta_y_76), (exp_vx2 << 2, exp_vy2 << 2))

        # 3. Arrival boundary: d4 = max(|dx|,|dy|) >> 1 == 7 (dx=15) enters mimic; dx=16 (d4=8) keeps approaching
        s_far = ShadeEntity(state_04=0x02, x_pos_08=184, y_pos_0c=100, ground_z_14=20)
        self.assertEqual(state02_summon_approach_homing(s_far, ctx), "APPROACHING")
        s_near = ShadeEntity(state_04=0x02, x_pos_08=185, y_pos_0c=100, ground_z_14=20)
        self.assertEqual(state02_summon_approach_homing(s_near, ctx), "ARRIVED_MIMIC")
        self.assertEqual((s_near.state_04, s_near.flags_38, s_near.x_pos_08), (0, 0x0D, 200))

    def test_descriptor_switch_and_pose_history_ring_buffer(self) -> None:
        # Frame T0: Enter doppelganger mimic mode with Ali moving (+dx, -dy)
        ali = ShadeEntity(
            x_pos_08=100, y_pos_0c=200, z_pos_10=4, ground_z_14=8,
            facing_16=1, sprite_frame_32=0x12, delta_x_72=0x00038000, delta_y_76=-0x00020000,
        )
        ctx = ShadeSimCtx(ali=ali)
        shade = ShadeEntity(
            state_04=0x04, piece_06=0x08, flags_39=0xFF, flags_6e=0xFF, flags_6f=0xFF,
            hit_guard_82=5, hitstun_83=9, damage_86=0x1234, delta_x_72=0x40000, delta_y_76=0x40000,
        )
        enter_doppelganger_mimic_mode(shade, ctx)
        self.assertEqual((shade.state_04, shade.piece_06, shade.flags_38), (0, 0, 0x0D))
        self.assertEqual((shade.ptr_1a, shade.ptr_1e, shade.ptr_26), ALI_DOPPELGANGER_DESC)
        self.assertEqual((shade.hit_guard_82, shade.hitstun_83, shade.damage_86, shade.prev_frame_34),
                         (0, 0, 0, 0xFFFF))
        self.assertEqual((shade.flags_39, shade.flags_6e, shade.flags_6f, shade.anim_timer_2e),
                         (0, 0, 0, 0x03DE))
        self.assertEqual((shade.delta_x_72, shade.delta_y_76), (0, 0))
        pose_t0 = (103, 198, 4, 8, 1, 0x12)
        self.assertEqual(ctx.ring_slot0_19a8, pose_t0)
        self.assertEqual(ctx.ring_slot1_19b4, pose_t0)

        # Frame T1: Ali moves to pose_t1; Shade still mirrors pose_t0 while slot0 captures pose_t1
        ali.x_pos_08, ali.y_pos_0c, ali.z_pos_10 = 105, 195, 6
        ali.facing_16, ali.sprite_frame_32 = 3, 0x15
        ali.delta_x_72, ali.delta_y_76 = -0x00010000, 0x00018000
        pose_t1 = (104, 196, 6, 8, 3, 0x15)
        step_pose_history_ring(shade, ctx)
        self.assertEqual((shade.x_pos_08, shade.y_pos_0c, shade.z_pos_10,
                          shade.ground_z_14, shade.facing_16, shade.sprite_frame_32), pose_t0)
        self.assertEqual(ctx.ring_slot1_19b4, pose_t0)
        self.assertEqual(ctx.ring_slot0_19a8, pose_t1)

        # Frame T2: Ali moves to pose_t2; Shade still reads primed slot1 (pose_t0), slot1 shifts to pose_t1
        ali.x_pos_08, ali.y_pos_0c, ali.z_pos_10 = 110, 190, 0
        ali.ground_z_14, ali.facing_16, ali.sprite_frame_32 = 12, 0, 0x18
        ali.delta_x_72, ali.delta_y_76 = 0, 0
        pose_t2 = (110, 190, 0, 12, 0, 0x18)
        step_pose_history_ring(shade, ctx)
        self.assertEqual((shade.x_pos_08, shade.y_pos_0c, shade.z_pos_10,
                          shade.ground_z_14, shade.facing_16, shade.sprite_frame_32), pose_t0)
        self.assertEqual(ctx.ring_slot1_19b4, pose_t1)
        self.assertEqual(ctx.ring_slot0_19a8, pose_t2)

        # Frame T3: Ali moves to pose_t3; Shade now mirrors pose_t1 (true 2-frame delay)
        ali.x_pos_08, ali.y_pos_0c, ali.facing_16 = 115, 185, 2
        pose_t3 = (115, 185, 0, 12, 2, 0x18)
        step_pose_history_ring(shade, ctx)
        self.assertEqual((shade.x_pos_08, shade.y_pos_0c, shade.z_pos_10,
                          shade.ground_z_14, shade.facing_16, shade.sprite_frame_32), pose_t1)
        self.assertEqual(ctx.ring_slot1_19b4, pose_t2)
        self.assertEqual(ctx.ring_slot0_19a8, pose_t3)


if __name__ == "__main__":
    unittest.main()
