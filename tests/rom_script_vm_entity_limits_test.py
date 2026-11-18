#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Scene Entity Limit Checker, Parameter Buffer Indexer
& ID Allocator Subsystem (0x01027C..0x0103B0, Milestone M15.471).
"""

import unittest

class ScriptEntityLimitEngine:
    def __init__(self):
        # 16-word parameter buffer at $FF17C6
        self.param_buffer = [0] * 16
        # 17 Pool 2 slots ($FF1CD8, stride 188 bytes)
        self.pool2 = [{'type': 0, 'id_18': 0} for _ in range(17)]

    def find_entity_id_in_pool2(self, target_id):
        # 0x010382: Scans 17 slots.
        # Returns True if candidate ID is NOT present in any active slot (Carry set = available).
        # Returns False if candidate ID is found in an active slot (Carry clear = already present).
        target = target_id & 0x7FF
        for slot in self.pool2:
            if slot['type'] > 0:
                slot_id = slot['id_18'] & 0x7FF
                if slot_id == target:
                    return False  # Match found -> unavailable
        return True  # Not found -> available

    def check_entity_slot_limit(self, range_word, flag_byte):
        # 0x01027C / 0x0102BE:
        if flag_byte == 0:
            return True, 0  # Success with 0

        start_idx = (range_word >> 8) & 0xFF
        end_idx = range_word & 0xFF
        count = end_idx - start_idx

        for i in range(count + 1):
            idx = start_idx + i
            if idx < len(self.param_buffer):
                cand = self.param_buffer[idx] & 0x7FF
                if self.find_entity_id_in_pool2(cand):
                    packed = cand | ((flag_byte << 8) & 0xF800)
                    return True, packed

        return False, None  # Limit exceeded

    def check_compound_entity_slot_limit(self, range1, range2, flag1, flag2):
        # 0x010300: Dual allocation for compound entity
        start1 = (range1 >> 8) & 0xFF
        end1 = range1 & 0xFF
        cand1 = None
        for i in range(end1 - start1 + 1):
            idx = start1 + i
            if idx < len(self.param_buffer):
                c = self.param_buffer[idx] & 0x7FF
                if self.find_entity_id_in_pool2(c):
                    cand1 = c
                    break

        if cand1 is None:
            return False, None

        start2 = (range2 >> 8) & 0xFF
        end2 = range2 & 0xFF
        cand2 = None
        for i in range(end2 - start2 + 1):
            idx = start2 + i
            if idx < len(self.param_buffer):
                c = self.param_buffer[idx] & 0x7FF
                if self.find_entity_id_in_pool2(c):
                    cand2 = c
                    break

        if cand2 is None:
            return False, None

        packed1 = cand1 | ((flag1 << 8) & 0xF800)
        packed2 = cand2 | ((flag2 << 8) & 0xF800)
        dual_packed = (packed1 << 16) | packed2
        return True, dual_packed


class TestScriptEntityLimitEngine(unittest.TestCase):
    def test_find_entity_id_in_pool2(self):
        engine = ScriptEntityLimitEngine()
        # Slot 0 active with ID 0x105
        engine.pool2[0] = {'type': 1, 'id_18': 0x105}
        # Slot 1 active with ID 0x106 (masked with high bits)
        engine.pool2[1] = {'type': 2, 'id_18': 0xF906}
        # Slot 2 inactive (type 0) with ID 0x107
        engine.pool2[2] = {'type': 0, 'id_18': 0x107}

        # 0x105 is active in pool -> unavailable (returns False)
        self.assertFalse(engine.find_entity_id_in_pool2(0x105))
        # 0x106 is active in pool (& 0x7FF == 0x106) -> unavailable
        self.assertFalse(engine.find_entity_id_in_pool2(0x106))
        # 0x107 is in inactive slot -> available (returns True)
        self.assertTrue(engine.find_entity_id_in_pool2(0x107))
        # 0x200 not in pool -> available
        self.assertTrue(engine.find_entity_id_in_pool2(0x200))

    def test_check_single_entity_limit_success(self):
        engine = ScriptEntityLimitEngine()
        # Param buffer candidates: [0x10, 0x11, 0x12]
        engine.param_buffer[0] = 0x10
        engine.param_buffer[1] = 0x11
        engine.param_buffer[2] = 0x12

        # 0x10 is already active in pool
        engine.pool2[0] = {'type': 1, 'id_18': 0x10}

        # Request range start=0, end=2 (range_word = 0x0002), flag = 0x40
        success, allocated_id = engine.check_entity_slot_limit(0x0002, 0x40)
        self.assertTrue(success)
        # Should allocate next candidate 0x11 packed with (0x40 << 8 & 0xF800 = 0x4000) -> 0x4011
        self.assertEqual(allocated_id, 0x4011)

    def test_check_single_entity_limit_exhausted(self):
        engine = ScriptEntityLimitEngine()
        engine.param_buffer[0] = 0x25
        engine.pool2[0] = {'type': 1, 'id_18': 0x25}

        # Range start=0, end=0 -> only candidate 0x25 is already active
        success, allocated_id = engine.check_entity_slot_limit(0x0000, 0x20)
        self.assertFalse(success)
        self.assertIsNone(allocated_id)

    def test_check_compound_entity_limit(self):
        engine = ScriptEntityLimitEngine()
        # Range 1 at param_buffer[0..1]: [0x30, 0x31]
        engine.param_buffer[0] = 0x30
        engine.param_buffer[1] = 0x31
        # Range 2 at param_buffer[2..3]: [0x40, 0x41]
        engine.param_buffer[2] = 0x40
        engine.param_buffer[3] = 0x41

        # 0x30 is active
        engine.pool2[0] = {'type': 1, 'id_18': 0x30}

        success, dual_id = engine.check_compound_entity_slot_limit(
            range1=0x0001, range2=0x0203, flag1=0x50, flag2=0x70
        )
        self.assertTrue(success)
        # Primary should pick 0x31 (packed with 0x5000 -> 0x5031)
        # Secondary should pick 0x40 (packed with 0x7000 -> 0x7040)
        expected = (0x5031 << 16) | 0x7040
        self.assertEqual(dual_id, expected)

if __name__ == '__main__':
    unittest.main()
