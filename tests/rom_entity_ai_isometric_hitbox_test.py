#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Isometric Projected Melee Hitbox & Reverse-Facing
Collision Detection Engine (0x010916..0x010A74, Milestone M15.475).
"""

import unittest

class EntityIsometricHitboxEngine:
    def __init__(self):
        self.ali_x = 200
        self.ali_y = 200
        self.ali_z = 0

    def _check_box(self, ent_x, ent_y, ent_z, facing, span_lat, reach_fwd):
        ali_iso_y = self.ali_y + self.ali_z
        ent_iso_y = ent_y + ent_z

        if facing == 0:  # Up
            if ali_iso_y > ent_iso_y:
                return False
            if ali_iso_y < (ent_iso_y - reach_fwd):
                return False
            if self.ali_x < (ent_x - span_lat) or self.ali_x > (ent_x + span_lat):
                return False
            return True

        elif facing == 1:  # Right
            if self.ali_x < ent_x:
                return False
            if self.ali_x > (ent_x + reach_fwd):
                return False
            if ali_iso_y < (ent_iso_y - span_lat) or ali_iso_y > (ent_iso_y + span_lat):
                return False
            return True

        elif facing == 2:  # Down
            if ali_iso_y < ent_iso_y:
                return False
            if ali_iso_y > (ent_iso_y + reach_fwd):
                return False
            if self.ali_x < (ent_x - span_lat) or self.ali_x > (ent_x + span_lat):
                return False
            return True

        elif facing == 3:  # Left
            if self.ali_x > ent_x:
                return False
            if self.ali_x < (ent_x - reach_fwd):
                return False
            if ali_iso_y < (ent_iso_y - span_lat) or ali_iso_y > (ent_iso_y + span_lat):
                return False
            return True

        return False

    def check_hit_reverse_no_elev(self, ent_x, ent_y, ent_z, facing, span_lat, reach_fwd):
        # 0x010916: inverts facing bit 1
        return self._check_box(ent_x, ent_y, ent_z, facing ^ 2, span_lat, reach_fwd)

    def check_hit_normal_no_elev(self, ent_x, ent_y, ent_z, facing, span_lat, reach_fwd):
        # 0x010920: normal facing
        return self._check_box(ent_x, ent_y, ent_z, facing, span_lat, reach_fwd)

    def check_hit_reverse_elev_gate(self, ent_x, ent_y, ent_z, facing, span_lat, reach_fwd):
        # 0x010926: elevation gate + inverts facing bit 1
        if abs(self.ali_z - ent_z) >= 24:
            return False
        return self._check_box(ent_x, ent_y, ent_z, facing ^ 2, span_lat, reach_fwd)

    def check_hit_normal_elev_gate(self, ent_x, ent_y, ent_z, facing, span_lat, reach_fwd):
        # 0x010930: elevation gate + normal facing
        if abs(self.ali_z - ent_z) >= 24:
            return False
        return self._check_box(ent_x, ent_y, ent_z, facing, span_lat, reach_fwd)


class TestEntityIsometricHitboxEngine(unittest.TestCase):
    def test_isometric_elevation_contribution(self):
        engine = EntityIsometricHitboxEngine()
        engine.ali_x = 200
        engine.ali_y = 190
        engine.ali_z = 10  # ali_iso_y = 190 + 10 = 200

        # Entity at (200, 200, 0) -> ent_iso_y = 200 + 0 = 200
        # Facing Right (1), Ali is at x=200, so inside reach=20, and iso_y matches!
        self.assertTrue(engine.check_hit_normal_elev_gate(
            ent_x=190, ent_y=200, ent_z=0, facing=1, span_lat=10, reach_fwd=30
        ))

    def test_reverse_facing(self):
        engine = EntityIsometricHitboxEngine()
        engine.ali_x = 200
        engine.ali_y = 200
        engine.ali_z = 0

        # Entity is to the Right of Ali (230, 200, 0), facing Right (1).
        # Ali is behind entity!
        # Normal facing Right -> False
        self.assertFalse(engine.check_hit_normal_no_elev(
            ent_x=230, ent_y=200, ent_z=0, facing=1, span_lat=15, reach_fwd=40
        ))
        # Reverse facing (1 ^ 2 = 3 = Left) -> looks behind -> True!
        self.assertTrue(engine.check_hit_reverse_no_elev(
            ent_x=230, ent_y=200, ent_z=0, facing=1, span_lat=15, reach_fwd=40
        ))

    def test_elevation_gate_skip(self):
        engine = EntityIsometricHitboxEngine()
        engine.ali_x = 200
        engine.ali_y = 200
        engine.ali_z = 0

        # Entity at elevation 30 (diff 30 >= 24)
        # Without elevation gate (0x010920): True
        self.assertTrue(engine.check_hit_normal_no_elev(
            ent_x=180, ent_y=170, ent_z=30, facing=1, span_lat=20, reach_fwd=30
        ))
        # With elevation gate (0x010930): False (filtered out)
        self.assertFalse(engine.check_hit_normal_elev_gate(
            ent_x=180, ent_y=170, ent_z=30, facing=1, span_lat=20, reach_fwd=30
        ))

    def test_4way_isometric_box(self):
        engine = EntityIsometricHitboxEngine()
        engine.ali_x = 200
        engine.ali_y = 200
        engine.ali_z = 0

        # Facing Up (0): entity below Ali at iso_y = 220, reach 30 covers 190..220 -> hit
        self.assertTrue(engine.check_hit_normal_elev_gate(200, 220, 0, facing=0, span_lat=10, reach_fwd=30))
        # Facing Down (2): entity above Ali at iso_y = 180, reach 30 covers 180..210 -> hit
        self.assertTrue(engine.check_hit_normal_elev_gate(200, 180, 0, facing=2, span_lat=10, reach_fwd=30))
        # Facing Left (3): entity to right of Ali at x = 220, reach 30 covers 190..220 -> hit
        self.assertTrue(engine.check_hit_normal_elev_gate(220, 200, 0, facing=3, span_lat=10, reach_fwd=30))

if __name__ == '__main__':
    unittest.main()
