#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Player Elemental Blade Attack Parry & Shield Interception Engine
(0x010A74..0x010BF0, Milestone M15.476).
"""

import unittest

class EntityBladeParryEngine:
    def check_blade_parry(self, projectile, enemy, shield_reach):
        # 0x010A74:
        if projectile.get('type') != 0x34:
            return False
        if projectile.get('state_2a') not in (2, 4):
            return False

        # Opposite facing required: enemy_facing ^ 2 == blade_facing
        if (enemy['facing'] ^ 2) != projectile['facing']:
            return False

        # Vertical elevation overlap
        blade_z = projectile['pos_z'] + projectile['elev_z']
        enemy_z = enemy['pos_z'] + enemy['elev_z']

        if (enemy_z + enemy['height']) < blade_z:
            return False
        if (blade_z + projectile['height']) < enemy_z:
            return False

        facing = enemy['facing']

        if facing in (0, 2):  # Facing Up (0) or Down (2)
            # Lateral X bounding box overlap
            blade_x1 = projectile['x'] - projectile['rx']
            blade_x2 = projectile['x'] + projectile['rx']
            enemy_x1 = enemy['x'] - enemy['rx']
            enemy_x2 = enemy['x'] + enemy['rx']

            if enemy_x2 < blade_x1 or enemy_x1 > blade_x2:
                return False

            if facing == 0:  # Up
                if (enemy['y'] - shield_reach) > (projectile['y'] + projectile['ry']):
                    return False
            else:  # Down (2)
                if (enemy['y'] + shield_reach) < (projectile['y'] - projectile['ry']):
                    return False
            return True

        else:  # Facing Right (1) or Left (3)
            # Lateral Y bounding box overlap
            blade_y1 = projectile['y'] - projectile['ry']
            blade_y2 = projectile['y'] + projectile['ry']
            enemy_y1 = enemy['y'] - enemy['ry']
            enemy_y2 = enemy['y'] + enemy['ry']

            if enemy_y2 < blade_y1 or enemy_y1 > blade_y2:
                return False

            if facing == 1:  # Right
                if (enemy['x'] + shield_reach) < (projectile['x'] - projectile['rx']):
                    return False
            else:  # Left (3)
                if (enemy['x'] - shield_reach) > (projectile['x'] + projectile['rx']):
                    return False
            return True


class TestEntityBladeParryEngine(unittest.TestCase):
    def setUp(self):
        self.engine = EntityBladeParryEngine()
        self.base_projectile = {
            'type': 0x34,
            'state_2a': 2,
            'facing': 3,  # Facing Left
            'x': 200, 'y': 200, 'rx': 10, 'ry': 10,
            'pos_z': 0, 'elev_z': 0, 'height': 16
        }
        self.base_enemy = {
            'facing': 1,  # Facing Right (head-on to Left-facing projectile)
            'x': 180, 'y': 200, 'rx': 12, 'ry': 12,
            'pos_z': 0, 'elev_z': 0, 'height': 24
        }

    def test_successful_head_on_parry(self):
        # Enemy at x=180 facing Right, Blade at x=200 facing Left. Reach 30 covers 180..210 -> intersects blade x-rx (190)!
        self.assertTrue(self.engine.check_blade_parry(self.base_projectile, self.base_enemy, shield_reach=30))

    def test_type_and_state_filters(self):
        proj = dict(self.base_projectile)
        # Type != 0x34
        proj['type'] = 0x3A
        self.assertFalse(self.engine.check_blade_parry(proj, self.base_enemy, shield_reach=30))

        # State_2a not in (2, 4)
        proj = dict(self.base_projectile)
        proj['state_2a'] = 0
        self.assertFalse(self.engine.check_blade_parry(proj, self.base_enemy, shield_reach=30))

    def test_same_direction_facing_rejected(self):
        enemy = dict(self.base_enemy)
        # Both facing Left (enemy struck from behind!)
        enemy['facing'] = 3
        self.assertFalse(self.engine.check_blade_parry(self.base_projectile, enemy, shield_reach=30))

    def test_vertical_elevation_overlap(self):
        proj = dict(self.base_projectile)
        # Blade elevated high (elev_z = 30) -> blade covers 30..46, enemy covers 0..24 -> no overlap
        proj['elev_z'] = 30
        self.assertFalse(self.engine.check_blade_parry(proj, self.base_enemy, shield_reach=30))

    def test_4way_parry_reach(self):
        # Test Up (0) vs Down (2)
        proj_down = dict(self.base_projectile, facing=2, x=200, y=180)  # Blade moving Down
        enemy_up = dict(self.base_enemy, facing=0, x=200, y=200)        # Enemy facing Up
        self.assertTrue(self.engine.check_blade_parry(proj_down, enemy_up, shield_reach=30))

        # Enemy too far below -> shield reach insufficient
        self.assertFalse(self.engine.check_blade_parry(proj_down, enemy_up, shield_reach=5))

if __name__ == '__main__':
    unittest.main()
