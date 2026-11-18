#!/usr/bin/env python3
"""
Unit tests for ROM Dytto Spirit 16-State Dispatcher, Healing Cast Sub-Entity
Spawner & Jewel-Rank HP Recovery Subsystem (0x011BA0..0x011D8A, 490 bytes).
Milestone M15.486.
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
    / "rom_dytto_healing_drop_m15_486.json"
)


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


def m68k_divs_w(num: int, den: int) -> int:
    quot = int(num / den)  # truncates toward zero like 68K DIVS.W
    return to_s16(quot)


def helper_00e45e_homing_velocity(
    orb_x: int, orb_y: int, target_x: int, target_y: int, facing_16: int = 0
) -> tuple[int, int, int, int]:
    """Faithful 68K model of 0x00E45E velocity/distance helper."""
    d1 = to_s16(target_x - orb_x)
    d4 = abs(d1)
    d2 = to_s16(target_y - orb_y)
    d5 = abs(d2)
    d3 = 1
    if d4 <= d5:
        d4 = d5
        d3 = 0
    d4 = d4 >> 1
    if d4 == 0:
        return 0, 0, facing_16, 0
    vx = (m68k_divs_w(d1 << 8, d4) << 8) & 0xFFFFFFFF
    vy = (m68k_divs_w(d2 << 8, d4) << 8) & 0xFFFFFFFF
    if (vx | vy) == 0:
        return 0, 0, facing_16, 0
    return vx, vy, d3, d4


@dataclass
class EntitySlot:
    type_00: int = 0
    state_04: int = 0
    x_08: int = 0
    y_0c: int = 0
    z_10: int = 0
    facing_16: int = 0
    priority_18: int = 0
    sprite_table_1a: int = 0
    action_2a: int = 0
    anim_frame_30: int = 0
    flags_36: int = 0
    anim_flags_37: int = 0
    flags_38: int = 0
    vel_x_72: int = 0
    vel_y_76: int = 0
    max_hp_88: int = 0
    hp_8a: int = 0
    sp_8e: int = 0
    sub_entity_a2: int = 0

    def copy_from(self, other: EntitySlot) -> None:
        for name in self.__dataclass_fields__:
            setattr(self, name, getattr(other, name))


@dataclass
class SimWorld:
    pool1: list[EntitySlot] = field(default_factory=lambda: [EntitySlot() for _ in range(18)])
    ali: EntitySlot = field(default_factory=lambda: EntitySlot(type_00=0x0001, max_hp_88=100, hp_8a=20, sp_8e=50))
    jewel_rank_ff0db6: int = 0
    frame_counter_ff1971: int = 0
    hp_suppress_ff1987: int = 0
    sfx_e7fc_log: list[int] = field(default_factory=list)
    audio_60004_log: list[int] = field(default_factory=list)
    rng_sequence: list[int] = field(default_factory=list)

    def alloc_pool1(self) -> EntitySlot | None:
        for slot in self.pool1:
            if slot.type_00 == 0:
                return slot
        return None

    def next_rng(self) -> int:
        return self.rng_sequence.pop(0) if self.rng_sequence else 0


def try_start_healing_cast(dytto: EntitySlot, world: SimWorld) -> str:
    """Simulation of AI_Dytto_TryStartHealingCast (0x011BCC..0x011BF2)."""
    rem = to_s16(dytto.sp_8e if dytto is world.ali else world.ali.sp_8e) - 0x14
    if rem < 1:
        world.audio_60004_log.append(0x00A00253)  # 0x01219A buzzer
        return "INSUFFICIENT_SP_01219A"
    world.ali.sp_8e = rem
    world.sfx_e7fc_log.append(0x00800258)
    dytto.facing_16 = 0
    dytto.state_04 = 0x18
    dytto.action_2a = 0x0E
    return "STARTED_STATE_18"


def state18_healing_cast_and_spawn_orb(dytto: EntitySlot, world: SimWorld) -> str:
    """Simulation of AI_Dytto_State18_HealingCastAndSpawnOrb (0x011BF2..0x011C7A)."""
    if dytto.anim_frame_30 != 0x28:
        if dytto.anim_flags_37 & 0x02:
            dytto.state_04 = 0x00
            return "RETURN_STATE_00_0121AA"
        return "CASTING"

    orb = world.alloc_pool1()
    if orb is None:
        return "POOL_FULL"

    orb.copy_from(EntitySlot(type_00=0x0018))  # jsr $008D06 zeroes fields & sets type $18
    orb.sprite_table_1a = 0x00141374
    orb.priority_18 = (dytto.priority_18 & 0x8000) | 0x07A8
    orb.flags_36 = 0x08
    orb.flags_38 = 0x0D
    orb.sub_entity_a2 = 0xFF
    orb.state_04 = 0x12
    orb.action_2a = 0x20
    orb.x_08 = dytto.x_08
    orb.y_0c = dytto.y_0c
    orb.z_10 = to_s16(dytto.z_10 + 0x20)
    orb.facing_16 = 0
    return "SPAWNED_ORB"


def compute_jewel_rank_heal_amount(max_hp: int, jewel_rank: int) -> int:
    """Exact ROM 0x011D22..0x011D58 jewel-rank heal calculation."""
    tier = ((jewel_rank + 1) & 0xFF) >> 2
    if tier == 4:
        return max_hp & 0xFFFF
    if tier == 3:
        half = (max_hp & 0xFFFF) >> 1
        return (half + (half >> 1)) & 0xFFFF
    if tier == 2:
        return (max_hp & 0xFFFF) >> 1
    if tier == 1:
        return (max_hp & 0xFFFF) >> 2
    return (max_hp & 0xFFFF) >> 3


def state12_healing_orb_tick(orb: EntitySlot, world: SimWorld) -> tuple[str, int, int]:
    """Simulation of AI_Dytto_State12_HealingOrbHomingAndHpRestore (0x011C7A..0x011D7C)."""
    target_z = to_s16(world.ali.z_10 + 0x20)
    old_z = to_s16(orb.z_10)
    delta_z = to_s16(target_z - old_z)
    if delta_z >= 0:
        step = min(delta_z, 3)
        abs_dz = delta_z
    else:
        step = max(delta_z, -3)
        abs_dz = (-delta_z) & 0xFFFF
    orb.z_10 = to_s16(old_z + step)

    vx, vy, _, d4 = helper_00e45e_homing_velocity(
        orb.x_08, orb.y_0c, world.ali.x_08, to_s16(world.ali.y_0c + 6), orb.facing_16
    )
    orb.vel_x_72 = (vx * 2) & 0xFFFFFFFF
    orb.vel_y_76 = (vy * 2) & 0xFFFFFFFF

    if abs_dz <= 4 and d4 <= 2:
        orb.type_00 = 0
        max_hp = world.ali.max_hp_88 & 0xFFFF
        heal = compute_jewel_rank_heal_amount(max_hp, world.jewel_rank_ff0db6)
        new_hp = (world.ali.hp_8a + heal) & 0xFFFF
        if new_hp > max_hp:
            new_hp = max_hp
        if (world.hp_suppress_ff1987 & 0xFF) == 0:
            world.ali.hp_8a = new_hp
        world.audio_60004_log.append(0x00D00355)
        return "RESTORED_HP", abs_dz, d4

    if (world.frame_counter_ff1971 & 0x01) != 0:
        return "HOMING_NO_SPARKLE", abs_dz, d4

    sparkle = world.alloc_pool1()
    if sparkle is None:
        return "HOMING_POOL_FULL", abs_dz, d4
    sparkle.copy_from(orb)
    sparkle.state_04 = 0x14
    sparkle.action_2a = 0x1C
    sparkle.x_08 = to_s16(sparkle.x_08 + ((world.next_rng() & 7) - 3))
    sparkle.y_0c = to_s16(sparkle.y_0c + ((world.next_rng() & 7) - 3))
    return "SPAWNED_SPARKLE", abs_dz, d4


def state14_healing_sparkle_despawn(sparkle: EntitySlot) -> bool:
    """Simulation of AI_Dytto_State14_HealingSparkleDespawn (0x011D7C..0x011D8A)."""
    if sparkle.anim_flags_37 & 0x02:
        sparkle.type_00 = 0
        return True
    return False


class TestROMDyttoHealingDrop(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.486")
        self.assertEqual(self.evidence["range"]["start"], "0x011BA0")
        self.assertEqual(self.evidence["range"]["end"], "0x011D8A")
        self.assertEqual(self.evidence["range"]["size_bytes"], 490)

        jt = self.evidence["jump_table"]
        self.assertEqual((jt["address"], jt["end"], jt["size_bytes"]), ("0x011BAC", "0x011BCC", 32))
        self.assertEqual(len(jt["entries"]), 16)
        targets = {e["state"]: e["target"] for e in jt["entries"]}
        expected_targets = {
            "0x00": "0x0121AA", "0x02": "0x0120D4", "0x04": "0x012150", "0x06": "0x012104",
            "0x08": "0x011FF6", "0x0A": "0x01202A", "0x0C": "0x012086", "0x0E": "0x011DB6",
            "0x10": "0x011ED2", "0x12": "0x011C7A", "0x14": "0x011D7C", "0x16": "0x011F98",
            "0x18": "0x011BF2", "0x1A": "0x01212E", "0x1C": "0x012342", "0x1E": "0x012394",
        }
        self.assertEqual(targets, expected_targets)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 5)
        self.assertEqual(sum(r["size_bytes"] for r in routines), 490)
        self.assertEqual(self.evidence["instruction_count"], 127)
        self.assertEqual(len(self.evidence["instructions"]), 127)
        self.assertEqual(self.evidence["instructions"][0]["address"], "0x011BA0")
        self.assertEqual(self.evidence["instructions"][-1]["address"], "0x011D88")

    def test_try_start_healing_cast_and_state18_spawn(self) -> None:
        # 1. SP == 20 rejects (must leave >= 1 SP) and plays buzzer 0x00A00253
        world = SimWorld()
        world.ali.sp_8e = 20
        dytto = EntitySlot(type_00=0x0018, x_08=150, y_0c=220, z_10=8, facing_16=2, priority_18=0x8200)
        self.assertEqual(try_start_healing_cast(dytto, world), "INSUFFICIENT_SP_01219A")
        self.assertEqual((world.ali.sp_8e, dytto.state_04, world.audio_60004_log), (20, 0, [0x00A00253]))

        # 2. SP == 21 succeeds -> SP=1, SFX 0x00800258, State $18, +$2A=$0E
        world.ali.sp_8e = 21
        self.assertEqual(try_start_healing_cast(dytto, world), "STARTED_STATE_18")
        self.assertEqual((world.ali.sp_8e, dytto.facing_16, dytto.state_04, dytto.action_2a), (1, 0, 0x18, 0x0E))
        self.assertEqual(world.sfx_e7fc_log, [0x00800258])

        # 3. State $18: non-40 frames continue or exit to State 0 on +$37 bit 1
        dytto.anim_frame_30 = 10
        self.assertEqual(state18_healing_cast_and_spawn_orb(dytto, world), "CASTING")
        dytto.anim_flags_37 = 0x02
        self.assertEqual(state18_healing_cast_and_spawn_orb(dytto, world), "RETURN_STATE_00_0121AA")
        self.assertEqual(dytto.state_04, 0)

        # 4. Frame 0x28 (40) spawns healing orb sub-entity in Pool 1
        dytto.state_04 = 0x18
        dytto.anim_frame_30 = 0x28
        self.assertEqual(state18_healing_cast_and_spawn_orb(dytto, world), "SPAWNED_ORB")
        orb = world.pool1[0]
        self.assertEqual((orb.type_00, orb.sprite_table_1a, orb.priority_18), (0x0018, 0x00141374, 0x87A8))
        self.assertEqual((orb.flags_36, orb.flags_38, orb.sub_entity_a2), (0x08, 0x0D, 0xFF))
        self.assertEqual((orb.state_04, orb.action_2a, orb.facing_16), (0x12, 0x20, 0))
        self.assertEqual((orb.x_08, orb.y_0c, orb.z_10), (150, 220, 40))

        # 5. Pool full gracefully skips spawn
        for slot in world.pool1:
            slot.type_00 = 1
        self.assertEqual(state18_healing_cast_and_spawn_orb(dytto, world), "POOL_FULL")

    def test_healing_orb_z_step_and_2d_homing(self) -> None:
        # 1. Z step clamp [+3, -3] and d6 = abs(target_z - z_before_step)
        for init_z, expected_z, expected_dz in [(20, 23, 22), (40, 42, 2), (42, 42, 0), (45, 42, 3), (60, 57, 18)]:
            world = SimWorld()
            world.ali.x_08, world.ali.y_0c, world.ali.z_10 = 200, 100, 10  # target_z = 42
            world.frame_counter_ff1971 = 1  # suppress sparkle
            orb = EntitySlot(type_00=0x0018, x_08=100, y_0c=100, z_10=init_z)
            _, abs_dz, d4 = state12_healing_orb_tick(orb, world)
            self.assertEqual((orb.z_10, abs_dz, d4), (expected_z, expected_dz, 50))

        # 2. Doubled 2D homing velocity toward (ali.x, ali.y + 6) via 0x00E45E
        world = SimWorld()
        world.ali.x_08, world.ali.y_0c, world.ali.z_10 = 140, 114, 0  # target = (140, 120, 32)
        world.frame_counter_ff1971 = 1
        orb = EntitySlot(type_00=0x0018, x_08=100, y_0c=100, z_10=32)
        # dx=40, dy=20 -> d4 = 40 >> 1 = 20; vx_raw = (40<<8)/20 << 8 = 0x20000 -> doubled = 0x40000
        # vy_raw = (20<<8)/20 << 8 = 0x10000 -> doubled = 0x20000
        status, abs_dz, d4 = state12_healing_orb_tick(orb, world)
        self.assertEqual((status, abs_dz, d4), ("HOMING_NO_SPARKLE", 0, 20))
        self.assertEqual((orb.vel_x_72, orb.vel_y_76), (0x00040000, 0x00020000))

    def test_healing_orb_sparkle_trail_and_state14_despawn(self) -> None:
        # Even frame ($FF1971 & 1 == 0) spawns sparkle with PRNG jitter [-3..+4]
        world = SimWorld(frame_counter_ff1971=4, rng_sequence=[0x0000, 0x0007])
        world.ali.x_08, world.ali.y_0c, world.ali.z_10 = 180, 120, 0
        orb = EntitySlot(type_00=0x0018, x_08=100, y_0c=100, z_10=32, sub_entity_a2=0xFF)
        world.pool1[0] = orb

        status, _, _ = state12_healing_orb_tick(orb, world)
        self.assertEqual(status, "SPAWNED_SPARKLE")
        sparkle = world.pool1[1]
        self.assertEqual((sparkle.type_00, sparkle.state_04, sparkle.action_2a), (0x0018, 0x14, 0x1C))
        self.assertEqual((sparkle.x_08, sparkle.y_0c), (97, 104))  # 100 + (0-3), 100 + (7-3)

        # Odd frame skips sparkle
        world.frame_counter_ff1971 = 5
        self.assertEqual(state12_healing_orb_tick(orb, world)[0], "HOMING_NO_SPARKLE")

        # State $14 despawn only when +$37 bit 1 is set
        self.assertFalse(state14_healing_sparkle_despawn(sparkle))
        self.assertEqual(sparkle.type_00, 0x0018)
        sparkle.anim_flags_37 = 0x02
        self.assertTrue(state14_healing_sparkle_despawn(sparkle))
        self.assertEqual(sparkle.type_00, 0)

    def test_healing_orb_arrival_and_jewel_rank_hp_restore(self) -> None:
        # Boundary: abs_dz == 5 or d4 == 3 does NOT trigger arrival
        for dz, dx in [(5, 0), (0, 6)]:
            world = SimWorld(frame_counter_ff1971=1)
            world.ali.x_08, world.ali.y_0c, world.ali.z_10 = 100, 100, 0
            orb = EntitySlot(type_00=0x0018, x_08=100 + dx, y_0c=106, z_10=32 + dz)
            status, _, _ = state12_healing_orb_tick(orb, world)
            self.assertEqual(status, "HOMING_NO_SPARKLE")
            self.assertEqual(orb.type_00, 0x0018)

        # Jewel rank tiers: tier = (($FF0DB6 + 1) & 0xFF) >> 2
        # Note tier 3 does (max_hp >> 1) + (max_hp >> 2); for max_hp=11 -> 5 + 2 = 7
        tier_cases = [
            (0, 11, 1),    # tier 0: 11 >> 3 = 1
            (2, 11, 1),    # tier 0: (2+1)>>2 = 0 -> 1
            (3, 11, 2),    # tier 1: (3+1)>>2 = 1 -> 11 >> 2 = 2
            (7, 11, 5),    # tier 2: (7+1)>>2 = 2 -> 11 >> 1 = 5
            (11, 11, 7),   # tier 3: (11+1)>>2 = 3 -> (11>>1)+(11>>2) = 5+2 = 7
            (15, 11, 11),  # tier 4: (15+1)>>2 = 4 -> full 11
            (19, 11, 1),   # tier 5: falls back to max_hp >> 3 = 1
        ]
        for rank, max_hp, expected_heal in tier_cases:
            world = SimWorld(jewel_rank_ff0db6=rank)
            world.ali = EntitySlot(type_00=0x0001, x_08=100, y_0c=100, z_10=0, max_hp_88=max_hp, hp_8a=1)
            orb = EntitySlot(type_00=0x0018, x_08=104, y_0c=106, z_10=36)  # abs_dz=4, d4=2
            status, abs_dz, d4 = state12_healing_orb_tick(orb, world)
            self.assertEqual((status, abs_dz, d4), ("RESTORED_HP", 4, 2))
            self.assertEqual(orb.type_00, 0)
            self.assertEqual(world.ali.hp_8a, min(max_hp, 1 + expected_heal))
            self.assertEqual(world.audio_60004_log, [0x00D00355])

        # Clamping to max_hp and suppression when $FF1987 != 0
        world = SimWorld(jewel_rank_ff0db6=15, hp_suppress_ff1987=1)
        world.ali = EntitySlot(type_00=0x0001, x_08=100, y_0c=100, z_10=0, max_hp_88=100, hp_8a=30)
        orb = EntitySlot(type_00=0x0018, x_08=100, y_0c=106, z_10=32)
        state12_healing_orb_tick(orb, world)
        self.assertEqual(world.ali.hp_8a, 30)
        self.assertEqual(world.audio_60004_log, [0x00D00355])


if __name__ == "__main__":
    unittest.main()
