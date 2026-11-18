#!/usr/bin/env python3
"""
Unit tests for ROM Dytto Spirit Idle Follow, Auto-Rescue Heal Trigger,
Circular Orbit & Dismissal Physics Subsystem (0x0120D4..0x0123E2, 782 bytes).
Milestone M15.488.
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
    / "rom_dytto_follow_dismissal_m15_488.json"
)


def to_s8(val: int) -> int:
    val &= 0xFF
    return val - 0x100 if val & 0x80 else val


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val


def m68k_divs_w(num: int, den: int) -> int:
    return to_s16(int(num / den))


def rom_trig_high_byte(index_256: int) -> int:
    """High byte of 16-bit signed sin table ($05D706): round(32767 * sin) >> 8."""
    idx = index_256 & 0xFF
    if idx == 64:
        return 127
    if idx == 192:
        return -128
    val16 = int(round(32767.0 * math.sin(2.0 * math.pi * idx / 256.0)))
    return to_s8((val16 >> 8) & 0xFF)


def helper_00e45e_homing_velocity(
    orb_x: int, orb_y: int, target_x: int, target_y: int, facing_16: int = 0
) -> tuple[int, int, int, int]:
    d1 = to_s16(target_x - orb_x)
    d4 = abs(d1)
    d2 = to_s16(target_y - orb_y)
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


@dataclass
class DyttoEntity:
    type_00: int = 0x0018
    state_04: int = 0
    x_08: int = 160
    y_0c: int = 120
    z_10_fixed: int = 0
    z_14: int = 0
    facing_16: int = 0
    priority_18: int = 0
    sprite_table_1a: int = 0
    action_2a: int = 0
    anim_2c: int = 0
    anim_2e: int = 0
    anim_frame_30: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    vel_z_56: int = 0
    vel_x_72: int = 0
    vel_y_76: int = 0
    max_hp_88: int = 100
    hp_8a: int = 100
    sp_8e: int = 50
    orbit_angle_9e: int = 0
    orbit_dir_flags_9f: int = 0
    orbit_timer_a0: int = 0
    orbit_delay_a1: int = 0
    auto_heal_cd_aa: int = 0

    @property
    def z_10_word(self) -> int:
        return to_s16((self.z_10_fixed >> 16) & 0xFFFF)

    @z_10_word.setter
    def z_10_word(self, val: int) -> None:
        self.z_10_fixed = ((val & 0xFFFF) << 16) | (self.z_10_fixed & 0xFFFF)


@dataclass
class SimCtx:
    ali: DyttoEntity = field(default_factory=lambda: DyttoEntity(type_00=0x0001))
    jewel_rank_ff0db6: int = 0
    cmd_ff19a2: int = 0
    dismiss_ff19a7: int = 0
    rng_val: int = 0x02
    sfx_e7fc: list[int] = field(default_factory=list)
    audio_60004: list[int] = field(default_factory=list)


def track_target_elevation(dytto: DyttoEntity, ali: DyttoEntity, offset_d4: int) -> None:
    """Simulation of AI_Dytto_TrackTargetElevation (0x0123AE / 0x0123B4)."""
    d5 = to_s16(ali.z_14)
    d0 = to_s16(dytto.z_14)
    if d0 < 0:
        d0 = d5
    d5 = to_s16(d5 + offset_d4)
    d4 = dytto.z_10_word
    delta = to_s16(d0 + d4 - d5)
    if delta == 0:
        return
    if delta < 0:
        d4 = to_s16(d4 + 1)
    else:
        if d4 <= 0:
            return
        d4 = to_s16(d4 - 1)
    dytto.z_10_word = d4


def homing_velocity_01215e(dytto: DyttoEntity, ali: DyttoEntity) -> tuple[int, int]:
    """Simulation of 0x01215E..0x01218A homing setup (returns raw doubled vx, vy)."""
    tx = ali.x_08
    ty = ali.y_0c
    if dytto.z_14 >= 0 and ali.z_14 >= 0:
        ty = to_s16(ty + to_s16(ali.z_14 - dytto.z_14))
    vx, vy, d3, _ = helper_00e45e_homing_velocity(dytto.x_08, dytto.y_0c, tx, ty, dytto.facing_16)
    d1 = to_s32(vx * 2)
    d2 = to_s32(vy * 2)
    dytto.vel_x_72 = d1
    dytto.vel_y_76 = d2
    dytto.facing_16 = d3
    return d1, d2


def state04_approach(dytto: DyttoEntity, ctx: SimCtx) -> str:
    track_target_elevation(dytto, ctx.ali, 0x30)
    dist_manhattan = abs(to_s16(ctx.ali.x_08 - dytto.x_08)) + abs(to_s16(ctx.ali.y_0c - dytto.y_0c))
    if dist_manhattan <= 0x20:
        dytto.state_04 = 0x06
        dytto.action_2a = 0x16
        return "ENTERED_STATE_06"
    homing_velocity_01215e(dytto, ctx.ali)
    return "HOMING_STATE_04"


def state02_materialize(dytto: DyttoEntity, ctx: SimCtx) -> str:
    if dytto.anim_flags_37 & 0x02:
        dytto.state_04 = 0x04
        dytto.action_2a = 0x08
        return state04_approach(dytto, ctx)
    track_target_elevation(dytto, ctx.ali, 0x30)
    d1, d2 = homing_velocity_01215e(dytto, ctx.ali)
    dytto.vel_x_72 = d1 >> 1
    dytto.vel_y_76 = d2 >> 1
    return "MATERIALIZING"


def state06_summon_finish(dytto: DyttoEntity, ctx: SimCtx) -> str:
    if dytto.anim_flags_37 & 0x02:
        dytto.state_04 = 0x00
        return state00_idle_tick(dytto, ctx)
    track_target_elevation(dytto, ctx.ali, 0x30)
    d1, d2 = homing_velocity_01215e(dytto, ctx.ali)
    dytto.vel_x_72 = d1 >> 1
    dytto.vel_y_76 = d2 >> 1
    return "FINISHING"


def state1a_anim_settle(dytto: DyttoEntity, ctx: SimCtx) -> str:
    if (dytto.anim_flags_37 & 0x02) == 0:
        return "SETTLING"
    if dytto.action_2a != 0:
        dytto.action_2a = 0
        return "CLEARED_ACTION"
    dytto.state_04 = 0x00
    return state00_idle_tick(dytto, ctx)


def start_dismissal_01231c(dytto: DyttoEntity, ctx: SimCtx) -> str:
    ctx.sfx_e7fc.append(0x00A0039E)
    dytto.facing_16 = 2
    dytto.state_04 = 0x1C
    dytto.action_2a = 0x26
    dytto.anim_2c = 0xFFFF
    dytto.anim_flags_37 &= 0xF9
    dytto.anim_frame_30 = 0
    dytto.anim_2e = 0
    dytto.vel_z_56 = 0
    return "DISMISS_STATE_1C"


def state1c_dismissal_fall(dytto: DyttoEntity) -> str:
    dytto.flags_36 &= ~0x02
    if dytto.anim_frame_30 < 0x14:
        return "WAIT_FALL"
    dytto.vel_z_56 = to_s32(dytto.vel_z_56 - 0x0000C000)
    dytto.z_10_fixed = to_s32(dytto.z_10_fixed + dytto.vel_z_56)
    if dytto.z_10_fixed >= 0:
        return "FALLING"
    dytto.z_10_fixed = 0
    dytto.state_04 = 0x1E
    dytto.action_2a = 0x24
    dytto.sprite_table_1a = 0x00141374
    dytto.priority_18 = 0x07A8
    dytto.flags_36 = (dytto.flags_36 & ~0x01) | 0x08
    dytto.facing_16 = 0
    return "LANDED_STATE_1E"


def state1e_dismissal_despawn(dytto: DyttoEntity) -> str:
    dytto.flags_36 = (dytto.flags_36 & ~0x02) | 0x08
    if dytto.anim_flags_37 & 0x02:
        dytto.type_00 = 0
        return "DESPAWNED"
    return "WAITING_DESPAWN"


def state00_idle_tick(dytto: DyttoEntity, ctx: SimCtx, skip_cmd: bool = False) -> str:
    if not skip_cmd:
        if (ctx.dismiss_ff19a7 & 0xFF) != 0:
            return start_dismissal_01231c(dytto, ctx)
        if ctx.cmd_ff19a2 in (1, 2, 3):
            return f"CMD_{ctx.cmd_ff19a2}"
    else:
        ctx.audio_60004.append(0x00A00253)

    if dytto.auto_heal_cd_aa != 0:
        dytto.auto_heal_cd_aa = (dytto.auto_heal_cd_aa - 1) & 0xFF
    else:
        if (ctx.jewel_rank_ff0db6 & 0xFF) >= 3 and ctx.ali.hp_8a < (ctx.ali.max_hp_88 >> 3):
            if to_s16(ctx.ali.sp_8e - 0x14) >= 1:
                ctx.ali.sp_8e -= 0x14
                dytto.auto_heal_cd_aa = 0x1E
                dytto.facing_16 = 0
                dytto.state_04 = 0x18
                dytto.action_2a = 0x0E
                return "AUTO_RESCUE_HEAL"

    track_target_elevation(dytto, ctx.ali, 0x30)
    tx, ty = ctx.ali.x_08, ctx.ali.y_0c
    if dytto.z_14 >= 0 and ctx.ali.z_14 >= 0:
        ty = to_s16(ty + to_s16(ctx.ali.z_14 - dytto.z_14))

    orbiting = False
    if dytto.orbit_delay_a1 != 0:
        dytto.orbit_delay_a1 = (dytto.orbit_delay_a1 - 1) & 0xFF
        if dytto.orbit_delay_a1 == 0 and dytto.orbit_timer_a0 != 0:
            orbiting = True
    elif dytto.orbit_timer_a0 != 0:
        orbiting = True

    if orbiting:
        dytto.orbit_timer_a0 = (dytto.orbit_timer_a0 - 1) & 0xFF
        delta_ang = -6 if (dytto.orbit_dir_flags_9f & 0x02) else 6
        dytto.orbit_angle_9e = (dytto.orbit_angle_9e + delta_ang) & 0xFF
        sin_b = rom_trig_high_byte(dytto.orbit_angle_9e)
        cos_b = rom_trig_high_byte(dytto.orbit_angle_9e + 64)
        dy_iso = (sin_b >> 3) + ((sin_b >> 3) >> 1)
        tx = to_s16(tx + (cos_b >> 2))
        ty = to_s16(ty + dy_iso)
    else:
        f = ctx.ali.facing_16 & 0xFF
        offsets = {0: (-0x12, 0x12), 1: (-0x12, -0x12), 2: (0x12, -0x12)}
        ox, oy = offsets.get(f, (0x12, 0x12))
        tx, ty = to_s16(tx + ox), to_s16(ty + oy)

    d1, d2, d3, d4 = helper_00e45e_homing_velocity(dytto.x_08, dytto.y_0c, tx, ty, dytto.facing_16)
    if d4 == 0:
        if dytto.orbit_delay_a1 == 0:
            dytto.orbit_timer_a0 = 0x78
            dytto.orbit_delay_a1 = 0x28
            dytto.orbit_dir_flags_9f = ctx.rng_val & 0xFF
        dytto.vel_x_72 = 0
        dytto.vel_y_76 = 0
        dytto.facing_16 = ctx.ali.facing_16
        dytto.action_2a = 0
        return "ORBIT" if orbiting else "IDLE_AT_TARGET"
    if d4 <= 2:
        dytto.vel_x_72 = d1 >> 1
        dytto.vel_y_76 = d2 >> 1
        dytto.facing_16 = ctx.ali.facing_16
        dytto.action_2a = 0
        return "ORBIT" if orbiting else "FOLLOW_SLOW"
    dytto.vel_x_72 = d1
    dytto.vel_y_76 = d2
    dytto.facing_16 = d3
    dytto.action_2a = 2
    return "ORBIT" if orbiting else "FOLLOW_FAST"


class TestROMDyttoFollowDismissal(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.488")
        self.assertEqual(self.evidence["range"], {"start": "0x0120D4", "end": "0x0123E2", "size_bytes": 782})
        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 4)
        self.assertEqual(sum(r["size_bytes"] for r in routines), 782)
        self.assertEqual(self.evidence["instruction_count"], 219)
        self.assertEqual(len(self.evidence["instructions"]), 219)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x0120D4")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x0123E0")

    def test_approach_states_02_06_1a_04_and_buzzer(self) -> None:
        # 1. State $02: half-speed homing until +$37 bit 1 -> State $04 (+$2A=8)
        ctx = SimCtx(ali=DyttoEntity(x_08=220, y_0c=120, z_14=10))
        dytto = DyttoEntity(state_04=0x02, x_08=160, y_0c=120, z_14=0)
        self.assertEqual(state02_materialize(dytto, ctx), "MATERIALIZING")
        self.assertEqual((dytto.vel_x_72, dytto.vel_y_76), (0x00020000, 0x00005500))  # half of doubled (4, ~0.66)
        dytto.anim_flags_37 = 0x02
        self.assertEqual(state02_materialize(dytto, ctx), "HOMING_STATE_04")
        self.assertEqual((dytto.state_04, dytto.action_2a), (0x04, 0x08))
        self.assertEqual((dytto.vel_x_72, dytto.vel_y_76), (0x00040000, 0x0000AA00))  # full doubled speed

        # 2. State $04 enters State $06 (+$2A=$16) when Manhattan dist <= 0x20
        dytto.x_08, dytto.y_0c = 200, 108  # |220-200| + |120-108| = 32 <= 0x20
        self.assertEqual(state04_approach(dytto, ctx), "ENTERED_STATE_06")
        self.assertEqual((dytto.state_04, dytto.action_2a), (0x06, 0x16))

        # 3. State $06 finishes into State $00 on +$37 bit 1
        dytto.anim_flags_37 = 0x00
        self.assertEqual(state06_summon_finish(dytto, ctx), "FINISHING")
        dytto.anim_flags_37 = 0x02
        state06_summon_finish(dytto, ctx)
        self.assertEqual(dytto.state_04, 0x00)

        # 4. State $1A two-step settle: first clears +$2A, next enters State $00
        dytto = DyttoEntity(state_04=0x1A, action_2a=0x14, anim_flags_37=0x02)
        self.assertEqual(state1a_anim_settle(dytto, ctx), "CLEARED_ACTION")
        self.assertEqual((dytto.action_2a, dytto.state_04), (0, 0x1A))
        state1a_anim_settle(dytto, ctx)
        self.assertEqual(dytto.state_04, 0x00)

        # 5. Insufficient-SP buzzer plays #$00A00253 and enters 0x0121D2 follow path
        ctx.cmd_ff19a2 = 2
        state00_idle_tick(dytto, ctx, skip_cmd=True)
        self.assertEqual(ctx.audio_60004, [0x00A00253])

    def test_state00_command_dispatch_and_auto_rescue_heal(self) -> None:
        # 1. Dismissal & command dispatch priority
        ctx = SimCtx(dismiss_ff19a7=1, cmd_ff19a2=2)
        dytto = DyttoEntity()
        self.assertEqual(state00_idle_tick(dytto, ctx), "DISMISS_STATE_1C")
        self.assertEqual((dytto.state_04, ctx.sfx_e7fc), (0x1C, [0x00A0039E]))

        for code in (1, 2, 3):
            self.assertEqual(state00_idle_tick(DyttoEntity(), SimCtx(cmd_ff19a2=code)), f"CMD_{code}")

        # 2. Auto-rescue heal gates: cooldown == 0, rank >= 3, hp < max_hp>>3, sp - 20 >= 1
        dytto = DyttoEntity(auto_heal_cd_aa=2)
        ctx = SimCtx(ali=DyttoEntity(max_hp_88=80, hp_8a=9, sp_8e=30), jewel_rank_ff0db6=3)
        self.assertNotEqual(state00_idle_tick(dytto, ctx), "AUTO_RESCUE_HEAL")
        self.assertEqual(dytto.auto_heal_cd_aa, 1)

        dytto.auto_heal_cd_aa = 0
        ctx.jewel_rank_ff0db6 = 2  # rank < 3 blocked
        self.assertNotEqual(state00_idle_tick(dytto, ctx), "AUTO_RESCUE_HEAL")

        ctx.jewel_rank_ff0db6 = 3
        ctx.ali.hp_8a = 10  # 80 >> 3 == 10, bls skips when hp >= threshold
        self.assertNotEqual(state00_idle_tick(dytto, ctx), "AUTO_RESCUE_HEAL")

        ctx.ali.hp_8a = 9
        ctx.ali.sp_8e = 20  # sp - 20 == 0 < 1 blocked
        self.assertNotEqual(state00_idle_tick(dytto, ctx), "AUTO_RESCUE_HEAL")

        ctx.ali.sp_8e = 21  # succeeds: consumes 20 SP, sets cd=30, State $18, +$2A=$0E (no SFX)
        self.assertEqual(state00_idle_tick(dytto, ctx), "AUTO_RESCUE_HEAL")
        self.assertEqual((ctx.ali.sp_8e, dytto.auto_heal_cd_aa, dytto.state_04, dytto.action_2a), (1, 0x1E, 0x18, 0x0E))
        self.assertEqual(ctx.sfx_e7fc, [])

    def test_state00_idle_follow_and_circular_orbit(self) -> None:
        # 1. 4-way 18px behind-Ali follow offsets (Up=0, Right=1, Down=2, Left=3)
        expected_targets = {0: (100 - 18, 100 + 18), 1: (100 - 18, 100 - 18), 2: (100 + 18, 100 - 18), 3: (100 + 18, 100 + 18)}
        for facing, (tx, ty) in expected_targets.items():
            ctx = SimCtx(ali=DyttoEntity(x_08=100, y_0c=100, facing_16=facing), rng_val=0x00)
            dytto = DyttoEntity(x_08=tx, y_0c=ty, facing_16=3)
            self.assertEqual(state00_idle_tick(dytto, ctx), "IDLE_AT_TARGET")
            self.assertEqual((dytto.facing_16, dytto.action_2a), (facing, 0))
            self.assertEqual((dytto.orbit_timer_a0, dytto.orbit_delay_a1), (0x78, 0x28))

        # 2. Fast (d4 > 2) and slow (1 <= d4 <= 2) follow velocity tiers
        ctx = SimCtx(ali=DyttoEntity(x_08=100, y_0c=100, facing_16=2))
        d_fast = DyttoEntity(x_08=100, y_0c=100, orbit_delay_a1=5)  # target (118, 82) -> d4=9 > 2
        self.assertEqual(state00_idle_tick(d_fast, ctx), "FOLLOW_FAST")
        self.assertEqual((d_fast.action_2a, d_fast.vel_x_72, d_fast.vel_y_76), (2, 0x00020000, -0x00020000))

        d_slow = DyttoEntity(x_08=114, y_0c=82, orbit_delay_a1=5)  # dx=4, dy=0 -> d4=2 -> halved
        self.assertEqual(state00_idle_tick(d_slow, ctx), "FOLLOW_SLOW")
        self.assertEqual((d_slow.action_2a, d_slow.facing_16, d_slow.vel_x_72), (0, 2, 0x00010000))

        # 3. Circular orbit: +6 (bit 1 clear) vs -6 (bit 1 set) and isometric trig scaling
        d_cw = DyttoEntity(x_08=100, y_0c=100, orbit_timer_a0=10, orbit_delay_a1=1, orbit_angle_9e=58, orbit_dir_flags_9f=0x00)
        self.assertEqual(state00_idle_tick(d_cw, SimCtx(ali=DyttoEntity(x_08=100, y_0c=100))), "ORBIT")
        self.assertEqual((d_cw.orbit_angle_9e, d_cw.orbit_timer_a0, d_cw.orbit_delay_a1), (64, 9, 0))
        # At angle=64: cos_b=0 -> dx=0, sin_b=127 -> (127>>3)+(15>>1)=22 -> target=(100, 122), d4=11
        self.assertEqual((d_cw.vel_x_72, d_cw.vel_y_76), (0, 0x00020000))

        d_ccw = DyttoEntity(x_08=100, y_0c=100, orbit_timer_a0=10, orbit_delay_a1=0, orbit_angle_9e=6, orbit_dir_flags_9f=0x02)
        state00_idle_tick(d_ccw, SimCtx(ali=DyttoEntity(x_08=100, y_0c=100)))
        self.assertEqual(d_ccw.orbit_angle_9e, 0)  # 6 - 6 = 0; target=(131, 100)

    def test_dismissal_physics_states_1c_1e_and_elevation_tracker(self) -> None:
        # 1. Dismissal physics State $1C gravity (-$C000) -> ground impact State $1E -> despawn
        dytto = DyttoEntity(state_04=0x1C, anim_frame_30=19, flags_36=0x03)
        dytto.z_10_fixed = 0x00010000
        self.assertEqual(state1c_dismissal_fall(dytto), "WAIT_FALL")
        self.assertEqual(dytto.flags_36 & 0x02, 0)

        dytto.anim_frame_30 = 20
        self.assertEqual(state1c_dismissal_fall(dytto), "FALLING")  # vz=-$C000, z=+$4000
        self.assertEqual((dytto.vel_z_56, dytto.z_10_fixed), (-0x0000C000, 0x00004000))
        self.assertEqual(state1c_dismissal_fall(dytto), "LANDED_STATE_1E")  # vz=-$18000, z<0 -> clamp 0
        self.assertEqual((dytto.z_10_fixed, dytto.state_04, dytto.action_2a), (0, 0x1E, 0x24))
        self.assertEqual((dytto.sprite_table_1a, dytto.priority_18, dytto.flags_36, dytto.facing_16), (0x00141374, 0x07A8, 0x08, 0))

        self.assertEqual(state1e_dismissal_despawn(dytto), "WAITING_DESPAWN")
        dytto.anim_flags_37 = 0x02
        self.assertEqual(state1e_dismissal_despawn(dytto), "DESPAWNED")
        self.assertEqual(dytto.type_00, 0)

        # 2. Elevation tracker 0x0123AE (+8) and 0x0123B4 (+48) with negative z_14 fallback & >= 0 clamp
        ali = DyttoEntity(z_14=10)
        d_el = DyttoEntity(z_14=-1)  # negative z_14 falls back to ali.z_14 (10)
        d_el.z_10_word = 0
        track_target_elevation(d_el, ali, 0x30)  # (10 + 0) - (10 + 48) < 0 -> +1
        self.assertEqual(d_el.z_10_word, 1)

        d_el.z_14 = 0
        d_el.z_10_word = 0
        track_target_elevation(d_el, ali, 0x08)  # (0 + 0) - (10 + 8) < 0 -> +1
        self.assertEqual(d_el.z_10_word, 1)

        d_floor = DyttoEntity(z_14=30)
        d_floor.z_10_word = 0
        track_target_elevation(d_floor, ali, 0x08)  # (30 + 0) - (10 + 8) > 0, but z_10 == 0 clamps at 0
        self.assertEqual(d_floor.z_10_word, 0)


if __name__ == "__main__":
    unittest.main()
