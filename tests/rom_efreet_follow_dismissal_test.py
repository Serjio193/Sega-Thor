#!/usr/bin/env python3
"""
Unit tests for ROM Efreet Idle/Wander/Follow AI, Vulnerability Scanner,
Dismissal & Hover Bobbing Subsystem (0x012988..0x012C4E, 710 bytes).
Milestone M15.491.
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
    / "rom_efreet_follow_dismissal_m15_491.json"
)


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val

def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val

def m68k_divs_w(num: int, den: int) -> int:
    return to_s16(int(num / den))

def helper_00e2d4_velocity(vx_q8: int, vy_q8: int, facing: int) -> tuple[int, int]:
    d1, d2 = ((vx_q8 & 0xFFFF) << 8) & 0xFFFFFFFF, ((vy_q8 & 0xFFFF) << 8) & 0xFFFFFFFF
    return {0: (0, to_s32(-d2)), 1: (to_s32(d1), 0), 2: (0, to_s32(d2)), 3: (to_s32(-d1), 0)}.get(
        facing & 7, (to_s32(d1), to_s32(d2))
    )

def helper_00e398_ali_vector(
    self_x: int, self_y: int, ali_x: int, ali_y: int
) -> tuple[int, int, int, int]:
    d1, d2 = to_s16(ali_x - self_x), to_s16(ali_y - self_y)
    d3, d4 = abs(d1), abs(d2)
    d0 = (2 if d2 >= 0 else 0) if d4 > d3 else (1 if d1 >= 0 else 3)
    return d0, d3, d4, (d3 + d4) & 0xFFFF

def helper_00e45e_homing(
    self_x: int, self_y: int, target_x: int, target_y: int, facing_16: int
) -> tuple[int, int, int, int]:
    d1, d2 = to_s16(target_x - self_x), to_s16(target_y - self_y)
    d4, d5 = abs(d1), abs(d2)
    d3 = 1
    if d4 <= d5:
        d4, d3 = d5, 0
    d4 >>= 1
    if d4 == 0:
        return 0, 0, facing_16, 0
    vx = to_s32(m68k_divs_w(d1 << 8, d4) << 8)
    vy = to_s32(m68k_divs_w(d2 << 8, d4) << 8)
    if (vx | vy) == 0:
        return 0, 0, facing_16, 0
    if d3 != 0:
        if vx < 0:
            d3 += 2
    elif vy >= 0:
        d3 += 2
    return vx, vy, d3, d4


@dataclass
class EntitySlot:
    type_00: int = 0x001A
    state_04: int = 0
    field_06: int = 0
    x_08: int = 160
    y_0c: int = 120
    z_10_fixed: int = 0
    z_14: int = 0
    facing_16: int = 0
    action_2a: int = 0
    anim_offset_30: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    flags_3a: int = 0
    rx_42: int = 8
    ry_44: int = 8
    height_4a: int = 16
    vel_x_4e: int = 0
    vel_y_52: int = 0
    vel_x_72: int = 0
    vel_y_76: int = 0
    wander_timer_9e: int = 0
    sub_entity_a2: int = 0
    flame_slot_ptr_a6: int = 0
    facing_debounce_ac: int = 0
    saved_facing_b2: int = 0

    @property
    def z_10_word(self) -> int:
        return to_s16((self.z_10_fixed >> 16) & 0xFFFF)

    @z_10_word.setter
    def z_10_word(self, val: int) -> None:
        self.z_10_fixed = to_s32(((val & 0xFFFF) << 16) | (self.z_10_fixed & 0xFFFF))


@dataclass
class SimCtx:
    ali: EntitySlot = field(default_factory=lambda: EntitySlot(type_00=0x0001))
    pool2_enemies: list[EntitySlot] = field(default_factory=list)
    hitbox_rows: list[dict] = field(default_factory=list)
    bobbing_words: list[int] = field(default_factory=list)
    dismiss_ff19a7: int = 0
    cmd_ff19a2: int = 0
    prng_queue: list[int] = field(default_factory=lambda: [0x0000])
    flame_slot_ff1c1c_type: int = 0
    sfx_log: list[int] = field(default_factory=list)

    def next_prng(self) -> int:
        if len(self.prng_queue) > 1:
            return self.prng_queue.pop(0) & 0xFFFF
        return self.prng_queue[0] & 0xFFFF


def helper_00bab8_enemy_scan(efreet: EntitySlot, ctx: SimCtx) -> bool:
    f = efreet.facing_16 & 0xFFFF
    if f == 3:
        r = ctx.hitbox_rows[1]
        z_min, z_max, x_min, y_min, x_max, y_max = (
            r["z_min"], r["z_max"], -r["x_max"], r["y_min"], -r["x_min"], r["y_max"]
        )
    else:
        r = ctx.hitbox_rows[f]
        z_min, z_max, x_min, y_min, x_max, y_max = (
            r["z_min"], r["z_max"], r["x_min"], r["y_min"], r["x_max"], r["y_max"]
        )
    zb = efreet.z_10_word + efreet.z_14
    z_min, z_max = z_min + zb, z_max + zb
    x_min, x_max = x_min + efreet.x_08, x_max + efreet.x_08
    yb = efreet.y_0c + efreet.z_14
    y_min, y_max = y_min + yb, y_max + yb

    for slot in ctx.pool2_enemies[:17]:
        if slot.type_00 <= 0 or slot is efreet:
            continue
        if (slot.flags_38 & 0x08) or (slot.flags_3a & 0x01):
            continue
        tz0 = slot.z_10_word + slot.z_14
        tz1 = tz0 + slot.height_4a
        if z_max < tz0 or z_min > tz1:
            continue
        tx0, tx1 = slot.x_08 - slot.rx_42, slot.x_08 + slot.rx_42
        if x_min > tx1 or x_max < tx0:
            continue
        ty0 = slot.y_0c + slot.z_14 - slot.ry_44
        ty1 = slot.y_0c + slot.z_14 + slot.ry_44
        if y_min > ty1 or y_max < ty0:
            continue
        return True
    return False


def track_efreet_elevation(efreet: EntitySlot, ali: EntitySlot) -> None:
    d5 = to_s16(ali.z_14)
    d0 = to_s16(efreet.z_14)
    if d0 < 0:
        d0 = d5
    d5 = to_s16(d5 + 0x10)
    d4 = efreet.z_10_word
    delta = to_s16(d0 + d4 - d5)
    if delta == 0:
        return
    if delta < 0:
        efreet.z_10_word = to_s16(d4 + 1)
    elif delta > 0x0C and d4 > 0:
        efreet.z_10_word = to_s16(d4 - 1)


def integrate_efreet_bobbing(efreet: EntitySlot, bobbing_words: list[int]) -> None:
    word_idx = (efreet.anim_offset_30 >> 2) & 0x03
    delta = to_s32(to_s16(bobbing_words[word_idx]) * 2)
    efreet.z_10_fixed = to_s32(efreet.z_10_fixed + delta)


def wander_oscillate_012ac6(vel_l: int, timer_9e: int) -> int:
    step = 0x00002000 if (timer_9e & 0xFF) > 0x0F else -0x00002000
    if vel_l == 0:
        return 0
    if vel_l > 0:
        return to_s32(vel_l + step)
    return to_s32(vel_l - step)


def start_dismissal_012ba8(efreet: EntitySlot, ctx: SimCtx) -> str:
    ctx.dismiss_ff19a7 = 0xFF
    efreet.state_04 = 0x06
    efreet.action_2a = 0x06
    efreet.facing_16 = 2
    efreet.flags_38 |= 0x08
    efreet.flags_36 &= ~0x02
    efreet.sub_entity_a2 = 0xFF
    ctx.sfx_log.append(0x00A0039E)
    if efreet.flame_slot_ptr_a6 == 0x00FF1C1C and ctx.flame_slot_ff1c1c_type == 0xFFFE:
        ctx.flame_slot_ff1c1c_type = 0
    efreet.field_06 = 0
    return "DISMISS_012BA8"


def state06_despawn_012bf0(efreet: EntitySlot) -> str:
    if efreet.anim_flags_37 & 0x02:
        efreet.type_00 = 0
        return "DESPAWNED"
    return "WAITING_DESPAWN"


def efreet_idle_wander_follow_tick(
    efreet: EntitySlot, ctx: SimCtx, skip_cmd: bool = False
) -> str:
    if not skip_cmd:
        if (ctx.dismiss_ff19a7 & 0xFF) != 0:
            return start_dismissal_012ba8(efreet, ctx)
        if ctx.cmd_ff19a2 in (1, 2, 3):
            return {1: "CMD_1_012608", 2: "CMD_2_0125CA", 3: "CMD_3_01263C"}[ctx.cmd_ff19a2]

    if efreet.state_04 == 0x10:
        return "STATE_10_012950"

    orig_facing = efreet.facing_16 & 0xFFFF
    efreet.saved_facing_b2 = orig_facing
    for candidate in (
        orig_facing,
        (orig_facing + 1) & 3,
        (orig_facing - 1) & 3,
        orig_facing ^ 2,
    ):
        efreet.facing_16 = candidate
        if helper_00bab8_enemy_scan(efreet, ctx):
            efreet.state_04 = 0x10
            efreet.action_2a = 0x0A
            return f"VULN_HIT_FACING_{candidate}"
    efreet.facing_16 = efreet.saved_facing_b2

    track_efreet_elevation(efreet, ctx.ali)
    if ctx.bobbing_words:
        integrate_efreet_bobbing(efreet, ctx.bobbing_words)

    if efreet.state_04 == 2:
        tx, ty = ctx.ali.x_08, ctx.ali.y_0c
        if efreet.z_14 >= 0 and ctx.ali.z_14 >= 0:
            ty = to_s16(ty + to_s16(ctx.ali.z_14 - efreet.z_14))
        efreet.action_2a = 0
        offsets = {0: (-0x20, 0x28), 1: (-0x28, -0x20), 2: (0x20, -0x28), 3: (0x28, 0x20)}
        ox, oy = offsets.get(ctx.ali.facing_16 & 0xFF, (0x28, 0x20))
        tx, ty = to_s16(tx + ox), to_s16(ty + oy)
        vx, vy, d3, d4 = helper_00e45e_homing(efreet.x_08, efreet.y_0c, tx, ty, efreet.facing_16)
        if efreet.facing_debounce_ac != 0:
            efreet.facing_debounce_ac = (efreet.facing_debounce_ac - 1) & 0xFF
        if d4 == 0:
            efreet.state_04 = 0
            efreet.wander_timer_9e = 0
            efreet.vel_x_72 = 0
            efreet.vel_y_76 = 0
            efreet.facing_16 = ctx.ali.facing_16
            return "FOLLOW_ARRIVED_STATE_00"
        if d4 > 4:
            efreet.vel_x_72 = to_s32(vx << 1)
            efreet.vel_y_76 = to_s32(vy << 1)
            tier = "FOLLOW_FAST"
        else:
            efreet.vel_x_72 = vx
            efreet.vel_y_76 = vy
            tier = "FOLLOW_NORMAL"
        if d3 != efreet.facing_16 and efreet.facing_debounce_ac == 0:
            efreet.facing_debounce_ac = 4
            efreet.facing_16 = d3
        return tier

    if efreet.wander_timer_9e != 0:
        efreet.vel_x_4e = wander_oscillate_012ac6(efreet.vel_x_4e, efreet.wander_timer_9e)
        efreet.vel_x_72 = to_s32(efreet.vel_x_72 + efreet.vel_x_4e)
        efreet.vel_y_52 = wander_oscillate_012ac6(efreet.vel_y_52, efreet.wander_timer_9e)
        efreet.vel_y_76 = to_s32(efreet.vel_y_76 + efreet.vel_y_52)
        efreet.wander_timer_9e = (efreet.wander_timer_9e - 1) & 0xFF
        return "WANDER_STEP"

    d0, dx_abs, dy_abs, _ = helper_00e398_ali_vector(efreet.x_08, efreet.y_0c, ctx.ali.x_08, ctx.ali.y_0c)
    d4 = max(dx_abs, dy_abs)
    if d4 <= 0x40:
        if ctx.next_prng() >= 0x7FFF:
            turn = (ctx.next_prng() & 2) - 1
            efreet.facing_16 = (efreet.facing_16 + turn) & 3
    elif d4 <= 0x80:
        efreet.facing_16 = d0
    else:
        efreet.state_04 = 2
        efreet.action_2a = 0
        return "SWITCH_STATE_02"

    efreet.wander_timer_9e = 0x1E
    efreet.vel_x_4e, efreet.vel_y_52 = helper_00e2d4_velocity(0x0100, 0x0100, efreet.facing_16)
    return "WANDER_INIT"


class TestROMEfreetFollowDismissal(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
        self.hitbox_rows = self.evidence["external_data_tables"]["vuln_scan_hitbox_table_0586e8"]["facings"]
        self.bobbing_words = [
            e["signed_word"] for e in self.evidence["embedded_data_tables"][0]["entries"]
        ]

    def test_evidence_structure_and_tables(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.491")
        self.assertEqual(
            self.evidence["range"],
            {
                "start": "0x012988",
                "end": "0x012C4E",
                "code_end": "0x012C46",
                "code_size_bytes": 702,
                "table_size_bytes": 8,
                "size_bytes": 710,
            },
        )
        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 2)
        self.assertEqual(sum(r["size_bytes"] for r in routines), 710)
        self.assertEqual((routines[0]["instruction_count"], routines[1]["instruction_count"]), (154, 46))
        self.assertEqual(self.evidence["instruction_count"], 200)
        self.assertEqual(len(self.evidence["instructions"]), 200)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x012988")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x012C44")

        bob_entries = self.evidence["embedded_data_tables"][0]["entries"]
        self.assertEqual([e["raw_word_hex"] for e in bob_entries], ["0x7000", "0x3000", "0x9000", "0xD000"])
        self.assertEqual([e["doubled_fixed32"] for e in bob_entries], [0xE000, 0x6000, -0xE000, -0x6000])
        self.assertEqual(len(self.hitbox_rows), 3)
        self.assertEqual(
            [(r["x_min"], r["y_min"], r["x_max"], r["y_max"]) for r in self.hitbox_rows],
            [(-24, -40, 23, -9), (8, -16, 47, 15), (-24, 8, 23, 39)],
        )

    def test_entry_dispatch_and_4dir_vuln_scanner(self) -> None:
        # 1. Dismissal $FF19A7 preempts $FF19A2 commands; commands 1/2/3 dispatch; State $10 -> 0x012950
        ctx = SimCtx(dismiss_ff19a7=1, cmd_ff19a2=2, hitbox_rows=self.hitbox_rows, bobbing_words=self.bobbing_words)
        self.assertEqual(efreet_idle_wander_follow_tick(EntitySlot(), ctx), "DISMISS_012BA8")
        for code, tgt in [(1, "CMD_1_012608"), (2, "CMD_2_0125CA"), (3, "CMD_3_01263C")]:
            self.assertEqual(
                efreet_idle_wander_follow_tick(EntitySlot(), SimCtx(cmd_ff19a2=code, hitbox_rows=self.hitbox_rows)),
                tgt,
            )
        self.assertEqual(
            efreet_idle_wander_follow_tick(EntitySlot(state_04=0x10), SimCtx(hitbox_rows=self.hitbox_rows)),
            "STATE_10_012950",
        )

        # 2. 4-dir scan order [f, (f+1)&3, (f-1)&3, f^2], Left mirroring [-47..-8], bit3/bit0 skips, restore +$B2
        ef = EntitySlot(x_08=200, y_0c=200, facing_16=0)
        e_dead = EntitySlot(type_00=0, x_08=200, y_0c=180)
        e_intang = EntitySlot(type_00=0x20, x_08=200, y_0c=180, flags_38=0x08)
        e_prot = EntitySlot(type_00=0x20, x_08=200, y_0c=180, flags_3a=0x01)
        ctx_miss = SimCtx(pool2_enemies=[e_dead, e_intang, e_prot], hitbox_rows=self.hitbox_rows)
        efreet_idle_wander_follow_tick(ef, ctx_miss)
        self.assertEqual((ef.facing_16, ef.saved_facing_b2, ef.state_04), (0, 0, 0))

        # Left (dx=-40 in [-47..-8]) + Down (y=228, outside Right y<=215): facing=0 picks (f-1)&3=3 before f^2=2
        e_left = EntitySlot(type_00=0x20, x_08=160, y_0c=200)
        e_down = EntitySlot(type_00=0x20, x_08=200, y_0c=228)
        self.assertEqual(
            efreet_idle_wander_follow_tick(ef, SimCtx(pool2_enemies=[e_down, e_left], hitbox_rows=self.hitbox_rows)),
            "VULN_HIT_FACING_3",
        )
        self.assertEqual((ef.facing_16, ef.state_04, ef.action_2a), (3, 0x10, 0x0A))

        # Opposite Down hit only -> f^2=2; and skip_cmd=True (0x0129B0 buzzer re-entry) ignores cmd/dismiss
        ef2 = EntitySlot(x_08=200, y_0c=200, facing_16=0)
        self.assertEqual(
            efreet_idle_wander_follow_tick(ef2, SimCtx(pool2_enemies=[e_down], hitbox_rows=self.hitbox_rows)),
            "VULN_HIT_FACING_2",
        )
        self.assertEqual(
            efreet_idle_wander_follow_tick(EntitySlot(state_04=0x10), ctx, skip_cmd=True),
            "STATE_10_012950",
        )

    def test_state00_chebyshev_wander_and_vel_oscillation(self) -> None:
        # 1. Chebyshev max(|dx|,|dy|): (64,64) Manhattan=128 stays <= $40 PRNG branch; >$80 switches to State $02
        ali = EntitySlot(type_00=1, x_08=164, y_0c=164)
        ef = EntitySlot(x_08=100, y_0c=100, facing_16=1)
        ctx_near = SimCtx(ali=ali, hitbox_rows=self.hitbox_rows, prng_queue=[0x8000, 0x0002])  # >=7FFF, +1 -> 2
        self.assertEqual(efreet_idle_wander_follow_tick(ef, ctx_near), "WANDER_INIT")
        self.assertEqual((ef.facing_16, ef.wander_timer_9e, ef.vel_x_4e, ef.vel_y_52), (2, 0x1E, 0, 0x10000))

        # $40 < d4 <= $80 faces Ali directly (dx=90, dy=10 -> Right=1)
        ef_mid = EntitySlot(x_08=100, y_0c=100, facing_16=0)
        ctx_mid = SimCtx(ali=EntitySlot(type_00=1, x_08=190, y_0c=110), hitbox_rows=self.hitbox_rows)
        self.assertEqual(efreet_idle_wander_follow_tick(ef_mid, ctx_mid), "WANDER_INIT")
        self.assertEqual((ef_mid.facing_16, ef_mid.vel_x_4e, ef_mid.vel_y_52), (1, 0x10000, 0))

        # d4 > $80 switches directly to State $02 without starting wander
        ef_far = EntitySlot(x_08=100, y_0c=100)
        ctx_far = SimCtx(ali=EntitySlot(type_00=1, x_08=229, y_0c=100), hitbox_rows=self.hitbox_rows)
        self.assertEqual(efreet_idle_wander_follow_tick(ef_far, ctx_far), "SWITCH_STATE_02")
        self.assertEqual((ef_far.state_04, ef_far.wander_timer_9e), (2, 0))

        # 2. Full 30-frame wander oscillation: 15 accel (+$2000 -> 0x2E000) then 15 decel (-$2000 -> 0x10000)
        peaks = []
        for _ in range(30):
            self.assertEqual(efreet_idle_wander_follow_tick(ef_mid, ctx_mid), "WANDER_STEP")
            peaks.append(ef_mid.vel_x_4e)
        self.assertEqual(ef_mid.wander_timer_9e, 0)
        self.assertEqual(peaks[0], 0x12000)
        self.assertEqual(peaks[14], 0x2E000)
        self.assertEqual(peaks[15], 0x2C000)
        self.assertEqual(peaks[29], 0x10000)
        # Negative velocity oscillation (-0x10000 -> -0x2E000 -> -0x10000) & PRNG < 0x7FFF keep-facing
        self.assertEqual(wander_oscillate_012ac6(-0x10000, 30), -0x12000)
        self.assertEqual(wander_oscillate_012ac6(-0x2E000, 15), -0x2C000)
        ef_keep = EntitySlot(x_08=100, y_0c=100, facing_16=3)
        efreet_idle_wander_follow_tick(ef_keep, SimCtx(ali=ali, hitbox_rows=self.hitbox_rows, prng_queue=[0x7FFE]))
        self.assertEqual((ef_keep.facing_16, ef_keep.vel_x_4e), (3, -0x10000))

    def test_state02_diagonal_follow_and_facing_debounce(self) -> None:
        # 1. 4-way ($28, $20) behind-Ali offsets + Z-delta y-compensation
        expected = {0: (100 - 32, 100 + 40), 1: (100 - 40, 100 - 32), 2: (100 + 32, 100 - 40), 3: (100 + 40, 100 + 32)}
        for ali_f, (tx, ty) in expected.items():
            ali = EntitySlot(type_00=1, x_08=100, y_0c=100, z_14=12, facing_16=ali_f)
            ef = EntitySlot(state_04=2, x_08=tx, y_0c=ty + 4, z_14=8, facing_16=3, wander_timer_9e=5)
            ctx = SimCtx(ali=ali, hitbox_rows=self.hitbox_rows)
            self.assertEqual(efreet_idle_wander_follow_tick(ef, ctx), "FOLLOW_ARRIVED_STATE_00")
            self.assertEqual((ef.state_04, ef.wander_timer_9e, ef.vel_x_72, ef.vel_y_76, ef.facing_16), (0, 0, 0, 0, ali_f))

        # 2. Fast (d4 > 4 -> 2x vel) vs normal (1 <= d4 <= 4 -> 1x vel) + 4-frame facing debounce +$AC
        ali = EntitySlot(type_00=1, x_08=200, y_0c=200, facing_16=2)  # target = (232, 160)
        ef = EntitySlot(state_04=2, x_08=200, y_0c=160, facing_16=0)  # dx=32 -> d4=16 > 4 -> fast Right(1)
        ctx = SimCtx(ali=ali, hitbox_rows=self.hitbox_rows)
        self.assertEqual(efreet_idle_wander_follow_tick(ef, ctx), "FOLLOW_FAST")
        self.assertEqual((ef.vel_x_72, ef.facing_16, ef.facing_debounce_ac), (0x00040000, 1, 4))

        # Move to (232, 170) -> target requires facing Up(0), but +$AC ticks 4->3->2->1 before allowing flip
        ef.x_08, ef.y_0c = 232, 166  # dy=-6 -> d4=3 <= 4 -> normal speed, desired facing=0
        for rem_cd in (3, 2, 1):
            self.assertEqual(efreet_idle_wander_follow_tick(ef, ctx), "FOLLOW_NORMAL")
            self.assertEqual((ef.facing_debounce_ac, ef.facing_16, ef.vel_y_76), (rem_cd, 1, -0x00020000))
        self.assertEqual(efreet_idle_wander_follow_tick(ef, ctx), "FOLLOW_NORMAL")
        self.assertEqual((ef.facing_debounce_ac, ef.facing_16), (4, 0))

    def test_dismissal_despawn_elevation_hysteresis_and_bobbing(self) -> None:
        # 1. Dismissal starter 0x012BA8 + reserved $FF1C1C slot release + State $06 despawn
        ef = EntitySlot(
            state_04=0, field_06=0x12, flags_36=0x03, flame_slot_ptr_a6=0x00FF1C1C
        )
        ctx = SimCtx(flame_slot_ff1c1c_type=0xFFFE, hitbox_rows=self.hitbox_rows)
        self.assertEqual(start_dismissal_012ba8(ef, ctx), "DISMISS_012BA8")
        self.assertEqual((ctx.dismiss_ff19a7, ef.state_04, ef.action_2a, ef.facing_16), (0xFF, 6, 6, 2))
        self.assertEqual((ef.flags_38 & 0x08, ef.flags_36 & 0x02, ef.sub_entity_a2, ef.field_06), (0x08, 0, 0xFF, 0))
        self.assertEqual((ctx.sfx_log, ctx.flame_slot_ff1c1c_type), ([0x00A0039E], 0))

        self.assertEqual(state06_despawn_012bf0(ef), "WAITING_DESPAWN")
        ef.anim_flags_37 = 0x02
        self.assertEqual(state06_despawn_012bf0(ef), "DESPAWNED")
        self.assertEqual(ef.type_00, 0)

        # 2. Elevation tracker 0x012BFE: +16 offset, <0 surface fallback, >$0C (12) downward hysteresis
        ali = EntitySlot(z_14=10)
        el = EntitySlot(z_14=-1)
        el.z_10_word = 15  # fallback self_surf=10; (10+15) - (10+16) = -1 -> +1
        track_efreet_elevation(el, ali)
        self.assertEqual(el.z_10_word, 16)

        el.z_14 = 10
        el.z_10_word = 28  # (10+28) - 26 = +12 <= $0C -> holds steady inside hysteresis band
        track_efreet_elevation(el, ali)
        self.assertEqual(el.z_10_word, 28)

        el.z_10_word = 29  # (10+29) - 26 = +13 > $0C -> descends by 1
        track_efreet_elevation(el, ali)
        self.assertEqual(el.z_10_word, 28)

        # 3. Bobbing integrator 0x012C32 + 4-word table 0x012C46 across 20-frame loop (5 frames per keyframe)
        bob = EntitySlot(z_10_fixed=0)
        z_trace = []
        for off in (0, 4, 8, 12):
            bob.anim_offset_30 = off
            for _ in range(5):
                integrate_efreet_bobbing(bob, self.bobbing_words)
                z_trace.append(bob.z_10_fixed)
        self.assertEqual(z_trace[9], 0x00064000)   # peak +6.25 px after +5*(0.875 + 0.375)
        self.assertEqual(z_trace[-1], 0)           # symmetric net 0 over full 4-keyframe cycle


if __name__ == "__main__":
    unittest.main()
