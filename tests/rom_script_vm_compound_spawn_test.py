#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Opcodes $0E & $17: Fixed Position &
Dual Cross-Linked Compound Entity Spawners (0x00FD8C..0x00FEDE, Milestone M15.467).
"""

import unittest

class CompoundSpawnEngine:
    @staticmethod
    def op_0e_spawn_fixed_pos(quest_var, threshold, entity_type, hp, drop_id, facing, visual_flag, alloc_fail=False):
        # 0x00FD90: cmp.b $FF17B7, d0
        if quest_var >= threshold:
            return "ABORT_SCRIPT"
        if alloc_fail:
            return "ABORT_SCRIPT"

        entity = {
            'type': entity_type,
            'hp': hp,
            'max_hp': hp,
            'drop_id': drop_id,
            'facing': facing,
            'visual_flag': visual_flag,
            'ai_active': entity_type >= 0x16
        }
        return ("SPAWNED", entity)

    @staticmethod
    def op_17_spawn_dual_compound(quest_var, threshold, shared_hp, drop_id, param_96, alloc_fail=False):
        # 0x00FE0E: cmp.b (threshold - 1), d0
        if quest_var >= (threshold - 1):
            return "ABORT_SCRIPT"
        if alloc_fail:
            return "ABORT_SCRIPT"

        # Allocate primary 0x72 and secondary 0x70
        primary = {
            'type': 0x0072,
            'hp': shared_hp,
            'max_hp': shared_hp,
            'visual_flag': 9,
            'pos_x': 200,
            'pos_y': 150,
            'linked_entity': None,
            'ai_routine': "0x01D140"
        }
        secondary = {
            'type': 0x0070,
            'hp': shared_hp,
            'max_hp': shared_hp,
            'visual_flag': 5,
            'pos_x': 200,
            'pos_y': 150,
            'drop_id': drop_id,
            'param_96': param_96,
            'linked_entity': None,
            'ai_routine': "0x01DF1C"
        }
        # 0x00FE80: mutual cross-linking at +$A6
        primary['linked_entity'] = secondary
        secondary['linked_entity'] = primary

        return ("SPAWNED_DUAL", primary, secondary)


class TestCompoundSpawnEngine(unittest.TestCase):
    def test_op_0e_fixed_pos_spawn(self):
        # Successful spawn
        res, ent = CompoundSpawnEngine.op_0e_spawn_fixed_pos(
            quest_var=5, threshold=10, entity_type=0x20, hp=120, drop_id=3, facing=1, visual_flag=4
        )
        self.assertEqual(res, "SPAWNED")
        self.assertEqual(ent['type'], 0x20)
        self.assertEqual(ent['hp'], 120)
        self.assertTrue(ent['ai_active'])

        # Abort on threshold
        res_abort = CompoundSpawnEngine.op_0e_spawn_fixed_pos(
            quest_var=10, threshold=10, entity_type=0x20, hp=120, drop_id=3, facing=1, visual_flag=4
        )
        self.assertEqual(res_abort, "ABORT_SCRIPT")

    def test_op_17_dual_compound_spawn(self):
        # Successful dual compound spawn
        res, p, s = CompoundSpawnEngine.op_17_spawn_dual_compound(
            quest_var=8, threshold=10, shared_hp=250, drop_id=0x05, param_96=0x1234
        )
        self.assertEqual(res, "SPAWNED_DUAL")
        self.assertEqual(p['type'], 0x0072)
        self.assertEqual(s['type'], 0x0070)
        self.assertEqual(p['hp'], 250)
        self.assertEqual(s['hp'], 250)
        self.assertEqual(p['visual_flag'], 9)
        self.assertEqual(s['visual_flag'], 5)

        # Mutual cross-linking verification
        self.assertIs(p['linked_entity'], s)
        self.assertIs(s['linked_entity'], p)

        # Cloned coordinates
        self.assertEqual(p['pos_x'], s['pos_x'])
        self.assertEqual(p['pos_y'], s['pos_y'])

        # AI routine verification
        self.assertEqual(p['ai_routine'], "0x01D140")
        self.assertEqual(s['ai_routine'], "0x01DF1C")

    def test_op_17_threshold_and_alloc_failure(self):
        # Abort when quest_var >= (threshold - 1), here threshold=10, threshold-1=9
        res_abort = CompoundSpawnEngine.op_17_spawn_dual_compound(
            quest_var=9, threshold=10, shared_hp=250, drop_id=0, param_96=0
        )
        self.assertEqual(res_abort, "ABORT_SCRIPT")

        # Abort on allocator exhaustion
        res_fail = CompoundSpawnEngine.op_17_spawn_dual_compound(
            quest_var=5, threshold=10, shared_hp=250, drop_id=0, param_96=0, alloc_fail=True
        )
        self.assertEqual(res_fail, "ABORT_SCRIPT")

if __name__ == '__main__':
    unittest.main()
