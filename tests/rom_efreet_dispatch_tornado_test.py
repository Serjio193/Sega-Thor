#!/usr/bin/env python3
"""
Unit tests for ROM Dytto Water Tornado Steering/Bounce & Efreet Spirit Master
14-State Dispatcher / Command Starters (0x0123E2..0x012680, 670 bytes).
Milestone M15.489.
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
    / "rom_efreet_dispatch_tornado_m15_489.json"
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


def rom_trig_high_byte(index_256: int) -> int:
    idx = index_256 & 0xFF
    if idx == 64:
        return 127
    if idx == 192:
        return -128
    val16 = int(round(32767.0 * math.sin(2.0 * math.pi * idx / 256.0)))
    return to_s8((val16 >> 8) & 0xFF)


def angle_to_sector_facing(angle: int) -> int:
    a = angle & 0xFF
    d0 = 1
    if a < 0x20:
        return d0
    d0 -= 1
    if a < 0x60:
        return d0
    d0 += 3
    if a < 0xA0:
        return d0
    d0 -= 1
    if a < 0xE0:
        return d0
    return d0 - 1


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
class TornadoState:
    x_08: int = 160
    y_0c: int = 120
    z_10_word: int = 10
    z_14: int = 0
    facing_16: int = 0
    flags_40: int = 0
    flags_41: int = 0
    angle_4e: int = 0
    turn_rate_50: int = 0
    drift_timer_51: int = 7
    speed_52: int = 0x00020000
    vel_x_72: int = 0
    vel_y_76: int = 0


@dataclass
class TornadoEnv:
    ali_x: int = 160
    ali_y: int = 120
    ali_z14: int = 0
    cam_a_x: int = 0
    cam_a_y: int = 0
    cam_b_x: int = 0
    cam_b_y: int = 0
    plane_flag_ff16f0: int = 0
    input_ff165e: int = 0
    hold_counter_ff199e: int = 10
    aim_angle_override: int | None = None
    prng_val: int = 0


def step_water_tornado_steering(t: TornadoState, env: TornadoEnv) -> None:
    """Faithful 68K simulation of AI_Dytto_WaterTornadoSteeringAndTrigVelocity."""
    self_surf = t.z_14 if t.z_14 >= 0 else env.ali_z14
    delta_z = to_s16(self_surf + t.z_10_word - (env.ali_z14 + 8))
    if delta_z < 0:
        t.z_10_word = to_s16(t.z_10_word + 1)
    elif delta_z > 0 and t.z_10_word > 0:
        t.z_10_word = to_s16(t.z_10_word - 1)

    d3 = t.angle_4e & 0xFFFF
    d4 = to_s8(t.turn_rate_50)
    d5 = t.drift_timer_51 & 0xFF
    d6 = t.speed_52 & 0xFFFFFFFF

    if t.flags_40 & 0x40:
        d3 = (0x80 - d3) & 0xFFFF if d3 <= 0x80 else (0x180 - d3) & 0xFFFF
        d4 = to_s8(-d4)
    if t.flags_41 & 0x40:
        d3 = ((0xFF - (d3 & 0xFF) + 1) & 0xFF)
        d4 = to_s8(-d4)

    cam_x, cam_y = (env.cam_b_x, env.cam_b_y) if (env.plane_flag_ff16f0 & 0x80) else (env.cam_a_x, env.cam_a_y)
    if t.x_08 < cam_x:
        d3, d4, d5 = 0x00, d4 >> 1, 1
    if t.x_08 > cam_x + 0x140:
        d3, d4, d5 = 0x80, d4 >> 1, 1
    if t.y_0c < cam_y:
        d3, d4, d5 = 0xC0, d4 >> 1, 1
    if t.y_0c > cam_y + 0xE0:
        d3, d4, d5 = 0x40, d4 >> 1, 1

    if env.input_ff165e & 0x40:
        env.hold_counter_ff199e = 0
        target_ang = env.aim_angle_override if env.aim_angle_override is not None else (
            int(round(math.atan2(t.y_0c - env.ali_y, env.ali_x - t.x_08) * 128.0 / math.pi)) & 0xFF
        )
        ccw = (d3 - target_ang) & 0xFF
        cw = (target_ang - d3) & 0xFF
        if ccw < cw:
            step = -min(ccw, 8)
        else:
            step = min(cw, 8)
        d3 = (d3 + step) & 0xFF
        d4 = 0
        d5 = 0x0A
    else:
        d5 = (d5 - 1) & 0xFF
        if d5 == 0:
            d5 = 7
            d4 = to_s8((env.prng_val & 0x08) - 4)
        d3 = (d3 + d4) & 0xFF

    if d6 <= 0x00080000:
        d6 = (d6 + 0x00002000) & 0xFFFFFFFF

    d3 &= 0xFF
    t.angle_4e = d3
    t.turn_rate_50 = to_s8(d4)
    t.drift_timer_51 = d5
    t.speed_52 = d6
    t.facing_16 = angle_to_sector_facing(d3)

    speed_word = to_s16((to_s32(d6) >> 7) & 0xFFFF)
    sin_b = rom_trig_high_byte(d3)
    cos_b = rom_trig_high_byte((d3 + 0x40) & 0xFF)
    t.vel_x_72 = to_s32(t.vel_x_72 + speed_word * cos_b)
    t.vel_y_76 = to_s32(t.vel_y_76 - speed_word * sin_b)


@dataclass
class EfreetSim:
    state_04: int = 0
    timer_02: int = 5
    facing_16: int = 0
    action_2a: int = 0
    flags_36: int = 0
    vel_x_4e: int = 0
    vel_y_52: int = 0
    drain_timer_8e: int = 0x23
    sub_entity_a2: int = 0
    flame_slot_ptr_a6: int = 0
    field_ae: int = 0
    field_af: int = 0
    ali_facing_16: int = 1
    ali_sp_8e: int = 40
    efreet_rank_ff0db7: int = 2
    slot0_type_ff1c1c: int = 0
    hitstun_block: bool = False
    polled_input_count: int = 0
    sfx_log: list[int] = field(default_factory=list)
    helper_12c4e_calls: int = 0


def efreet_master_tick(e: EfreetSim, jump_table: dict[str, str]) -> str:
    if e.sub_entity_a2 == 0:
        if e.hitstun_block:
            return "EARLY_RETURN_00E682"
        e.polled_input_count += 1
        if e.drain_timer_8e != 0:
            e.drain_timer_8e -= 1
        else:
            e.drain_timer_8e = 0x23
            if e.ali_sp_8e == 0:
                return "DISMISS_012BA8"
            e.ali_sp_8e -= 1
            if e.ali_sp_8e == 0:
                return "DISMISS_012BA8"
        if e.ali_sp_8e <= 1:
            e.flags_36 ^= 0x02
        else:
            e.flags_36 &= ~0x02
    return jump_table[f"0x{e.state_04:02X}"]


def efreet_try_start_dash(e: EfreetSim) -> str:
    if to_s16(e.ali_sp_8e - 8) < 1:
        return "INSUFFICIENT_SP_012978"
    e.ali_sp_8e -= 8
    e.state_04 = 0x08
    e.action_2a = 0x0C
    e.facing_16 = e.ali_facing_16
    e.vel_x_4e, e.vel_y_52 = helper_00e2d4_velocity(0x0100, 0x0100, e.facing_16)
    e.sfx_log.append(0x00800258)
    return "STARTED_DASH_08"


def efreet_try_start_flame(e: EfreetSim) -> str:
    if to_s16(e.ali_sp_8e - 6) < 1 or e.slot0_type_ff1c1c != 0:
        return "REJECTED_012978"
    e.ali_sp_8e -= 6
    e.slot0_type_ff1c1c = 0xFFFE
    e.flame_slot_ptr_a6 = 0x00FF1C1C
    e.helper_12c4e_calls += 1
    e.state_04 = 0x0A
    e.action_2a = 0x02
    return "STARTED_FLAME_0A"


def efreet_try_start_meltdown(e: EfreetSim) -> str:
    if to_s16(e.ali_sp_8e - 0x19) < 1:
        return "INSUFFICIENT_SP_012978"
    e.ali_sp_8e -= 0x19
    e.state_04 = 0x0C
    e.action_2a = 0x08
    e.facing_16 = 2
    e.field_ae = 0x60
    e.timer_02 = 0
    e.field_af = (e.efreet_rank_ff0db7 + 0x10) & 0xFF
    return "STARTED_MELTDOWN_0C"


def efreet_start_hit_reaction(e: EfreetSim) -> None:
    e.state_04 = 0x10
    e.action_2a = 0x0A


class TestROMEfreetDispatchTornado(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
        self.jump_map = {e["state"]: e["target"] for e in self.evidence["jump_table"]["entries"]}

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.489")
        self.assertEqual(self.evidence["range"], {"start": "0x0123E2", "end": "0x012680", "size_bytes": 670})

        jt = self.evidence["jump_table"]
        self.assertEqual((jt["address"], jt["end"], jt["size_bytes"]), ("0x0125AE", "0x0125CA", 28))
        expected_targets = {
            "0x00": "0x012988", "0x02": "0x012988", "0x04": "0x012680", "0x06": "0x012BF0",
            "0x08": "0x0126AC", "0x0A": "0x0127A4", "0x0C": "0x012802", "0x0E": "0x0127C0",
            "0x10": "0x012988", "0x12": "0x0126CE", "0x14": "0x012702", "0x16": "0x012914",
            "0x18": "0x012818", "0x1A": "0x01284C",
        }
        self.assertEqual(self.jump_map, expected_targets)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 6)
        self.assertEqual(sum(r["size_bytes"] for r in routines), 670)
        self.assertEqual(self.evidence["instruction_count"], 188)
        self.assertEqual(len(self.evidence["instructions"]), 188)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x0123E2")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x01267E")

    def test_tornado_wall_reflection_and_viewport_bounce(self) -> None:
        # 1. Horizontal wall bounce (+$40 bit 6): angle <= 0x80 and > 0x80, negates turn rate
        t = TornadoState(flags_40=0x40, angle_4e=0x20, turn_rate_50=4, drift_timer_51=5)
        step_water_tornado_steering(t, TornadoEnv())
        self.assertEqual((t.angle_4e, t.turn_rate_50), (0x5C, -4))  # 0x80-0x20=0x60, then +(-4)=0x5C

        t2 = TornadoState(flags_40=0x40, angle_4e=0xE0, turn_rate_50=4, drift_timer_51=5)
        step_water_tornado_steering(t2, TornadoEnv())
        self.assertEqual((t2.angle_4e, t2.turn_rate_50), (0x9C, -4))  # 0x180-0xE0=0xA0, then +(-4)=0x9C

        # 2. Vertical wall bounce (+$41 bit 6): (256 - angle) & 0xFF, negates turn rate
        t3 = TornadoState(flags_41=0x40, angle_4e=0x40, turn_rate_50=6, drift_timer_51=5)
        step_water_tornado_steering(t3, TornadoEnv())
        self.assertEqual((t3.angle_4e, t3.turn_rate_50), (0xBA, -6))  # 0x100-0x40=0xC0, then +(-6)=0xBA

        # 3. Viewport edge bounce using Plane A ($FF1716) vs Plane B ($FF173E via $FF16F0 bit 7)
        env = TornadoEnv(cam_a_x=100, cam_a_y=100, cam_b_x=500, cam_b_y=400, plane_flag_ff16f0=0x80, prng_val=0x08)
        for x, y, expected_ang in [(499, 450, 0x00), (821, 450, 0x80), (600, 399, 0xC0), (600, 625, 0x40)]:
            tb = TornadoState(x_08=x, y_0c=y, angle_4e=0x25, turn_rate_50=-8, drift_timer_51=5)
            step_water_tornado_steering(tb, env)
            # timer=1 decrements to 0 -> reloads 7 & PRNG (0x08 & 8)-4 = +4 -> expected_ang + 4
            self.assertEqual((tb.angle_4e, tb.turn_rate_50, tb.drift_timer_51), ((expected_ang + 4) & 0xFF, 4, 7))

    def test_tornado_steering_speed_ramp_and_trig_velocity(self) -> None:
        # 1. Button A aimed steering: shortest-arc wraparound (+8 and -8) + clearing $FF199E + timer=10
        t_cw = TornadoState(angle_4e=250, turn_rate_50=7, drift_timer_51=3, speed_52=0x7E000)
        env_cw = TornadoEnv(input_ff165e=0x40, hold_counter_ff199e=19, aim_angle_override=10)
        step_water_tornado_steering(t_cw, env_cw)
        self.assertEqual((env_cw.hold_counter_ff199e, t_cw.angle_4e, t_cw.turn_rate_50, t_cw.drift_timer_51), (0, 2, 0, 10))
        self.assertEqual(t_cw.speed_52, 0x80000)

        t_ccw = TornadoState(angle_4e=10, turn_rate_50=7, speed_52=0x80000)
        step_water_tornado_steering(t_ccw, TornadoEnv(input_ff165e=0x40, aim_angle_override=250))
        self.assertEqual((t_ccw.angle_4e, t_ccw.speed_52), (2, 0x82000))  # 0x80000 <= 0x80000 ramps once more
        step_water_tornado_steering(t_ccw, TornadoEnv(input_ff165e=0x40, aim_angle_override=250))
        self.assertEqual(t_ccw.speed_52, 0x82000)  # > 0x80000 stops ramping

        # 2. Sector facing boundaries ($20/$60/$A0/$E0 -> 1,0,3,2,1)
        for ang, exp_facing in [(0x1F, 1), (0x20, 0), (0x5F, 0), (0x60, 3), (0x9F, 3), (0xA0, 2), (0xDF, 2), (0xE0, 1)]:
            self.assertEqual(angle_to_sector_facing(ang), exp_facing)

        # 3. Trig velocity integration at angle 64 (Up): speed 0x80000 -> >>7 = 4096; cos=0, sin=127
        t_up = TornadoState(angle_4e=64, turn_rate_50=0, drift_timer_51=5, speed_52=0x7E000)
        step_water_tornado_steering(t_up, TornadoEnv())
        self.assertEqual((t_up.facing_16, t_up.vel_x_72, t_up.vel_y_76), (0, 0, -4096 * 127))

    def test_efreet_master_tick_sp_drain_and_dispatcher(self) -> None:
        # 1. Sub-entity +$A2 != 0 bypasses input, hitstun, drain, blink directly to dispatcher
        sub = EfreetSim(state_04=0x0C, sub_entity_a2=0xFF, ali_sp_8e=0, drain_timer_8e=0, flags_36=0x82, hitstun_block=True)
        self.assertEqual(efreet_master_tick(sub, self.jump_map), "0x012802")
        self.assertEqual((sub.polled_input_count, sub.drain_timer_8e, sub.flags_36), (0, 0, 0x82))

        # 2. Hitstun early return 0x00E682 blocks master tick
        stun = EfreetSim(hitstun_block=True)
        self.assertEqual(efreet_master_tick(stun, self.jump_map), "EARLY_RETURN_00E682")

        # 3. 35-frame ($23) SP drain cadence & low-SP blink bit 1 on +$36
        ef = EfreetSim(state_04=0x00, drain_timer_8e=0x23, ali_sp_8e=2, flags_36=0x82)
        for rem_timer in range(0x22, -1, -1):
            self.assertEqual(efreet_master_tick(ef, self.jump_map), "0x012988")
            self.assertEqual((ef.drain_timer_8e, ef.ali_sp_8e, ef.flags_36), (rem_timer, 2, 0x80))
        # Drain 2 -> 1 reloads 0x23 and toggles bit 1 on
        self.assertEqual(efreet_master_tick(ef, self.jump_map), "0x012988")
        self.assertEqual((ef.drain_timer_8e, ef.ali_sp_8e, ef.flags_36), (0x23, 1, 0x82))
        self.assertEqual(efreet_master_tick(ef, self.jump_map), "0x012988")
        self.assertEqual(ef.flags_36, 0x80)
        # Drain 1 -> 0 dismisses via 0x012BA8
        ef.drain_timer_8e = 0
        self.assertEqual(efreet_master_tick(ef, self.jump_map), "DISMISS_012BA8")
        self.assertEqual(ef.ali_sp_8e, 0)

    def test_efreet_command_starters_and_hit_reaction(self) -> None:
        # 1. Dash Charge (0x0125CA, $FF19A2==2): 8 SP, State $08, +$2A=$0C, 0x00E2D4 velocity, SFX #$800258
        e_low = EfreetSim(ali_sp_8e=8)
        self.assertEqual(efreet_try_start_dash(e_low), "INSUFFICIENT_SP_012978")
        for facing, exp_vel in [(0, (0, -0x10000)), (1, (0x10000, 0)), (2, (0, 0x10000)), (3, (-0x10000, 0))]:
            e = EfreetSim(ali_sp_8e=9, ali_facing_16=facing)
            self.assertEqual(efreet_try_start_dash(e), "STARTED_DASH_08")
            self.assertEqual((e.ali_sp_8e, e.state_04, e.action_2a, e.facing_16), (1, 0x08, 0x0C, facing))
            self.assertEqual((e.vel_x_4e, e.vel_y_52, e.sfx_log), (*exp_vel, [0x00800258]))

        # 2. Flame Breath (0x012608, $FF19A2==1): 6 SP + $FF1C1C slot 0 free -> $FFFE, +$A6, 0x012C4E, State $0A
        self.assertEqual(efreet_try_start_flame(EfreetSim(ali_sp_8e=6)), "REJECTED_012978")
        self.assertEqual(efreet_try_start_flame(EfreetSim(ali_sp_8e=20, slot0_type_ff1c1c=0x0018)), "REJECTED_012978")
        ef_fl = EfreetSim(ali_sp_8e=7)
        self.assertEqual(efreet_try_start_flame(ef_fl), "STARTED_FLAME_0A")
        self.assertEqual((ef_fl.ali_sp_8e, ef_fl.slot0_type_ff1c1c, ef_fl.flame_slot_ptr_a6), (1, 0xFFFE, 0x00FF1C1C))
        self.assertEqual((ef_fl.helper_12c4e_calls, ef_fl.state_04, ef_fl.action_2a), (1, 0x0A, 0x02))

        # 3. Fireball Meltdown (0x01263C, $FF19A2==3): 25 SP, State $0C, +$2A=8, facing 2, +$AE=$60, +$02=0, +$AF=$FF0DB7+$10
        self.assertEqual(efreet_try_start_meltdown(EfreetSim(ali_sp_8e=25)), "INSUFFICIENT_SP_012978")
        ef_mb = EfreetSim(ali_sp_8e=26, efreet_rank_ff0db7=3)
        self.assertEqual(efreet_try_start_meltdown(ef_mb), "STARTED_MELTDOWN_0C")
        self.assertEqual((ef_mb.ali_sp_8e, ef_mb.state_04, ef_mb.action_2a, ef_mb.facing_16), (1, 0x0C, 0x08, 2))
        self.assertEqual((ef_mb.field_ae, ef_mb.timer_02, ef_mb.field_af), (0x60, 0, 0x13))

        # 4. Hit Reaction (0x012672): State $10, +$2A=$0A
        ef_hit = EfreetSim()
        efreet_start_hit_reaction(ef_hit)
        self.assertEqual((ef_hit.state_04, ef_hit.action_2a), (0x10, 0x0A))


if __name__ == "__main__":
    unittest.main()
