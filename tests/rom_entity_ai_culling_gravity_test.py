#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Viewport Culling, Parent Targeting,
Cutscene Locks & 3D Ballistic Gravity Arc Physics Subsystem (0x010EEA..0x01108C).
Milestone M15.479.
"""

import unittest
import json
import os

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_culling_gravity_m15_479.json"
)

def check_viewport_culling(camera_x, camera_y, ent_x, ent_y, margin):
    min_x = camera_x - margin
    min_y = camera_y - margin
    max_x = camera_x + 320 + margin
    max_y = camera_y + 224 + margin
    if ent_x < min_x or ent_y < min_y or ent_x > max_x or ent_y > max_y:
        return True  # Culled (carry set)
    return False  # Visible (carry clear)

def face_towards_parent(self_x, self_y, parent_x, parent_y):
    dx = parent_x - self_x
    dy = parent_y - self_y
    abs_dx = abs(dx)
    abs_dy = abs(dy)
    if abs_dx <= abs_dy:
        return 2 if dy >= 0 else 0
    else:
        return 1 if dx >= 0 else 3

def check_cutscene_locks(player_lock, cutscene_lock, fade_lock, menu_lock):
    if player_lock != 0 or cutscene_lock != 0 or fade_lock != 0 or menu_lock != 0:
        return False  # Locked (carry clear)
    return True  # Unlocked (carry set)

def step_gravity_arc_simple(entity):
    # vel_z: 32-bit signed (fixed 16.16)
    # z: 32-bit signed (fixed 16.16)
    vel_z = entity["vel_z"] - 0xC000
    if vel_z < -0x80000000:
        vel_z += 0x100000000
    entity["vel_z"] = vel_z
    
    z = entity["z"] + vel_z
    if z < 0:
        entity["z"] = 0
        entity["vel_z"] = 0
        entity["val_2e"] = 0
        entity["status_39"] |= (1 << 2)
        entity["timer_9a"] = 15
        return 1  # Landed
    else:
        entity["z"] = z
        col = (entity["col_40"] | entity["col_41"]) & 0xC0
        if col == 0:
            entity["pos_x"] += entity["vel_x"]
            entity["pos_y"] += entity["vel_y"]
        return 0  # Airborne

class TestROMEntityAICullingGravity(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.479")
        self.assertEqual(self.evidence["range"]["start"], "0x010EEA")
        self.assertEqual(self.evidence["range"]["end"], "0x01108C")
        self.assertEqual(self.evidence["range"]["size_bytes"], 418)
        self.assertEqual(len(self.evidence["routines"]), 6)
        self.assertEqual(self.evidence["instruction_count"], 117)

    def test_viewport_culling(self):
        cam_x, cam_y = 1000, 500
        margin = 32
        # Inside center
        self.assertFalse(check_viewport_culling(cam_x, cam_y, 1160, 612, margin))
        # Exact bounds
        self.assertFalse(check_viewport_culling(cam_x, cam_y, cam_x - margin, cam_y - margin, margin))
        self.assertFalse(check_viewport_culling(cam_x, cam_y, cam_x + 320 + margin, cam_y + 224 + margin, margin))
        # 1 pixel beyond left
        self.assertTrue(check_viewport_culling(cam_x, cam_y, cam_x - margin - 1, 600, margin))
        # 1 pixel beyond right
        self.assertTrue(check_viewport_culling(cam_x, cam_y, cam_x + 320 + margin + 1, 600, margin))
        # 1 pixel beyond top
        self.assertTrue(check_viewport_culling(cam_x, cam_y, 1100, cam_y - margin - 1, margin))
        # 1 pixel beyond bottom
        self.assertTrue(check_viewport_culling(cam_x, cam_y, 1100, cam_y + 224 + margin + 1, margin))

    def test_face_towards_parent(self):
        # 0=Up, 1=Right, 2=Down, 3=Left
        # Parent directly above (dy < 0)
        self.assertEqual(face_towards_parent(100, 100, 100, 50), 0)
        # Parent directly below (dy > 0)
        self.assertEqual(face_towards_parent(100, 100, 100, 150), 2)
        # Parent directly right (dx > 0)
        self.assertEqual(face_towards_parent(100, 100, 150, 100), 1)
        # Parent directly left (dx < 0)
        self.assertEqual(face_towards_parent(100, 100, 50, 100), 3)
        # Equal distance (abs_dx == abs_dy) branches to Y-dominant:
        # dx = 50, dy = 50 -> Y dominant dy >= 0 -> Down (2)
        self.assertEqual(face_towards_parent(100, 100, 150, 150), 2)
        # dx = 50, dy = -50 -> Y dominant dy < 0 -> Up (0)
        self.assertEqual(face_towards_parent(100, 100, 150, 50), 0)

    def test_cutscene_player_locks(self):
        # All zero -> Unlocked (True)
        self.assertTrue(check_cutscene_locks(0, 0, 0, 0))
        # Any non-zero -> Locked (False)
        self.assertFalse(check_cutscene_locks(1, 0, 0, 0))
        self.assertFalse(check_cutscene_locks(0, 1, 0, 0))
        self.assertFalse(check_cutscene_locks(0, 0, 1, 0))
        self.assertFalse(check_cutscene_locks(0, 0, 0, 1))

    def test_gravity_arc_step(self):
        ent = {
            "z": 0x00030000,      # 3.0 units high
            "vel_z": 0x00010000,  # upward 1.0 unit/frame
            "pos_x": 0x1000,
            "pos_y": 0x2000,
            "vel_x": 0x0100,
            "vel_y": 0x0200,
            "col_40": 0,
            "col_41": 0,
            "val_2e": 10,
            "status_39": 0,
            "timer_9a": 0,
        }
        # Step 1: vel_z = 0x10000 - 0xC000 = 0x4000. z = 0x30000 + 0x4000 = 0x34000
        res = step_gravity_arc_simple(ent)
        self.assertEqual(res, 0)
        self.assertEqual(ent["vel_z"], 0x00004000)
        self.assertEqual(ent["z"], 0x00034000)
        self.assertEqual(ent["pos_x"], 0x1100)
        self.assertEqual(ent["pos_y"], 0x2200)

        # Step 2: vel_z = 0x4000 - 0xC000 = -0x8000. z = 0x34000 - 0x8000 = 0x2C000
        res = step_gravity_arc_simple(ent)
        self.assertEqual(res, 0)
        self.assertEqual(ent["z"], 0x0002C000)

        # Force downward plunge causing landing (< 0)
        ent["vel_z"] = -0x00030000
        res = step_gravity_arc_simple(ent)
        self.assertEqual(res, 1)  # Landed!
        self.assertEqual(ent["z"], 0)
        self.assertEqual(ent["vel_z"], 0)
        self.assertEqual(ent["val_2e"], 0)
        self.assertEqual(ent["status_39"] & 4, 4)
        self.assertEqual(ent["timer_9a"], 15)

if __name__ == "__main__":
    unittest.main()
