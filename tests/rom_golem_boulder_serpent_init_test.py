#!/usr/bin/env python3
"""
Unit tests for ROM Golem Boulder/Debris Projectile Ballistics & Composite Serpent/
Multi-Segment Boss Constructor (0x0159E2..0x015C4A, Milestone M15.511).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_golem_boulder_serpent_init_m15_511.json"
)


def step_boulder_tick(boulder, ali, terrain_flags=0, colliding_entity=None, aabb_hit=False):
    """
    Emulate AI_GolemBoulder_Tick (0x0159E2..0x015A8C).
    Returns (action_taken, hit_result_or_None).
    """
    state = boulder.get("state", 0)
    if state != 0:
        # Check anim finished bit 1 in +$37
        if (boulder.get("flags_37", 0) & 0x02) != 0:
            boulder["active_0"] = 0
            return "DESPAWNED", None
        return "FADING", None

    # State == 0: Active flight
    # Accumulate velocities
    boulder["x_accum"] = boulder.get("x_accum", 0) + boulder.get("x_vel", 0)
    boulder["y_accum"] = boulder.get("y_accum", 0) + boulder.get("y_vel", 0)

    # Apply gravity -$C000 to Z velocity
    boulder["z_vel"] = boulder.get("z_vel", 0) - 0xC000
    boulder["z"] = boulder.get("z", 0) + boulder["z_vel"]

    if boulder["z"] < 0:
        # Ground impact: clamp Z=0, shatter
        boulder["z"] = 0
        boulder["state"] = boulder.get("state", 0) + 2
        boulder["anim_id"] = 0x0E
        return "GROUND_SHATTER", None

    hit_res = None
    if aabb_hit:
        hit_res = apply_boulder_hit(boulder, ali)

    # Collision flags check
    flag_40 = terrain_flags & 0xFF
    flag_41 = (terrain_flags >> 8) & 0xFF
    merged_flags = flag_40 | flag_41

    if (merged_flags & 0x40) != 0:
        # Wall collision (bit 6)
        if (flag_41 & 0x13) == 0x12:
            # Z-bounce: sub.l +$52, +$10
            boulder["z"] -= boulder.get("y_vel", 0)
            return "WALL_BOUNCE", hit_res
        else:
            # Shatter
            boulder["state"] = boulder.get("state", 0) + 2
            boulder["anim_id"] = 0x0E
            return "WALL_SHATTER", hit_res

    if (merged_flags & 0x80) != 0:
        # Entity collision (bit 7)
        if colliding_entity == 0xFF19E8:
            boulder["state"] = boulder.get("state", 0) + 2
            boulder["anim_id"] = 0x0E
            return "ENTITY_SHATTER", hit_res

    return "IN_FLIGHT", hit_res


def apply_boulder_hit(boulder, ali):
    """
    Emulate AI_GolemBoulder_HitCallback (0x015A8C..0x015AE2).
    """
    boulder["state"] = boulder.get("state", 0) + 2
    boulder["anim_id"] = 0x0E

    # Check Ali invulnerability (+0x38:3) or guard (+0x82)
    is_immune = (ali.get("flags_38", 0) & 0x08) != 0 or ali.get("guard_82", False)
    if is_immune:
        return "IMMUNE_BLOCKED"

    ali["attacker"] = boulder
    ali["guard_82"] = False
    ali["damage_86"] = 0x8006  # 6 HP
    ali["field_9d"] = False
    ali["knockback_mode_84"] = 3
    ali["x_vel"] = -0x40000    # -4.0 px horizontal impulse
    ali["y_vel"] = 0
    ali["z_vel"] = 0x30000     # +3.0 px vertical impulse
    ali["sfx"] = 0x800207
    return "DAMAGE_DEALT"


def route_composite_submode(sub_mode_66):
    """
    Emulate AI_CompositeSerpent_SubModeRouter (0x015AE2..0x015AFC).
    """
    if sub_mode_66 == 0:
        return 0x016294
    elif sub_mode_66 == 1:
        return 0x015AFE
    elif sub_mode_66 == 2:
        return 0x015CCA
    elif sub_mode_66 == 3:
        return 0x015CB6
    return None


def init_composite_serpent(root_entity):
    """
    Emulate AI_CompositeSerpent_InitCompositeSegments (0x015AFE..0x015C4A).
    Returns list of 9 child segments and updated root entity.
    """
    root_entity["sub_mode_66"] = root_entity.get("sub_mode_66", 0) + 3
    root_entity["z"] = 0
    root_entity["facing_16"] = 1
    root_entity["sprite_attr_18"] = 0x4200

    current_facing = root_entity["facing_16"]
    child_slots = []

    # Slot 1: body 1
    current_facing += 2  # 3
    child_slots.append({
        "slot": 1,
        "anim_id": 0x3A,
        "facing": current_facing,
        "sprite_attr": root_entity["sprite_attr_18"]
    })

    # Slot 2: body 2
    child_slots.append({
        "slot": 2,
        "anim_id": 0x3A,
        "facing": current_facing,
        "sprite_attr": root_entity["sprite_attr_18"]
    })

    # Slot 3: body 3
    current_facing += 2  # 5
    child_slots.append({
        "slot": 3,
        "anim_id": 0x3A,
        "facing": current_facing,
        "sprite_attr": root_entity["sprite_attr_18"]
    })

    # Slot 4: body 4
    child_slots.append({
        "slot": 4,
        "anim_id": 0x3A,
        "facing": current_facing,
        "sprite_attr": root_entity["sprite_attr_18"]
    })

    # Slot 5: HEAD
    child_slots.append({
        "slot": 5,
        "anim_id": 0x3C,
        "sub_mode_66": 2,
        "size_3d": 0x10,
        "bounds_w": 8,
        "bounds_d": 6,
        "bounds_h": 16,
        "bounds_extent": 4,
        "x": root_entity.get("x", 0) - 0x20,
        "y": root_entity.get("y", 0) + 0x28,
        "hp": 20,
        "max_hp": 20,
        "vram_tile_18": 0x0200
    })

    # Slot 6: Tail orb 1
    child_slots.append({
        "slot": 6,
        "anim_id": 0x2C,
        "size_3d": 8,
        "vram_tile_18": 0x0200
    })

    # Slot 7: Tail orb 2
    child_slots.append({
        "slot": 7,
        "anim_id": 0x2E,
        "size_3d": 0,
        "vram_tile_18": 0x0200
    })

    # Slot 8: Tail orb 3
    child_slots.append({
        "slot": 8,
        "anim_id": 0x30,
        "size_3d": 0,
        "vram_tile_18": 0x0200
    })

    # Slot 9: Tail orb 4
    child_slots.append({
        "slot": 9,
        "anim_id": 0x30,
        "size_3d": 0,
        "vram_tile_18": 0x0200
    })

    # Root restoration
    root_entity["sub_mode_66"] = 0
    root_entity["facing_16"] = 0
    root_entity["state"] = 0x18
    root_entity["anim_id"] = 4
    root_entity["hp"] = 50
    root_entity["max_hp"] = 50
    root_entity["flag_b0"] = 0
    root_entity["exit_target"] = 0x02ED20

    return child_slots, root_entity


class TestROMGolemBoulderSerpentInit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(__file__)
        ev_file = os.path.join(self_dir, "..", "data", "thor-evidence", "rom_golem_boulder_serpent_init_m15_511.json")
        with open(ev_file, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        self.assertEqual(self.evidence["milestone"], "M15.511")
        self.assertEqual(self.evidence["range"]["start"], "0x0159E2")
        self.assertEqual(self.evidence["range"]["end"], "0x015C4A")
        self.assertEqual(self.evidence["range"]["size_bytes"], 616)
        self.assertEqual(self.evidence["instruction_count"], 135)
        self.assertEqual(len(self.evidence["routines"]), 4)
        self.assertEqual(len(self.evidence["composite_segment_config"]), 9)

    def test_boulder_fadeout_despawn(self):
        boulder = {"state": 2, "flags_37": 0, "active_0": 0x1234}
        action, hit = step_boulder_tick(boulder, None)
        self.assertEqual(action, "FADING")
        self.assertEqual(boulder["active_0"], 0x1234)

        boulder["flags_37"] = 0x02  # bit 1 anim finished
        action, hit = step_boulder_tick(boulder, None)
        self.assertEqual(action, "DESPAWNED")
        self.assertEqual(boulder["active_0"], 0)

    def test_boulder_flight_and_ground_shatter(self):
        boulder = {
            "state": 0,
            "x_accum": 0x10000,
            "y_accum": 0x20000,
            "x_vel": 0x8000,
            "y_vel": 0x4000,
            "z_vel": 0x24000,
            "z": 0x10000
        }
        # Step 1: in flight
        # z_vel: 0x24000 - 0xC000 = 0x18000; z = 0x10000 + 0x18000 = 0x28000
        action, hit = step_boulder_tick(boulder, None)
        self.assertEqual(action, "IN_FLIGHT")
        self.assertEqual(boulder["z"], 0x28000)
        self.assertEqual(boulder["z_vel"], 0x18000)
        self.assertEqual(boulder["x_accum"], 0x18000)
        self.assertEqual(boulder["y_accum"], 0x24000)

        # Step 2: high downward gravity causing ground impact
        boulder["z_vel"] = 0
        boulder["z"] = 0x4000
        # z_vel: -0xC000; z = 0x4000 - 0xC000 = -0x8000 < 0 -> ground shatter!
        action, hit = step_boulder_tick(boulder, None)
        self.assertEqual(action, "GROUND_SHATTER")
        self.assertEqual(boulder["z"], 0)
        self.assertEqual(boulder["state"], 2)
        self.assertEqual(boulder["anim_id"], 0x0E)

    def test_boulder_aabb_ali_hit(self):
        boulder = {"state": 0, "z_vel": 0x10000, "z": 0x10000}
        ali_vuln = {"flags_38": 0, "guard_82": False}

        action, hit = step_boulder_tick(boulder, ali_vuln, aabb_hit=True)
        self.assertEqual(hit, "DAMAGE_DEALT")
        self.assertEqual(boulder["state"], 2)
        self.assertEqual(boulder["anim_id"], 0x0E)
        self.assertEqual(ali_vuln["damage_86"], 0x8006)
        self.assertEqual(ali_vuln["knockback_mode_84"], 3)
        self.assertEqual(ali_vuln["x_vel"], -0x40000)
        self.assertEqual(ali_vuln["y_vel"], 0)
        self.assertEqual(ali_vuln["z_vel"], 0x30000)
        self.assertEqual(ali_vuln["sfx"], 0x800207)

        # Immune test (guard or invulnerability)
        boulder_immune = {"state": 0, "z_vel": 0x10000, "z": 0x10000}
        ali_immune = {"flags_38": 0x08, "guard_82": True}
        action, hit = step_boulder_tick(boulder_immune, ali_immune, aabb_hit=True)
        self.assertEqual(hit, "IMMUNE_BLOCKED")
        self.assertEqual(boulder_immune["state"], 2)
        self.assertEqual(boulder_immune["anim_id"], 0x0E)
        self.assertNotIn("damage_86", ali_immune)

    def test_boulder_wall_and_entity_collision(self):
        # Wall bounce: bit 6 set, and (flag_41 & 0x13) == 0x12
        # terrain_flags: flag_40=0, flag_41=0x12 -> (0x12 << 8) | 0x40 = 0x1240
        boulder_bounce = {"state": 0, "z_vel": 0x10000, "z": 0x50000, "y_vel": 0x20000}
        action, _ = step_boulder_tick(boulder_bounce, None, terrain_flags=0x1240)
        self.assertEqual(action, "WALL_BOUNCE")
        # z: (0x50000 + (0x10000 - 0xC000)) - 0x20000 = 0x54000 - 0x20000 = 0x34000
        self.assertEqual(boulder_bounce["z"], 0x34000)

        # Wall shatter: bit 6 set, but (flag_41 & 0x13) != 0x12
        boulder_wall_shatter = {"state": 0, "z_vel": 0x10000, "z": 0x50000}
        action, _ = step_boulder_tick(boulder_wall_shatter, None, terrain_flags=0x0040)
        self.assertEqual(action, "WALL_SHATTER")
        self.assertEqual(boulder_wall_shatter["state"], 2)
        self.assertEqual(boulder_wall_shatter["anim_id"], 0x0E)

        # Entity collision: bit 7 set, colliding entity is Ali (0xFF19E8)
        boulder_entity = {"state": 0, "z_vel": 0x10000, "z": 0x50000}
        action, _ = step_boulder_tick(boulder_entity, None, terrain_flags=0x0080, colliding_entity=0xFF19E8)
        self.assertEqual(action, "ENTITY_SHATTER")
        self.assertEqual(boulder_entity["state"], 2)

    def test_composite_serpent_submode_router(self):
        self.assertEqual(route_composite_submode(0), 0x016294)
        self.assertEqual(route_composite_submode(1), 0x015AFE)
        self.assertEqual(route_composite_submode(2), 0x015CCA)
        self.assertEqual(route_composite_submode(3), 0x015CB6)
        self.assertIsNone(route_composite_submode(4))

    def test_composite_serpent_9slot_constructor(self):
        root = {"x": 200, "y": 150}
        children, final_root = init_composite_serpent(root)

        self.assertEqual(len(children), 9)

        # Check Body segments 1..4
        for i in range(4):
            self.assertEqual(children[i]["anim_id"], 0x3A)
        self.assertEqual(children[0]["facing"], 3)
        self.assertEqual(children[1]["facing"], 3)
        self.assertEqual(children[2]["facing"], 5)
        self.assertEqual(children[3]["facing"], 5)

        # Check Head segment (Slot 5)
        head = children[4]
        self.assertEqual(head["anim_id"], 0x3C)
        self.assertEqual(head["sub_mode_66"], 2)
        self.assertEqual(head["size_3d"], 0x10)
        self.assertEqual(head["bounds_w"], 8)
        self.assertEqual(head["bounds_d"], 6)
        self.assertEqual(head["bounds_h"], 16)
        self.assertEqual(head["bounds_extent"], 4)
        self.assertEqual(head["x"], 200 - 0x20)
        self.assertEqual(head["y"], 150 + 0x28)
        self.assertEqual(head["hp"], 20)
        self.assertEqual(head["max_hp"], 20)
        self.assertEqual(head["vram_tile_18"], 0x0200)

        # Check Tail orb segments (Slots 6..9)
        self.assertEqual(children[5]["anim_id"], 0x2C)
        self.assertEqual(children[5]["size_3d"], 8)
        self.assertEqual(children[6]["anim_id"], 0x2E)
        self.assertEqual(children[6]["size_3d"], 0)
        self.assertEqual(children[7]["anim_id"], 0x30)
        self.assertEqual(children[7]["size_3d"], 0)
        self.assertEqual(children[8]["anim_id"], 0x30)
        self.assertEqual(children[8]["size_3d"], 0)

        # Check Root entity restoration
        self.assertEqual(final_root["sub_mode_66"], 0)
        self.assertEqual(final_root["facing_16"], 0)
        self.assertEqual(final_root["state"], 0x18)
        self.assertEqual(final_root["anim_id"], 4)
        self.assertEqual(final_root["hp"], 50)
        self.assertEqual(final_root["max_hp"], 50)
        self.assertEqual(final_root["flag_b0"], 0)
        self.assertEqual(final_root["exit_target"], 0x02ED20)


if __name__ == "__main__":
    unittest.main()
