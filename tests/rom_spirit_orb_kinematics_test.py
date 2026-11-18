#!/usr/bin/env python3
"""
Unit tests for ROM Spirit Entity Orb Kinematics, Trail Table & Diamond Vertex Generator
(0x0115C8..0x01171C).
Milestone M15.483.
"""

import unittest
import json
import os

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_spirit_orb_kinematics_m15_483.json"
)

SPIRIT_ORB_TABLE = [3, 5, 7, 8, 9, 9, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11]

def apply_friction_deceleration(vel):
    step = 0x18000
    if vel > 0:
        vel -= step
        if vel < 0:
            vel = 0
    elif vel < 0:
        vel += step
        if vel > 0:
            vel = 0
    return vel

def get_orb_scale_value(timer, state):
    if state == 2:
        idx = timer - 1
        return SPIRIT_ORB_TABLE[idx] if idx < len(SPIRIT_ORB_TABLE) else None
    elif state == 1:
        if timer < 9:
            idx = timer - 1
            return SPIRIT_ORB_TABLE[idx]
        elif timer < 27:
            idx = 26 - timer
            return SPIRIT_ORB_TABLE[idx]
        else:
            return "SNAP_TARGET"
    return None

def build_diamond_vertices(timer, scale_d1):
    d0 = (timer % 3) + 11
    d2 = (-scale_d1) & 0xFF
    # Emulate exact bytes written at 0x0116F4..0x01170A:
    # d0, d1, d1, d0, d1, d2, d0, d2, d1, d0, d2, d2
    return [
        d0, scale_d1,
        scale_d1, d0,
        scale_d1, d2,
        d0, d2,
        scale_d1, d0,
        d2, d2
    ]

class TestROMSpiritOrbKinematics(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.483")
        self.assertEqual(self.evidence["range"]["start"], "0x0115C8")
        self.assertEqual(self.evidence["range"]["end"], "0x01171C")
        self.assertEqual(self.evidence["range"]["size_bytes"], 340)
        self.assertEqual(len(self.evidence["table"]["values"]), 18)
        self.assertEqual(self.evidence["table"]["values"], SPIRIT_ORB_TABLE)
        self.assertEqual(len(self.evidence["routines"]), 3)
        self.assertEqual(self.evidence["instruction_count"], 100)

    def test_friction_deceleration(self):
        # Positive velocity decelerates by 0x18000
        self.assertEqual(apply_friction_deceleration(0x40000), 0x28000)
        self.assertEqual(apply_friction_deceleration(0x10000), 0)  # clamps to 0

        # Negative velocity accelerates toward zero by 0x18000
        self.assertEqual(apply_friction_deceleration(-0x30000), -0x18000)
        self.assertEqual(apply_friction_deceleration(-0x08000), 0)  # clamps to 0

    def test_orb_scale_table_indexing(self):
        # State 2: frames 1..6
        self.assertEqual(get_orb_scale_value(1, state=2), 3)
        self.assertEqual(get_orb_scale_value(2, state=2), 5)
        self.assertEqual(get_orb_scale_value(3, state=2), 7)

        # State 1: forward indexing (timer < 9)
        self.assertEqual(get_orb_scale_value(1, state=1), 3)
        self.assertEqual(get_orb_scale_value(8, state=1), 10)  # idx 7

        # State 1: reverse indexing (9 <= timer < 27)
        # timer 9 -> 26 - 9 = 17 -> val 11
        self.assertEqual(get_orb_scale_value(9, state=1), 11)
        # timer 26 -> 26 - 26 = 0 -> val 3
        self.assertEqual(get_orb_scale_value(26, state=1), 3)

        # State 1: timer 27 -> snap
        self.assertEqual(get_orb_scale_value(27, state=1), "SNAP_TARGET")

    def test_diamond_vertex_generation(self):
        # Timer 0 -> d0 = (0 % 3) + 11 = 11. Scale d1 = 8. d2 = -8 & 0xFF = 248.
        verts = build_diamond_vertices(timer=0, scale_d1=8)
        self.assertEqual(len(verts), 12)
        self.assertEqual(verts[0], 11)
        self.assertEqual(verts[1], 8)
        self.assertEqual(verts[5], 248)

        # Timer 4 -> d0 = (4 % 3) + 11 = 12. Scale d1 = 5.
        verts2 = build_diamond_vertices(timer=4, scale_d1=5)
        self.assertEqual(verts2[0], 12)
        self.assertEqual(verts2[1], 5)

if __name__ == "__main__":
    unittest.main()
