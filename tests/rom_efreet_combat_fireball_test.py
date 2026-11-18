#!/usr/bin/env python3
"""
Unit tests for ROM Efreet Dash Charge Attack, Flame Breath State Controller &
Radial Fireball Supernova Spawner (0x012680..0x012988, 776 bytes).
Milestone M15.490.
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
    / "rom_efreet_combat_fireball_m15_490.json"
)

DASH_CLAMP = 0x00080000


def to_s8(val: int) -> int:
    val &= 0xFF
    return val - 0x100 if val & 0x80 else val


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def compute_8dir_velocity(vx_u16: int, vy_u16: int, direction: int) -> tuple[int, int]:
    """Model of 0x00E2D4 (8-dir 16.16 fixed-point velocity vector)."""
    vx = vx_u16 << 8
    vy = vy_u16 << 8
    d = direction & 7
    if d >= 4:
        vx = (vx >> 1) + (vx >> 2)
        vy = (vy >> 1) + (vy >> 2)
    return {
        0: (0, -vy),
        1: (vx, 0),
        2: (0, vy),
        3: (-vx, 0),
        4: (vx, -vy),
        5: (vx, vy),
        6: (-vx, vy),
        7: (-vx, -vy),
    }[d]


def compute_cardinal_facing(vx: int, vy: int) -> int:
    """Model of 0x00E338 (maps velocity vector to 0=N, 1=E, 2=S, 3=W)."""
    if abs(vx) <= abs(vy):
        return 0 if vy < 0 else 2
    return 3 if vx < 0 else 1


def q15_sin_hi(angle_byte: int) -> int:
    """Cleanroom Q15 high-byte sine matching $05D706 (256-step circle)."""
    q15 = int(math.floor(math.sin(2.0 * math.pi * (angle_byte & 0xFF) / 256.0) * 32767.0 + 0.5))
    if q15 > 32767:
        q15 = 32767
    return to_s8(q15 >> 8)


@dataclass
class EfreetEntity:
    type_id: int = 0x001A
    timer_02: int = 0
    state_04: int = 0
    piece_flag_06: int = 1
    x_pos_08: int = 0
    y_pos_0c: int = 0
    facing_16: int = 2
    vram_attr_18: int = 0x8000
    anim_2a: int = 0
    flags_36: int = 0x01
    anim_flags_37: int = 0
    vx_4e: int = 0
    vy_52: int = 0
    delta_x_72: int = 0
    delta_y_76: int = 0
    bonus_dmg_90: int = 0
    cooldown_9e: int = 0
    sub_entity_a2: int = 0
    flame_slot: list[int] = field(default_factory=lambda: [0xFFFE])
    orbit_angle_ae: int = 0x60
    fireball_rem_af: int = 0x10


@dataclass
class SimCtx:
    efreet_level_db7: int = 0
    frame_counter_1971: int = 0
    screen_shake_17c4: int = 0
    pool1_full: bool = False
    spawned_fireballs: list[EfreetEntity] = field(default_factory=list)
    attack_scans: list[tuple[int, int, int]] = field(default_factory=list)  # (idx, dmg90, facing16)
    sfx_log: list[int] = field(default_factory=list)
    flame_calls: list[str] = field(default_factory=list)


def return_to_idle(ent: EfreetEntity) -> None:
    """0x01269A..0x0126AC."""
    ent.state_04 = 0
    ent.anim_2a = 0
    ent.cooldown_9e = 0


def state_04_recovery(ent: EfreetEntity) -> None:
    """0x012680..0x01269A."""
    if (ent.anim_flags_37 & 0x02) != 0:
        ent.state_04 = 2
        ent.anim_2a = 0


def step_clamped_dash_velocity(ent: EfreetEntity) -> None:
    """0x01274C..0x0127A4."""
    d1, d2 = compute_8dir_velocity(0x0100, 0x0100, ent.facing_16)
    vx = max(-DASH_CLAMP, min(DASH_CLAMP, ent.vx_4e + d1))
    vy = max(-DASH_CLAMP, min(DASH_CLAMP, ent.vy_52 + d2))
    ent.vx_4e = vx
    ent.vy_52 = vy
    ent.delta_x_72 += vx
    ent.delta_y_76 += vy


def state_08_dash_startup(ent: EfreetEntity) -> None:
    """0x0126AC..0x0126CE."""
    step_clamped_dash_velocity(ent)
    if (ent.anim_flags_37 & 0x02) != 0:
        ent.state_04 = 0x12
        ent.anim_2a = 0x1A
        ent.timer_02 = 0x0A


def state_12_dash_active(ent: EfreetEntity, ctx: SimCtx) -> None:
    """0x0126CE..0x012702."""
    ent.bonus_dmg_90 = ((ctx.efreet_level_db7 & 0xFF) + 1) >> 2
    ctx.attack_scans.append((0, ent.bonus_dmg_90, ent.facing_16))
    ent.bonus_dmg_90 = 0
    step_clamped_dash_velocity(ent)
    ent.timer_02 = (ent.timer_02 - 1) & 0xFFFF
    if ent.timer_02 == 0:
        ent.state_04 = 0x14
        ent.anim_2a = 0x1C


def state_14_dash_decel(ent: EfreetEntity) -> None:
    """0x012702..0x01274C."""
    d1, d2 = compute_8dir_velocity(0x00C0, 0x00C0, ent.facing_16)
    ent.vx_4e -= d1
    ent.vy_52 -= d2
    ent.delta_x_72 += ent.vx_4e
    ent.delta_y_76 += ent.vy_52
    if (ent.anim_flags_37 & 0x02) != 0:
        ent.state_04 = 0
        ent.anim_2a = 0
        ent.cooldown_9e = 0x0E


def state_0a_flame_startup(ent: EfreetEntity) -> None:
    """0x0127A4..0x0127C0."""
    if (ent.anim_flags_37 & 0x02) != 0:
        ent.state_04 = 0x0E
        ent.anim_2a = 0x18
        ent.timer_02 = 0x46  # 70 frames


def state_0e_flame_active(ent: EfreetEntity, ctx: SimCtx) -> None:
    """0x0127C0..0x012802."""
    ent.timer_02 = (ent.timer_02 - 1) & 0xFFFF
    if ent.timer_02 == 0:
        ent.flame_slot[0] = 0
        ent.piece_flag_06 = 0
        return_to_idle(ent)
        return
    if ent.timer_02 >= 0x14 and (ent.timer_02 & 3) == 0:
        ctx.flame_calls.append(f"0x012DFA@{ent.timer_02}")
        ctx.sfx_log.append(0x0080034F)
    ctx.flame_calls.extend(["0x012CDC", "0x012EA8"])


def state_0c_supernova_windup(ent: EfreetEntity) -> None:
    """0x012802..0x012818."""
    if (ent.anim_flags_37 & 0x02) != 0:
        ent.state_04 = 0x18
        ent.anim_2a = 0x1E


def state_18_supernova_spawner(ent: EfreetEntity, ctx: SimCtx) -> None:
    """0x012818..0x01284C + 0x012858..0x012914."""
    ctx.screen_shake_17c4 = (ctx.frame_counter_1971 & 1) + 3
    d0 = to_s16(ent.timer_02) - 1
    if d0 >= 0:
        ent.timer_02 = d0
        ctx.sfx_log.extend([0x00B00224, 0x00700383])
        return

    rem = to_s8(ent.fireball_rem_af) - 1
    if rem < 0:
        ent.fireball_rem_af = rem & 0xFF
        ent.state_04 = 0x1A
        ent.anim_2a = 0x20
        return
    ent.fireball_rem_af = rem & 0xFF
    ent.timer_02 = 2
    angle = (ent.orbit_angle_ae - 0x20) & 0xFF
    ent.orbit_angle_ae = angle
    sin_hi = q15_sin_hi(angle)
    cos_hi = q15_sin_hi((angle + 0x40) & 0xFF)
    vx = cos_hi * 0x1000
    vy = -(sin_hi * 0x1000)

    if ctx.pool1_full:
        ent.timer_02 = 0
        ent.orbit_angle_ae = (ent.orbit_angle_ae + 0x20) & 0xFF
        ent.fireball_rem_af = (ent.fireball_rem_af + 1) & 0xFF
        return

    fb = EfreetEntity(
        type_id=ent.type_id,
        state_04=0x16,
        anim_2a=0x16,
        x_pos_08=ent.x_pos_08 + 2 * vx,
        y_pos_0c=ent.y_pos_0c + 2 * vy,
        facing_16=0,
        vram_attr_18=(ent.vram_attr_18 & 0x8000) | 0x07A8,
        flags_36=ent.flags_36 & ~0x01,
        anim_flags_37=ent.anim_flags_37 & ~0x02,
        vx_4e=vx,
        vy_52=vy,
        sub_entity_a2=0xFF,
    )
    ctx.spawned_fireballs.append(fb)


def state_1a_supernova_recovery(ent: EfreetEntity) -> None:
    """0x01284C..0x012858."""
    if (ent.anim_flags_37 & 0x02) != 0:
        return_to_idle(ent)


def state_16_fireball_flight(fb: EfreetEntity, ctx: SimCtx) -> None:
    """0x012914..0x012950."""
    fb.facing_16 = compute_cardinal_facing(fb.vx_4e, fb.vy_52)
    ctx.attack_scans.append((2, fb.bonus_dmg_90, fb.facing_16))
    fb.facing_16 = 0
    if (fb.anim_flags_37 & 0x02) != 0:
        fb.type_id = 0
        return
    fb.delta_x_72 += fb.vx_4e
    fb.delta_y_76 += fb.vy_52


def state_10_melee_scan(ent: EfreetEntity, ctx: SimCtx) -> None:
    """0x012950..0x012978."""
    ent.bonus_dmg_90 = ((ctx.efreet_level_db7 & 0xFF) + 1) >> 2
    ctx.attack_scans.append((0, ent.bonus_dmg_90, ent.facing_16))
    ent.bonus_dmg_90 = 0
    if (ent.anim_flags_37 & 0x02) != 0:
        return_to_idle(ent)


def insufficient_sp_buzzer(ctx: SimCtx) -> str:
    """0x012978..0x012988."""
    ctx.sfx_log.append(0x00A00253)
    return "POST_DISPATCH_0129B0"


class TestROMEfreetCombatFireball(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.490")
        self.assertEqual(self.evidence["range"]["start"], "0x012680")
        self.assertEqual(self.evidence["range"]["end"], "0x012988")
        self.assertEqual(self.evidence["range"]["size_bytes"], 776)
        self.assertEqual(self.evidence["embedded_data_tables"], [])
        self.assertEqual(self.evidence["instruction_count"], 206)
        self.assertEqual(len(self.evidence["instructions"]), 206)
        self.assertEqual(sum(i["size"] for i in self.evidence["instructions"]), 776)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertEqual(
            [(name, routines[name]["size_bytes"], routines[name]["instruction_count"]) for name in routines],
            [
                ("AI_Efreet_DashChargeStates", 292, 81),
                ("AI_Efreet_FlameBreathStates", 94, 24),
                ("AI_Efreet_FireballSupernovaStates", 274, 71),
                ("AI_Efreet_FireballFlightMeleeAndBuzzer", 116, 30),
            ],
        )

        cursor = 0x012680
        by_addr = {}
        for insn in self.evidence["instructions"]:
            addr = int(insn["address"], 16)
            self.assertEqual(addr, cursor)
            cursor += insn["size"]
            by_addr[addr] = insn
        self.assertEqual(cursor, 0x012988)
        self.assertEqual(by_addr[0x012764]["op_str"], "#$80000, d0")
        self.assertEqual(by_addr[0x0127B8]["op_str"], "#$46, $2(a6)")
        self.assertEqual(by_addr[0x012890]["op_str"], "$5d706.l, a0")
        self.assertEqual(by_addr[0x012924]["op_str"], "#$2, d0")
        self.assertEqual(by_addr[0x012984]["op_str"], "$129b0")

    def test_dash_charge_states_and_velocity_clamping(self) -> None:
        # State $04 -> $02 on +$37 bit 1
        ent = EfreetEntity(state_04=4, anim_2a=9, facing_16=1)
        state_04_recovery(ent)
        self.assertEqual(ent.state_04, 4)
        ent.anim_flags_37 = 0x02
        state_04_recovery(ent)
        self.assertEqual((ent.state_04, ent.anim_2a), (2, 0))

        # Clamped dash accelerator [-0x80000..+0x80000] & diagonal 0.75 scaling
        ent = EfreetEntity(state_04=8, facing_16=1, vx_4e=0x78000)
        state_08_dash_startup(ent)
        self.assertEqual((ent.vx_4e, ent.delta_x_72), (0x80000, 0x80000))
        ent.anim_flags_37 = 0x02
        state_08_dash_startup(ent)
        self.assertEqual((ent.state_04, ent.anim_2a, ent.timer_02), (0x12, 0x1A, 0x0A))

        # Negative clamp check for facing Left (3)
        neg_ent = EfreetEntity(facing_16=3, vx_4e=-0x78000)
        step_clamped_dash_velocity(neg_ent)
        self.assertEqual(neg_ent.vx_4e, -0x80000)

        # State $12: jewel-rank damage (($FF0DB7 + 1) >> 2), 10 frames -> State $14
        for lvl, exp_dmg in ((0, 0), (2, 0), (3, 1), (7, 2), (15, 4)):
            c = SimCtx(efreet_level_db7=lvl)
            e = EfreetEntity(state_04=0x12, timer_02=2, facing_16=5)
            state_12_dash_active(e, c)
            self.assertEqual(c.attack_scans[-1], (0, exp_dmg, 5))
            self.assertEqual(e.bonus_dmg_90, 0)
            self.assertEqual((e.vx_4e, e.vy_52), (0xC000, 0xC000))

        ctx = SimCtx(efreet_level_db7=3)
        ent.anim_flags_37 = 0
        for _ in range(9):
            state_12_dash_active(ent, ctx)
            self.assertEqual(ent.state_04, 0x12)
        state_12_dash_active(ent, ctx)
        self.assertEqual((ent.state_04, ent.anim_2a, ent.timer_02), (0x14, 0x1C, 0))

        # State $14: decel by (0xC0, 0xC0) & cooldown +$9E = $0E on +$37 bit 1
        state_14_dash_decel(ent)
        self.assertEqual(ent.vx_4e, 0x80000 - 0xC000)
        ent.anim_flags_37 = 0x02
        state_14_dash_decel(ent)
        self.assertEqual((ent.state_04, ent.anim_2a, ent.cooldown_9e), (0, 0, 0x0E))

    def test_flame_breath_states_particle_emit_and_slot_release(self) -> None:
        ent = EfreetEntity(state_04=0x0A, piece_flag_06=1, cooldown_9e=5, flame_slot=[0xFFFE])
        state_0a_flame_startup(ent)
        self.assertEqual(ent.state_04, 0x0A)
        ent.anim_flags_37 = 0x02
        state_0a_flame_startup(ent)
        self.assertEqual((ent.state_04, ent.anim_2a, ent.timer_02), (0x0E, 0x18, 70))

        ctx = SimCtx()
        for _ in range(69):
            state_0e_flame_active(ent, ctx)
        self.assertEqual(ent.timer_02, 1)
        self.assertEqual(ent.flame_slot[0], 0xFFFE)
        spawned_timers = [int(s.split("@")[1]) for s in ctx.flame_calls if s.startswith("0x012DFA@")]
        self.assertEqual(spawned_timers, list(range(68, 19, -4)))  # 13 bursts at >= 20
        self.assertEqual(ctx.sfx_log, [0x0080034F] * 13)

        # Final frame (timer 1 -> 0) frees $FF1C1C slot, clears +$6, calls 0x01269A
        state_0e_flame_active(ent, ctx)
        self.assertEqual((ent.timer_02, ent.flame_slot[0], ent.piece_flag_06), (0, 0, 0))
        self.assertEqual((ent.state_04, ent.anim_2a, ent.cooldown_9e), (0, 0, 0))

    def test_fireball_supernova_spawner_and_pool_full_retry(self) -> None:
        ent = EfreetEntity(
            state_04=0x0C,
            x_pos_08=0x100000,
            y_pos_0c=0x80000,
            vram_attr_18=0x8000,
            flags_36=0x05,
            anim_flags_37=0x00,
            orbit_angle_ae=0x60,
            fireball_rem_af=2,
        )
        ent.anim_flags_37 = 0x02
        state_0c_supernova_windup(ent)
        self.assertEqual((ent.state_04, ent.anim_2a), (0x18, 0x1E))
        ent.anim_flags_37 = 0x02

        # Pool-full retry reverts +$2, +$AE, +$AF without spawning
        ctx_full = SimCtx(frame_counter_1971=1, pool1_full=True)
        state_18_supernova_spawner(ent, ctx_full)
        self.assertEqual(ctx_full.screen_shake_17c4, 4)  # (1 & 1) + 3
        self.assertEqual((ent.timer_02, ent.orbit_angle_ae, ent.fireball_rem_af), (0, 0x60, 2))
        self.assertEqual(len(ctx_full.spawned_fireballs), 0)

        # Unblocked spawn #1: angle 0x60 -> 0x40 (Up), spawns child + resets timer=2
        ctx = SimCtx(frame_counter_1971=2)
        state_18_supernova_spawner(ent, ctx)
        self.assertEqual(ctx.screen_shake_17c4, 3)  # (2 & 1) + 3
        self.assertEqual((ent.timer_02, ent.orbit_angle_ae, ent.fireball_rem_af), (2, 0x40, 1))
        self.assertEqual(len(ctx.spawned_fireballs), 1)
        fb = ctx.spawned_fireballs[0]
        self.assertEqual((fb.state_04, fb.anim_2a, fb.sub_entity_a2, fb.facing_16), (0x16, 0x16, 0xFF, 0))
        self.assertEqual((fb.vram_attr_18, fb.flags_36, fb.anim_flags_37), (0x87A8, 0x04, 0x00))
        self.assertEqual((fb.vx_4e, fb.vy_52), (0, -0x7F000))
        self.assertEqual((fb.x_pos_08, fb.y_pos_0c), (0x100000, 0x80000 - 2 * 0x7F000))

        # Next 2 ticks count timer 2 -> 1 -> 0 playing SFX #$B00224 & #$700383
        state_18_supernova_spawner(ent, ctx)
        state_18_supernova_spawner(ent, ctx)
        self.assertEqual(ent.timer_02, 0)
        self.assertEqual(ctx.sfx_log, [0x00B00224, 0x00700383, 0x00B00224, 0x00700383])

        # Tick 3 spawns fireball #2 (angle 0x20 NE); 3 more ticks exhaust +$AF -> State $1A
        state_18_supernova_spawner(ent, ctx)
        self.assertEqual((ent.orbit_angle_ae, ent.fireball_rem_af), (0x20, 0))
        for _ in range(3):
            state_18_supernova_spawner(ent, ctx)
        self.assertEqual((ent.state_04, ent.anim_2a), (0x1A, 0x20))
        state_1a_supernova_recovery(ent)
        self.assertEqual((ent.state_04, ent.anim_2a, ent.cooldown_9e), (0, 0, 0))

    def test_fireball_flight_state10_melee_and_insufficient_sp_buzzer(self) -> None:
        # State $16 fireball flight: cardinal facing 0x00E338 -> 0x00BCA4(2), clear facing, integrate/despawn
        ctx = SimCtx()
        fb = EfreetEntity(state_04=0x16, vx_4e=-0x7F000, vy_52=0x10000, anim_flags_37=0)
        state_16_fireball_flight(fb, ctx)
        self.assertEqual(ctx.attack_scans[-1], (2, 0, 3))  # facing Left (3) during scan
        self.assertEqual(fb.facing_16, 0)
        self.assertEqual((fb.delta_x_72, fb.delta_y_76), (-0x7F000, 0x10000))
        fb.anim_flags_37 = 0x02
        state_16_fireball_flight(fb, ctx)
        self.assertEqual(fb.type_id, 0)

        # State $10 melee attack scan: jewel bonus +$90, 0x00BCA4(0), exit via 0x01269A on +$37 bit 1
        ctx.efreet_level_db7 = 7
        melee = EfreetEntity(state_04=0x10, facing_16=1, cooldown_9e=9, anim_flags_37=0x02)
        state_10_melee_scan(melee, ctx)
        self.assertEqual(ctx.attack_scans[-1], (0, 2, 1))
        self.assertEqual((melee.bonus_dmg_90, melee.state_04, melee.anim_2a, melee.cooldown_9e), (0, 0, 0, 0))

        # Insufficient-SP buzzer 0x012978..0x012988
        self.assertEqual(insufficient_sp_buzzer(ctx), "POST_DISPATCH_0129B0")
        self.assertEqual(ctx.sfx_log[-1], 0x00A00253)


if __name__ == "__main__":
    unittest.main()
