#!/usr/bin/env python3
"""
Unit tests for ROM Dytto Spirit Magic Bubble Projectile, Flame/Enemy Stun AABB
Scanner & Water Tornado Vortex Subsystem (0x011D8A..0x0120D4, 842 bytes).
Milestone M15.487.
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
    / "rom_dytto_bubble_tornado_m15_487.json"
)


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def compute_8dir_velocity(vx_u16: int, vy_u16: int, direction: int) -> tuple[int, int]:
    """Faithful model of 0x00E2D4 (8-dir fixed-point 16.16 velocity vector)."""
    vx = vx_u16 << 8
    vy = vy_u16 << 8
    d = direction & 7
    if d >= 4:
        vx = (vx >> 1) + (vx >> 2)
        vy = (vy >> 1) + (vy >> 2)
    table = {
        0: (0, -vy),
        1: (vx, 0),
        2: (0, vy),
        3: (-vx, 0),
        4: (vx, -vy),
        5: (vx, vy),
        6: (-vx, vy),
        7: (-vx, -vy),
    }
    return table[d]


@dataclass
class EntitySlot:
    type_id: int = 0
    timer_02: int = 0
    state_04: int = 0
    x_pos_08: int = 0
    y_pos_0c: int = 0
    z_pos_10: int = 0
    ground_z_14: int = 0
    facing_16: int = 0
    vram_attr_18: int = 0
    sprite_ptr_1a: int = 0
    anim_2a: int = 0
    cast_frame_30: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    flags_3a: int = 0
    flags_3d: int = 0
    wall_40: int = 0
    wall_41: int = 0
    hitbox_46: int = 0
    hitbox_48: int = 0
    hitbox_49: int = 0
    hitbox_4a: int = 0
    vx_4e: int = 0
    step_50: int = 0
    radial_step_51: int = 0
    vy_or_radius_52: int = 0
    trigger_66: int = 0
    delta_x_72: int = 0
    delta_y_76: int = 0
    drain_timer_8e: int = 45
    stun_param_9c: int = 0
    stun_timer_9d: int = 0
    sub_type_9e: int = 0
    sub_entity_a2: int = 0
    reserved_offset_a4: int = 0


@dataclass
class SimContext:
    player_sp: int = 100
    player_facing_19fe: int = 2
    latched_facing_19a4: int = 2
    hold_counter_199e: int = 0
    spirit_level_db6: int = 0
    pool1_full: bool = False
    reserved_slot_offset: int = 0x240
    reserved_slot: EntitySlot = field(default_factory=lambda: EntitySlot(type_id=0x002E))
    scan_hit_target: EntitySlot | None = None
    offscreen: bool = False
    ground_probe_z: int = 0
    sfx_log: list[int] = field(default_factory=list)
    calls: list[str] = field(default_factory=list)


def try_start_magic_bubble(dytto: EntitySlot, ctx: SimContext) -> str:
    """0x011D8A..0x011DB6: AI_Dytto_TryStartMagicBubble."""
    if ctx.player_sp - 5 < 1:
        return "FAIL_01219A"
    ctx.player_sp -= 5
    if ctx.pool1_full:
        return "FAIL_01219A"
    ctx.reserved_slot = EntitySlot(type_id=0x002E, timer_02=0x3C)
    dytto.reserved_offset_a4 = ctx.reserved_slot_offset
    dytto.facing_16 = ctx.latched_facing_19a4 & 0xFFFF
    dytto.state_04 = 0x0E
    dytto.anim_2a = 0x06
    return "OK"


def state0e_cast_and_launch_magic_bubble(dytto: EntitySlot, ctx: SimContext) -> str:
    """0x011DB6..0x011ED2: AI_Dytto_State0E_CastAndLaunchMagicBubble."""
    if dytto.cast_frame_30 < 0x18:
        z_hi = to_s16(dytto.z_pos_10 >> 16)
        if z_hi - 2 >= 0:
            dytto.z_pos_10 = ((z_hi - 2) << 16) | (dytto.z_pos_10 & 0xFFFF)
        else:
            ctx.calls.append("0x0123B4")
    else:
        ctx.calls.append("0x0123B4")

    if dytto.cast_frame_30 == 0x18:
        slot = ctx.reserved_slot
        if slot.type_id == 0x002E:
            slot.type_id = 0x0018
            ctx.calls.append("0x008D06")
            slot.sprite_ptr_1a = 0x00141374
            slot.vram_attr_18 = (dytto.vram_attr_18 & 0x8000) | 0x07A8
            slot.flags_3d = 8
            slot.flags_36 = 0
            slot.flags_38 = 0x0E
            slot.hitbox_46 = 8
            slot.hitbox_48 = 8
            slot.hitbox_49 = 8
            slot.hitbox_4a = 0x10
            slot.sub_entity_a2 = 0xFF
            slot.state_04 = 0x10
            slot.anim_2a = 0x1A
            slot.x_pos_08 = dytto.x_pos_08
            slot.y_pos_0c = dytto.y_pos_0c
            z_hi = (dytto.z_pos_10 >> 16) + 0x0A
            slot.z_pos_10 = ((z_hi & 0xFFFF) << 16) | (dytto.z_pos_10 & 0xFFFF)
            d1, d2 = compute_8dir_velocity(0x1800, 0x1800, dytto.facing_16)
            if (dytto.facing_16 & 0xFF) == 2:
                d1 += 0x00020000
            slot.delta_x_72 = d1
            slot.delta_y_76 = d2
            vx, vy = compute_8dir_velocity(0x0400, 0x03C0, dytto.facing_16)
            slot.vx_4e = vx
            slot.vy_or_radius_52 = vy
            slot.facing_16 = 0
            ctx.calls.extend(["0x009D00", "0x009AD6"])
            slot.ground_z_14 = ctx.ground_probe_z
            ctx.sfx_log.append(0x00800354)
        return "RTS"

    if (dytto.anim_flags_37 & 0x02) != 0:
        dytto.state_04 = 0x00
        return "STATE0_0121AA"
    return "RTS"


def state10_magic_bubble_flight(bubble: EntitySlot, ctx: SimContext) -> str:
    """0x011ED2..0x011F98: State $10 bubble flight, scan & stun."""
    ctx.calls.append("0x00E36E")
    if ((bubble.wall_40 | bubble.wall_41) & 0x40) != 0:
        bubble.state_04 = 0x16
        bubble.anim_2a = 0x24
        return "POP_STATE16"

    ctx.calls.append("0x00B922(d0=0xF0,[-9..9,-9..9,-1..17])")
    target = ctx.scan_hit_target
    if target is not None:
        is_flame = target.type_id == 0x004C or (
            target.type_id == 0x0098 and target.sub_type_9e == 1
        )
        if is_flame:
            target.trigger_66 = 0xFF
            ctx.sfx_log.append(0x008004D4)
            bubble.state_04 = 0x16
            bubble.anim_2a = 0x24
            return "POP_STATE16"
        if (target.flags_3a & 0x02) != 0:
            bubble.state_04 = 0x16
            bubble.anim_2a = 0x24
            return "POP_STATE16"
        if (target.flags_38 & 0x08) == 0:
            lvl = ctx.spirit_level_db6 & 0xFF
            target.stun_timer_9d = ((lvl << 3) + 0x50) & 0xFF
            target.stun_param_9c = ((lvl << 2) + 0x14) & 0xFF
            ctx.sfx_log.append(0x008004D4)
            bubble.state_04 = 0x16
            bubble.anim_2a = 0x24
            return "POP_STATE16"

    ctx.calls.append("0x00E5AE(margin=0x18)")
    if ctx.offscreen:
        bubble.type_id = 0
        return "DESPAWNED"
    bubble.delta_x_72 += bubble.vx_4e
    bubble.delta_y_76 += bubble.vy_or_radius_52
    return "FLYING"


def state16_magic_bubble_pop(bubble: EntitySlot) -> str:
    """0x011F98..0x011FA6: State $16 bubble pop cleanup."""
    if (bubble.anim_flags_37 & 0x02) != 0:
        bubble.type_id = 0
        return "DESPAWNED"
    return "POPPING"


def tornado_start_command3(dytto: EntitySlot, ctx: SimContext) -> None:
    """0x011FA6..0x011FF6: Command 3 Water Tornado startup."""
    ctx.hold_counter_199e = 0
    z_hi = to_s16(dytto.z_pos_10 >> 16) - 0x28
    if z_hi < 0:
        dytto.z_pos_10 = 0
    else:
        dytto.z_pos_10 = ((z_hi & 0xFFFF) << 16) | (dytto.z_pos_10 & 0xFFFF)
    dytto.state_04 = 0x08
    dytto.anim_2a = 0x0A
    ctx.sfx_log.append(0x0080034E)
    dytto.vx_4e = ((5 - (ctx.player_facing_19fe & 0xFFFF)) & 3) << 6
    dytto.step_50 = 4
    dytto.radial_step_51 = 0x14
    dytto.vy_or_radius_52 = 0x00020000


def tornado_state08_startup(dytto: EntitySlot, ctx: SimContext) -> None:
    """0x011FF6..0x01202A: State $08 tornado wind-up."""
    ctx.calls.append("0x0123E2")
    if (dytto.anim_flags_37 & 0x02) != 0:
        dytto.state_04 = 0x0A
        dytto.anim_2a = 0x0C
        dytto.flags_3d = 0x10
        dytto.timer_02 = ((ctx.spirit_level_db6 & 0xFF) << 3) + 0x78


def tornado_state0a_vortex(dytto: EntitySlot, ctx: SimContext) -> None:
    """0x01202A..0x012086: State $0A active Water Tornado vortex."""
    if dytto.drain_timer_8e != 0:
        t = to_s16(dytto.drain_timer_8e) - 8
        dytto.drain_timer_8e = 0 if t < 0 else t
    ctx.calls.extend(["0x00BCA4(d0=0)", "0x0123E2"])
    if (dytto.timer_02 & 7) == 0:
        ctx.sfx_log.append(0x00700340)
    if ctx.player_sp != 0:
        dytto.timer_02 = (dytto.timer_02 - 1) & 0xFFFF
        if dytto.timer_02 != 0:
            return
    dytto.state_04 = 0x0C
    dytto.anim_2a = 0x18
    dytto.flags_3d = 0
    z_hi = ((dytto.z_pos_10 >> 16) + 0x28) & 0xFFFF
    dytto.z_pos_10 = (z_hi << 16) | (dytto.z_pos_10 & 0xFFFF)
    dytto.timer_02 = 0x1E


def tornado_state0c_winddown(dytto: EntitySlot, ctx: SimContext) -> str:
    """0x012086..0x0120D4: State $0C Water Tornado wind-down."""
    ctx.calls.append("0x00BCA4(d0=0)")
    rad = dytto.vy_or_radius_52 - 0x5000
    dytto.vy_or_radius_52 = 0 if rad < 0 else rad
    ctx.calls.append("0x0123E2")
    z_hi = dytto.z_pos_10 >> 16
    if dytto.flags_3d != 0:
        dytto.flags_3d = 0
        z_hi = (z_hi + 0x28) & 0xFFFF
    else:
        dytto.flags_3d = 0x10
        z_hi = (z_hi - 0x28) & 0xFFFF
    dytto.z_pos_10 = (z_hi << 16) | (dytto.z_pos_10 & 0xFFFF)
    dytto.timer_02 = (dytto.timer_02 - 1) & 0xFFFF
    if dytto.timer_02 == 0:
        dytto.state_04 = 0
        dytto.flags_3d = 0
        return "STATE0_0121AA"
    return "WINDDOWN"


class TestROMDyttoBubbleTornado(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.487")
        self.assertEqual(self.evidence["range"]["start"], "0x011D8A")
        self.assertEqual(self.evidence["range"]["end"], "0x0120D4")
        self.assertEqual(self.evidence["range"]["size_bytes"], 842)
        self.assertEqual(self.evidence["instruction_count"], 203)
        self.assertEqual(len(self.evidence["instructions"]), 203)
        self.assertEqual(sum(i["size"] for i in self.evidence["instructions"]), 842)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertEqual(
            list(routines.keys()),
            [
                "AI_Dytto_TryStartMagicBubble",
                "AI_Dytto_State0E_CastAndLaunchMagicBubble",
                "AI_Dytto_State10_16_MagicBubbleFlightAndImpact",
                "AI_Dytto_WaterTornadoLifecycle",
            ],
        )
        self.assertEqual(routines["AI_Dytto_TryStartMagicBubble"]["size_bytes"], 44)
        self.assertEqual(routines["AI_Dytto_TryStartMagicBubble"]["instruction_count"], 10)
        self.assertEqual(routines["AI_Dytto_State0E_CastAndLaunchMagicBubble"]["size_bytes"], 284)
        self.assertEqual(routines["AI_Dytto_State0E_CastAndLaunchMagicBubble"]["instruction_count"], 63)
        self.assertEqual(routines["AI_Dytto_State10_16_MagicBubbleFlightAndImpact"]["size_bytes"], 212)
        self.assertEqual(routines["AI_Dytto_State10_16_MagicBubbleFlightAndImpact"]["instruction_count"], 56)
        self.assertEqual(routines["AI_Dytto_WaterTornadoLifecycle"]["size_bytes"], 302)
        self.assertEqual(routines["AI_Dytto_WaterTornadoLifecycle"]["instruction_count"], 74)

        cursor = 0x011D8A
        for insn in self.evidence["instructions"]:
            self.assertEqual(int(insn["address"], 16), cursor)
            cursor += insn["size"]
        self.assertEqual(cursor, 0x0120D4)

    def test_try_start_magic_bubble_and_state0e_launch(self) -> None:
        # 1. SP guard (< 6 fails with SP untouched)
        ctx = SimContext(player_sp=5)
        dytto = EntitySlot(z_pos_10=1 << 16, vram_attr_18=0x8000, x_pos_08=0x100 << 16, y_pos_0c=0x80 << 16)
        self.assertEqual(try_start_magic_bubble(dytto, ctx), "FAIL_01219A")
        self.assertEqual(ctx.player_sp, 5)

        # 2. Pool 1 full after SP spend fails to 0x01219A
        ctx_full = SimContext(player_sp=20, pool1_full=True)
        self.assertEqual(try_start_magic_bubble(dytto, ctx_full), "FAIL_01219A")
        self.assertEqual(ctx_full.player_sp, 15)

        # 3. Successful start + State $0E elevation drop/fallback + launch at frame 0x18
        ctx = SimContext(player_sp=20, latched_facing_19a4=2, ground_probe_z=12)
        self.assertEqual(try_start_magic_bubble(dytto, ctx), "OK")
        self.assertEqual(ctx.player_sp, 15)
        self.assertEqual(dytto.reserved_offset_a4, 0x240)
        self.assertEqual(dytto.facing_16, 2)
        self.assertEqual(dytto.state_04, 0x0E)
        self.assertEqual(dytto.anim_2a, 6)

        # +$10 < 2 skips decrement and calls 0x0123B4; with +$10 >= 2 it drops by 2
        self.assertEqual(state0e_cast_and_launch_magic_bubble(dytto, ctx), "RTS")
        self.assertEqual(ctx.calls[-1], "0x0123B4")
        dytto.z_pos_10 = 10 << 16
        dytto.cast_frame_30 = 5
        self.assertEqual(state0e_cast_and_launch_magic_bubble(dytto, ctx), "RTS")
        self.assertEqual(dytto.z_pos_10 >> 16, 8)

        # Frame 0x18 launches bubble (facing Down=2 adds +0x20000 to delta_x_72)
        dytto.cast_frame_30 = 0x18
        self.assertEqual(state0e_cast_and_launch_magic_bubble(dytto, ctx), "RTS")
        b = ctx.reserved_slot
        self.assertEqual(b.type_id, 0x0018)
        self.assertEqual(b.sprite_ptr_1a, 0x00141374)
        self.assertEqual(b.vram_attr_18, 0x87A8)
        self.assertEqual((b.flags_3d, b.flags_36, b.flags_38, b.sub_entity_a2), (8, 0, 0x0E, 0xFF))
        self.assertEqual((b.hitbox_46, b.hitbox_48, b.hitbox_49, b.hitbox_4a), (8, 8, 8, 0x10))
        self.assertEqual((b.state_04, b.anim_2a, b.z_pos_10 >> 16, b.ground_z_14), (0x10, 0x1A, 18, 12))
        self.assertEqual((b.delta_x_72, b.delta_y_76), (0x00020000, 0x00180000))
        self.assertEqual((b.vx_4e, b.vy_or_radius_52, b.facing_16), (0, 0x0003C000, 0))
        self.assertEqual(ctx.sfx_log[-1], 0x00800354)

        # Anim complete returns Dytto to State 0 (0x0121AA)
        dytto.cast_frame_30 = 0x19
        dytto.anim_flags_37 = 0x02
        self.assertEqual(state0e_cast_and_launch_magic_bubble(dytto, ctx), "STATE0_0121AA")
        self.assertEqual(dytto.state_04, 0)

    def test_magic_bubble_flight_wall_cull_and_pop(self) -> None:
        # Flight integration when clear & onscreen
        ctx = SimContext()
        bubble = EntitySlot(type_id=0x0018, state_04=0x10, vx_4e=0x40000, vy_or_radius_52=-0x3C000)
        self.assertEqual(state10_magic_bubble_flight(bubble, ctx), "FLYING")
        self.assertEqual((bubble.delta_x_72, bubble.delta_y_76), (0x40000, -0x3C000))

        # Viewport cull clears +$0(a6)
        ctx.offscreen = True
        self.assertEqual(state10_magic_bubble_flight(bubble, ctx), "DESPAWNED")
        self.assertEqual(bubble.type_id, 0)

        # Wall contact (+$40 | +$41 bit 6) enters State $16 Pop, then despawns on +$37 bit 1
        bubble = EntitySlot(type_id=0x0018, state_04=0x10, wall_41=0x40)
        self.assertEqual(state10_magic_bubble_flight(bubble, SimContext()), "POP_STATE16")
        self.assertEqual((bubble.state_04, bubble.anim_2a), (0x16, 0x24))
        self.assertEqual(state16_magic_bubble_pop(bubble), "POPPING")
        bubble.anim_flags_37 = 0x02
        self.assertEqual(state16_magic_bubble_pop(bubble), "DESPAWNED")
        self.assertEqual(bubble.type_id, 0)

    def test_magic_bubble_flame_and_enemy_stun_scanner(self) -> None:
        # 1. Flame $004C & Type $0098 (+$9E == 1) set +$66 = $FF + SFX $8004D4 -> State $16
        for target in (EntitySlot(type_id=0x004C), EntitySlot(type_id=0x0098, sub_type_9e=1)):
            ctx = SimContext(scan_hit_target=target)
            bubble = EntitySlot(type_id=0x0018, state_04=0x10)
            self.assertEqual(state10_magic_bubble_flight(bubble, ctx), "POP_STATE16")
            self.assertEqual(target.trigger_66, 0xFF)
            self.assertEqual(ctx.sfx_log[-1], 0x008004D4)
            self.assertEqual((bubble.state_04, bubble.anim_2a), (0x16, 0x24))

        # 2. Immune enemy (+$3A bit 1) pops bubble without stun or hit SFX
        immune = EntitySlot(type_id=0x0048, flags_3a=0x02)
        ctx_imm = SimContext(scan_hit_target=immune)
        b_imm = EntitySlot(type_id=0x0018, state_04=0x10)
        self.assertEqual(state10_magic_bubble_flight(b_imm, ctx_imm), "POP_STATE16")
        self.assertEqual((immune.stun_timer_9d, immune.stun_param_9c, ctx_imm.sfx_log), (0, 0, []))

        # 3. Intangible enemy (+$38 bit 3) lets bubble keep flying
        intang = EntitySlot(type_id=0x0048, flags_38=0x08)
        b_int = EntitySlot(type_id=0x0018, state_04=0x10, vx_4e=0x40000)
        self.assertEqual(state10_magic_bubble_flight(b_int, SimContext(scan_hit_target=intang)), "FLYING")
        self.assertEqual(b_int.delta_x_72, 0x40000)

        # 4. Vulnerable enemy receives level-scaled stun +$9D=(lvl<<3)+80, +$9C=(lvl<<2)+20
        for lvl, exp_9d, exp_9c in ((0, 80, 20), (3, 104, 32)):
            enemy = EntitySlot(type_id=0x0048)
            ctx_hit = SimContext(spirit_level_db6=lvl, scan_hit_target=enemy)
            b_hit = EntitySlot(type_id=0x0018, state_04=0x10)
            self.assertEqual(state10_magic_bubble_flight(b_hit, ctx_hit), "POP_STATE16")
            self.assertEqual((enemy.stun_timer_9d, enemy.stun_param_9c), (exp_9d, exp_9c))
            self.assertEqual(ctx_hit.sfx_log[-1], 0x008004D4)

    def test_water_tornado_command3_startup_vortex_and_winddown(self) -> None:
        # 1. Angle mapping & Z clamp on Command 3 startup
        for facing, exp_angle in ((0, 0x40), (1, 0x00), (2, 0xC0), (3, 0x80)):
            d = EntitySlot(z_pos_10=(0x10 << 16) | 0x1234)
            c = SimContext(player_facing_19fe=facing, hold_counter_199e=25)
            tornado_start_command3(d, c)
            self.assertEqual(c.hold_counter_199e, 0)
            self.assertEqual(d.z_pos_10, 0)  # full 32-bit cleared on borrow
            self.assertEqual((d.state_04, d.anim_2a, d.vx_4e), (0x08, 0x0A, exp_angle))
            self.assertEqual((d.step_50, d.radial_step_51, d.vy_or_radius_52), (4, 20, 0x20000))
            self.assertEqual(c.sfx_log[-1], 0x0080034E)

        # 2. State $08 -> State $0A duration = (level << 3) + 120
        dytto = EntitySlot(z_pos_10=0x50 << 16, drain_timer_8e=12)
        ctx = SimContext(player_sp=10, player_facing_19fe=1, spirit_level_db6=2)
        tornado_start_command3(dytto, ctx)
        self.assertEqual(dytto.z_pos_10 >> 16, 0x28)
        dytto.anim_flags_37 = 0x02
        tornado_state08_startup(dytto, ctx)
        self.assertEqual((dytto.state_04, dytto.anim_2a, dytto.flags_3d, dytto.timer_02), (0x0A, 0x0C, 0x10, 136))

        # 3. State $0A drains +$8E by 8 (clamped >= 0), plays #$700340 on (timer&7)==0, transitions on SP==0
        ctx.sfx_log.clear()
        tornado_state0a_vortex(dytto, ctx)  # timer 136 -> &7==0 plays SFX, drain 12->4, timer->135
        self.assertEqual((dytto.drain_timer_8e, dytto.timer_02), (4, 135))
        self.assertEqual(ctx.sfx_log, [0x00700340])
        tornado_state0a_vortex(dytto, ctx)  # drain 4->0 (clamped), timer->134
        self.assertEqual((dytto.drain_timer_8e, dytto.timer_02), (0, 134))
        ctx.player_sp = 0
        tornado_state0a_vortex(dytto, ctx)  # SP==0 forces immediate State $0C
        self.assertEqual((dytto.state_04, dytto.anim_2a, dytto.flags_3d, dytto.z_pos_10 >> 16, dytto.timer_02),
                         (0x0C, 0x18, 0, 0x50, 0x1E))

        # 4. State $0C decays radius by $5000, alternates (+$3D, +$10), exits at frame 30
        res = tornado_state0c_winddown(dytto, ctx)  # frame 1: +$3D was 0 -> 0x10, Z -= 0x28
        self.assertEqual((res, dytto.vy_or_radius_52, dytto.flags_3d, dytto.z_pos_10 >> 16), ("WINDDOWN", 0x1B000, 0x10, 0x28))
        res = tornado_state0c_winddown(dytto, ctx)  # frame 2: +$3D was 0x10 -> 0, Z += 0x28
        self.assertEqual((res, dytto.vy_or_radius_52, dytto.flags_3d, dytto.z_pos_10 >> 16), ("WINDDOWN", 0x16000, 0, 0x50))
        for _ in range(27):
            self.assertEqual(tornado_state0c_winddown(dytto, ctx), "WINDDOWN")
        self.assertEqual(tornado_state0c_winddown(dytto, ctx), "STATE0_0121AA")
        self.assertEqual((dytto.vy_or_radius_52, dytto.state_04, dytto.flags_3d), (0, 0, 0))


if __name__ == "__main__":
    unittest.main()
