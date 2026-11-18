#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Opcodes $15, $16, $18, $19:
Persistent Gater, Spawner & Ballistic Projectile Engine (0x00FEDE..0x010034, Milestone M15.468).
"""

import unittest

class PersistentProjectileEngine:
    @staticmethod
    def op_15_persistent_gater(pc, bank_id, bit_idx, banks):
        bank_key = f'bank{bank_id}'
        bit = bit_idx & 7
        byte_idx = bit_idx >> 3
        is_set = (banks.get(bank_key, {}).get(byte_idx, 0) & (1 << bit)) != 0
        if is_set:
            return ("SKIPPED", pc + 18)
        return ("SPAWN_FALLTHROUGH", pc)

    @staticmethod
    def op_16_spawn_persistent_entity(pc, record):
        # 18-byte record unpack
        entity = {
            'bank_id': record['bank_id'],
            'type': record['entity_type'],
            'bit_idx': record['bit_idx'],
            'pos_x': record['pos_x'],
            'pos_y': record['pos_y'],
            'drop_id': record['drop_id'],
            'priority': record['priority'],
            'hp': 10,
            'max_hp': 10,
            'ai_active': record['entity_type'] >= 0x16
        }
        return (entity, pc + 18)

    @staticmethod
    def op_18_spawn_particle(flag_4c, entity_type, init_ptr, x, y, priority, alloc_fail=False):
        if alloc_fail:
            return "YIELD_FRAME"
        particle = {
            'flag_4c': flag_4c,
            'type': entity_type,
            'init_ptr': init_ptr,
            'pos_x': x,
            'pos_y': y,
            'priority': priority
        }
        return ("SPAWNED", particle)

    @staticmethod
    def op_19_spawn_ballistic_3d(flag_4c, entity_type, x, y, priority, vx, vy, vz, alloc_fail=False):
        if alloc_fail:
            return "YIELD_FRAME"
        projectile = {
            'flag_4c': flag_4c,
            'type': entity_type,
            'physics_routine': "0x017A96",
            'pos_x': x,
            'pos_y': y,
            'priority': priority,
            'vel_x': vx,
            'vel_y': vy,
            'vel_z': vz
        }
        return ("SPAWNED", projectile)


class TestPersistentProjectileEngine(unittest.TestCase):
    def test_op_15_gater_skip_vs_fallthrough(self):
        pc = 0x2000
        banks = {'bank1': {1: 0x08}}  # Byte 1, bit 3 is set (bit index 11)

        # Bit 11 is set: skip 18 bytes
        res, new_pc = PersistentProjectileEngine.op_15_persistent_gater(pc, bank_id=1, bit_idx=11, banks=banks)
        self.assertEqual(res, "SKIPPED")
        self.assertEqual(new_pc, pc + 18)

        # Bit 12 is clear: fall through to spawn
        res2, new_pc2 = PersistentProjectileEngine.op_15_persistent_gater(pc, bank_id=1, bit_idx=12, banks=banks)
        self.assertEqual(res2, "SPAWN_FALLTHROUGH")
        self.assertEqual(new_pc2, pc)

    def test_op_16_spawn_persistent_entity(self):
        pc = 0x3000
        rec = {
            'bank_id': 2, 'entity_type': 0x1A, 'bit_idx': 45,
            'pos_x': 180, 'pos_y': 220, 'drop_id': 0x04, 'priority': 0x400
        }
        ent, next_pc = PersistentProjectileEngine.op_16_spawn_persistent_entity(pc, rec)
        self.assertEqual(next_pc, pc + 18)
        self.assertEqual(ent['type'], 0x1A)
        self.assertEqual(ent['hp'], 10)
        self.assertTrue(ent['ai_active'])
        self.assertEqual(ent['pos_x'], 180)

    def test_op_18_particle_spawn(self):
        res, part = PersistentProjectileEngine.op_18_spawn_particle(
            flag_4c=1, entity_type=0x34, init_ptr=0x012345, x=100, y=150, priority=0x200
        )
        self.assertEqual(res, "SPAWNED")
        self.assertEqual(part['type'], 0x34)
        self.assertEqual(part['init_ptr'], 0x012345)

        # Allocator exhaustion
        res_fail = PersistentProjectileEngine.op_18_spawn_particle(
            flag_4c=1, entity_type=0x34, init_ptr=0, x=0, y=0, priority=0, alloc_fail=True
        )
        self.assertEqual(res_fail, "YIELD_FRAME")

    def test_op_19_ballistic_projectile_3d(self):
        res, proj = PersistentProjectileEngine.op_19_spawn_ballistic_3d(
            flag_4c=2, entity_type=0x38, x=120, y=140, priority=0x300,
            vx=0x0400, vy=-0x0200, vz=0x0800
        )
        self.assertEqual(res, "SPAWNED")
        self.assertEqual(proj['physics_routine'], "0x017A96")
        self.assertEqual(proj['vel_x'], 0x0400)
        self.assertEqual(proj['vel_y'], -0x0200)
        self.assertEqual(proj['vel_z'], 0x0800)

if __name__ == '__main__':
    unittest.main()
