#!/usr/bin/env python3
"""
Unit tests for ROM Shade Spirit Grapple Retraction, Entity/Item Reel-In Sync,
Dynamic Tether Stretch & Tip Probe Offset Table (0x01341A..0x01366A, 592 bytes).
Milestone M15.495.
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
    / "rom_shade_retract_tether_m15_495.json"
)
ITEM_POOL_BASE = 0x00FF2954


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val


def helper_00e2d4_velocity(vx_q8: int, vy_q8: int, facing: int) -> tuple[int, int]:
    d1, d2 = ((vx_q8 & 0xFFFF) << 8) & 0xFFFFFFFF, ((vy_q8 & 0xFFFF) << 8) & 0xFFFFFFFF
    return {0: (0, to_s32(-d2)), 1: (to_s32(d1), 0), 2: (0, to_s32(d2)), 3: (to_s32(-d1), 0)}.get(
        facing & 7, (to_s32(d1), to_s32(d2))
    )


@dataclass
class EntitySlot:
    addr: int = 0x00FF1AA4
    type_00: int = 0x001C
    timer_02: int = 10
    state_04: int = 0x0A
    x_08_fixed: int = 0x00A00000
    y_0c_fixed: int = 0x00780000
    z_10_fixed: int = 0
    z_14: int = 0
    facing_16: int = 1
    sprite_frame_32: int = 0x16
    flags_36: int = 0
    flags_38: int = 0
    rx_42: int = 8
    ry_44: int = 8
    vel_x_4e: int = 0
    vel_y_52: int = 0
    vel_x_72: int = 0
    vel_y_76: int = 0
    tether_ptr_a6: int = 0x00FF1C1C
    target_ptr_aa: int = 0
    anchor_dx_ae: int = 0
    anchor_dy_b2: int = 0

    @property
    def x_08_word(self) -> int:
        return to_s16((self.x_08_fixed >> 16) & 0xFFFF)

    @x_08_word.setter
    def x_08_word(self, val: int) -> None:
        self.x_08_fixed = to_s32(((val & 0xFFFF) << 16) | (self.x_08_fixed & 0xFFFF))

    @property
    def y_0c_word(self) -> int:
        return to_s16((self.y_0c_fixed >> 16) & 0xFFFF)

    @y_0c_word.setter
    def y_0c_word(self, val: int) -> None:
        self.y_0c_fixed = to_s32(((val & 0xFFFF) << 16) | (self.y_0c_fixed & 0xFFFF))

    @property
    def z_10_word(self) -> int:
        return to_s16((self.z_10_fixed >> 16) & 0xFFFF)


@dataclass
class SimCtx:
    ali: EntitySlot = field(default_factory=lambda: EntitySlot(addr=0x00FF19E8, type_00=1, flags_38=0x08))
    shade: EntitySlot = field(default_factory=lambda: EntitySlot(addr=0x00FF1AA4, type_00=0x1C))
    tether: EntitySlot = field(default_factory=lambda: EntitySlot(addr=0x00FF1C1C, type_00=0xFFFF))
    entities: dict[int, EntitySlot] = field(default_factory=dict)
    tip_table: list[list[tuple[int, int]]] = field(default_factory=list)
    lock_ff1982: int = 0xFF
    flags_ff17b8: int = 0x04
    terrain_refreshed_009d00: list[int] = field(default_factory=list)
    render_00b022_log: list[tuple[int, int, int, int]] = field(default_factory=list)


def lookup_tip_offset_013622(shade: EntitySlot, tip_table: list[list[tuple[int, int]]]) -> tuple[int, int]:
    f = shade.facing_16 & 3
    frame = shade.sprite_frame_32 & 0xFFFF
    tier = 0 if frame <= 0x17 else (1 if frame <= 0x1A else 2)
    return tip_table[tier][f]


def drag_item_to_tip_013550(shade: EntitySlot, ctx: SimCtx) -> bool:
    if (shade.sprite_frame_32 & 0xFFFF) < 0x15:
        return False
    target = ctx.entities[shade.target_ptr_aa]
    dx, dy = lookup_tip_offset_013622(shade, ctx.tip_table)
    target.x_08_word = to_s16(shade.x_08_word + dx)
    target.y_0c_word = to_s16(shade.y_0c_word + dy - shade.z_10_word)
    return True


def sync_ali_to_target_01352a(shade: EntitySlot, ctx: SimCtx) -> str:
    if shade.target_ptr_aa >= ITEM_POOL_BASE:
        drag_item_to_tip_013550(shade, ctx)
        return "DRAGGED_ITEM"
    target = ctx.entities[shade.target_ptr_aa]
    ctx.ali.x_08_fixed = to_s32(target.x_08_fixed + shade.anchor_dx_ae)
    ctx.ali.y_0c_fixed = to_s32(target.y_0c_fixed + shade.anchor_dy_b2)
    return "SYNCED_ALI"


def tether_anchor_lock_0134de(shade: EntitySlot, ctx: SimCtx) -> str:
    if shade.target_ptr_aa == 0:
        return "NO_TARGET"
    if shade.target_ptr_aa >= ITEM_POOL_BASE:
        drag_item_to_tip_013550(shade, ctx)
        return "DRAGGED_ITEM"
    target = ctx.entities[shade.target_ptr_aa]
    d1 = to_s32(ctx.ali.x_08_fixed - target.x_08_fixed)
    d2 = to_s32(ctx.ali.y_0c_fixed - target.y_0c_fixed)
    if ctx.ali.facing_16 & 1:
        shade.anchor_dx_ae = d1
        shade.anchor_dy_b2 = 0
        shade.y_0c_fixed = to_s32(shade.y_0c_fixed - d2)
        ctx.ali.y_0c_fixed = to_s32(ctx.ali.y_0c_fixed - d2)
        return "LOCKED_HORIZONTAL"
    shade.anchor_dy_b2 = d2
    shade.anchor_dx_ae = 0
    shade.x_08_fixed = to_s32(shade.x_08_fixed - d1)
    ctx.ali.x_08_fixed = to_s32(ctx.ali.x_08_fixed - d1)
    return "LOCKED_VERTICAL"


def complete_retract_01348e(shade: EntitySlot, ctx: SimCtx) -> str:
    tgt_ptr = shade.target_ptr_aa
    if tgt_ptr != 0:
        if tgt_ptr < ITEM_POOL_BASE:
            ctx.terrain_refreshed_009d00.append(tgt_ptr)
        else:
            item = ctx.entities[tgt_ptr]
            item.x_08_fixed = ctx.ali.x_08_fixed
            item.y_0c_fixed = ctx.ali.y_0c_fixed
    ctx.lock_ff1982 = 0
    ctx.ali.flags_38 &= ~0x08
    ctx.flags_ff17b8 &= ~0x04
    if ctx.tether.type_00 == 0xFFFF:
        ctx.tether.type_00 = 0
    shade.state_04 = 0
    return "COMPLETED_01378A"


def step_state0a_retract(shade: EntitySlot, ctx: SimCtx) -> str:
    ctx.ali.vel_x_72 = 0
    ctx.ali.vel_y_76 = 0
    shade.timer_02 = to_s16(shade.timer_02 - 1)
    if shade.timer_02 <= 0:
        return complete_retract_01348e(shade, ctx)
    if shade.target_ptr_aa == 0:
        d5 = abs(to_s16(ctx.ali.x_08_word - shade.x_08_word)) + abs(to_s16(ctx.ali.y_0c_word - shade.y_0c_word))
        if d5 <= 0x10:
            return complete_retract_01348e(shade, ctx)
    vx, vy = helper_00e2d4_velocity(0x0060, 0x0060, shade.facing_16)
    shade.vel_x_4e = to_s32(shade.vel_x_4e + vx)
    shade.vel_y_52 = to_s32(shade.vel_y_52 + vy)
    if shade.target_ptr_aa == 0:
        shade.vel_x_72 = to_s32(shade.vel_x_72 + shade.vel_x_4e)
        shade.vel_y_76 = to_s32(shade.vel_y_76 + shade.vel_y_52)
        return "RETRACT_EMPTY"
    if shade.target_ptr_aa < ITEM_POOL_BASE:
        shade.anchor_dx_ae = to_s32(shade.anchor_dx_ae - shade.vel_x_4e)
        shade.anchor_dy_b2 = to_s32(shade.anchor_dy_b2 - shade.vel_y_52)
        sync_ali_to_target_01352a(shade, ctx)
        return "RETRACT_ENTITY_REEL"
    shade.vel_x_72 = to_s32(shade.vel_x_72 + shade.vel_x_4e)
    shade.vel_y_76 = to_s32(shade.vel_y_76 + shade.vel_y_52)
    drag_item_to_tip_013550(shade, ctx)
    return "RETRACT_ITEM_DRAG"


def update_tether_stretch_013576(tether: EntitySlot, ctx: SimCtx) -> str:
    parent = ctx.shade if tether.tether_ptr_a6 == ctx.shade.addr else ctx.entities[tether.tether_ptr_a6]
    if parent.type_00 != 0x1C:
        tether.type_00 = 0
        return "DESPAWNED"
    if (parent.flags_36 & 0x02) != 0:
        return "BLINK_HIDDEN_SKIPPED"
    d0 = ctx.shade.x_08_word
    d1 = ctx.ali.x_08_word
    d2 = to_s16(ctx.shade.y_0c_word - ctx.shade.z_10_word)
    d3 = ctx.ali.y_0c_word
    cheb = max(abs(to_s16(d1 - d0)), abs(to_s16(d3 - d2)))
    if cheb <= 0x14:
        return "TOO_CLOSE_SKIPPED"
    idx = (cheb - 0x14) >> 3
    if idx >= 0x0C:
        idx = 0x0B
    tether.sprite_frame_32 = 0x60 + idx * 3
    tether.x_08_word = d0
    tether.y_0c_word = ctx.shade.y_0c_word
    tether.z_10_fixed = ctx.shade.z_10_fixed
    ctx.render_00b022_log.append((tether.sprite_frame_32, tether.x_08_word, tether.y_0c_word, tether.z_10_word))
    tether.y_0c_fixed = 0
    return "RENDERED_TETHER"


def probe_items_at_tip_013608(shade: EntitySlot, items: list[EntitySlot], tip_table: list[list[tuple[int, int]]]) -> EntitySlot | None:
    dx, dy = lookup_tip_offset_013622(shade, tip_table)
    x0 = shade.x_08_word + dx - 4
    x1 = shade.x_08_word + dx + 4
    yb = shade.y_0c_word + shade.z_14
    y0, y1 = yb + dy - 4, yb + dy + 4
    z_probe = shade.z_10_word + shade.z_14
    for item in items[:8]:
        if item.type_00 <= 0:
            continue
        iz0 = item.z_10_word + item.z_14
        if z_probe < iz0 or z_probe > iz0 + 16:
            continue
        if x0 <= item.x_08_word + item.rx_42 and x1 >= item.x_08_word - item.rx_42:
            iy = item.y_0c_word + item.z_14
            if y0 <= iy + item.ry_44 and y1 >= iy - item.ry_44:
                return item
    return None


class TestROMShadeRetractTether(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
        self.tip_table = [
            [(f["dx"], f["dy"]) for f in tier["facings"]]
            for tier in self.evidence["embedded_data_tables"][0]["tiers"]
        ]

    def test_evidence_structure_and_tip_table(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.495")
        self.assertEqual(
            self.evidence["range"],
            {
                "start": "0x01341A",
                "end": "0x01366A",
                "code_end": "0x013652",
                "code_size_bytes": 568,
                "table_size_bytes": 24,
                "size_bytes": 592,
            },
        )
        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 4)
        self.assertEqual(sum(r["size_bytes"] for r in routines), 592)
        self.assertEqual([r["instruction_count"] for r in routines], [51, 41, 42, 25])
        self.assertEqual(self.evidence["instruction_count"], 159)
        self.assertEqual(len(self.evidence["instructions"]), 159)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x01341A")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x013650")
        self.assertEqual(
            self.tip_table,
            [
                [(0, -24), (32, 0), (0, 24), (-32, 0)],
                [(0, -14), (16, 0), (0, 10), (-16, 0)],
                [(0, -2), (8, 0), (0, 0), (-8, 0)],
            ],
        )

    def test_state0a_grapple_retract_modes_and_completion(self) -> None:
        # 1. Empty retract: clears Ali vel, accelerates (0x60,0x60) into Shade vel, finishes at Manhattan <= $10
        ctx = SimCtx(tip_table=self.tip_table)
        ctx.ali.vel_x_72, ctx.ali.vel_y_76 = 0x10000, -0x10000
        sh = EntitySlot(timer_02=5, x_08_fixed=0x00800000, y_0c_fixed=0x00780000, facing_16=1, vel_x_4e=-0x00020000)
        self.assertEqual(step_state0a_retract(sh, ctx), "RETRACT_EMPTY")
        self.assertEqual((ctx.ali.vel_x_72, ctx.ali.vel_y_76), (0, 0))
        self.assertEqual((sh.vel_x_4e, sh.vel_x_72, sh.vel_y_76), (-0x0001A000, -0x0001A000, 0))

        sh.x_08_word, sh.y_0c_word = 152, 112  # |160-152| + |120-112| = 16 <= $10 -> early finish
        self.assertEqual(step_state0a_retract(sh, ctx), "COMPLETED_01378A")
        self.assertEqual((ctx.lock_ff1982, ctx.ali.flags_38, ctx.flags_ff17b8, ctx.tether.type_00, sh.state_04), (0, 0, 0, 0, 0))

        # 2. Entity pool target (< $FF2954): subtracts vel from +$AE/+$B2 & syncs Ali; completes via 0x009D00
        enemy = EntitySlot(addr=0x00FF1CD8, type_00=0x20, x_08_fixed=0x00E00000, y_0c_fixed=0x00780000)
        ctx2 = SimCtx(entities={enemy.addr: enemy}, tip_table=self.tip_table)
        sh_ent = EntitySlot(timer_02=2, target_ptr_aa=enemy.addr, facing_16=1, vel_x_4e=-0x00010000, anchor_dx_ae=-0x00400000)
        self.assertEqual(step_state0a_retract(sh_ent, ctx2), "RETRACT_ENTITY_REEL")
        self.assertEqual((sh_ent.vel_x_4e, sh_ent.anchor_dx_ae, ctx2.ali.x_08_fixed), (-0x0000A000, -0x003F6000, 0x00E00000 - 0x003F6000))
        self.assertEqual(step_state0a_retract(sh_ent, ctx2), "COMPLETED_01378A")
        self.assertEqual(ctx2.terrain_refreshed_009d00, [enemy.addr])

        # 3. Item pool target (>= $FF2954): drags item & snaps item to Ali on timer <= 0
        item = EntitySlot(addr=0x00FF2ABC, type_00=0x08, x_08_fixed=0x00C00000, y_0c_fixed=0x00780000)
        ctx3 = SimCtx(entities={item.addr: item}, tip_table=self.tip_table)
        sh_itm = EntitySlot(timer_02=2, target_ptr_aa=item.addr, facing_16=1, sprite_frame_32=0x16, vel_x_4e=-0x00010000)
        self.assertEqual(step_state0a_retract(sh_itm, ctx3), "RETRACT_ITEM_DRAG")
        self.assertEqual((item.x_08_word, item.y_0c_word), (160 + 32, 120))
        self.assertEqual(step_state0a_retract(sh_itm, ctx3), "COMPLETED_01378A")
        self.assertEqual((item.x_08_fixed, item.y_0c_fixed), (ctx3.ali.x_08_fixed, ctx3.ali.y_0c_fixed))

    def test_tether_anchor_lock_sync_and_item_drag(self) -> None:
        # 1. Empty and item-pool branches in 0x0134DE & +$32 < $15 guard in 0x013550
        item = EntitySlot(addr=0x00FF2B00, type_00=0x08, x_08_fixed=0x00500000, y_0c_fixed=0x00500000)
        ctx = SimCtx(entities={item.addr: item}, tip_table=self.tip_table)
        sh = EntitySlot(target_ptr_aa=0, sprite_frame_32=0x14, facing_16=2)
        self.assertEqual(tether_anchor_lock_0134de(sh, ctx), "NO_TARGET")
        sh.target_ptr_aa = item.addr
        self.assertEqual(tether_anchor_lock_0134de(sh, ctx), "DRAGGED_ITEM")
        self.assertEqual(item.x_08_fixed, 0x00500000)  # skipped because +$32 (0x14) < 0x15
        sh.sprite_frame_32 = 0x18
        sh.z_10_fixed = 0x00040000
        self.assertEqual(sync_ali_to_target_01352a(sh, ctx), "DRAGGED_ITEM")
        self.assertEqual((item.x_08_word, item.y_0c_word), (160, 120 + 10 - 4))

        # 2. Horizontal facing (bit 0 set): locks +$AE, clears +$B2, aligns Shade & Ali Y to target Y
        tgt = EntitySlot(addr=0x00FF1CD8, type_00=0x20, x_08_fixed=0x01000000, y_0c_fixed=0x00900000)
        ctx_h = SimCtx(entities={tgt.addr: tgt}, tip_table=self.tip_table)
        ctx_h.ali.facing_16 = 1
        ctx_h.ali.x_08_fixed, ctx_h.ali.y_0c_fixed = 0x00A00000, 0x00780000
        sh_h = EntitySlot(target_ptr_aa=tgt.addr, x_08_fixed=0x00E00000, y_0c_fixed=0x00780000)
        self.assertEqual(tether_anchor_lock_0134de(sh_h, ctx_h), "LOCKED_HORIZONTAL")
        self.assertEqual((sh_h.anchor_dx_ae, sh_h.anchor_dy_b2), (-0x00600000, 0))
        self.assertEqual((ctx_h.ali.y_0c_fixed, sh_h.y_0c_fixed), (0x00900000, 0x00900000))

        # 3. Vertical facing (bit 0 clear): locks +$B2, clears +$AE, aligns Shade & Ali X to target X
        ctx_v = SimCtx(entities={tgt.addr: tgt}, tip_table=self.tip_table)
        ctx_v.ali.facing_16 = 2
        ctx_v.ali.x_08_fixed, ctx_v.ali.y_0c_fixed = 0x00A00000, 0x00780000
        sh_v = EntitySlot(target_ptr_aa=tgt.addr, x_08_fixed=0x00A00000, y_0c_fixed=0x00880000)
        self.assertEqual(tether_anchor_lock_0134de(sh_v, ctx_v), "LOCKED_VERTICAL")
        self.assertEqual((sh_v.anchor_dx_ae, sh_v.anchor_dy_b2), (0, -0x00180000))
        self.assertEqual((ctx_v.ali.x_08_fixed, sh_v.x_08_fixed), (0x01000000, 0x01000000))

    def test_tether_stretch_entity_callback(self) -> None:
        # 1. Despawns if parent type != $1C; skips render (stays alive) if parent +$36 bit 1 set
        ctx = SimCtx(tip_table=self.tip_table)
        tether = EntitySlot(addr=0x00FF1C1C, type_00=0xFFFF, tether_ptr_a6=ctx.shade.addr)
        ctx.shade.type_00 = 0x18
        self.assertEqual(update_tether_stretch_013576(tether, ctx), "DESPAWNED")
        self.assertEqual(tether.type_00, 0)

        tether.type_00 = 0xFFFF
        ctx.shade.type_00 = 0x1C
        ctx.shade.flags_36 = 0x02
        self.assertEqual(update_tether_stretch_013576(tether, ctx), "BLINK_HIDDEN_SKIPPED")
        self.assertEqual(tether.type_00, 0xFFFF)

        # 2. Chebyshev <= 20 skipped; > 20 maps ((dist - 20) >> 3) clamped <= 11 to $60 + idx*3 & zeroes +$0C
        tether.type_00 = 0xFFFF
        ctx.shade.flags_36 = 0
        ctx.ali.x_08_word, ctx.ali.y_0c_word = 100, 100
        ctx.shade.x_08_word, ctx.shade.y_0c_word, ctx.shade.z_10_fixed = 120, 100, 0
        self.assertEqual(update_tether_stretch_013576(tether, ctx), "TOO_CLOSE_SKIPPED")

        # Vertical Chebyshev using shade_y - shade_z: |100 - (136 - 8)| = 28 -> (28 - 20) >> 3 = 1 -> $63
        ctx.shade.x_08_word, ctx.shade.y_0c_word, ctx.shade.z_10_fixed = 100, 136, 0x00080000
        self.assertEqual(update_tether_stretch_013576(tether, ctx), "RENDERED_TETHER")
        self.assertEqual((tether.sprite_frame_32, tether.y_0c_fixed), (0x63, 0))
        self.assertEqual(ctx.render_00b022_log[-1], (0x63, 100, 136, 8))

        # Far distance clamps index to 11 ($0B -> $60 + 33 = $81)
        ctx.shade.x_08_word = 300  # dist=200 -> (200-20)>>3 = 22 -> clamped 11 -> $81
        self.assertEqual(update_tether_stretch_013576(tether, ctx), "RENDERED_TETHER")
        self.assertEqual(tether.sprite_frame_32, 0x81)

    def test_grapple_tip_probe_and_aabb_lookup(self) -> None:
        # 1. Exact tier thresholds: <= $17 (tier 0), $18..$1A (tier 1), > $1A (tier 2) across all 4 facings
        for frame, exp in [
            (0x17, [(0, -24), (32, 0), (0, 24), (-32, 0)]),
            (0x18, [(0, -14), (16, 0), (0, 10), (-16, 0)]),
            (0x1A, [(0, -14), (16, 0), (0, 10), (-16, 0)]),
            (0x1B, [(0, -2), (8, 0), (0, 0), (-8, 0)]),
        ]:
            got = [lookup_tip_offset_013622(EntitySlot(facing_16=f, sprite_frame_32=frame), self.tip_table) for f in range(4)]
            self.assertEqual(got, exp)

        # 2. AABB [dx-4..dx+4, dy-4..dy+4] probe at grapple tip via 0x00B9EC
        sh = EntitySlot(x_08_fixed=0x00640000, y_0c_fixed=0x00640000, z_14=4, facing_16=3, sprite_frame_32=0x16)
        # Tip at x = 100 - 32 = 68 ([64..72]), y = 100 + 4 + 0 = 104 ([100..108])
        miss_item = EntitySlot(addr=0x00FF2ABC, type_00=0x08, x_08_fixed=0x00370000, y_0c_fixed=0x00640000, z_14=4, rx_42=8)
        hit_item = EntitySlot(addr=0x00FF2B16, type_00=0x08, x_08_fixed=0x00380000, y_0c_fixed=0x00640000, z_14=4, rx_42=8)
        self.assertIsNone(probe_items_at_tip_013608(sh, [miss_item], self.tip_table))
        self.assertIs(probe_items_at_tip_013608(sh, [miss_item, hit_item], self.tip_table), hit_item)


if __name__ == "__main__":
    unittest.main()
