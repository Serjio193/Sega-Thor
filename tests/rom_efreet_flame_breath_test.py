#!/usr/bin/env python3
"""
Unit tests for ROM Efreet 7-Particle Flame Breath Ring Engine, Enemy Ignite
Handler & Composite Vertex Exporter (0x012C4E..0x012F1E, 720 bytes).
Milestone M15.492.
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
    / "rom_efreet_flame_breath_m15_492.json"
)

SWEEP_START_ANGLES = (0x20, 0xE0, 0xA0, 0x60)
ANIM_TABLE = (
    (0x0063, 1),
    (0x0065, 1),
    (0x0067, 0),
    (0x0068, 0),
    (0x0069, 0),
    (0x0068, 0),
    (0x0067, 0),
    (0x0068, 0),
    (0x0069, 0),
    (0x006A, 0),
    (0x006B, 0),
    (-1, 0),  # 0xFFFF terminator at 0x012DAC
)
SPAWN_OFFSETS = (
    (0, -40),   # 0: Up
    (32, -12),  # 1: Right
    (0, 12),    # 2: Down
    (-32, -12), # 3: Left
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


def q15_sin_hi(angle_byte: int) -> int:
    """Cleanroom Q15 high-byte sine matching $05D706 (256-step circle)."""
    q15 = int(math.floor(math.sin(2.0 * math.pi * (angle_byte & 0xFF) / 256.0) * 32767.0 + 0.5))
    if q15 > 32767:
        q15 = 32767
    return to_s8(q15 >> 8)


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
class FlameParticleSlot:
    status_00: int = 0xFF  # 0xFF = free, 0x00 = active
    tile_01: int = 0x00
    pos_x_02: int = 0
    pos_y_06: int = 0
    vel_x_0a: int = 0
    vel_y_0e: int = 0
    timer_12: int = 0
    anim_idx_13: int = 0


@dataclass
class EfreetState:
    facing_16: int = 2
    piece_mode_06: int = 0
    attach_priority_5a: int = 0
    attach_rom_ptr_5e: int = 0x123456
    attach_ram_ptr_62: int = 0
    sweep_enabled_aa: int = 0
    flame_angle_ab: int = 0
    particles: list[FlameParticleSlot] = field(
        default_factory=lambda: [FlameParticleSlot() for _ in range(7)]
    )


@dataclass
class TargetEntity:
    slot_idx: int = 4
    type_id: int = 0x0048
    x_rel: int = 0
    y_rel: int = 0
    z_rel: int = 8
    flags_38: int = 0
    vx_4e: int = 0
    vy_52: int = 0
    vz_56: int = 0
    attacker_7e: EfreetState | None = None
    hit_guard_82: int = 0
    hit_facing_84: int = 0
    damage_86: int = 0
    stun_9d: int = 0x33


@dataclass
class FlameSimCtx:
    latched_dir_19a3: int = 0
    efreet_level_db7: int = 0
    vertex_header_19a8: int = 0
    vertex_payload_19a9: list[tuple[int, int, int]] = field(default_factory=list)
    targets: list[TargetEntity] = field(default_factory=list)
    aabb_scans: list[tuple[int, int, int, int, int, int, int]] = field(default_factory=list)
    sfx_log: list[int] = field(default_factory=list)


def init_flame_breath_particles_and_angle(efreet: EfreetState, ctx: FlameSimCtx) -> None:
    """0x012C4E..0x012CDC: AI_Efreet_InitFlameBreathParticlesAndAngle."""
    for slot in efreet.particles:
        slot.status_00 = 0xFF
    d = ctx.latched_dir_19a3 & 0xFF
    if d & 0x01:
        efreet.facing_16 = 0
        efreet.sweep_enabled_aa = 0x00
        efreet.flame_angle_ab = 0x40
    elif d & 0x02:
        efreet.facing_16 = 2
        efreet.sweep_enabled_aa = 0x00
        efreet.flame_angle_ab = 0xC0
    elif d & 0x08:
        efreet.facing_16 = 1
        efreet.sweep_enabled_aa = 0x00
        efreet.flame_angle_ab = 0x00
    elif d & 0x04:
        efreet.facing_16 = 3
        efreet.sweep_enabled_aa = 0x00
        efreet.flame_angle_ab = 0x80
    else:
        efreet.sweep_enabled_aa = 0xFF
        efreet.flame_angle_ab = SWEEP_START_ANGLES[efreet.facing_16]


def ignite_enemy_on_flame_hit(
    efreet: EfreetState, target: TargetEntity, ctx: FlameSimCtx
) -> None:
    """0x012DAE..0x012DFA: AI_Efreet_IgniteEnemyOnFlameHit."""
    target.attacker_7e = efreet
    target.stun_9d = 0
    ctx.sfx_log.append(0x00700345)
    target.hit_facing_84 = efreet.facing_16
    vx, vy = compute_8dir_velocity(0x0300, 0x0300, efreet.facing_16)
    target.vx_4e = vx
    target.vy_52 = vy
    target.vz_56 = 0x00020000
    tier = ((ctx.efreet_level_db7 & 0xFF) + 1) >> 2
    target.damage_86 = (0xA004 + tier) & 0xFFFF


def update_flame_breath_particles(efreet: EfreetState, ctx: FlameSimCtx) -> None:
    """0x012CDC..0x012DAE: AI_Efreet_UpdateFlameBreathParticles."""
    for slot in efreet.particles:
        if (slot.status_00 & 0xFF) != 0:
            continue
        slot.timer_12 = (slot.timer_12 - 1) & 0xFF
        if slot.timer_12 & 0x80:
            slot.anim_idx_13 = (slot.anim_idx_13 + 1) & 0xFF
            tile, dur = ANIM_TABLE[slot.anim_idx_13]
            if tile < 0:
                slot.status_00 = 0xFF
                continue
            slot.status_00 = (tile >> 8) & 0xFF
            slot.tile_01 = tile & 0xFF
            slot.timer_12 = dur & 0xFF
        slot.pos_x_02 = to_s32(slot.pos_x_02 + slot.vel_x_0a)
        slot.vel_y_0e = to_s32(slot.vel_y_0e - 0x00004000)
        slot.pos_y_06 = to_s32(slot.pos_y_06 + slot.vel_y_0e)

        px = to_s16(slot.pos_x_02 >> 16)
        py = to_s16(slot.pos_y_06 >> 16)
        box = (0xF0, px - 8, px + 8, py - 8, py + 8, 4, 20)
        ctx.aabb_scans.append(box)
        for target in ctx.targets:
            if target.slot_idx < 4 or target.type_id <= 0:
                continue
            if (
                (px - 8) <= target.x_rel <= (px + 8)
                and (py - 8) <= target.y_rel <= (py + 8)
                and 4 <= target.z_rel <= 20
            ):
                if (target.flags_38 & 0x08) == 0 and (target.hit_guard_82 & 0xFF) == 0:
                    ignite_enemy_on_flame_hit(efreet, target, ctx)
                break


def spawn_flame_particle(efreet: EfreetState) -> bool:
    """0x012DFA..0x012EA8: AI_Efreet_SpawnFlameParticle. Returns True on Carry clear."""
    free_slot = None
    for slot in efreet.particles:
        if (slot.status_00 & 0xFF) != 0:
            free_slot = slot
            break
    if free_slot is None:
        return False
    free_slot.status_00 = 0x00
    free_slot.tile_01 = 0x63
    free_slot.timer_12 = 3
    free_slot.anim_idx_13 = 0

    angle = efreet.flame_angle_ab & 0xFF
    if (efreet.sweep_enabled_aa & 0xFF) != 0:
        angle = (angle + 5) & 0xFF
        efreet.flame_angle_ab = angle

    sin_val = q15_sin_hi(angle)
    cos_val = q15_sin_hi((angle + 0x40) & 0xFF)
    free_slot.vel_x_0a = to_s32(cos_val * 0x0700)
    free_slot.vel_y_0e = to_s32(-(sin_val * 0x0700) + 0x00020000)

    dx, dy = SPAWN_OFFSETS[efreet.facing_16]
    free_slot.pos_x_02 = to_s32(dx << 16)
    free_slot.pos_y_06 = to_s32(dy << 16)
    return True


def export_flame_vertex_buffer(efreet: EfreetState, ctx: FlameSimCtx) -> int:
    """0x012EA8..0x012F1E: AI_Efreet_ExportFlameVertexBuffer."""
    payload: list[tuple[int, int, int]] = []
    for slot in efreet.particles:
        if (slot.status_00 & 0xFF) != 0:
            continue
        dx_b = (slot.pos_x_02 >> 16) & 0xFF
        if efreet.facing_16 == 3:
            dx_b = (-dx_b) & 0xFF
        dy_b = (slot.pos_y_06 >> 16) & 0xFF
        payload.append((slot.tile_01 & 0xFF, to_s8(dx_b), to_s8(dy_b)))
    count = len(payload)
    ctx.vertex_payload_19a9 = payload
    if count == 0:
        efreet.piece_mode_06 = 0
        return 0
    header = (count - 1) & 0xFF
    if efreet.facing_16 != 2:
        header |= 0x80
    ctx.vertex_header_19a8 = header
    efreet.piece_mode_06 = 0x001A
    efreet.attach_rom_ptr_5e = 0
    efreet.attach_priority_5a = 0x07A8
    efreet.attach_ram_ptr_62 = 0x00FF19A8
    return count


class TestROMEfreetFlameBreath(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure_and_inline_tables(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.492")
        self.assertEqual(self.evidence["range"]["start"], "0x012C4E")
        self.assertEqual(self.evidence["range"]["end"], "0x012F1E")
        self.assertEqual(self.evidence["range"]["size_bytes"], 720)
        self.assertEqual(self.evidence["range"]["code_bytes"], 662)
        self.assertEqual(self.evidence["range"]["table_bytes"], 58)
        self.assertEqual(self.evidence["instruction_count"], 181)
        self.assertEqual(len(self.evidence["instructions"]), 181)
        self.assertEqual(sum(i["size"] for i in self.evidence["instructions"]), 662)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertEqual(
            list(routines.keys()),
            [
                "AI_Efreet_InitFlameBreathParticlesAndAngle",
                "AI_Efreet_UpdateFlameBreathParticles",
                "AI_Efreet_IgniteEnemyOnFlameHit",
                "AI_Efreet_SpawnFlameParticle",
                "AI_Efreet_ExportFlameVertexBuffer",
            ],
        )
        self.assertEqual(routines["AI_Efreet_InitFlameBreathParticlesAndAngle"]["size_bytes"], 142)
        self.assertEqual(routines["AI_Efreet_InitFlameBreathParticlesAndAngle"]["instruction_count"], 35)
        self.assertEqual(routines["AI_Efreet_UpdateFlameBreathParticles"]["size_bytes"], 210)
        self.assertEqual(routines["AI_Efreet_UpdateFlameBreathParticles"]["instruction_count"], 47)
        self.assertEqual(routines["AI_Efreet_IgniteEnemyOnFlameHit"]["size_bytes"], 76)
        self.assertEqual(routines["AI_Efreet_IgniteEnemyOnFlameHit"]["instruction_count"], 19)
        self.assertEqual(routines["AI_Efreet_SpawnFlameParticle"]["size_bytes"], 174)
        self.assertEqual(routines["AI_Efreet_SpawnFlameParticle"]["instruction_count"], 48)
        self.assertEqual(routines["AI_Efreet_ExportFlameVertexBuffer"]["size_bytes"], 118)
        self.assertEqual(routines["AI_Efreet_ExportFlameVertexBuffer"]["instruction_count"], 32)

        tables = {t["name"]: t for t in self.evidence["embedded_data_tables"]}
        self.assertEqual(tables["EfreetFlameSweepStartAngleTable"]["raw_hex"], "20e0a060")
        self.assertEqual(len(tables["EfreetFlameAnimTable"]["entries"]), 11)
        self.assertEqual(tables["EfreetFlameAnimTable"]["size_bytes"], 46)
        self.assertEqual(tables["EfreetFlameSpawnOffsetTable"]["raw_hex"], "00d820f4000ce0f4")

        by_addr = {int(i["address"], 16): i for i in self.evidence["instructions"]}
        self.assertEqual(by_addr[0x012C84]["op_str"], "$12c92(pc, d0.w), d0")
        self.assertEqual(by_addr[0x012D5A]["op_str"], "$b922")
        self.assertEqual(by_addr[0x012DB6]["op_str"], "#$700345, d0")
        self.assertEqual(by_addr[0x012DF0]["op_str"], "#$a004, d0")
        self.assertEqual(by_addr[0x012E52]["op_str"], "$5d706.l, a0")
        self.assertEqual(by_addr[0x012F0C]["op_str"], "#$7a8, $5a(a6)")

    def test_init_particles_and_joypad_or_sweep_angle(self) -> None:
        # 1. Clears all 7 slots (+$00 = $FF) and tests D-pad priority Up(0) > Down(1) > Right(3) > Left(2)
        for dir_bits, exp_facing, exp_ang in (
            (0x01, 0, 0x40),
            (0x02, 2, 0xC0),
            (0x08, 1, 0x00),
            (0x04, 3, 0x80),
            (0x0F, 0, 0x40),  # bit 0 wins
            (0x0E, 2, 0xC0),  # bit 1 wins over 3, 2
            (0x0C, 1, 0x00),  # bit 3 wins over 2
        ):
            efreet = EfreetState(
                facing_16=2,
                sweep_enabled_aa=0xFF,
                particles=[FlameParticleSlot(status_00=0x00, tile_01=0x63) for _ in range(7)],
            )
            init_flame_breath_particles_and_angle(efreet, FlameSimCtx(latched_dir_19a3=dir_bits))
            self.assertTrue(all(s.status_00 == 0xFF for s in efreet.particles))
            self.assertEqual((efreet.facing_16, efreet.sweep_enabled_aa, efreet.flame_angle_ab),
                             (exp_facing, 0x00, exp_ang))

        # 2. Neutral $FF19A3 enables sweep mode (+$AA=$FF) and loads 0x012C92[+$16]
        for facing, exp_start in enumerate((0x20, 0xE0, 0xA0, 0x60)):
            efreet = EfreetState(facing_16=facing)
            init_flame_breath_particles_and_angle(efreet, FlameSimCtx(latched_dir_19a3=0x00))
            self.assertEqual((efreet.facing_16, efreet.sweep_enabled_aa, efreet.flame_angle_ab),
                             (facing, 0xFF, exp_start))

    def test_spawn_flame_particle_ring_sweep_and_trig(self) -> None:
        # 1. Fixed mode (+$AA=0) vs 7-slot exhaustion & reuse
        efreet = EfreetState(facing_16=1, sweep_enabled_aa=0x00, flame_angle_ab=0x00)
        for idx in range(7):
            self.assertTrue(spawn_flame_particle(efreet))
            slot = efreet.particles[idx]
            self.assertEqual((slot.status_00, slot.tile_01, slot.timer_12, slot.anim_idx_13), (0x00, 0x63, 3, 0))
            self.assertEqual((slot.pos_x_02, slot.pos_y_06), (32 << 16, -12 << 16))
            self.assertEqual((slot.vel_x_0a, slot.vel_y_0e), (127 * 0x0700, 0x00020000))
            self.assertEqual(efreet.flame_angle_ab, 0x00)

        self.assertFalse(spawn_flame_particle(efreet))  # 8th spawn fails (Carry set)
        efreet.particles[2].status_00 = 0xFF
        self.assertTrue(spawn_flame_particle(efreet))   # reuses slot 2
        self.assertEqual(efreet.particles[2].status_00, 0x00)

        # 2. Sweep mode (+$AA=$FF) advances +$AB by +5 mod 256 before trig lookup & uses 0x012EA0 offsets
        for facing, exp_off, exp_angles in (
            (0, (0, -40), (0x25, 0x2A, 0x2F)),
            (1, (32, -12), (0xE5, 0xEA, 0xEF)),
            (2, (0, 12), (0xA5, 0xAA, 0xAF)),
            (3, (-32, -12), (0x65, 0x6A, 0x6F)),
        ):
            ef = EfreetState(facing_16=facing)
            init_flame_breath_particles_and_angle(ef, FlameSimCtx(latched_dir_19a3=0))
            for i, ang in enumerate(exp_angles):
                self.assertTrue(spawn_flame_particle(ef))
                self.assertEqual(ef.flame_angle_ab, ang)
                s = ef.particles[i]
                self.assertEqual((s.pos_x_02 >> 16, s.pos_y_06 >> 16), exp_off)
                exp_vx = q15_sin_hi((ang + 0x40) & 0xFF) * 0x0700
                exp_vy = -(q15_sin_hi(ang) * 0x0700) + 0x00020000
                self.assertEqual((s.vel_x_0a, s.vel_y_0e), (exp_vx, exp_vy))

        # Wrap-around test across 0xFA -> 0xFF -> 0x04
        ef_wrap = EfreetState(facing_16=1, sweep_enabled_aa=0xFF, flame_angle_ab=0xFA)
        self.assertTrue(spawn_flame_particle(ef_wrap))
        self.assertEqual(ef_wrap.flame_angle_ab, 0xFF)
        self.assertTrue(spawn_flame_particle(ef_wrap))
        self.assertEqual(ef_wrap.flame_angle_ab, 0x04)

    def test_update_particles_anim_physics_and_ignite_handler(self) -> None:
        # 1. Full 14-tick active lifecycle across 11 table entries + 0xFFFF terminator on tick 15
        efreet = EfreetState(facing_16=1, sweep_enabled_aa=0x00, flame_angle_ab=0x00)
        self.assertTrue(spawn_flame_particle(efreet))
        ctx = FlameSimCtx()
        observed_tiles = []
        for _ in range(14):
            update_flame_breath_particles(efreet, ctx)
            self.assertEqual(efreet.particles[0].status_00, 0x00)
            observed_tiles.append(efreet.particles[0].tile_01)
        expected_tiles = [
            0x63, 0x63, 0x63,
            0x65, 0x65,
            0x67, 0x68, 0x69, 0x68, 0x67, 0x68, 0x69, 0x6A, 0x6B,
        ]
        self.assertEqual(observed_tiles, expected_tiles)

        # Verify 14 ticks of physics: x += 14*(127*0x700), vy_0e = 0x20000 - 14*0x4000 = -0x18000
        p = efreet.particles[0]
        self.assertEqual(p.pos_x_02, (32 << 16) + 14 * 127 * 0x0700)
        self.assertEqual(p.vel_y_0e, 0x20000 - 14 * 0x4000)
        self.assertEqual(len(ctx.aabb_scans), 14)
        self.assertEqual(ctx.aabb_scans[0], (0xF0, 35 - 8, 35 + 8, -11 - 8, -11 + 8, 4, 20))

        # 15th tick hits 0xFFFF terminator -> deactivates slot without extra physics/scan
        pre_x, pre_y, pre_scans = p.pos_x_02, p.pos_y_06, len(ctx.aabb_scans)
        update_flame_breath_particles(efreet, ctx)
        self.assertEqual((p.status_00, p.pos_x_02, p.pos_y_06, len(ctx.aabb_scans)),
                         (0xFF, pre_x, pre_y, pre_scans))

        # 2. AABB collision gates (+$38 bit 3, +$82 != 0) and Ignite handler + jewel-rank damage
        ef2 = EfreetState(facing_16=2)
        ef2.particles[0] = FlameParticleSlot(status_00=0x00, tile_01=0x63, timer_12=3, pos_x_02=10 << 16, pos_y_06=20 << 16)
        intangible = TargetEntity(slot_idx=4, x_rel=10, y_rel=22, z_rel=10, flags_38=0x08)
        busy = TargetEntity(slot_idx=5, x_rel=10, y_rel=22, z_rel=10, hit_guard_82=1)
        update_flame_breath_particles(ef2, FlameSimCtx(targets=[intangible]))
        self.assertEqual((intangible.damage_86, intangible.stun_9d), (0, 0x33))
        update_flame_breath_particles(ef2, FlameSimCtx(targets=[busy]))
        self.assertEqual((busy.damage_86, busy.stun_9d), (0, 0x33))

        for rank, exp_dmg in ((0, 0xA004), (2, 0xA004), (3, 0xA005), (7, 0xA006), (11, 0xA007), (15, 0xA008)):
            victim = TargetEntity(slot_idx=6, x_rel=10, y_rel=22, z_rel=10, stun_9d=0x50)
            ef2.particles[0].pos_y_06 = 20 << 16
            ef2.particles[0].vel_y_0e = 0
            ef2.particles[0].timer_12 = 3
            c = FlameSimCtx(efreet_level_db7=rank, targets=[victim])
            update_flame_breath_particles(ef2, c)
            self.assertIs(victim.attacker_7e, ef2)
            self.assertEqual((victim.stun_9d, victim.hit_facing_84, victim.damage_86), (0, 2, exp_dmg))
            self.assertEqual((victim.vx_4e, victim.vy_52, victim.vz_56), (0, 0x00030000, 0x00020000))
            self.assertEqual(c.sfx_log, [0x00700345])

    def test_export_flame_vertex_buffer_and_header_flags(self) -> None:
        # 1. Empty buffer clears +$06(a6) = 0 and preserves +$62(a6)
        ef_empty = EfreetState(facing_16=1, piece_mode_06=0x001A, attach_ram_ptr_62=0x112233)
        ctx_empty = FlameSimCtx(vertex_header_19a8=0x99)
        self.assertEqual(export_flame_vertex_buffer(ef_empty, ctx_empty), 0)
        self.assertEqual((ef_empty.piece_mode_06, ef_empty.attach_ram_ptr_62, ctx_empty.vertex_header_19a8),
                         (0, 0x112233, 0x99))

        # 2. Facing Down (2) keeps header bit 7 CLEAR; Up/Right/Left set bit 7; Left (3) negates dx
        for facing, exp_header, exp_dx in (
            (0, 0x82, -12),
            (1, 0x82, -12),
            (2, 0x02, -12),
            (3, 0x82, 12),
        ):
            ef = EfreetState(facing_16=facing)
            ef.particles[1] = FlameParticleSlot(status_00=0x00, tile_01=0x63, pos_x_02=(-12 << 16) | 0x8000, pos_y_06=4 << 16)
            ef.particles[4] = FlameParticleSlot(status_00=0x00, tile_01=0x67, pos_x_02=20 << 16, pos_y_06=(-8 << 16) | 0x1234)
            ef.particles[6] = FlameParticleSlot(status_00=0x00, tile_01=0x6B, pos_x_02=0, pos_y_06=0)
            ctx = FlameSimCtx()
            count = export_flame_vertex_buffer(ef, ctx)
            self.assertEqual(count, 3)
            self.assertEqual(ctx.vertex_header_19a8, exp_header)
            self.assertEqual(ctx.vertex_payload_19a9[0], (0x63, exp_dx, 4))
            self.assertEqual(ctx.vertex_payload_19a9[1], (0x67, -20 if facing == 3 else 20, -8))
            self.assertEqual(ctx.vertex_payload_19a9[2], (0x6B, 0, 0))
            self.assertEqual((ef.piece_mode_06, ef.attach_rom_ptr_5e, ef.attach_priority_5a, ef.attach_ram_ptr_62),
                             (0x001A, 0, 0x07A8, 0x00FF19A8))


if __name__ == "__main__":
    unittest.main()
