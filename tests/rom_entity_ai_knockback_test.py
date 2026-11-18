#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Mass Confusion, Palette Transfer, Ali Range Tracker
& Impact Knockback Dispatcher (0x011276..0x0114AE).
Milestone M15.481.
"""

import unittest
import json
import os

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_knockback_m15_481.json"
)

def apply_fear_confusion_pool2(pool_slots, self_index):
    affected = []
    for idx, slot in enumerate(pool_slots[:17]):
        if idx == self_index:
            continue
        if slot.get("type", 0) <= 0:
            continue
        if (slot.get("status_38", 0) & (1 << 3)) != 0:
            continue
        if (slot.get("status_3a", 0) & (1 << 0)) != 0:
            continue
        if slot.get("state_82", 0) != 0:
            continue
        # Invert bit 1 of facing: 0->2, 2->0, 1->3, 3->1
        cur_facing = slot.get("facing_16", 0)
        inv_facing = cur_facing ^ 2
        slot["facing_84"] = inv_facing
        slot["timer_86"] = 0xCFFF
        affected.append(idx)
    return affected

def find_slot_with_type(pool_slots, target_type):
    for slot in pool_slots[:17]:
        if slot.get("type", 0) == target_type:
            return 0  # Found
    return -1  # Not found

def compute_ali_distance_and_facing(self_x, self_y, ali_x, ali_y):
    dx = ali_x - self_x
    dy = ali_y - self_y
    abs_dx = abs(dx)
    abs_dy = abs(dy)
    if abs_dx > abs_dy:
        facing = 1 if dx >= 0 else 3
        dist = abs_dx
    else:
        facing = 2 if dy >= 0 else 0
        dist = abs_dy
    return facing, dist

def apply_impact_knockback(target, attacker_id, param_d0):
    # Check immunity bits
    if (target.get("status_38", 0) & (1 << 3)) != 0:
        return False
    if (target.get("status_3a", 0) & (1 << 0)) != 0:
        return False
    if target.get("state_82", 0) != 0:
        return False

    target["attacker_7e"] = attacker_id
    target["target_param_86"] = param_d0 & 0xFFFF
    target["state_82"] = 3
    target["timer_9d"] = 0

    hi_nibble = param_d0 & 0xF000
    double_speed = False
    if hi_nibble != 0:
        target["elevation_10"] = target.get("elevation_10", 0) + 1
        if hi_nibble == 0xC000:
            double_speed = True

    if target.get("elevation_10", 0) > 0:
        target["target_param_86"] |= 0x8000

    target["substate_83"] = 6
    target["vel_z_56"] = 0x00020000

    speed_x = 0x410
    speed_y = 0x400
    if double_speed:
        speed_x *= 2
        speed_y *= 2

    target["speed_x"] = speed_x
    target["speed_y"] = speed_y
    return True

class TestROMEntityAIKnockback(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.481")
        self.assertEqual(self.evidence["range"]["start"], "0x011276")
        self.assertEqual(self.evidence["range"]["end"], "0x0114AE")
        self.assertEqual(self.evidence["range"]["size_bytes"], 568)
        self.assertEqual(len(self.evidence["routines"]), 6)
        self.assertEqual(self.evidence["instruction_count"], 156)

    def test_fear_confusion_filtering(self):
        slots = [{"type": 1, "facing_16": 0, "status_38": 0, "status_3a": 0, "state_82": 0} for _ in range(17)]
        # Self index = 0 -> excluded
        # Slot 1: normal -> affected, facing 0 ^ 2 = 2
        # Slot 2: immune (status_38 bit 3 set) -> excluded
        slots[2]["status_38"] = 8
        # Slot 3: immune (status_3a bit 0 set) -> excluded
        slots[3]["status_3a"] = 1
        # Slot 4: already busy (state_82 != 0) -> excluded
        slots[4]["state_82"] = 1
        # Slot 5: inactive (type <= 0) -> excluded
        slots[5]["type"] = 0

        affected = apply_fear_confusion_pool2(slots, 0)
        self.assertNotIn(0, affected)
        self.assertIn(1, affected)
        self.assertNotIn(2, affected)
        self.assertNotIn(3, affected)
        self.assertNotIn(4, affected)
        self.assertNotIn(5, affected)
        self.assertEqual(slots[1]["facing_84"], 2)
        self.assertEqual(slots[1]["timer_86"], 0xCFFF)

    def test_find_slot_with_type(self):
        slots = [{"type": 0} for _ in range(17)]
        slots[4]["type"] = 0x2A
        self.assertEqual(find_slot_with_type(slots, 0x2A), 0)
        self.assertEqual(find_slot_with_type(slots, 0x55), -1)

    def test_ali_distance_and_facing(self):
        # Ali to the right: dx = 60, dy = 10 -> facing 1 (Right), dist 60
        facing, dist = compute_ali_distance_and_facing(100, 100, 160, 110)
        self.assertEqual(facing, 1)
        self.assertEqual(dist, 60)

        # Ali to the left: dx = -80, dy = 20 -> facing 3 (Left), dist 80
        facing, dist = compute_ali_distance_and_facing(100, 100, 20, 120)
        self.assertEqual(facing, 3)
        self.assertEqual(dist, 80)

        # Ali below: dx = 10, dy = 50 -> facing 2 (Down), dist 50
        facing, dist = compute_ali_distance_and_facing(100, 100, 110, 150)
        self.assertEqual(facing, 2)
        self.assertEqual(dist, 50)

        # Ali above: dx = 20, dy = -70 -> facing 0 (Up), dist 70
        facing, dist = compute_ali_distance_and_facing(100, 100, 120, 30)
        self.assertEqual(facing, 0)
        self.assertEqual(dist, 70)

    def test_impact_knockback_normal_and_boosted(self):
        # Normal impact: param = 0x0010 (no high nibble flags)
        target1 = {"status_38": 0, "status_3a": 0, "state_82": 0, "elevation_10": 0}
        hit1 = apply_impact_knockback(target1, 0x1234, 0x0010)
        self.assertTrue(hit1)
        self.assertEqual(target1["attacker_7e"], 0x1234)
        self.assertEqual(target1["state_82"], 3)
        self.assertEqual(target1["substate_83"], 6)
        self.assertEqual(target1["vel_z_56"], 0x00020000)
        self.assertEqual(target1["speed_x"], 0x410)
        self.assertEqual(target1["speed_y"], 0x400)

        # Boosted impact: param = 0xC010 (0xC000 flag set)
        target2 = {"status_38": 0, "status_3a": 0, "state_82": 0, "elevation_10": 0}
        hit2 = apply_impact_knockback(target2, 0x5678, 0xC010)
        self.assertTrue(hit2)
        self.assertEqual(target2["elevation_10"], 1)  # incremented
        self.assertEqual(target2["speed_x"], 0x820)   # doubled
        self.assertEqual(target2["speed_y"], 0x800)   # doubled
        self.assertEqual(target2["target_param_86"] & 0x8000, 0x8000)  # airborne flag

if __name__ == "__main__":
    unittest.main()
