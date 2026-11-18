#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Target Angle Projector, Hover Bobbing Elevation Tracker
& Multi-Segment Kinematics Engine (0x01108C..0x011276).
Milestone M15.480.
"""

import unittest
import json
import os

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_kinematics_m15_480.json"
)

# 8-entry tangent threshold table from ROM 0x05D906
TANGENT_TABLE = [25, 78, 137, 210, 312, 479, 844, 2599]

def octant_arctangent(dx, dy):
    if dx == 0:
        return 0x40
    ratio = (dy << 8) // dx
    angle = 0
    for threshold in TANGENT_TABLE:
        if ratio <= threshold:
            break
        angle += 8
    return angle

def compute_target_angle(self_x, self_y, target_x, target_y):
    dx = target_x - self_x
    dy = self_y - target_y  # Note: dy = self_y - target_y

    if dx >= 0:
        if dy >= 0:
            ang = octant_arctangent(dx, dy)
        else:
            ang = (-octant_arctangent(dx, -dy)) & 0xFF
    else:
        neg_dx = -dx
        if dy >= 0:
            ang = ((-octant_arctangent(neg_dx, dy)) + 0x80) & 0xFF
        else:
            ang = (octant_arctangent(neg_dx, -dy) + 0x80) & 0xFF
    return ang

def update_hover_elevation(ent, ali_elevation):
    ent["phase_bb"] = (ent["phase_bb"] + 1) & 0xFF
    if (ent["phase_bb"] & 0x10) != 0:
        ent["hover_a2"] -= 0x8000
    else:
        ent["hover_a2"] += 0x8000

    target = (ali_elevation - ent["base_14"] + (ent["hover_a2"] >> 16)) & 0xFFFF
    if target >= 0x8000:
        target -= 0x10000

    cur_z = ent["z_10"]
    if target == cur_z:
        pass
    elif target < cur_z:
        cur_z = max(0, cur_z - 1)
    else:
        cur_z = cur_z + 1
    ent["z_10"] = cur_z

def count_vacant_slots(pool_slots):
    count = 0
    for slot in pool_slots[:17]:
        if slot.get("type", 0) == 0:
            count += 1
    return count

def update_multisegment_kinematics(child, parent=None, disp_table=None):
    if child.get("active_9e", 0) != 0 and disp_table:
        idx = child.get("anim_idx_a4", 0)
        disp_x, disp_y = disp_table[idx]
        child["x"] += disp_x
        child["y"] += disp_y
        child["a0"] += child.get("aa", 0)
        child["a2"] += child.get("ab", 0)

    if parent is not None:
        # Shift 8 words (16 bytes) in parent history buffer
        for i in range(len(parent["history"]) - 1, 0, -1):
            parent["history"][i] = parent["history"][i - 1]
        parent["anchor_aa"] = (child["x"] - parent["pos_a0"]) & 0xFF
        if parent["anchor_aa"] >= 0x80:
            parent["anchor_aa"] -= 0x100
        parent["anchor_ab"] = (child["y"] - parent["pos_a2"]) & 0xFF
        if parent["anchor_ab"] >= 0x80:
            parent["anchor_ab"] -= 0x100

class TestROMEntityAIKinematics(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.480")
        self.assertEqual(self.evidence["range"]["start"], "0x01108C")
        self.assertEqual(self.evidence["range"]["end"], "0x011276")
        self.assertEqual(self.evidence["range"]["size_bytes"], 490)
        self.assertEqual(len(self.evidence["routines"]), 7)
        self.assertEqual(self.evidence["instruction_count"], 138)

    def test_target_angle_octants(self):
        # Target directly right (dy = 0, dx > 0)
        self.assertEqual(compute_target_angle(100, 100, 200, 100), 0)
        # Target directly above (dx = 0, dy > 0 since self_y - target_y = 100 - 50 = 50)
        self.assertEqual(compute_target_angle(100, 100, 100, 50), 0x40)
        # Target directly left (dx < 0, dy = 0)
        self.assertEqual(compute_target_angle(100, 100, 0, 100), 0x80)
        # Target directly below (dx = 0, dy < 0)
        self.assertEqual(compute_target_angle(100, 100, 100, 150), 0xC0)

    def test_hover_elevation_bobbing(self):
        ent = {
            "phase_bb": 0,
            "hover_a2": 0,
            "base_14": 10,
            "z_10": 20
        }
        ali_z = 30
        # Target = ali_z (30) - base_14 (10) + hover (0) = 20. Current z = 20 -> unchanged
        update_hover_elevation(ent, ali_z)
        self.assertEqual(ent["phase_bb"], 1)
        self.assertEqual(ent["z_10"], 20)

        # Ali jumps to 50: target = 50 - 10 = 40. Current = 20 -> nudges up to 21
        update_hover_elevation(ent, 50)
        self.assertEqual(ent["z_10"], 21)

        # Ali lands at 0: target = 0 - 10 = -10. Current = 21 -> nudges down to 20
        update_hover_elevation(ent, 0)
        self.assertEqual(ent["z_10"], 20)

    def test_count_vacant_slots(self):
        # Create 17 slots, 5 active, 12 vacant
        slots = [{"type": 0} for _ in range(17)]
        slots[0]["type"] = 1
        slots[3]["type"] = 2
        slots[8]["type"] = 3
        slots[10]["type"] = 4
        slots[16]["type"] = 5
        self.assertEqual(count_vacant_slots(slots), 12)

    def test_multisegment_kinematics(self):
        disp_table = [(5, -2), (3, 4)]
        child = {
            "active_9e": 1,
            "anim_idx_a4": 0,
            "x": 100,
            "y": 200,
            "a0": 50,
            "a2": 60,
            "aa": 2,
            "ab": -3
        }
        parent = {
            "history": [10, 20, 30, 40, 50, 60, 70, 80],
            "pos_a0": 90,
            "pos_a2": 190,
            "anchor_aa": 0,
            "anchor_ab": 0
        }
        update_multisegment_kinematics(child, parent, disp_table)
        self.assertEqual(child["x"], 105)
        self.assertEqual(child["y"], 198)
        self.assertEqual(child["a0"], 52)
        self.assertEqual(child["a2"], 57)
        self.assertEqual(parent["history"][1], 10)  # shifted
        self.assertEqual(parent["anchor_aa"], 15)   # 105 - 90
        self.assertEqual(parent["anchor_ab"], 8)    # 198 - 190

if __name__ == "__main__":
    unittest.main()
