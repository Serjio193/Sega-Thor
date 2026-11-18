#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Ali Facing Tracker, Elevation Gate & 4-Way Directional
Melee Attack Hitbox Collision Engine (0x01074A..0x010916, Milestone M15.474).
"""

import unittest

class EntityCombatFacingEngine:
    def __init__(self):
        self.ali_x = 200
        self.ali_y = 200
        self.ali_z = 0
        self.ali_rx = 12
        self.ali_ry = 12

    def face_towards_ali(self, ent_x, ent_y):
        # 0x010752:
        dx = self.ali_x - ent_x
        dy = self.ali_y - ent_y
        abs_dx = abs(dx)
        abs_dy = abs(dy)

        if abs_dx > abs_dy:
            if dx > 0:
                facing = 1  # Right
            else:
                facing = 3  # Left
            primary_dist = abs_dx
        else:
            if dy > 0:
                facing = 2  # Down
            else:
                facing = 0  # Up
            primary_dist = abs_dy

        return facing, primary_dist

    def check_melee_attack_hit(self, ent_x, ent_y, ent_z, facing, reach_fwd, span_left, span_right):
        # 0x0107AA:
        # Elevation gate: threshold 24px
        if abs(self.ali_z - ent_z) >= 24:
            return False

        if facing == 0:  # Up
            # Forward reach along -Y
            if ent_y < self.ali_y:
                return False
            if (self.ali_y + self.ali_ry) < (ent_y - reach_fwd):
                return False
            # Lateral span along X
            if (self.ali_x + self.ali_rx) < (ent_x - span_left):
                return False
            if (ent_x + span_right) < (self.ali_x - self.ali_rx):
                return False
            return True

        elif facing == 1:  # Right
            # Forward reach along +X
            if self.ali_x < ent_x:
                return False
            if (self.ali_x - self.ali_rx) > (ent_x + reach_fwd):
                return False
            # Lateral span along Y
            if (self.ali_y + self.ali_ry) < (ent_y - span_left):
                return False
            if (ent_y + span_right) < (self.ali_y - self.ali_ry):
                return False
            return True

        elif facing == 2:  # Down
            # Forward reach along +Y
            if self.ali_y < ent_y:
                return False
            if (self.ali_y - self.ali_ry) > (ent_y + reach_fwd):
                return False
            # Lateral span along X (d1 is span_right, d0 is span_left)
            if (self.ali_x + self.ali_rx) < (ent_x - span_right):
                return False
            if (ent_x + span_left) < (self.ali_x - self.ali_rx):
                return False
            return True

        elif facing == 3:  # Left
            # Forward reach along -X
            if ent_x < self.ali_x:
                return False
            if (self.ali_x + self.ali_rx) < (ent_x - reach_fwd):
                return False
            # Lateral span along Y (d1 is span_right, d0 is span_left)
            if (self.ali_y + self.ali_ry) < (ent_y - span_right):
                return False
            if (ent_y + span_left) < (self.ali_y - self.ali_ry):
                return False
            return True

        return False


class TestEntityCombatFacingEngine(unittest.TestCase):
    def test_face_towards_ali(self):
        engine = EntityCombatFacingEngine()
        engine.ali_x = 200
        engine.ali_y = 200

        # Entity to the left (100, 200) -> Ali is Right -> facing 1
        facing, dist = engine.face_towards_ali(100, 200)
        self.assertEqual(facing, 1)
        self.assertEqual(dist, 100)

        # Entity to the right (300, 200) -> Ali is Left -> facing 3
        facing, dist = engine.face_towards_ali(300, 200)
        self.assertEqual(facing, 3)
        self.assertEqual(dist, 100)

        # Entity above (200, 100) -> Ali is Down -> facing 2
        facing, dist = engine.face_towards_ali(200, 100)
        self.assertEqual(facing, 2)
        self.assertEqual(dist, 100)

        # Entity below (200, 300) -> Ali is Up -> facing 0
        facing, dist = engine.face_towards_ali(200, 300)
        self.assertEqual(facing, 0)
        self.assertEqual(dist, 100)

    def test_elevation_threshold(self):
        engine = EntityCombatFacingEngine()
        engine.ali_x = 200
        engine.ali_y = 200
        engine.ali_z = 0

        # In range horizontally (facing Right, reach 50)
        # Ali at z=0, entity at z=20 (diff 20 < 24) -> hit
        self.assertTrue(engine.check_melee_attack_hit(
            ent_x=180, ent_y=200, ent_z=20, facing=1, reach_fwd=50, span_left=16, span_right=16
        ))

        # Ali at z=0, entity at z=25 (diff 25 >= 24) -> miss
        self.assertFalse(engine.check_melee_attack_hit(
            ent_x=180, ent_y=200, ent_z=25, facing=1, reach_fwd=50, span_left=16, span_right=16
        ))

    def test_4way_directional_hitboxes(self):
        engine = EntityCombatFacingEngine()
        engine.ali_x = 200
        engine.ali_y = 200
        engine.ali_z = 0

        # Facing Up (0): entity below Ali (200, 230). Reach 40, spans 16.
        # ali_y = 200, ent_y = 230 -> reach 40 covers 230..190 -> includes Ali (200) -> HIT
        self.assertTrue(engine.check_melee_attack_hit(200, 230, 0, facing=0, reach_fwd=40, span_left=16, span_right=16))
        # Entity too far below (200, 260) -> miss
        self.assertFalse(engine.check_melee_attack_hit(200, 260, 0, facing=0, reach_fwd=40, span_left=16, span_right=16))

        # Facing Right (1): entity to left of Ali (170, 200). Reach 40.
        self.assertTrue(engine.check_melee_attack_hit(170, 200, 0, facing=1, reach_fwd=40, span_left=16, span_right=16))
        # Lateral miss: entity at (170, 250) -> Y diff 50 > span 16 + ali_ry 12 = 28 -> miss
        self.assertFalse(engine.check_melee_attack_hit(170, 250, 0, facing=1, reach_fwd=40, span_left=16, span_right=16))

        # Facing Down (2): entity above Ali (200, 170). Reach 40 -> HIT
        self.assertTrue(engine.check_melee_attack_hit(200, 170, 0, facing=2, reach_fwd=40, span_left=16, span_right=16))

        # Facing Left (3): entity to right of Ali (230, 200). Reach 40 -> HIT
        self.assertTrue(engine.check_melee_attack_hit(230, 200, 0, facing=3, reach_fwd=40, span_left=16, span_right=16))

if __name__ == '__main__':
    unittest.main()
