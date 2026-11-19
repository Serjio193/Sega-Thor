#!/usr/bin/env python3
"""
Unit tests for ROM Boss AI Feather/Spore & 3-Way Fireball Spread Projectile Spawners
(0x015136..0x015316).
Milestone M15.507.
"""

import json
import os
import struct
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_ai_projectile_spawners_m15_507.json"
)
ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")

FEATHER_TABLE_EXPECTED = [
    {"facing": 1, "dx": 8, "dy": 8, "vx_hi": 4, "vy_hi": 3},
    {"facing": 2, "dx": 0, "dy": 40, "vx_hi": 0, "vy_hi": 5},
    {"facing": 3, "dx": -8, "dy": 8, "vx_hi": -4, "vy_hi": 3}
]

FIREBALL_TABLE_EXPECTED = [
    {"facing": 1, "dx": 48, "dy": 8, "base_angle": 234, "pad": 0},
    {"facing": 2, "dx": 0, "dy": 40, "base_angle": 192, "pad": 0},
    {"facing": 3, "dx": -48, "dy": 8, "base_angle": 170, "pad": 0}
]


def load_rom_sin_cos():
    """Load the 256-entry signed byte sin table from ROM 0x05D706."""
    with open(ROM_PATH, "rb") as f:
        rom = f.read()
    sin_table = []
    for i in range(256):
        val = struct.unpack_from(">b", rom, 0x05D706 + i * 2)[0]
        sin_table.append(val)
    return sin_table


SIN_TABLE = load_rom_sin_cos()


def spawn_feather_projectile(boss, pool_alloc_success=True):
    """
    Emulate 0x015136..0x0151FA AI_Boss_SpawnFeatherSporeProjectile.
    Returns: spawned projectile dict or None if alloc fails.
    """
    if not pool_alloc_success:
        return None

    facing = boss.get("facing", 2)
    tbl_idx = max(0, min(2, facing - 1))
    tbl = FEATHER_TABLE_EXPECTED[tbl_idx]

    proj = {
        "type": 0x24,
        "vram_tile": 0x0200,
        "bounds": 8,
        "facing": 2,
        "size_3d": 8,
        "flags_38": 0x0C,  # bits 2 (flying) and 3 (invulnerable)
        "layer_3c": 2,
        "flag_b4": 0xFF,
        "flag_a2": 0xFF,
        "anim_id": 6,
        "state": 0x0C,
        "z_vel": -0x50000,
        "x": boss.get("x", 0) + tbl["dx"],
        "y": boss.get("y", 0) + tbl["dy"],
        "z": boss.get("z", 0) + 0x10,
        "x_vel": boss.get("x_vel", 0) + (tbl["vx_hi"] << 16),
        "y_vel": boss.get("y_vel", 0) + (tbl["vy_hi"] << 16)
    }
    return proj


def spawn_fireball_projectile(boss, spread_delta=0, pool_alloc_success=True):
    """
    Emulate 0x015226..0x0152FC AI_Boss_Spawn3WayFireballSpread (single branch).
    spread_delta (d6): 0 (center), +0x1C (right), -0x1C (left).
    """
    if not pool_alloc_success:
        return None

    facing = boss.get("facing", 2)
    tbl_idx = max(0, min(2, facing - 1))
    tbl = FIREBALL_TABLE_EXPECTED[tbl_idx]

    base_angle = tbl["base_angle"]
    angle = (base_angle + spread_delta) & 0xFF
    cos_angle = (angle + 0x40) & 0xFF

    sin_val = SIN_TABLE[angle]
    cos_val = SIN_TABLE[cos_angle]

    vx = cos_val * 0x600
    vy = -(sin_val * 0x600)

    proj = {
        "type": 0x24,
        "vram_tile": 0x0200,
        "bounds": 5,
        "height": 8,
        "facing": 2,
        "size_3d": 8,
        "flags_38": 0x04,  # bit 2 (flying)
        "layer_3c": 2,
        "flag_b4": 0xFF,
        "flag_a2": 0xFF,
        "anim_id": 0x14,
        "state": 0x12,
        "x": boss.get("x", 0) + tbl["dx"],
        "y": boss.get("y", 0) + tbl["dy"],
        "z": boss.get("z", 0) + 0x10,
        "x_vel": vx,
        "y_vel": vy,
        "z_vel": -0xC000
    }
    return proj


def spawn_3way_fireball_spread(boss, pool_alloc_success=True):
    """Emulate 0x015214..0x015226 (3 calls: d6=0, d6=+0x1C, d6=-0x1C)."""
    deltas = [0, 0x1C, -0x1C]
    fireballs = []
    for d6 in deltas:
        p = spawn_fireball_projectile(boss, spread_delta=d6, pool_alloc_success=pool_alloc_success)
        if p:
            fireballs.append(p)
    return fireballs


class TestROMBossAIProjectileSpawners(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.507")
        self.assertEqual(self.evidence["range"]["start"], "0x015136")
        self.assertEqual(self.evidence["range"]["end"], "0x015316")
        self.assertEqual(self.evidence["range"]["size_bytes"], 480)
        self.assertEqual(len(self.evidence["routines"]), 2)
        self.assertEqual(self.evidence["instruction_count"], 109)

    def test_embedded_tables_unpacked(self):
        t1 = self.evidence["tables"]["feather_spawn_table"]["entries"]
        self.assertEqual(len(t1), 3)
        for i in range(3):
            self.assertEqual(t1[i]["facing"], FEATHER_TABLE_EXPECTED[i]["facing"])
            self.assertEqual(t1[i]["dx"], FEATHER_TABLE_EXPECTED[i]["dx"])
            self.assertEqual(t1[i]["dy"], FEATHER_TABLE_EXPECTED[i]["dy"])
            self.assertEqual(t1[i]["vx_hi"], FEATHER_TABLE_EXPECTED[i]["vx_hi"])
            self.assertEqual(t1[i]["vy_hi"], FEATHER_TABLE_EXPECTED[i]["vy_hi"])

        t2 = self.evidence["tables"]["fireball_spawn_table"]["entries"]
        self.assertEqual(len(t2), 3)
        for i in range(3):
            self.assertEqual(t2[i]["facing"], FIREBALL_TABLE_EXPECTED[i]["facing"])
            self.assertEqual(t2[i]["dx"], FIREBALL_TABLE_EXPECTED[i]["dx"])
            self.assertEqual(t2[i]["dy"], FIREBALL_TABLE_EXPECTED[i]["dy"])
            self.assertEqual(t2[i]["base_angle"], FIREBALL_TABLE_EXPECTED[i]["base_angle"])
            self.assertEqual(t2[i]["pad"], 0)

    def test_spawn_feather_spore_all_facings(self):
        boss = {"x": 200, "y": 150, "z": 40, "x_vel": 0x10000, "y_vel": 0x20000}

        # Facing 1: dx=+8, dy=+8, vx_hi=+4, vy_hi=+3
        boss["facing"] = 1
        p1 = spawn_feather_projectile(boss)
        self.assertEqual(p1["type"], 0x24)
        self.assertEqual(p1["vram_tile"], 0x0200)
        self.assertEqual(p1["bounds"], 8)
        self.assertEqual(p1["state"], 0x0C)
        self.assertEqual(p1["anim_id"], 6)
        self.assertEqual(p1["flags_38"], 0x0C)
        self.assertEqual(p1["z_vel"], -0x50000)
        self.assertEqual(p1["x"], 208)
        self.assertEqual(p1["y"], 158)
        self.assertEqual(p1["z"], 56)  # 40 + 16
        self.assertEqual(p1["x_vel"], 0x10000 + 0x40000)
        self.assertEqual(p1["y_vel"], 0x20000 + 0x30000)

        # Facing 2: dx=0, dy=+40, vx_hi=0, vy_hi=+5
        boss["facing"] = 2
        p2 = spawn_feather_projectile(boss)
        self.assertEqual(p2["x"], 200)
        self.assertEqual(p2["y"], 190)
        self.assertEqual(p2["x_vel"], 0x10000)
        self.assertEqual(p2["y_vel"], 0x20000 + 0x50000)

        # Facing 3: dx=-8, dy=+8, vx_hi=-4, vy_hi=+3
        boss["facing"] = 3
        p3 = spawn_feather_projectile(boss)
        self.assertEqual(p3["x"], 192)
        self.assertEqual(p3["y"], 158)
        self.assertEqual(p3["x_vel"], 0x10000 - 0x40000)
        self.assertEqual(p3["y_vel"], 0x20000 + 0x30000)

    def test_spawn_3way_fireball_spread_facing2(self):
        boss = {"x": 200, "y": 150, "z": 40, "facing": 2}
        fireballs = spawn_3way_fireball_spread(boss)
        self.assertEqual(len(fireballs), 3)

        center, right, left = fireballs[0], fireballs[1], fireballs[2]

        # Common attributes
        for fb in [center, right, left]:
            self.assertEqual(fb["type"], 0x24)
            self.assertEqual(fb["vram_tile"], 0x0200)
            self.assertEqual(fb["bounds"], 5)
            self.assertEqual(fb["height"], 8)
            self.assertEqual(fb["state"], 0x12)
            self.assertEqual(fb["anim_id"], 0x14)
            self.assertEqual(fb["flags_38"], 0x04)
            self.assertEqual(fb["z_vel"], -0xC000)
            self.assertEqual(fb["x"], 200)
            self.assertEqual(fb["y"], 190)  # 150 + 40
            self.assertEqual(fb["z"], 56)   # 40 + 16

        # Center (angle 192): vx = 0, vy = +0x30000 (sin=-128, cos=0)
        self.assertEqual(center["x_vel"], 0)
        self.assertEqual(center["y_vel"], 0x30000)

        # Right (angle 220): vx = 81 * 0x600 = +0x1E600, vy = -(-99 * 0x600) = +0x25200
        self.assertEqual(right["x_vel"], 0x1E600)
        self.assertEqual(right["y_vel"], 0x25200)

        # Left (angle 164): vx = -82 * 0x600 = -0x1EC00, vy = -(-99 * 0x600) = +0x25200
        self.assertEqual(left["x_vel"], -0x1EC00)
        self.assertEqual(left["y_vel"], 0x25200)

    def test_pool_allocation_failure(self):
        boss = {"x": 100, "y": 100, "z": 0, "facing": 2}
        # When Pool 2 allocation fails, spawner returns None without altering boss
        p = spawn_feather_projectile(boss, pool_alloc_success=False)
        self.assertIsNone(p)

        fbs = spawn_3way_fireball_spread(boss, pool_alloc_success=False)
        self.assertEqual(fbs, [])


if __name__ == "__main__":
    unittest.main()
