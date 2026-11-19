#!/usr/bin/env python3
"""
Unit tests for ROM Bow Spirit (0x1E) Dismissal, States $10/$02, Neck/Vine
Chain Interpolator, SAT Sub-Entity Renderer & Leash Velocity Clamper
(0x013F22..0x014242, 800 bytes). Milestone M15.500.
"""

from __future__ import annotations

from dataclasses import dataclass, field
import json
from pathlib import Path
from typing import Callable
import unittest

EVIDENCE_PATH = (
    Path(__file__).resolve().parent.parent
    / "data"
    / "thor-evidence"
    / "rom_bow_dismiss_vine_sat_m15_500.json"
)


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def to_s32(val: int) -> int:
    val &= 0xFFFFFFFF
    return val - 0x100000000 if val & 0x80000000 else val


def m68k_divs_w(num: int, den: int) -> int:
    return to_s16(int(num / den))


def m68k_asr_w(val: int, shift: int) -> int:
    return to_s16(to_s16(val) >> shift)


def m68k_asl_w(val: int, shift: int) -> int:
    return to_s16(to_s16(val) << shift)


def compute_cardinal_facing_00e338(dx: int, dy: int) -> int:
    """Model of 0x00E338 (32-bit dx, dy -> 0=Up, 1=Right, 2=Down, 3=Left)."""
    abs_x = -dx if dx < 0 else dx
    abs_y = -dy if dy < 0 else dy
    if abs_x > abs_y:
        return 3 if dx < 0 else 1
    return 0 if dy < 0 else 2


@dataclass
class BowEntity:
    type_00: int = 0x001E
    state_04: int = 0
    x_pos_08: int = 0
    y_pos_0c: int = 0
    z_pos_10: int = 0
    facing_16: int = 1
    vram_attr_18: int = 0x8000
    render_cb_22: int = 0
    action_2a: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0x04
    anchor_x_4e: int = 0
    anchor_y_52: int = 0
    anchor_z_56: int = 0
    delta_x_72: int = 0
    delta_y_76: int = 0
    flags_9e: int = 1
    link_a6: BowEntity | None = None


@dataclass
class BowSimCtx:
    plane_flag_16f0: int = 0
    cam_a: tuple[int, int] = (0, 0)
    cam_b: tuple[int, int] = (0, 0)
    sat_count_188a: int = 0
    sat_offset_188c: int = 0
    sat_entries: list[tuple[int, int, int, int]] = field(default_factory=list)
    vine_nodes: list[tuple[int, int, int]] = field(
        default_factory=lambda: [(0, 0, 0)] * 4
    )
    vine_slot_ff1c1c: BowEntity = field(default_factory=lambda: BowEntity(type_00=0))
    spirit_slot_ff1aa4: BowEntity = field(default_factory=lambda: BowEntity(type_00=0x001E))
    sfx_log: list[int] = field(default_factory=list)
    calls: list[str] = field(default_factory=list)


def update_facing_from_anchor(bow: BowEntity, vine: BowEntity) -> int:
    """0x013FDC..0x013FFA."""
    dx = to_s32(bow.x_pos_08 - vine.anchor_x_4e)
    dy = to_s32(bow.y_pos_0c - vine.anchor_y_52)
    bow.facing_16 = compute_cardinal_facing_00e338(dx, dy)
    return bow.facing_16


def interpolate_vine_chain_nodes(bow: BowEntity, ctx: BowSimCtx) -> list[tuple[int, int, int]]:
    """0x013FFC..0x014084."""
    vine = ctx.vine_slot_ff1c1c
    head_y = to_s16(bow.y_pos_0c >> 16)
    anchor_y = to_s16(vine.anchor_y_52 >> 16)
    sort_y = to_s16(head_y - 2)
    if sort_y > anchor_y:
        sort_y = anchor_y
    vine.y_pos_0c = (sort_y << 16) | (vine.y_pos_0c & 0xFFFF)

    d1 = to_s16(vine.anchor_x_4e >> 16)
    d2 = anchor_y
    d3 = to_s16(vine.anchor_z_56 >> 16)
    d4 = to_s16((bow.x_pos_08 >> 16) - d1)
    d5 = to_s16(head_y - d2)
    d6 = to_s16((bow.z_pos_10 >> 16) - d3)

    step_x = m68k_divs_w(d4 << 4, 5)
    step_y = m68k_divs_w(d5 << 4, 5)
    step_z = m68k_divs_w(d6 << 4, 5)

    d1 = m68k_asl_w(d1, 4)
    d2 = m68k_asl_w(d2, 4)
    d3 = m68k_asl_w(d3, 4)

    nodes: list[tuple[int, int, int]] = []
    for _ in range(4):
        d1 = to_s16(d1 + step_x)
        d2 = to_s16(d2 + step_y)
        d3 = to_s16(d3 + step_z)
        nx = m68k_asr_w(d1, 4)
        ny = m68k_asr_w(d2, 4)
        nz = m68k_asr_w(d3, 4)
        nodes.append((nx, ny, nz))
        d1 = m68k_asl_w(nx, 4)
        d2 = m68k_asl_w(ny, 4)
        d3 = m68k_asl_w(nz, 4)
    ctx.vine_nodes = nodes
    return nodes


def ai_bow_dismissal(bow: BowEntity, ctx: BowSimCtx) -> str:
    """0x013F22..0x013F5A."""
    if bow.state_04 != 0:
        ctx.calls.append("0x0138B8")
        return "BLINK_CHECK_0138B8"
    ctx.sfx_log.append(0x00A0039E)
    if bow.link_a6 is not None:
        bow.link_a6.type_00 = 0
    bow.state_04 = 0x10
    bow.action_2a = 0x1E
    bow.facing_16 = 0
    bow.vram_attr_18 = 0x07A8
    bow.flags_36 &= ~0x01
    return "DISMISS_STATE10"


def state10_descend_despawn(bow: BowEntity) -> str:
    """0x013F5A..0x013F6E."""
    bow.flags_36 &= ~0x02
    if (bow.anim_flags_37 & 0x02) != 0:
        bow.type_00 = 0
        return "DESPAWNED"
    return "DESCENDING"


def state02_return_and_settle(bow: BowEntity, ctx: BowSimCtx) -> str:
    """0x013F6E..0x013FDC."""
    if (bow.anim_flags_37 & 0x02) == 0:
        return "WAITING"
    bow.state_04 = 0
    bow.action_2a = 0
    bow.flags_9e = 0
    bow.facing_16 = 1
    bow.flags_38 &= ~0x04

    vine = ctx.vine_slot_ff1c1c
    vine.type_00 = 0xFFFF
    vine.render_cb_22 = 0x00014084
    vine.vram_attr_18 = (bow.vram_attr_18 & 0x8000) | 0x07A8
    vine.anchor_x_4e = bow.x_pos_08
    vine.anchor_y_52 = bow.y_pos_0c
    vine.anchor_z_56 = bow.z_pos_10
    vine.link_a6 = bow
    bow.link_a6 = vine
    interpolate_vine_chain_nodes(bow, ctx)
    return "SETTLED"


def render_vine_sub_entity_sat(vine: BowEntity, ctx: BowSimCtx) -> str:
    """0x014084..0x014200."""
    parent = vine.link_a6
    if parent is None or parent.type_00 != 0x001E:
        vine.type_00 = 0
        return "FREED_SELF"
    if (parent.flags_36 & 0x02) != 0:
        return "BLINK_HIDDEN"

    cam_x, cam_y = ctx.cam_b if (ctx.plane_flag_16f0 & 0x80) else ctx.cam_a
    d5 = ctx.sat_count_188a

    def emit(sx: int, sy: int, tile_d3: int, size_d4: int) -> None:
        nonlocal d5
        if d5 >= 0x50:
            return
        if sx < 0x68 or sx >= 0x1C0 or sy < 0x70 or sy >= 0x160:
            return
        d5 += 1
        ctx.sat_entries.append((sy, (size_d4 + d5) & 0xFFFF, tile_d3 & 0xFFFF, sx))
        ctx.sat_offset_188c += 8

    for nx, ny, nz in reversed(ctx.vine_nodes):
        emit(to_s16(nx - cam_x + 0x7C), to_s16(ny - cam_y + 0x7C - nz), vine.vram_attr_18, 0x0000)

    sp = ctx.spirit_slot_ff1aa4
    emit(
        to_s16((sp.x_pos_08 >> 16) - cam_x + 0x78),
        to_s16((sp.y_pos_0c >> 16) - cam_y + 0x7D),
        0x0549,
        0x0400,
    )

    ax = to_s16(vine.anchor_x_4e >> 16)
    ay = to_s16(vine.anchor_y_52 >> 16)
    az = to_s16(vine.anchor_z_56 >> 16)
    emit(
        to_s16(ax - cam_x + 0x71),
        to_s16(ay - cam_y + 0x74 - az),
        vine.vram_attr_18 + 9,
        0x0E00,
    )

    ctx.sat_count_188a = d5
    return "RENDERED"


def step_and_clamp_velocity_by_leash(
    bow: BowEntity, leash_clamp_014242: Callable[[BowEntity], None]
) -> None:
    """0x014200..0x014242."""
    orig_x = bow.x_pos_08
    orig_y = bow.y_pos_0c
    bow.x_pos_08 = to_s32(orig_x + bow.delta_x_72)
    bow.y_pos_0c = to_s32(orig_y + bow.delta_y_76)
    leash_clamp_014242(bow)
    clamped_x = bow.x_pos_08
    clamped_y = bow.y_pos_0c
    bow.x_pos_08 = orig_x
    bow.y_pos_0c = orig_y
    bow.delta_x_72 = to_s32(clamped_x - orig_x)
    bow.delta_y_76 = to_s32(clamped_y - orig_y)


class TestROMBowDismissVineSat(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure_and_disassembly(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.500")
        self.assertEqual(self.evidence["range"]["start"], "0x013F22")
        self.assertEqual(self.evidence["range"]["end"], "0x014242")
        self.assertEqual(self.evidence["range"]["size_bytes"], 800)
        self.assertEqual(self.evidence["instruction_count"], 206)
        self.assertEqual(len(self.evidence["instructions"]), 206)
        self.assertEqual(sum(i["size"] for i in self.evidence["instructions"]), 800)

        cursor = 0x013F22
        for insn in self.evidence["instructions"]:
            self.assertEqual(int(insn["address"], 16), cursor)
            cursor += insn["size"]
        self.assertEqual(cursor, 0x014242)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertEqual(
            list(routines.keys()),
            [
                "AI_Bow_Dismissal",
                "AI_Bow_State10_DescendDespawn",
                "AI_Bow_State02_ReturnAndSettle",
                "AI_Bow_InterpolateVineChainNodes",
                "AI_Bow_RenderVineSubEntitySAT",
                "AI_Bow_StepAndClampVelocityByLeash",
            ],
        )
        expected_meta = {
            "AI_Bow_Dismissal": (56, 12),
            "AI_Bow_State10_DescendDespawn": (20, 5),
            "AI_Bow_State02_ReturnAndSettle": (110, 24),
            "AI_Bow_InterpolateVineChainNodes": (168, 55),
            "AI_Bow_RenderVineSubEntitySAT": (380, 92),
            "AI_Bow_StepAndClampVelocityByLeash": (66, 18),
        }
        for name, (sz, cnt) in expected_meta.items():
            self.assertEqual((routines[name]["size_bytes"], routines[name]["instruction_count"]), (sz, cnt))

        by_addr = {int(i["address"], 16): i for i in self.evidence["instructions"]}
        self.assertEqual(by_addr[0x013F2A]["op_str"], "#$a0039e, d0")
        self.assertEqual(by_addr[0x013FA2]["op_str"], "#$14084, $22(a6)")
        self.assertEqual(by_addr[0x014040]["op_str"], "#$5, d4")
        self.assertEqual(by_addr[0x0140EA]["op_str"], "#$549, d3")
        self.assertEqual(by_addr[0x014102]["op_str"], "#$e00, d4")
        self.assertEqual(by_addr[0x0141E8]["op_str"], "#$160, d2")
        self.assertEqual(by_addr[0x01421C]["op_str"], "$14242")

    def test_dismissal_and_states_10_02_settle(self) -> None:
        # 1. Dismissal guard: state_04 != 0 branches to 0x0138B8
        ctx = BowSimCtx()
        child = BowEntity(type_00=0xFFFF)
        bow_busy = BowEntity(state_04=0x04, link_a6=child)
        self.assertEqual(ai_bow_dismissal(bow_busy, ctx), "BLINK_CHECK_0138B8")
        self.assertEqual((child.type_00, len(ctx.sfx_log)), (0xFFFF, 0))

        # 2. Idle dismissal frees child, sets State $10, anim $1E, priority $07A8, clears +$36 bit 0
        bow = BowEntity(state_04=0, facing_16=2, vram_attr_18=0x87A8, flags_36=0x03, link_a6=child)
        self.assertEqual(ai_bow_dismissal(bow, ctx), "DISMISS_STATE10")
        self.assertEqual(ctx.sfx_log, [0x00A0039E])
        self.assertEqual((child.type_00, bow.state_04, bow.action_2a, bow.facing_16), (0, 0x10, 0x1E, 0))
        self.assertEqual((bow.vram_attr_18, bow.flags_36), (0x07A8, 0x02))

        # 3. State $10 clears +$36 bit 1 and despawns (+$00 = 0) on +$37 bit 1
        self.assertEqual(state10_descend_despawn(bow), "DESCENDING")
        self.assertEqual((bow.flags_36, bow.type_00), (0x00, 0x001E))
        bow.anim_flags_37 = 0x02
        self.assertEqual(state10_descend_despawn(bow), "DESPAWNED")
        self.assertEqual(bow.type_00, 0)

        # 4. State $02 waits for +$37 bit 1, then initializes $FF1C1C vine sub-entity & settles
        b2 = BowEntity(
            state_04=0x02, action_2a=0x12, x_pos_08=120 << 16, y_pos_0c=90 << 16,
            z_pos_10=10 << 16, vram_attr_18=0x8000, flags_38=0x04, flags_9e=3,
        )
        self.assertEqual(state02_return_and_settle(b2, ctx), "WAITING")
        b2.anim_flags_37 = 0x02
        self.assertEqual(state02_return_and_settle(b2, ctx), "SETTLED")
        v = ctx.vine_slot_ff1c1c
        self.assertEqual((b2.state_04, b2.action_2a, b2.flags_9e, b2.facing_16, b2.flags_38), (0, 0, 0, 1, 0))
        self.assertEqual((v.type_00, v.render_cb_22, v.vram_attr_18), (0xFFFF, 0x00014084, 0x87A8))
        self.assertEqual((v.anchor_x_4e, v.anchor_y_52, v.anchor_z_56), (120 << 16, 90 << 16, 10 << 16))
        self.assertIs(v.link_a6, b2)
        self.assertIs(b2.link_a6, v)

    def test_vine_facing_and_4node_fixed_point_chain_interpolation(self) -> None:
        # 1. Facing away from anchor (0x013FDC)
        vine = BowEntity(anchor_x_4e=100 << 16, anchor_y_52=100 << 16)
        for hx, hy, exp_f in ((100, 80, 0), (130, 100, 1), (100, 120, 2), (70, 100, 3)):
            b = BowEntity(x_pos_08=hx << 16, y_pos_0c=hy << 16)
            self.assertEqual(update_facing_from_anchor(b, vine), exp_f)

        # 2. Sort Y clamp: min(head_y - 2, anchor_y) and 12.4 fixed-point quantization
        ctx = BowSimCtx()
        ctx.vine_slot_ff1c1c = BowEntity(anchor_x_4e=100 << 16, anchor_y_52=100 << 16, anchor_z_56=20 << 16)
        bow = BowEntity(x_pos_08=125 << 16, y_pos_0c=85 << 16, z_pos_10=5 << 16)
        nodes = interpolate_vine_chain_nodes(bow, ctx)
        self.assertEqual(ctx.vine_slot_ff1c1c.y_pos_0c >> 16, 83)  # head_y - 2 < anchor_y
        # dx=+25 (step=+80=5.0), dy=-15 (step=-48=-3.0), dz=-15 (step=-48=-3.0)
        self.assertEqual(nodes, [(105, 97, 17), (110, 94, 14), (115, 91, 11), (120, 88, 8)])

        # Anchor Y wins when head_y - 2 > anchor_y; non-divisible deltas test truncation quantization
        bow2 = BowEntity(x_pos_08=111 << 16, y_pos_0c=104 << 16, z_pos_10=21 << 16)
        nodes2 = interpolate_vine_chain_nodes(bow2, ctx)
        self.assertEqual(ctx.vine_slot_ff1c1c.y_pos_0c >> 16, 100)  # min(102, 100)
        # dx=11 -> step=(11<<4)//5=35; dy=4 -> step=12; dz=1 -> step=3
        # step 1: 100+(35>>4)=102; step 2: 102+(35>>4)=104; etc. (truncates frac each step)
        self.assertEqual(nodes2, [(102, 100, 20), (104, 100, 20), (106, 100, 20), (108, 100, 20)])

    def test_render_vine_subentity_sat_and_clipping(self) -> None:
        # 1. Parent != $001E frees self; +$36 bit 1 hides SAT output
        ctx = BowSimCtx()
        parent = BowEntity(type_00=0x001E, flags_36=0x02)
        vine = BowEntity(type_00=0xFFFF, vram_attr_18=0x87A8, link_a6=parent)
        self.assertEqual(render_vine_sub_entity_sat(vine, ctx), "BLINK_HIDDEN")
        self.assertEqual((len(ctx.sat_entries), vine.type_00), (0, 0xFFFF))

        parent.type_00 = 0
        self.assertEqual(render_vine_sub_entity_sat(vine, ctx), "FREED_SELF")
        self.assertEqual(vine.type_00, 0)

        # 2. Full 6-sprite emission on Plane B ($FF16F0 bit 7 set): 4 nodes (reversed) + shadow + root
        parent = BowEntity(type_00=0x001E, flags_36=0x00)
        vine = BowEntity(
            type_00=0xFFFF, vram_attr_18=0x87A8, link_a6=parent,
            anchor_x_4e=160 << 16, anchor_y_52=120 << 16, anchor_z_56=4 << 16,
        )
        ctx = BowSimCtx(
            plane_flag_16f0=0x80,
            cam_a=(999, 999),
            cam_b=(100, 50),
            sat_count_188a=10,
            sat_offset_188c=80,
            vine_nodes=[(164, 118, 2), (168, 116, 4), (172, 114, 6), (176, 112, 8)],
            spirit_slot_ff1aa4=BowEntity(x_pos_08=180 << 16, y_pos_0c=110 << 16),
        )
        self.assertEqual(render_vine_sub_entity_sat(vine, ctx), "RENDERED")
        self.assertEqual((ctx.sat_count_188a, ctx.sat_offset_188c, len(ctx.sat_entries)), (16, 128, 6))
        # Node 3 first (176-100+7C=C8, 112-50+7C-8=B2)
        self.assertEqual(ctx.sat_entries[0], (0xB2, 0x000B, 0x87A8, 0xC8))
        # Node 0 fourth (164-100+7C=BC, 118-50+7C-2=BE)
        self.assertEqual(ctx.sat_entries[3], (0xBE, 0x000E, 0x87A8, 0xBC))
        # Ground shadow fifth (180-100+78=C8, 110-50+7D=B9)
        self.assertEqual(ctx.sat_entries[4], (0xB9, 0x040F, 0x0549, 0xC8))
        # Root anchor sixth (160-100+71=AD, 120-50+74-4=B6, tile 0x87A8+9=0x87B1)
        self.assertEqual(ctx.sat_entries[5], (0xB6, 0x0E10, 0x87B1, 0xAD))

        # 3. Viewport clipping [0x68..0x1C0) x [0x70..0x160) and 80-sprite cap (0x50)
        ctx_clip = BowSimCtx(
            vine_nodes=[(0x68 - 0x7C, 0x70 - 0x7C, 0), (0x67 - 0x7C, 0x70 - 0x7C, 0),
                        (0x1C0 - 0x7C, 0x70 - 0x7C, 0), (0x68 - 0x7C, 0x160 - 0x7C, 0)],
            spirit_slot_ff1aa4=BowEntity(x_pos_08=-100 << 16, y_pos_0c=0),
        )
        vine_clip = BowEntity(
            type_00=0xFFFF, vram_attr_18=0x07A8, link_a6=parent, anchor_x_4e=-100 << 16,
        )
        render_vine_sub_entity_sat(vine_clip, ctx_clip)
        self.assertEqual(ctx_clip.sat_entries, [(0x70, 0x0001, 0x07A8, 0x68)])  # Only min corner fits

        ctx_cap = BowSimCtx(sat_count_188a=0x50, vine_nodes=[(100, 100, 0)] * 4)
        render_vine_sub_entity_sat(vine, ctx_cap)
        self.assertEqual((ctx_cap.sat_count_188a, len(ctx_cap.sat_entries)), (0x50, 0))

    def test_step_and_clamp_velocity_by_leash(self) -> None:
        # 1. Preserves +$08/+$0C and clamps +$72/+$76 using 0x014242 clamped position
        bow = BowEntity(x_pos_08=100 << 16, y_pos_0c=100 << 16, delta_x_72=20 << 16, delta_y_76=-15 << 16)

        def clamp_radius_10(b: BowEntity) -> None:
            if (b.x_pos_08 >> 16) > 110:
                b.x_pos_08 = (110 << 16) | (b.x_pos_08 & 0xFFFF)
            if (b.y_pos_0c >> 16) < 90:
                b.y_pos_0c = (90 << 16) | (b.y_pos_0c & 0xFFFF)

        step_and_clamp_velocity_by_leash(bow, clamp_radius_10)
        self.assertEqual((bow.x_pos_08, bow.y_pos_0c), (100 << 16, 100 << 16))
        self.assertEqual((bow.delta_x_72, bow.delta_y_76), (10 << 16, -10 << 16))

        # 2. Unclamped step leaves +$72/+$76 exact
        bow2 = BowEntity(x_pos_08=100 << 16, y_pos_0c=100 << 16, delta_x_72=4 << 16, delta_y_76=-3 << 16)
        step_and_clamp_velocity_by_leash(bow2, clamp_radius_10)
        self.assertEqual((bow2.x_pos_08, bow2.y_pos_0c), (100 << 16, 100 << 16))
        self.assertEqual((bow2.delta_x_72, bow2.delta_y_76), (4 << 16, -3 << 16))


if __name__ == "__main__":
    unittest.main()
