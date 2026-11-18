#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Opcodes $01, $06, $07, $08, $0A, $0B:
Loop Engine, Frame Sleep & Dynamic Entity Spawner (0x00FC6C..0x00FD8C, Milestone M15.466).
"""

import unittest

class ScriptVmLoopSpawnEngine:
    def __init__(self):
        self.loop_counters = {}
        self.delay_timer = 0
        self.scenario_threshold = 0xFF
        self.events_dispatched = []
        self.entities_spawned = []

    def op_07_set_loop_counter(self, idx, val):
        self.loop_counters[idx] = val & 0xFFFF

    def op_06_loop_decrement_branch(self, pc, idx, branch_disp):
        val = self.loop_counters.get(idx, 0)
        if val == 0:
            return pc + 4  # already zero: advance
        val = (val - 1) & 0xFFFF
        self.loop_counters[idx] = val
        if val != 0:
            return pc + branch_disp  # loop branch taken
        return pc + 4  # loop finished: advance past instruction

    def op_08_wait_frames(self, frames):
        self.delay_timer = frames
        return "YIELD_FRAME"

    def op_0a_send_event(self, entity_id, event_cmd):
        self.events_dispatched.append((entity_id, event_cmd))

    def op_0b_set_threshold(self, threshold):
        self.scenario_threshold = threshold

    def op_01_spawn_entity(self, quest_var, entity_type, hp, drop_id, facing, alloc_fail=False):
        # 1. Threshold check
        if quest_var >= self.scenario_threshold:
            return "ABORT_SCRIPT"
        # 2. Allocator check
        if alloc_fail:
            return "ABORT_SCRIPT"
        # 3. Create entity
        entity = {
            'type': entity_type,
            'facing': facing & 3,
            'cur_hp': hp,
            'max_hp': hp,
            'drop_id': drop_id,
            'ai_active': entity_type >= 0x16
        }
        self.entities_spawned.append(entity)
        return "SPAWNED"


class TestScriptVmLoopSpawnEngine(unittest.TestCase):
    def test_loop_counter_and_decrement_branch(self):
        engine = ScriptVmLoopSpawnEngine()
        engine.op_07_set_loop_counter(0, 3)
        pc = 0x1000
        disp = -0x20

        # Iteration 1: 3 -> 2 (branch taken)
        pc1 = engine.op_06_loop_decrement_branch(pc, 0, disp)
        self.assertEqual(pc1, pc + disp)
        self.assertEqual(engine.loop_counters[0], 2)

        # Iteration 2: 2 -> 1 (branch taken)
        pc2 = engine.op_06_loop_decrement_branch(pc, 0, disp)
        self.assertEqual(pc2, pc + disp)
        self.assertEqual(engine.loop_counters[0], 1)

        # Iteration 3: 1 -> 0 (loop ends, advances past instruction)
        pc3 = engine.op_06_loop_decrement_branch(pc, 0, disp)
        self.assertEqual(pc3, pc + 4)
        self.assertEqual(engine.loop_counters[0], 0)

    def test_wait_frames_and_scenario_threshold(self):
        engine = ScriptVmLoopSpawnEngine()
        res = engine.op_08_wait_frames(15)
        self.assertEqual(res, "YIELD_FRAME")
        self.assertEqual(engine.delay_timer, 15)

        engine.op_0b_set_threshold(10)
        self.assertEqual(engine.scenario_threshold, 10)

    def test_send_entity_event(self):
        engine = ScriptVmLoopSpawnEngine()
        engine.op_0a_send_event(0xFF19E8, 0x0004)
        self.assertEqual(engine.events_dispatched, [(0xFF19E8, 0x0004)])

    def test_spawn_script_entity(self):
        engine = ScriptVmLoopSpawnEngine()
        engine.op_0b_set_threshold(20)

        # Spawn succeeds: quest_var 15 < threshold 20
        res = engine.op_01_spawn_entity(
            quest_var=15, entity_type=0x18, hp=50, drop_id=0x01, facing=2
        )
        self.assertEqual(res, "SPAWNED")
        self.assertEqual(len(engine.entities_spawned), 1)
        ent = engine.entities_spawned[0]
        self.assertEqual(ent['type'], 0x18)
        self.assertEqual(ent['cur_hp'], 50)
        self.assertTrue(ent['ai_active'])

        # Spawn aborted: quest_var 20 >= threshold 20
        res_abort = engine.op_01_spawn_entity(
            quest_var=20, entity_type=0x18, hp=50, drop_id=0x01, facing=2
        )
        self.assertEqual(res_abort, "ABORT_SCRIPT")

        # Spawn aborted: allocation failure
        res_fail = engine.op_01_spawn_entity(
            quest_var=10, entity_type=0x18, hp=50, drop_id=0x01, facing=2, alloc_fail=True
        )
        self.assertEqual(res_fail, "ABORT_SCRIPT")

if __name__ == '__main__':
    unittest.main()
