#!/usr/bin/env python3
"""
Unit tests for ROM Entity AI Enemy Projectile Spawner Subsystem: Elemental Slash Waves,
Fast Arrows & High-Arc Mortar Lob Trajectories (0x010CFE..0x010EEA, Milestone M15.478).
"""

import unittest

class EnemyProjectileSpawnerEngine:
    @staticmethod
    def project_velocity_8way(speed_x, speed_y, facing):
        # Directional mapping (0=Up, 1=Right, 2=Down, 3=Left)
        if facing == 0:  # Up
            return 0, -speed_y
        elif facing == 1:  # Right
            return speed_x, 0
        elif facing == 2:  # Down
            return 0, speed_y
        else:  # Left
            return -speed_x, 0

    def spawn_slash_wave(self, parent, elevation, offset_table, params):
        # 0x010CFE:
        facing = parent['facing']
        off_x, off_y = offset_table[facing]

        proj = {
            'type': 0x34,
            'priority': 0x55E,
            'rx': 6, 'ry': 6, 'rz': 6, 'height': 8,
            'facing': facing,
            'x': parent['x'], 'y': parent['y'],
            'elev_z': elevation,
            'anchor_x': off_x << 16,
            'anchor_y': (off_y + elevation) << 16,
            'damage': params['damage'],
            'state_2a': params['state_2a'],
            'flag_38': 6,
            'flag_a6': 1,
            'flag_3c_bit1': (params['state_2a'] == 4),
            'vel_z': 0x0000C000
        }
        vx, vy = self.project_velocity_8way(params['speed_x'], params['speed_y'], facing)
        proj['vel_x'] = vx
        proj['vel_y'] = vy
        return proj

    def spawn_fast_arrow(self, parent, elevation, offset_table, param_record):
        # 0x010DB2:
        facing = parent['facing']
        off_x, off_y = offset_table[facing]

        proj = {
            'type': 0x3A,
            'priority': parent.get('priority_5a', 0),
            'facing': 0,
            'x': parent['x'], 'y': parent['y'],
            'elev_z': elevation,
            'anchor_x': off_x << 16,
            'anchor_y': (off_y + elevation) << 16,
            'subtype': param_record['subtype'],
            'lifetime': param_record['lifetime'],
            'damage': param_record['damage'],
            'state_2a': 0,
            'state_04': 0,
            'vel_z': 0x00028000
        }
        vx, vy = self.project_velocity_8way(0x0400, 0x0400, facing)
        proj['vel_x'] = vx
        proj['vel_y'] = vy
        return proj

    def spawn_high_arc_arrow(self, parent, elevation, offset_table, param_record):
        # 0x010E4E:
        facing = parent['facing']
        off_x, off_y = offset_table[facing]

        proj = {
            'type': 0x3A,
            'priority': parent.get('priority_5a', 0),
            'facing': 0,
            'x': parent['x'], 'y': parent['y'],
            'elev_z': elevation,
            'anchor_x': off_x << 16,
            'anchor_y': (off_y + elevation) << 16,
            'subtype': param_record['subtype'],
            'lifetime': param_record['lifetime'],
            'damage': param_record['damage'],
            'state_2a': 0,
            'state_04': 0,
            'vel_z': 0x00040000
        }
        vx, vy = self.project_velocity_8way(0x0100, 0x0100, facing)
        proj['vel_x'] = vx
        proj['vel_y'] = vy
        return proj


class TestEnemyProjectileSpawnerEngine(unittest.TestCase):
    def setUp(self):
        self.engine = EnemyProjectileSpawnerEngine()
        self.parent = {
            'x': 250, 'y': 180,
            'facing': 1,  # Facing Right
            'priority_5a': 0x400
        }
        self.offset_table = [(0, -20), (20, 0), (0, 20), (-20, 0)]

    def test_spawn_slash_wave(self):
        params = {
            'damage': 0x2005,
            'state_2a': 4,
            'speed_x': 0x0C00,
            'speed_y': 0x0BC0
        }
        proj = self.engine.spawn_slash_wave(self.parent, elevation=10, offset_table=self.offset_table, params=params)
        self.assertEqual(proj['type'], 0x34)
        self.assertEqual(proj['priority'], 0x55E)
        self.assertEqual(proj['rx'], 6)
        self.assertEqual(proj['height'], 8)
        self.assertEqual(proj['damage'], 0x2005)
        self.assertTrue(proj['flag_3c_bit1'])  # state 4 sets bit 1
        self.assertEqual(proj['vel_x'], 0x0C00)
        self.assertEqual(proj['vel_y'], 0)
        self.assertEqual(proj['vel_z'], 0x0000C000)

    def test_spawn_fast_arrow(self):
        param_rec = {'subtype': 1, 'lifetime': 120, 'damage': 0x8002}
        proj = self.engine.spawn_fast_arrow(self.parent, elevation=5, offset_table=self.offset_table, param_record=param_rec)
        self.assertEqual(proj['type'], 0x3A)
        self.assertEqual(proj['subtype'], 1)
        self.assertEqual(proj['lifetime'], 120)
        self.assertEqual(proj['damage'], 0x8002)
        self.assertEqual(proj['vel_x'], 0x0400)
        self.assertEqual(proj['vel_z'], 0x00028000)

    def test_spawn_high_arc_arrow(self):
        param_rec = {'subtype': 2, 'lifetime': 180, 'damage': 0x8004}
        proj = self.engine.spawn_high_arc_arrow(self.parent, elevation=5, offset_table=self.offset_table, param_record=param_rec)
        self.assertEqual(proj['type'], 0x3A)
        self.assertEqual(proj['subtype'], 2)
        self.assertEqual(proj['lifetime'], 180)
        self.assertEqual(proj['vel_x'], 0x0100)  # low horizontal speed
        self.assertEqual(proj['vel_z'], 0x00040000)  # high upward launch velocity

if __name__ == '__main__':
    unittest.main()
