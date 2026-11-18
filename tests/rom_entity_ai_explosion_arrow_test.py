#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Environmental Explosion Waves, Screen Shake
& Arrow Proximity AABB Interceptor (0x0114AE..0x0115C8).
Milestone M15.482.
"""

import unittest
import json
import os

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_explosion_arrow_m15_482.json"
)

def compute_screen_shake(frame_counter):
    return (frame_counter & 1) + 2

def compute_environmental_explosion_base(plane_a_x, plane_a_y, plane_b_x, plane_b_y):
    base_x = 0x100 - plane_b_x + plane_a_x
    base_y = 0x80 - plane_b_y + plane_a_y
    return base_x, base_y

def check_bow_arrow_proximity(arrow_proj, entity_x, entity_y, entity_z, radius_d0):
    if arrow_proj.get("type", 0) != 0x3A:
        return False
    if arrow_proj.get("state_2a", 0) == 0:
        return False
    if arrow_proj.get("z_14", 0) != entity_z:
        return False

    arrow_x = arrow_proj.get("x", 0)
    arrow_y = arrow_proj.get("y", 0)

    # Check Y bounds [entity_y - radius, entity_y + radius]
    if arrow_y < entity_y - radius_d0 or arrow_y > entity_y + radius_d0:
        return False

    # Check X bounds [entity_x - radius, entity_x + radius]
    if arrow_x < entity_x - radius_d0 or arrow_x > entity_x + radius_d0:
        return False

    return True

class TestROMEntityAIExplosionArrow(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.482")
        self.assertEqual(self.evidence["range"]["start"], "0x0114AE")
        self.assertEqual(self.evidence["range"]["end"], "0x0115C8")
        self.assertEqual(self.evidence["range"]["size_bytes"], 282)
        self.assertEqual(len(self.evidence["routines"]), 4)
        self.assertEqual(self.evidence["instruction_count"], 78)

    def test_screen_shake_timing(self):
        # Even frame -> (0 & 1) + 2 = 2
        self.assertEqual(compute_screen_shake(0), 2)
        self.assertEqual(compute_screen_shake(10), 2)
        # Odd frame -> (1 & 1) + 2 = 3
        self.assertEqual(compute_screen_shake(1), 3)
        self.assertEqual(compute_screen_shake(15), 3)

    def test_environmental_explosion_base(self):
        # Plane A = (200, 150), Plane B = (50, 40)
        # base_x = 0x100 - 50 + 200 = 256 - 50 + 200 = 406
        # base_y = 0x80 - 40 + 150 = 128 - 40 + 150 = 238
        bx, by = compute_environmental_explosion_base(200, 150, 50, 40)
        self.assertEqual(bx, 406)
        self.assertEqual(by, 238)

    def test_bow_arrow_proximity_hitbox(self):
        entity = {"x": 500, "y": 300, "z": 16}
        radius = 24

        # Hit: matching type 0x3A, active state, matching elevation, within radius
        arrow = {"type": 0x3A, "state_2a": 1, "z_14": 16, "x": 510, "y": 315}
        self.assertTrue(check_bow_arrow_proximity(arrow, entity["x"], entity["y"], entity["z"], radius))

        # Miss: wrong type (e.g. 0x34 blade wave)
        arrow_wrong_type = {"type": 0x34, "state_2a": 1, "z_14": 16, "x": 510, "y": 315}
        self.assertFalse(check_bow_arrow_proximity(arrow_wrong_type, entity["x"], entity["y"], entity["z"], radius))

        # Miss: inactive state (state_2a == 0)
        arrow_inactive = {"type": 0x3A, "state_2a": 0, "z_14": 16, "x": 510, "y": 315}
        self.assertFalse(check_bow_arrow_proximity(arrow_inactive, entity["x"], entity["y"], entity["z"], radius))

        # Miss: elevation mismatch
        arrow_wrong_z = {"type": 0x3A, "state_2a": 1, "z_14": 20, "x": 510, "y": 315}
        self.assertFalse(check_bow_arrow_proximity(arrow_wrong_z, entity["x"], entity["y"], entity["z"], radius))

        # Miss: outside X radius
        arrow_out_x = {"type": 0x3A, "state_2a": 1, "z_14": 16, "x": 525, "y": 315}
        self.assertFalse(check_bow_arrow_proximity(arrow_out_x, entity["x"], entity["y"], entity["z"], radius))

        # Miss: outside Y radius
        arrow_out_y = {"type": 0x3A, "state_2a": 1, "z_14": 16, "x": 510, "y": 325}
        self.assertFalse(check_bow_arrow_proximity(arrow_out_y, entity["x"], entity["y"], entity["z"], radius))

if __name__ == "__main__":
    unittest.main()
