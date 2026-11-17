#!/usr/bin/env python3
"""
rom_ali_spirit_power_inspector_test.py — M15.390: Unit tests for ROM Ali Spirit Power & Object Inspector
Tests:
  1. test_spirit_power_invocation
  2. test_item_proximity_scanner
  3. test_interactive_object_inspector
  4. test_facing_direction_normalizer
"""

import unittest


def invoke_spirit_power(moving_dpad, current_state):
    # 0x00822C..0x0082AC
    if moving_dpad and current_state == 4:
        # Air Spirit Call
        return {
            "action": "air_spirit_call",
            "state": 0x16,
            "anim": 0x16,
            "vz": 0x70000,
            "counter_197e_add": 8,
            "clear_ring_count": True,
            "unroll_stack": True,
        }
    else:
        # Ground Spirit Summon
        return {
            "action": "ground_spirit_summon",
            "state": 0x10,
            "anim": 0x10,
            "vz": 0x70000,
            "vx": 0,
            "vy": 0,
            "clear_1980": True,
            "clear_ring_count": True,
            "unroll_stack": True,
        }


def scan_pickup_proximity(nearby_entities):
    # 0x0082AE..0x0082F6
    for ent in nearby_entities:
        if ent.get("type") == 8:
            # Absorb pickup
            ent["type"] = 0
            packed_id = (ent.get("item_id", 0) << 8) | (ent.get("item_sub", 0) & 0xFF)
            return {
                "success": True,
                "packed_id": packed_id,
                "state": ent.get("state", 0),
                "trajectory": ent.get("trajectory", 0),
            }
    return {"success": False}


def inspect_interactive_object(facing_dir, entity_type, in_front):
    # 0x0082F8..0x0083D2
    if not in_front:
        return {"action": "none"}
    if entity_type == 0x32:
        return {"action": "npc_dialog", "jump": 0x18BBC}
    elif entity_type == 0x20:
        return {"action": "save_book", "jump": 0x18CAA}
    elif entity_type in (0x94, 0x96, 0xBE):
        return {"action": "cutscene_event", "jump": 0x18EBE}
    elif entity_type == 0xB0:
        return {
            "action": "open_chest",
            "chest_anim": 2,
            "ali_state": 0x3F,
            "sfx": 0xA00264,
        }
    return {"action": "none"}


def normalize_facing_direction(facing):
    # 0x0083D4..0x0083F8
    lut = {0: 0, 1: 3, 2: 1, 3: 2}
    return lut.get(facing % 4, 0)


class TestRomAliSpiritPowerInspector(unittest.TestCase):
    def test_spirit_power_invocation(self):
        # 1. Air call (moving + airborne state 4)
        res_air = invoke_spirit_power(moving_dpad=True, current_state=4)
        self.assertEqual(res_air["action"], "air_spirit_call")
        self.assertEqual(res_air["state"], 0x16)
        self.assertEqual(res_air["anim"], 0x16)
        self.assertEqual(res_air["vz"], 0x70000)
        self.assertEqual(res_air["counter_197e_add"], 8)

        # 2. Ground summon (stationary)
        res_ground = invoke_spirit_power(moving_dpad=False, current_state=0)
        self.assertEqual(res_ground["action"], "ground_spirit_summon")
        self.assertEqual(res_ground["state"], 0x10)
        self.assertEqual(res_ground["anim"], 0x10)
        self.assertEqual(res_ground["vz"], 0x70000)
        self.assertEqual(res_ground["vx"], 0)
        self.assertEqual(res_ground["vy"], 0)

    def test_item_proximity_scanner(self):
        entities = [{"type": 2}, {"type": 8, "item_id": 5, "item_sub": 2, "state": 1, "trajectory": 0x10000}]
        res = scan_pickup_proximity(entities)
        self.assertTrue(res["success"])
        self.assertEqual(res["packed_id"], (5 << 8) | 2)
        self.assertEqual(entities[1]["type"], 0)  # Absorbed

        # No pickup entity
        entities_none = [{"type": 2}, {"type": 3}]
        res_none = scan_pickup_proximity(entities_none)
        self.assertFalse(res_none["success"])

    def test_interactive_object_inspector(self):
        # NPC Dialog
        res_npc = inspect_interactive_object(facing_dir=0, entity_type=0x32, in_front=True)
        self.assertEqual(res_npc["action"], "npc_dialog")

        # Save Book
        res_save = inspect_interactive_object(facing_dir=1, entity_type=0x20, in_front=True)
        self.assertEqual(res_save["action"], "save_book")

        # Open Chest
        res_chest = inspect_interactive_object(facing_dir=2, entity_type=0xB0, in_front=True)
        self.assertEqual(res_chest["action"], "open_chest")
        self.assertEqual(res_chest["sfx"], 0xA00264)
        self.assertEqual(res_chest["ali_state"], 0x3F)

        # Not in front
        res_blocked = inspect_interactive_object(facing_dir=2, entity_type=0xB0, in_front=False)
        self.assertEqual(res_blocked["action"], "none")

    def test_facing_direction_normalizer(self):
        self.assertEqual(normalize_facing_direction(0), 0)
        self.assertEqual(normalize_facing_direction(1), 3)
        self.assertEqual(normalize_facing_direction(2), 1)
        self.assertEqual(normalize_facing_direction(3), 2)


if __name__ == "__main__":
    unittest.main()
