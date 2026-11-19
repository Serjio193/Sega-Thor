#!/usr/bin/env python3
"""
Unit tests for ROM Bow Spirit (0x1E) State $00 Idle, Command Dispatch,
Vulnerability/Wander & Leashed Follow AI (0x013CB4..0x013F22, 622 bytes).
Milestone M15.499.
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
    / "rom_bow_idle_follow_m15_499.json"
)


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val


@dataclass
class BowEntity:
    timer_02: int = 10
    state_04: int = 0
    x_08: int = 160
    y_0c: int = 120
    z_10_fixed: int = 0
    action_2a: int = 0
    anim_delay_2e: int = 0
    sprite_frame_32: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    vel_x_4e: int = 0
    vel_y_52: int = 0
    vel_z_56: int = 0
    vel_x_72: int = 0
    vel_y_76: int = 0
    attack_bonus_90: int = 0
    substate_9e: int = 0


@dataclass
class SimCtx:
    dismiss_ff19a7: int = 0
    cmd_ff19a2: int = 0
    bow_rank_ff0db9: int = 3
    prng_queue: list[int] = field(default_factory=lambda: [0x0080])
    pounce_scan_triggers: bool = False
    pounce_hitbox_00bba8_hit: bool = False
    calls: list[tuple[str, int]] = field(default_factory=list)
    sfx_log: list[int] = field(default_factory=list)
    observed_bonus_90: list[int] = field(default_factory=list)

    def next_prng(self) -> int:
        if len(self.prng_queue) > 1:
            return self.prng_queue.pop(0) & 0xFFFF
        return self.prng_queue[0] & 0xFFFF


def substate03_exit_013e96(bow: BowEntity, ctx: SimCtx) -> str:
    bow.action_2a = 0
    bow.substate_9e = 2
    bow.vel_x_4e = 0
    bow.vel_y_52 = 0
    bow.vel_z_56 = 0
    bow.flags_38 &= ~0x04
    ctx.calls.append(("0x014200", 6))
    return "POUNCE_EXIT_TO_02"


def step_substate03_auto_pounce(bow: BowEntity, ctx: SimCtx) -> str:
    bow.timer_02 = to_s16(bow.timer_02 - 1)
    if bow.timer_02 == 0:
        return substate03_exit_013e96(bow, ctx)
    if bow.action_2a != 8:
        ctx.calls.append(("0x00BBA8", 0x05887C))
        if ctx.pounce_hitbox_00bba8_hit:
            bow.action_2a = 8
    else:
        bonus = (((ctx.bow_rank_ff0db9 & 0xFF) + 1) >> 2) * 2
        bow.attack_bonus_90 = bonus
        ctx.observed_bonus_90.append(bonus)
        ctx.calls.append(("0x00BCA4", 0))
        bow.attack_bonus_90 = 0
        if bow.anim_flags_37 & 0x02:
            return substate03_exit_013e96(bow, ctx)
    bow.vel_x_72 = to_s32(bow.vel_x_72 + bow.vel_x_4e)
    bow.vel_y_76 = to_s32(bow.vel_y_76 + bow.vel_y_52)
    bow.z_10_fixed = to_s32(bow.z_10_fixed + bow.vel_z_56)
    if bow.z_10_fixed < 0:
        bow.z_10_fixed = 0
    ctx.calls.append(("0x014200", 8))
    return "POUNCE_ACTIVE"


def step_substate04_emote_settle(bow: BowEntity, ctx: SimCtx) -> str:
    if bow.sprite_frame_32 == 0x3F and bow.anim_delay_2e == 0:
        ctx.sfx_log.append(0x00700235)
    if bow.anim_flags_37 & 0x02:
        bow.substate_9e = 2
        bow.action_2a = 0
    bow.vel_x_72 = to_s32(bow.vel_x_72 + bow.vel_x_4e)
    bow.vel_y_76 = to_s32(bow.vel_y_76 + bow.vel_y_52)
    bow.vel_z_56 = to_s32(bow.vel_z_56 - 0x0000C000)
    bow.z_10_fixed = to_s32(bow.z_10_fixed + bow.vel_z_56)
    if bow.z_10_fixed < 0:
        bow.z_10_fixed = 0
        bow.vel_x_4e = 0
        bow.vel_y_52 = 0
    ctx.calls.append(("0x014200", 6))
    return "SUBSTATE_04_STEP"


def step_bow_state00(bow: BowEntity, ctx: SimCtx, entry_013cdc: bool = False) -> str:
    if not entry_013cdc:
        if (ctx.dismiss_ff19a7 & 0xFF) != 0:
            return "DISMISS_013F2A"
        if ctx.cmd_ff19a2 in (1, 2, 3):
            return {1: "CMD_1_013924", 2: "CMD_2_013996", 3: "CMD_3_013A18"}[ctx.cmd_ff19a2]

    if bow.substate_9e == 3:
        return step_substate03_auto_pounce(bow, ctx)

    ctx.calls.append(("0x013FDC", 0))
    ctx.calls.append(("0x0143C8", 0))
    if ctx.pounce_scan_triggers:
        bow.action_2a = 0x22
        bow.timer_02 = 0x0C
        bow.substate_9e = 3

    s = bow.substate_9e & 0xFF
    if s == 1:
        ctx.calls.append(("0x014242", 6))
        bow.timer_02 = to_s16(bow.timer_02 - 1)
        if bow.timer_02 == 0:
            bow.substate_9e = 0
        return "SUBSTATE_01_REST"
    if s == 2:
        bow.vel_x_72 = to_s32(bow.vel_x_72 + bow.vel_x_4e)
        bow.vel_y_76 = to_s32(bow.vel_y_76 + bow.vel_y_52)
        bow.vel_z_56 = to_s32(bow.vel_z_56 - 0x0000C000)
        bow.z_10_fixed = to_s32(bow.z_10_fixed + bow.vel_z_56)
        if bow.z_10_fixed < 0:
            bow.z_10_fixed = 0
            bow.substate_9e = 0
        ctx.calls.append(("0x014200", 6))
        return "SUBSTATE_02_HOP"
    if s == 3:
        return step_substate03_auto_pounce(bow, ctx)
    if s == 4:
        return step_substate04_emote_settle(bow, ctx)

    bow.flags_38 &= ~0x04
    ctx.calls.append(("0x014242", 6))
    r0 = ctx.next_prng() & 0xFF
    if r0 <= 0xC0:
        bow.vel_z_56 = to_s32(((ctx.next_prng() & 0xFFFF) << 2) + 0x00030000)
        bow.vel_x_4e = to_s32(to_s16(ctx.next_prng()) << 1)
        bow.vel_y_52 = to_s32(to_s16(ctx.next_prng()) << 1)
        bow.substate_9e = 2
        return "START_HOP_02"
    if r0 <= 0xC8:
        bow.action_2a, bow.substate_9e = 2, 4
        ctx.sfx_log.append(0x0070025C)
        return "EMOTE_02"
    if r0 <= 0xD0:
        bow.action_2a, bow.substate_9e = 4, 4
        ctx.sfx_log.append(0x00700221)
        return "EMOTE_04"
    if r0 <= 0xD8:
        bow.action_2a, bow.substate_9e = 0x0A, 4
        return "EMOTE_0A"
    if r0 <= 0xE0:
        bow.action_2a, bow.substate_9e = 0x0E, 4
        ctx.sfx_log.append(0x00700234)
        return "EMOTE_0E"
    bow.timer_02 = (ctx.next_prng() & 0x0F) + 0x0C
    bow.substate_9e = 1
    return "START_REST_01"


class TestROMBowIdleFollow(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure_and_constants(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.499")
        self.assertEqual(
            self.evidence["range"],
            {"start": "0x013CB4", "end": "0x013F22", "size_bytes": 622},
        )
        self.assertEqual(self.evidence["embedded_data_tables"], [])
        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 4)
        self.assertEqual(sum(r["size_bytes"] for r in routines), 622)
        self.assertEqual([r["size_bytes"] for r in routines], [246, 124, 150, 102])
        self.assertEqual([r["instruction_count"] for r in routines], [61, 37, 40, 25])
        self.assertEqual(self.evidence["instruction_count"], 163)
        self.assertEqual(len(self.evidence["instructions"]), 163)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x013CB4")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x013F20")
        self.assertEqual(self.evidence["instructions"][-1]["mnemonic"], "rts")

    def test_entry_dispatch_reject_bypass_and_pounce_scan(self) -> None:
        # 1. $FF19A7 dismissal preempts commands; $FF19A2 1/2/3 dispatch
        self.assertEqual(
            step_bow_state00(BowEntity(), SimCtx(dismiss_ff19a7=1, cmd_ff19a2=2)),
            "DISMISS_013F2A",
        )
        for code, tgt in [(1, "CMD_1_013924"), (2, "CMD_2_013996"), (3, "CMD_3_013A18")]:
            self.assertEqual(step_bow_state00(BowEntity(), SimCtx(cmd_ff19a2=code)), tgt)

        # 2. 0x013CDC reject re-entry bypasses commands/dismissal; +$9E==3 fast-paths past 0x013FDC/0x0143C8
        ctx_fast = SimCtx(dismiss_ff19a7=1, cmd_ff19a2=1)
        b_fast = BowEntity(substate_9e=3, timer_02=5)
        self.assertEqual(step_bow_state00(b_fast, ctx_fast, entry_013cdc=True), "POUNCE_ACTIVE")
        self.assertEqual(ctx_fast.calls, [("0x00BBA8", 0x05887C), ("0x014200", 8)])

        # 3. Normal path calls 0x013FDC + 0x0143C8; auto-pounce trigger switches into Substate 3 immediately
        ctx_scan = SimCtx(pounce_scan_triggers=True)
        b_scan = BowEntity(substate_9e=0)
        self.assertEqual(step_bow_state00(b_scan, ctx_scan), "POUNCE_ACTIVE")
        self.assertEqual(b_scan.substate_9e, 3)
        self.assertEqual(ctx_scan.calls[:2], [("0x013FDC", 0), ("0x0143C8", 0)])

    def test_idle_prng_selector_emotes_rest_and_hop(self) -> None:
        # 1. PRNG <= $C0 starts hop (+$9E=2) with unsigned vz and signed vx/vy
        ctx_hop = SimCtx(prng_queue=[0x00C0, 0x0001, 0xFFFF, 0x0002])
        b_hop = BowEntity(flags_38=0x04)
        self.assertEqual(step_bow_state00(b_hop, ctx_hop), "START_HOP_02")
        self.assertEqual(b_hop.flags_38 & 0x04, 0)
        self.assertEqual((b_hop.substate_9e, b_hop.vel_z_56), (2, 0x00030004))
        self.assertEqual((b_hop.vel_x_4e, b_hop.vel_y_52), (-2, 4))

        # 2. All 4 emote bands ($C1..$E0) and rest band (>$E0)
        for rng, exp_act, exp_sfx in [
            (0xC8, 2, [0x0070025C]),
            (0xD0, 4, [0x00700221]),
            (0xD8, 0x0A, []),
            (0xE0, 0x0E, [0x00700234]),
        ]:
            ctx_e = SimCtx(prng_queue=[rng])
            b_e = BowEntity()
            step_bow_state00(b_e, ctx_e)
            self.assertEqual((b_e.action_2a, b_e.substate_9e, ctx_e.sfx_log), (exp_act, 4, exp_sfx))

        ctx_r = SimCtx(prng_queue=[0xE1, 0x00FF])
        b_r = BowEntity()
        self.assertEqual(step_bow_state00(b_r, ctx_r), "START_REST_01")
        self.assertEqual((b_r.substate_9e, b_r.timer_02), (1, 0x0F + 0x0C))

        # 3. Substate 1 counts down timer -> +$9E=0; Substate 2 applies gravity -$C000 & clamps z>=0 -> +$9E=0
        b_r.timer_02 = 1
        self.assertEqual(step_bow_state00(b_r, SimCtx()), "SUBSTATE_01_REST")
        self.assertEqual((b_r.timer_02, b_r.substate_9e), (0, 0))

        b_arc = BowEntity(substate_9e=2, z_10_fixed=0x00004000, vel_z_56=0, vel_x_4e=0x1000)
        ctx_arc = SimCtx()
        self.assertEqual(step_bow_state00(b_arc, ctx_arc), "SUBSTATE_02_HOP")
        self.assertEqual((b_arc.z_10_fixed, b_arc.substate_9e, b_arc.vel_x_4e), (0, 0, 0x1000))
        self.assertEqual(ctx_arc.calls[-1], ("0x014200", 6))

    def test_substate03_auto_pounce_and_leash_physics(self) -> None:
        # 1. Hitbox probe 0x00BBA8 ($05887C) triggers +$2A=8; z clamps >=0 without gravity; 8px leash
        b = BowEntity(substate_9e=3, timer_02=5, action_2a=0x22, z_10_fixed=0x00002000, vel_z_56=-0x00004000)
        ctx = SimCtx(pounce_hitbox_00bba8_hit=True, bow_rank_ff0db9=3)
        self.assertEqual(step_bow_state00(b, ctx), "POUNCE_ACTIVE")
        self.assertEqual((b.action_2a, b.z_10_fixed), (8, 0))
        self.assertEqual(ctx.calls, [("0x00BBA8", 0x05887C), ("0x014200", 8)])

        # 2. +$2A==8 sets 2x bonus (($FF0DB9+1)>>2)*2 = 2, calls 0x00BCA4(d0=0), clears +$90
        ctx.calls.clear()
        self.assertEqual(step_bow_state00(b, ctx), "POUNCE_ACTIVE")
        self.assertEqual((ctx.observed_bonus_90, b.attack_bonus_90), ([2], 0))
        self.assertIn(("0x00BCA4", 0), ctx.calls)

        # 3. Exit on +$37 bit 1 or timer==0 -> 0x013E96 resets +$2A=0, +$9E=2, zeroes vel, clears +$38 bit 2
        b.anim_flags_37 = 0x02
        b.flags_38 = 0x04
        b.vel_x_4e, b.vel_y_52, b.vel_z_56 = 0x100, 0x200, 0x300
        self.assertEqual(step_bow_state00(b, ctx), "POUNCE_EXIT_TO_02")
        self.assertEqual((b.action_2a, b.substate_9e, b.flags_38), (0, 2, 0))
        self.assertEqual((b.vel_x_4e, b.vel_y_52, b.vel_z_56), (0, 0, 0))
        self.assertEqual(ctx.calls[-1], ("0x014200", 6))

        b_to = BowEntity(substate_9e=3, timer_02=1, flags_38=0x04)
        self.assertEqual(step_bow_state00(b_to, SimCtx()), "POUNCE_EXIT_TO_02")
        self.assertEqual((b_to.timer_02, b_to.substate_9e), (0, 2))

    def test_substate04_emote_sfx_and_ballistic_settle(self) -> None:
        # 1. Plays SFX #$700235 only when +$32==$3F and +$2E==0
        ctx = SimCtx()
        b_mute = BowEntity(substate_9e=4, sprite_frame_32=0x3F, anim_delay_2e=1)
        step_bow_state00(b_mute, ctx)
        self.assertEqual(ctx.sfx_log, [])

        b_snd = BowEntity(substate_9e=4, sprite_frame_32=0x3F, anim_delay_2e=0, z_10_fixed=0x00020000)
        step_bow_state00(b_snd, ctx)
        self.assertEqual(ctx.sfx_log, [0x00700235])
        self.assertEqual((b_snd.vel_z_56, b_snd.z_10_fixed), (-0x0000C000, 0x00014000))

        # 2. Landing (< 0) zeroes +$10, +$4E, and +$52 while preserving +$9E; +$37 bit 1 switches +$9E=2, +$2A=0
        b_land = BowEntity(substate_9e=4, action_2a=6, vel_x_4e=0x1234, vel_y_52=-0x5678, vel_z_56=0)
        step_bow_state00(b_land, ctx)
        self.assertEqual((b_land.z_10_fixed, b_land.vel_x_4e, b_land.vel_y_52), (0, 0, 0))
        self.assertEqual((b_land.substate_9e, b_land.action_2a), (4, 6))

        b_land.anim_flags_37 = 0x02
        step_bow_state00(b_land, ctx)
        self.assertEqual((b_land.substate_9e, b_land.action_2a), (2, 0))
        self.assertEqual(ctx.calls[-1], ("0x014200", 6))


if __name__ == "__main__":
    unittest.main()
