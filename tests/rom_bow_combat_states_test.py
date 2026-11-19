#!/usr/bin/env python3
"""
Unit tests for ROM Bow Spirit (0x1E) Combat States $08, $04 Gravity Leap/Chomp & $06
(0x013B36..0x013CB4, 382 bytes).
Milestone M15.498.
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
    / "rom_bow_combat_states_m15_498.json"
)


def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val


@dataclass
class EntitySlot:
    type_id: int = 0x001E
    timer_02: int = 0
    state_04: int = 0
    x_pos_08: int = 0
    y_pos_0c: int = 0
    z_pos_10: int = 0
    facing_16: int = 0
    anim_2a: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    vx_4e: int = 0
    vy_52: int = 0
    vz_56: int = 0
    delta_x_72: int = 0
    delta_y_76: int = 0
    bonus_dmg_90: int = 0
    substate_9e: int = 0
    min_dist_ac: int = 0x2710
    backup_x_ae: int = 0
    backup_y_b2: int = 0


@dataclass
class BowSimCtx:
    bow_level_db9: int = 0
    hit_callback_1940: int = 0
    anchor_ff1c1c: EntitySlot = field(default_factory=lambda: EntitySlot(type_id=0xFFFF))
    proximity_hit: bool = False
    enemy_target: EntitySlot | None = None
    sfx_log: list[int] = field(default_factory=list)
    calls: list[str] = field(default_factory=list)


def chomp_hit_callback_013bde(enemy: EntitySlot, ctx: BowSimCtx) -> None:
    """0x013BDE..0x013BF2: custom chomp hit callback stored into $FF1940."""
    ctx.calls.append("0x00BE98")
    if enemy.vz_56 == 0:
        enemy.vx_4e = 0
        enemy.vy_52 = 0


def _state08_exit_to_idle(bow: EntitySlot, ctx: BowSimCtx) -> None:
    """0x013BB2..0x013BDC."""
    bow.state_04 = 0
    bow.anim_2a = 0
    bow.substate_9e = 2
    bow.vx_4e = 0
    bow.vy_52 = 0
    bow.vz_56 = 0
    bow.flags_38 &= ~(1 << 2)
    ctx.calls.append("0x014200(d7=9)")


def state08_chomp_attack_and_trail(bow: EntitySlot, ctx: BowSimCtx) -> None:
    """0x013B36..0x013BF2."""
    bow.timer_02 = (bow.timer_02 - 1) & 0xFFFF
    if bow.timer_02 == 0:
        _state08_exit_to_idle(bow, ctx)
        return

    if bow.anim_2a != 0x24:
        ctx.calls.append("0x00BBA8(a1=0x05887C)")
        if ctx.proximity_hit:
            bow.anim_2a = 0x24
    else:
        bow.bonus_dmg_90 = ((ctx.bow_level_db9 & 0xFF) + 1) >> 2
        ctx.hit_callback_1940 = 0x00013BDE
        ctx.calls.append(f"0x00BCAE(d0=2,dmg90={bow.bonus_dmg_90})")
        if ctx.enemy_target is not None:
            chomp_hit_callback_013bde(ctx.enemy_target, ctx)
        bow.bonus_dmg_90 = 0
        bow.timer_02 = (bow.timer_02 + 1) & 0xFFFF
        if (bow.anim_flags_37 & 0x02) != 0:
            _state08_exit_to_idle(bow, ctx)
            return

    bow.delta_x_72 += bow.vx_4e
    bow.delta_y_76 += bow.vy_52
    z_next = to_s32(bow.z_pos_10 + bow.vz_56)
    bow.z_pos_10 = 0 if z_next < 0 else z_next
    ctx.calls.append("0x014200(d7=12)")


def _enter_state06_from_state04(bow: EntitySlot, ctx: BowSimCtx) -> None:
    """0x013C52..0x013C7C."""
    bow.state_04 = 6
    bow.anim_2a = 0x12
    bow.facing_16 = 0
    bow.z_pos_10 = 0
    ctx.anchor_ff1c1c.type_id = 0
    ctx.sfx_log.append(0x0080037E)


def state04_gravity_leap_and_seek(bow: EntitySlot, ctx: BowSimCtx) -> None:
    """0x013BF2..0x013C7C."""
    vz = to_s32(bow.vz_56 - 0x0000C000)
    bow.vz_56 = vz
    z_next = to_s32(bow.z_pos_10 + vz)
    if z_next < 0:
        bow.z_pos_10 = 0
        bow.vz_56 = 0
    else:
        bow.z_pos_10 = z_next

    ctx.calls.append("0x013FFC")
    tx = (ctx.anchor_ff1c1c.vx_4e >> 16) & 0xFFFF
    ty = (ctx.anchor_ff1c1c.vy_52 >> 16) & 0xFFFF
    bx = (bow.x_pos_08 >> 16) & 0xFFFF
    by = (bow.y_pos_0c >> 16) & 0xFFFF
    dx = ((tx - bx) + 0x8000) % 0x10000 - 0x8000
    dy = ((ty - by) + 0x8000) % 0x10000 - 0x8000
    d5 = (abs(dx) + abs(dy)) & 0xFFFF
    ctx.calls.append(f"0x00E3A4(d5={d5})")

    if d5 <= 6 or d5 > (bow.min_dist_ac & 0xFFFF):
        _enter_state06_from_state04(bow, ctx)
        return
    d0 = (bow.timer_02 - 1) & 0xFFFF
    if d0 == 0:
        _enter_state06_from_state04(bow, ctx)
        return
    bow.timer_02 = d0
    bow.min_dist_ac = d5
    bow.delta_x_72 += bow.vx_4e
    bow.delta_y_76 += bow.vy_52


def state06_popup_and_restore_pos(bow: EntitySlot, ctx: BowSimCtx) -> None:
    """0x013C7C..0x013CB4."""
    if (bow.anim_flags_37 & 0x02) == 0:
        return
    bow.state_04 = 2
    bow.anim_2a = 0x14
    bow.x_pos_08 = bow.backup_x_ae
    bow.y_pos_0c = bow.backup_y_b2
    bow.z_pos_10 = 0
    ctx.calls.append("0x009D00")
    ctx.sfx_log.append(0x0080037E)


class TestROMBowCombatStates(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.498")
        self.assertEqual(self.evidence["range"]["start"], "0x013B36")
        self.assertEqual(self.evidence["range"]["end"], "0x013CB4")
        self.assertEqual(self.evidence["range"]["size_bytes"], 382)
        self.assertEqual(self.evidence["embedded_data_tables"], [])
        self.assertEqual(self.evidence["instruction_count"], 95)
        self.assertEqual(len(self.evidence["instructions"]), 95)
        self.assertEqual(sum(i["size"] for i in self.evidence["instructions"]), 382)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertEqual(
            [(n, routines[n]["size_bytes"], routines[n]["instruction_count"]) for n in routines],
            [
                ("AI_Bow_State08_ChompAttackAndTrail", 188, 48),
                ("AI_Bow_State04_GravityLeapAndSeek", 138, 35),
                ("AI_Bow_State06_PopupAndRestorePos", 56, 12),
            ],
        )

        cursor = 0x013B36
        by_addr = {}
        for insn in self.evidence["instructions"]:
            addr = int(insn["address"], 16)
            self.assertEqual(addr, cursor)
            cursor += insn["size"]
            by_addr[addr] = insn
        self.assertEqual(cursor, 0x013CB4)
        self.assertEqual(by_addr[0x013B44]["op_str"], "$5887c.l, a1")
        self.assertEqual(by_addr[0x013B6C]["op_str"], "#$13bde, $ff1940.l")
        self.assertEqual(by_addr[0x013BF6]["op_str"], "#$c000, d0")
        self.assertEqual(by_addr[0x013C24]["op_str"], "#$6, d5")
        self.assertEqual(by_addr[0x013CA2]["op_str"], "$9d00.l")

    def test_state08_proximity_probe_chomp_callback_and_exit(self) -> None:
        # 1. Pre-chomp: miss integrates velocity + Z floor clamp + trail d7=12
        bow = EntitySlot(
            state_04=8, timer_02=5, anim_2a=8, flags_38=0x04,
            vx_4e=0x10000, vy_52=-0x8000, vz_56=-0x20000, z_pos_10=0x10000,
        )
        ctx = BowSimCtx(proximity_hit=False)
        state08_chomp_attack_and_trail(bow, ctx)
        self.assertEqual((bow.timer_02, bow.anim_2a, bow.z_pos_10), (4, 8, 0))
        self.assertEqual((bow.delta_x_72, bow.delta_y_76), (0x10000, -0x8000))
        self.assertEqual(ctx.calls, ["0x00BBA8(a1=0x05887C)", "0x014200(d7=12)"])

        # 2. Timer expiry exits to idle 0x013BB2 with d7=9
        exp = EntitySlot(state_04=8, timer_02=1, flags_38=0x0C, vx_4e=99, vy_52=99, vz_56=99)
        ctx_exp = BowSimCtx()
        state08_chomp_attack_and_trail(exp, ctx_exp)
        self.assertEqual((exp.state_04, exp.anim_2a, exp.substate_9e, exp.flags_38), (0, 0, 2, 0x08))
        self.assertEqual((exp.vx_4e, exp.vy_52, exp.vz_56, ctx_exp.calls[-1]), (0, 0, 0, "0x014200(d7=9)"))

        # 3. Proximity hit latches chomp anim $24; next tick runs 0x00BCAE(2) + callback 0x013BDE
        ctx.calls.clear()
        ctx.proximity_hit = True
        state08_chomp_attack_and_trail(bow, ctx)
        self.assertEqual((bow.timer_02, bow.anim_2a), (3, 0x24))

        for lvl, exp_dmg in ((0, 0), (3, 1), (7, 2), (15, 4)):
            grounded = EntitySlot(vx_4e=0x30000, vy_52=0x40000, vz_56=0)
            c = BowSimCtx(bow_level_db9=lvl, enemy_target=grounded)
            b = EntitySlot(state_04=8, timer_02=3, anim_2a=0x24, flags_38=0x04)
            state08_chomp_attack_and_trail(b, c)
            self.assertEqual(c.hit_callback_1940, 0x00013BDE)
            self.assertEqual(b.bonus_dmg_90, 0)
            self.assertEqual(b.timer_02, 3)  # subq #1 + addq #1 = net 0 decay during chomp
            self.assertIn(f"0x00BCAE(d0=2,dmg90={exp_dmg})", c.calls)
            self.assertEqual((grounded.vx_4e, grounded.vy_52), (0, 0))

        # Airborne enemy keeps XY velocity; +$37 bit 1 finishes chomp -> 0x013BB2
        airborne = EntitySlot(vx_4e=0x30000, vy_52=0x40000, vz_56=0x10000)
        bow.anim_flags_37 = 0x02
        ctx.enemy_target = airborne
        state08_chomp_attack_and_trail(bow, ctx)
        self.assertEqual((airborne.vx_4e, airborne.vy_52), (0x30000, 0x40000))
        self.assertEqual((bow.state_04, bow.anim_2a, bow.substate_9e), (0, 0, 2))

    def test_state04_gravity_leap_floor_clamp_and_seek(self) -> None:
        # Active closing leap: gravity -0xC000, anchor sync, min_dist update, XY integration
        anchor = EntitySlot(type_id=0xFFFF, vx_4e=0x0140 << 16, vy_52=0x0080 << 16)
        ctx = BowSimCtx(anchor_ff1c1c=anchor)
        bow = EntitySlot(
            state_04=4, timer_02=30, x_pos_08=0x0100 << 16, y_pos_0c=0x0080 << 16,
            z_pos_10=0x00020000, vx_4e=0x00040000, vy_52=0, vz_56=0x00010000, min_dist_ac=0x2710,
        )
        state04_gravity_leap_and_seek(bow, ctx)
        self.assertEqual((bow.vz_56, bow.z_pos_10), (0x00004000, 0x00024000))
        self.assertEqual((bow.state_04, bow.timer_02, bow.min_dist_ac), (4, 29, 64))
        self.assertEqual((bow.delta_x_72, bow.delta_y_76), (0x00040000, 0))
        self.assertEqual(ctx.calls, ["0x013FFC", "0x00E3A4(d5=64)"])

        # Z floor clamp: if z_pos + vz < 0, clears both +$10 and +$56 to 0
        bow.z_pos_10 = 0x00004000
        bow.vz_56 = -0x00010000  # -> -0x1C000, sum < 0
        bow.x_pos_08 = 0x0120 << 16  # dist = 32 <= 64
        state04_gravity_leap_and_seek(bow, ctx)
        self.assertEqual((bow.z_pos_10, bow.vz_56, bow.min_dist_ac), (0, 0, 32))

    def test_state04_exit_conditions_to_state06(self) -> None:
        # 1. Proximity d5 <= 6 (boundary d5 == 6)
        ctx1 = BowSimCtx(anchor_ff1c1c=EntitySlot(type_id=0xFFFF, vx_4e=0x106 << 16, vy_52=0x80 << 16))
        b1 = EntitySlot(state_04=4, timer_02=10, facing_16=3, x_pos_08=0x100 << 16, y_pos_0c=0x80 << 16, z_pos_10=0x50000)
        state04_gravity_leap_and_seek(b1, ctx1)
        self.assertEqual((b1.state_04, b1.anim_2a, b1.facing_16, b1.z_pos_10), (6, 0x12, 0, 0))
        self.assertEqual((ctx1.anchor_ff1c1c.type_id, ctx1.sfx_log), (0, [0x0080037E]))

        # 2. Receding distance d5 > +$AC(a6)
        ctx2 = BowSimCtx(anchor_ff1c1c=EntitySlot(type_id=0xFFFF, vx_4e=0x115 << 16, vy_52=0x80 << 16))
        b2 = EntitySlot(state_04=4, timer_02=10, x_pos_08=0x100 << 16, y_pos_0c=0x80 << 16, min_dist_ac=20)
        state04_gravity_leap_and_seek(b2, ctx2)
        self.assertEqual((b2.state_04, b2.anim_2a, ctx2.anchor_ff1c1c.type_id), (6, 0x12, 0))

        # 3. Leap timer expiry (1 -> 0) while still closing in
        ctx3 = BowSimCtx(anchor_ff1c1c=EntitySlot(type_id=0xFFFF, vx_4e=0x120 << 16, vy_52=0x80 << 16))
        b3 = EntitySlot(state_04=4, timer_02=1, x_pos_08=0x100 << 16, y_pos_0c=0x80 << 16, min_dist_ac=40)
        state04_gravity_leap_and_seek(b3, ctx3)
        self.assertEqual((b3.state_04, b3.anim_2a, ctx3.anchor_ff1c1c.type_id, ctx3.sfx_log), (6, 0x12, 0, [0x0080037E]))

    def test_state06_popup_and_position_restore(self) -> None:
        ctx = BowSimCtx()
        bow = EntitySlot(
            state_04=6, anim_2a=0x12, x_pos_08=0x200 << 16, y_pos_0c=0x140 << 16,
            z_pos_10=0x30000, backup_x_ae=0x080 << 16, backup_y_b2=0x090 << 16,
        )
        state06_popup_and_restore_pos(bow, ctx)
        self.assertEqual((bow.state_04, ctx.calls, ctx.sfx_log), (6, [], []))

        bow.anim_flags_37 = 0x02
        state06_popup_and_restore_pos(bow, ctx)
        self.assertEqual((bow.state_04, bow.anim_2a), (2, 0x14))
        self.assertEqual((bow.x_pos_08, bow.y_pos_0c, bow.z_pos_10), (0x080 << 16, 0x090 << 16, 0))
        self.assertEqual((ctx.calls, ctx.sfx_log), (["0x009D00"], [0x0080037E]))


if __name__ == "__main__":
    unittest.main()
