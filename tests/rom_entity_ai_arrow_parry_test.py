#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Player Bow Arrow Attack Parry & Shield Interception Engine
(0x010BF0..0x010CFE, Milestone M15.477).
"""

import unittest

class EntityArrowParryEngine:
    def check_arrow_parry(self, projectile, enemy, span_lat, reach_fwd):
        # 0x010BF0:
        if projectile.get('type') != 0x3A:
            return False
        if projectile.get('pos_z') != enemy.get('pos_z'):
            return False

        facing = enemy['facing']
        arr_x, arr_y = projectile['x'], projectile['y']
        ent_x, ent_y = enemy['x'], enemy['y']

        if facing == 0:  # Up
            if arr_y > ent_y:
                return False
            if arr_y < (ent_y - reach_fwd):
                return False
            if arr_x < (ent_x - span_lat) or arr_x > (ent_x + span_lat):
                return False
            return True

        elif facing == 1:  # Right
            if arr_x < ent_x:
                return False
            if arr_x > (ent_x + reach_fwd):
                return False
            if arr_y < (ent_y - span_lat) or arr_y > (ent_y + span_lat):
                return False
            return True

        elif facing == 2:  # Down
            if arr_y < ent_y:
                return False
            if arr_y > (ent_y + reach_fwd):
                return False
            if arr_x < (ent_x - span_lat) or arr_x > (ent_x + span_lat):
                return False
            return True

        elif facing == 3:  # Left
            if arr_x > ent_x:
                return False
            if arr_x < (ent_x - reach_fwd):
                return False
            if arr_y < (ent_y - span_lat) or arr_y > (ent_y + span_lat):
                return False
            return True

        return False


class TestEntityArrowParryEngine(unittest.TestCase):
    def setUp(self):
        self.engine = EntityArrowParryEngine()
        self.base_projectile = {
            'type': 0x3A,
            'x': 200, 'y': 180,
            'pos_z': 0
        }
        self.base_enemy = {
            'facing': 0,  # Facing Up
            'x': 200, 'y': 200,
            'pos_z': 0
        }

    def test_successful_arrow_parry_up(self):
        # Enemy at (200, 200) facing Up, Arrow at (200, 180) above enemy.
        # Reach 30 covers 200..170 -> includes arrow (180). Lateral span 10 -> includes x=200!
        self.assertTrue(self.engine.check_arrow_parry(self.base_projectile, self.base_enemy, span_lat=10, reach_fwd=30))

    def test_type_and_elevation_filters(self):
        # Wrong type
        proj = dict(self.base_projectile, type=0x34)
        self.assertFalse(self.engine.check_arrow_parry(proj, self.base_enemy, span_lat=10, reach_fwd=30))

        # Different elevation
        proj = dict(self.base_projectile, pos_z=10)
        self.assertFalse(self.engine.check_arrow_parry(proj, self.base_enemy, span_lat=10, reach_fwd=30))

    def test_behind_enemy_rejected(self):
        # Arrow is below enemy (arr_y = 220) while enemy faces Up
        proj = dict(self.base_projectile, y=220)
        self.assertFalse(self.engine.check_arrow_parry(proj, self.base_enemy, span_lat=10, reach_fwd=30))

    def test_4way_arrow_parry(self):
        # Facing Right (1): enemy at (180, 200), arrow at (200, 200)
        enemy_r = dict(self.base_enemy, facing=1, x=180, y=200)
        proj_r = dict(self.base_projectile, x=200, y=200)
        self.assertTrue(self.engine.check_arrow_parry(proj_r, enemy_r, span_lat=10, reach_fwd=30))

        # Facing Down (2): enemy at (200, 180), arrow at (200, 200)
        enemy_d = dict(self.base_enemy, facing=2, x=200, y=180)
        proj_d = dict(self.base_projectile, x=200, y=200)
        self.assertTrue(self.engine.check_arrow_parry(proj_d, enemy_d, span_lat=10, reach_fwd=30))

        # Facing Left (3): enemy at (220, 200), arrow at (200, 200)
        enemy_l = dict(self.base_enemy, facing=3, x=220, y=200)
        proj_l = dict(self.base_projectile, x=200, y=200)
        self.assertTrue(self.engine.check_arrow_parry(proj_l, enemy_l, span_lat=10, reach_fwd=30))

if __name__ == '__main__':
    unittest.main()
