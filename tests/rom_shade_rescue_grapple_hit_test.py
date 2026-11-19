#!/usr/bin/env python3
"""
Unit tests for ROM Shade Spirit Buzzer, Dark Hole Fall-Rescue Drain & Teleport
Recovery (States $0C/$0E), and Shadow Grapple Extension & Hit Latch (States $06/$08)
(0x01319E..0x01341A, 636 bytes).
Milestone M15.494.
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
    / "rom_shade_rescue_grapple_hit_m15_494.json"
)


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


@dataclass
class EntitySlot:
    name: str = "ent"
    timer_02: int = 0
    state_04: int = 0
    piece_flag_06: int = 0
    x_pos_08: int = 0
    y_pos_0c: int = 0
    z_pos_10: int = 0
    ground_z_14: int = 0
    facing_16: int = 0
    anim_2a: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    wall_40: int = 0
    wall_41: int = 0
    vx_4e: int = 0
    vy_52: int = 0
    vz_56: int = 0
    contact_ptr_68: EntitySlot | None = None
    delta_x_72: int = 0
    delta_y_76: int = 0
    attacker_ptr_7e: EntitySlot | None = None
    hitstop_82: int = 0
    hit_cooldown_83: int = 0
    kb_facing_84: int = 0
    dmg_word_86: int = 0
    sp_or_drain_8e: int = 0
    stun_timer_9d: int = 0
    grapple_target_aa: EntitySlot | None = None


@dataclass
class ShadeSimCtx:
    ali_piece_backup_d7e: int = 0x1234
    shade_level_db8: int = 0
    hud_fade_164f: int = 0xFF
    game_flags_16f1: int = 0
    frame_counter_1971: int = 0
    pause_lock_17b8: int = 0x04
    teleport_17ba: int = 0
    hud_enemy_timer_184f: int = 0
    hud_enemy_ptr_1850: EntitySlot | None = None
    warp_fade_1896: int = 0
    rescue_flag_1985: int = 0
    hold_counter_199e: int = 12
    prev_btn_a_19a0: int = 0x40
    dtap_timer_19a1: int = 5
    item_probe_target: EntitySlot | None = None
    sfx_log: list[int] = field(default_factory=list)
    calls: list[str] = field(default_factory=list)


def insufficient_sp_buzzer(ctx: ShadeSimCtx) -> str:
    """0x01319E..0x0131AE."""
    ctx.sfx_log.append(0x00A00253)
    return "IDLE_CONT_0136A0"


def state_0c_dark_hole_rescue_and_drain(
    shade: EntitySlot, ali: EntitySlot, ctx: ShadeSimCtx
) -> None:
    """0x0131AE..0x01325E."""
    shade.delta_x_72 = 0
    shade.delta_y_76 = 0
    if ctx.rescue_flag_1985 != 1 and ali.sp_or_drain_8e != 0:
        if shade.sp_or_drain_8e != 0:
            d0 = shade.sp_or_drain_8e - 4
            shade.sp_or_drain_8e = 0 if d0 < 0 else d0
        if (ctx.frame_counter_1971 & 0x01) != 0:
            ali.flags_36 &= ~0x02
        else:
            ali.flags_36 |= 0x02
        return

    ali.flags_36 &= ~0x02
    ali.flags_38 &= ~((1 << 2) | (1 << 6))
    ali.piece_flag_06 = ctx.ali_piece_backup_d7e
    ali.state_04 = 0
    ali.anim_2a = 0
    ali.facing_16 = 2
    ali.hitstop_82 = 0x0A
    ali.x_pos_08 = shade.x_pos_08
    ali.y_pos_0c = shade.y_pos_0c
    ali.z_pos_10 = 0
    ctx.calls.append("0x009D00(ali)")
    ctx.calls.append("0x01378A")
    shade.state_04 = 0x0E
    ctx.hud_fade_164f = 0
    ctx.warp_fade_1896 = 1
    ctx.calls.append("0x002E1E")
    ctx.teleport_17ba = 0xFF


def state_0e_finish_dark_hole_recovery(
    shade: EntitySlot, ali: EntitySlot, ctx: ShadeSimCtx
) -> None:
    """0x01325E..0x0132AA."""
    ctx.rescue_flag_1985 = 0
    ali.flags_38 &= ~(1 << 3)
    ctx.pause_lock_17b8 &= ~(1 << 2)
    shade.state_04 = 0
    ctx.hold_counter_199e = 0
    ctx.dtap_timer_19a1 = 0
    ctx.prev_btn_a_19a0 = 0xFF
    ctx.calls.append("0x002D84")
    ctx.warp_fade_1896 = 0
    if (ctx.game_flags_16f1 & 0x02) != 0:
        ctx.hud_fade_164f = 0xFF


def grapple_retract_init_state_0a(shade: EntitySlot) -> None:
    """0x0133E6..0x01341A."""
    shade.state_04 = 0x0A
    shade.anim_2a = 6
    shade.vx_4e = -shade.vx_4e
    shade.vy_52 = -shade.vy_52
    shade.timer_02 = (0x12 - shade.timer_02) & 0xFFFF
    shade.flags_38 |= (1 << 2)


def _enter_state_08_default(shade: EntitySlot) -> None:
    """0x013302..0x01330E."""
    shade.anim_2a = 0x12
    shade.state_04 = 0x08


def state_06_grapple_extend(
    shade: EntitySlot, ali: EntitySlot, ctx: ShadeSimCtx
) -> None:
    """0x0132AA..0x0133D0."""
    ali.delta_x_72 = 0
    ali.delta_y_76 = 0
    contact = (shade.wall_40 | shade.wall_41) & 0xFF
    if (contact & 0x40) != 0:
        grapple_retract_init_state_0a(shade)
        return
    if (contact & 0x80) != 0:
        target = shade.contact_ptr_68
        assert target is not None
        if (target.anim_flags_37 & 0x80) != 0:
            if target.ground_z_14 == shade.ground_z_14:
                shade.grapple_target_aa = target
                ctx.calls.append("0x0134DE")
            _enter_state_08_default(shade)
            return
        if (target.flags_38 & 0x08) == 0 and target.hitstop_82 == 0 and target.hit_cooldown_83 == 0:
            ctx.hud_enemy_timer_184f = 0x5A
            ctx.hud_enemy_ptr_1850 = target
            target.attacker_ptr_7e = shade
            target.hitstop_82 = 0
            target.hit_cooldown_83 = 0x0A
            target.stun_timer_9d = 0
            target.kb_facing_84 = shade.facing_16
            vx, vy = compute_8dir_velocity(0x0500, 0x04C0, shade.facing_16)
            target.vx_4e = vx
            target.vy_52 = vy
            target.vz_56 = 0x00028000
            dmg = 1 + ((ctx.shade_level_db8 & 0xFF) >> 1)
            if target.z_pos_10 != 0:
                dmg |= 0x8000  # big-endian bset.b #7, $86(a0) sets bit 15 of word +$86
            target.dmg_word_86 = dmg & 0xFFFF
            ctx.sfx_log.append(0x00700345)
        _enter_state_08_default(shade)
        return

    ctx.calls.append("0x013608")
    item = ctx.item_probe_target
    if item is not None and item.ground_z_14 == shade.ground_z_14:
        shade.grapple_target_aa = item
        ctx.calls.append("0x013550")
        shade.anim_2a = 4
        shade.state_04 = 0x08
        return

    shade.timer_02 = (shade.timer_02 - 1) & 0xFFFF
    if shade.timer_02 == 0:
        _enter_state_08_default(shade)
        return
    ax, ay = compute_8dir_velocity(0x0080, 0x0080, shade.facing_16)
    shade.vx_4e += ax
    shade.vy_52 += ay
    shade.delta_x_72 += shade.vx_4e
    shade.delta_y_76 += shade.vy_52


def state_08_grapple_latch_hold(
    shade: EntitySlot, ali: EntitySlot, ctx: ShadeSimCtx
) -> None:
    """0x0133D0..0x01341A."""
    ctx.calls.append("0x0134DE")
    ali.delta_x_72 = 0
    ali.delta_y_76 = 0
    if (shade.anim_flags_37 & 0x02) != 0:
        grapple_retract_init_state_0a(shade)


class TestROMShadeRescueGrappleHit(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.494")
        self.assertEqual(self.evidence["range"]["start"], "0x01319E")
        self.assertEqual(self.evidence["range"]["end"], "0x01341A")
        self.assertEqual(self.evidence["range"]["size_bytes"], 636)
        self.assertEqual(self.evidence["embedded_data_tables"], [])
        self.assertEqual(self.evidence["instruction_count"], 147)
        self.assertEqual(len(self.evidence["instructions"]), 147)
        self.assertEqual(sum(i["size"] for i in self.evidence["instructions"]), 636)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertEqual(
            [(name, routines[name]["size_bytes"], routines[name]["instruction_count"]) for name in routines],
            [
                ("AI_Shade_InsufficientSpBuzzer", 16, 3),
                ("AI_Shade_State0C_DarkHoleRescueAndDrain", 176, 39),
                ("AI_Shade_State0E_FinishDarkHoleRecovery", 76, 13),
                ("AI_Shade_State06_08_GrappleExtendAndHitLatch", 368, 92),
            ],
        )

        cursor = 0x01319E
        by_addr = {}
        for insn in self.evidence["instructions"]:
            addr = int(insn["address"], 16)
            self.assertEqual(addr, cursor)
            cursor += insn["size"]
            by_addr[addr] = insn
        self.assertEqual(cursor, 0x01341A)
        self.assertEqual(by_addr[0x01319E]["op_str"], "#$a00253, d0")
        self.assertEqual(by_addr[0x0131AA]["op_str"], "$136a0")
        self.assertEqual(by_addr[0x013250]["op_str"], "$2e1e.l")
        self.assertEqual(by_addr[0x01328C]["op_str"], "$2d84.l")
        self.assertEqual(by_addr[0x0133AE]["op_str"], "#$7, $86(a0)")
        self.assertEqual(by_addr[0x013406]["op_str"], "#$12, d1")

    def test_insufficient_sp_buzzer_and_dark_hole_rescue_drain(self) -> None:
        ctx = ShadeSimCtx()
        self.assertEqual(insufficient_sp_buzzer(ctx), "IDLE_CONT_0136A0")
        self.assertEqual(ctx.sfx_log, [0x00A00253])

        # Drain + blink loop while $FF1985 != 1 and Ali SP != 0
        shade = EntitySlot(x_pos_08=0x1200000, y_pos_0c=0x800000, sp_or_drain_8e=6, delta_x_72=9, delta_y_76=9)
        ali = EntitySlot(sp_or_drain_8e=15, flags_36=0x01, flags_38=0x44, z_pos_10=0x300000)

        ctx.frame_counter_1971 = 0  # bit 0 clear -> bset #1 on $36(a5)
        state_0c_dark_hole_rescue_and_drain(shade, ali, ctx)
        self.assertEqual((shade.delta_x_72, shade.delta_y_76, shade.sp_or_drain_8e), (0, 0, 2))
        self.assertEqual(ali.flags_36, 0x03)

        ctx.frame_counter_1971 = 1  # bit 0 set -> bclr #1 on $36(a5), drain 2 -> 0 clamped
        state_0c_dark_hole_rescue_and_drain(shade, ali, ctx)
        self.assertEqual(shade.sp_or_drain_8e, 0)
        self.assertEqual(ali.flags_36, 0x01)

        # Trigger teleport via $FF1985 == 1, and verify SP == 0 also triggers it
        for trigger_case in ("flag_1985", "zero_sp"):
            s = EntitySlot(x_pos_08=0x1500000, y_pos_0c=0x900000)
            a = EntitySlot(sp_or_drain_8e=10 if trigger_case == "flag_1985" else 0, flags_36=0x03, flags_38=0x4C, z_pos_10=0x200000)
            c = ShadeSimCtx(rescue_flag_1985=1 if trigger_case == "flag_1985" else 0, ali_piece_backup_d7e=0x0456)
            state_0c_dark_hole_rescue_and_drain(s, a, c)
            self.assertEqual((a.flags_36, a.flags_38, a.piece_flag_06), (0x01, 0x08, 0x0456))
            self.assertEqual((a.state_04, a.anim_2a, a.facing_16, a.hitstop_82), (0, 0, 2, 0x0A))
            self.assertEqual((a.x_pos_08, a.y_pos_0c, a.z_pos_10), (0x1500000, 0x900000, 0))
            self.assertEqual((s.state_04, c.hud_fade_164f, c.warp_fade_1896, c.teleport_17ba), (0x0E, 0, 1, 0xFF))
            self.assertEqual(c.calls, ["0x009D00(ali)", "0x01378A", "0x002E1E"])

    def test_state0e_finish_dark_hole_recovery(self) -> None:
        # $FF16F1 bit 1 clear -> $FF164F stays 0
        shade = EntitySlot(state_04=0x0E)
        ali = EntitySlot(flags_38=0x08)
        ctx = ShadeSimCtx(rescue_flag_1985=1, hud_fade_164f=0, warp_fade_1896=1, game_flags_16f1=0xFD)
        state_0e_finish_dark_hole_recovery(shade, ali, ctx)
        self.assertEqual((ctx.rescue_flag_1985, ali.flags_38, ctx.pause_lock_17b8, shade.state_04), (0, 0, 0, 0))
        self.assertEqual((ctx.hold_counter_199e, ctx.dtap_timer_19a1, ctx.prev_btn_a_19a0), (0, 0, 0xFF))
        self.assertEqual((ctx.warp_fade_1896, ctx.hud_fade_164f), (0, 0))
        self.assertEqual(ctx.calls, ["0x002D84"])

        # $FF16F1 bit 1 set -> restores $FF164F = 0xFF
        ctx2 = ShadeSimCtx(hud_fade_164f=0, game_flags_16f1=0x02)
        state_0e_finish_dark_hole_recovery(EntitySlot(state_04=0x0E), EntitySlot(), ctx2)
        self.assertEqual(ctx2.hud_fade_164f, 0xFF)

    def test_state06_grapple_extend_item_probe_and_retract(self) -> None:
        # 1. Acceleration & timer expiry -> State $08 (+$2A=$12)
        ali = EntitySlot(delta_x_72=50, delta_y_76=60)
        shade = EntitySlot(state_04=0x06, facing_16=1, timer_02=2, ground_z_14=8)
        ctx = ShadeSimCtx(item_probe_target=EntitySlot(ground_z_14=4))  # mismatched Z ignored
        state_06_grapple_extend(shade, ali, ctx)
        self.assertEqual((ali.delta_x_72, ali.delta_y_76), (0, 0))
        self.assertEqual((shade.timer_02, shade.vx_4e, shade.delta_x_72), (1, 0x8000, 0x8000))
        state_06_grapple_extend(shade, ali, ctx)  # timer 1 -> 0 exits to State $08
        self.assertEqual((shade.state_04, shade.anim_2a), (0x08, 0x12))

        # 2. Item probe match -> latches +$AA, calls 0x013550, +$2A=4, State $08
        item = EntitySlot(name="apple", ground_z_14=8)
        shade_item = EntitySlot(state_04=0x06, timer_02=10, ground_z_14=8)
        ctx_item = ShadeSimCtx(item_probe_target=item)
        state_06_grapple_extend(shade_item, ali, ctx_item)
        self.assertIs(shade_item.grapple_target_aa, item)
        self.assertEqual((shade_item.state_04, shade_item.anim_2a), (0x08, 4))
        self.assertIn("0x013550", ctx_item.calls)

        # 3. Wall contact (+$40|+$41 bit 6) -> immediate State $0A retract
        wall_shade = EntitySlot(state_04=0x06, wall_40=0x40, vx_4e=0x18000, vy_52=-0x8000, timer_02=5)
        state_06_grapple_extend(wall_shade, ali, ShadeSimCtx())
        self.assertEqual((wall_shade.state_04, wall_shade.anim_2a, wall_shade.timer_02, wall_shade.flags_38),
                         (0x0A, 6, 13, 0x04))
        self.assertEqual((wall_shade.vx_4e, wall_shade.vy_52), (-0x18000, 0x8000))

        # 4. State $08 hold -> State $0A retract on +$37 bit 1
        hold_shade = EntitySlot(state_04=0x08, vx_4e=0x10000, timer_02=6)
        ctx_hold = ShadeSimCtx()
        state_08_grapple_latch_hold(hold_shade, ali, ctx_hold)
        self.assertEqual((hold_shade.state_04, ctx_hold.calls), (0x08, ["0x0134DE"]))
        hold_shade.anim_flags_37 = 0x02
        state_08_grapple_latch_hold(hold_shade, ali, ctx_hold)
        self.assertEqual((hold_shade.state_04, hold_shade.anim_2a, hold_shade.timer_02, hold_shade.flags_38),
                         (0x0A, 6, 12, 0x04))

    def test_state06_grapple_entity_latch_and_enemy_strike(self) -> None:
        ali = EntitySlot()

        # 1. Grabbable (+$37 bit 7): mismatched Z skips latch; matching Z latches +$AA & calls 0x0134DE
        grab_bad = EntitySlot(anim_flags_37=0x80, ground_z_14=12)
        s1 = EntitySlot(state_04=0x06, wall_41=0x80, ground_z_14=4, contact_ptr_68=grab_bad)
        c1 = ShadeSimCtx()
        state_06_grapple_extend(s1, ali, c1)
        self.assertIsNone(s1.grapple_target_aa)
        self.assertEqual((s1.state_04, s1.anim_2a), (0x08, 0x12))

        grab_ok = EntitySlot(anim_flags_37=0x80, ground_z_14=4)
        s2 = EntitySlot(state_04=0x06, wall_40=0x80, ground_z_14=4, contact_ptr_68=grab_ok)
        c2 = ShadeSimCtx()
        state_06_grapple_extend(s2, ali, c2)
        self.assertIs(s2.grapple_target_aa, grab_ok)
        self.assertEqual((s2.state_04, s2.anim_2a, c2.calls), (0x08, 0x12, ["0x0134DE"]))

        # 2. Protected enemies (+$38:3, +$82!=0, or +$83!=0) do not take damage
        for kwargs in ({"flags_38": 0x08}, {"hitstop_82": 2}, {"hit_cooldown_83": 3}):
            e = EntitySlot(**kwargs)
            s = EntitySlot(state_04=0x06, wall_40=0x80, contact_ptr_68=e)
            c = ShadeSimCtx()
            state_06_grapple_extend(s, ali, c)
            self.assertEqual((e.dmg_word_86, c.sfx_log, s.state_04, s.anim_2a), (0, [], 0x08, 0x12))

        # 3. Vulnerable enemy: grounded vs airborne (+$10 != 0 -> +$86 |= 0x8000), level scaling
        for z_pos, lvl, exp_dmg in ((0, 0, 1), (0, 5, 3), (0x100000, 4, 0x8003)):
            enemy = EntitySlot(z_pos_10=z_pos, stun_timer_9d=44)
            shade = EntitySlot(state_04=0x06, wall_41=0x80, facing_16=1, contact_ptr_68=enemy)
            ctx = ShadeSimCtx(shade_level_db8=lvl)
            state_06_grapple_extend(shade, ali, ctx)
            self.assertEqual((ctx.hud_enemy_timer_184f, ctx.hud_enemy_ptr_1850), (0x5A, enemy))
            self.assertIs(enemy.attacker_ptr_7e, shade)
            self.assertEqual((enemy.hitstop_82, enemy.hit_cooldown_83, enemy.stun_timer_9d), (0, 0x0A, 0))
            self.assertEqual((enemy.kb_facing_84, enemy.vx_4e, enemy.vy_52, enemy.vz_56), (1, 0x50000, 0, 0x28000))
            self.assertEqual(enemy.dmg_word_86, exp_dmg)
            self.assertEqual(ctx.sfx_log, [0x00700345])
            self.assertEqual((shade.state_04, shade.anim_2a), (0x08, 0x12))


if __name__ == "__main__":
    unittest.main()
