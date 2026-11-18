#!/usr/bin/env python3
"""
Unit tests for ROM Drop Item Slot Allocation Eviction Probe, Ground Shadow Spawner
& Ali Kill Counter Engine (0x00F64C..0x00F788, Milestone M15.462).
"""

import unittest

class DropProbeShadowEngine:
    ITEM_POOL_BASE = 0xFF2ABC
    SLOT_STRIDE = 90
    SLOT_COUNT = 8
    BASE_TILE = 0x0694
    TILE_STRIDE = 9
    ALI_ENTITY_ADDR = 0xFF19E8

    @staticmethod
    def allocate_slot_eviction_probe(slots, candidate_attrib, dist_checker):
        """
        Simulates 0x00F64C..0x00F6E2.
        slots: list of 8 dicts, each with 'item_id', 'attrib_byte'
        candidate_attrib: attribute byte of candidate item to drop
        dist_checker: fn(slot_idx) -> bool (True if distance >= 16 / out of range)
        Returns: ('EVICT', slot_idx, tile_offset) or ('FAIL_DROP',) or ('PANIC_TRAP',)
        """
        # Pass 1: check distance threshold 16 on unpreserved slots
        tile = DropProbeShadowEngine.BASE_TILE
        for i in range(DropProbeShadowEngine.SLOT_COUNT):
            attrib = slots[i].get('attrib_byte', 0)
            if (attrib & 0x80) == 0:  # bit 7 clear = unpreserved
                if dist_checker(i):  # carry set from distance check
                    return ('EVICT', i, tile)
            tile += DropProbeShadowEngine.TILE_STRIDE

        # Pass 2: evict first unpreserved slot regardless of distance
        tile = DropProbeShadowEngine.BASE_TILE
        for i in range(DropProbeShadowEngine.SLOT_COUNT):
            attrib = slots[i].get('attrib_byte', 0)
            if (attrib & 0x80) == 0:
                return ('EVICT', i, tile)
            tile += DropProbeShadowEngine.TILE_STRIDE

        # Fallback: check candidate item attribute bit 5 (key quest item)
        if (candidate_attrib & 0x20) == 0:
            return ('FAIL_DROP',)
        return ('PANIC_TRAP',)

    @staticmethod
    def spawn_ground_shadow(parent_slot, allocator_fn):
        """
        Simulates 0x00F6E2..0x00F740.
        allocator_fn: returns new child slot dict or None if allocation fails.
        """
        child_slot = allocator_fn()
        if child_slot is None:
            return None  # carry set: gracefully exit

        # Clear 94 words = 188 bytes
        child_slot.clear()
        child_slot['raw_words'] = [0] * 94

        # Copy 45 words = 90 bytes from parent
        for k, v in parent_slot.items():
            if k != 'raw_words':
                child_slot[k] = v

        # Configure parent
        parent_slot['flag_36'] = parent_slot.get('flag_36', 0) | 0x02
        parent_slot['elevation_priority'] = 0x7D00

        # Configure child
        child_slot['entity_type'] = 0x006A
        child_slot['flag_37'] = child_slot.get('flag_37', 0) | 0x60
        child_slot['flag_38'] = child_slot.get('flag_38', 0) & ~0x04
        child_slot['parent_ptr'] = id(parent_slot)

        return child_slot

    @staticmethod
    def credit_ali_kill_and_special_drop(enemy_slot, global_state):
        """
        Simulates 0x00F740..0x00F788.
        """
        if enemy_slot.get('drop_field_8c', 0) == 0:
            if global_state.get('special_drop_flag_dbb', False):
                global_state['special_drop_flag_dbb'] = False
                enemy_slot['drop_field_8c'] = 0x1101

        if enemy_slot.get('attacker_ptr') == DropProbeShadowEngine.ALI_ENTITY_ADDR:
            if (enemy_slot.get('flag_3a', 0) & 0x01) == 0:
                kc = global_state.get('ali_kill_counter', 0)
                kc = (kc + 1) & 0xFFFF
                if kc == 0:  # wrapped
                    kc = 0xFFFF  # saturating clamp
                global_state['ali_kill_counter'] = kc


class TestDropProbeShadowEngine(unittest.TestCase):
    def test_eviction_probe_pass1(self):
        # 8 slots: slot 3 has bit 7 clear and distance >= 16
        slots = [{'attrib_byte': 0x80} for _ in range(8)]
        slots[3]['attrib_byte'] = 0x00
        res = DropProbeShadowEngine.allocate_slot_eviction_probe(
            slots, candidate_attrib=0x00, dist_checker=lambda idx: idx == 3
        )
        self.assertEqual(res, ('EVICT', 3, 0x0694 + 3 * 9))

    def test_eviction_probe_pass2_and_fallback(self):
        # All slots close (distance < 16), slot 2 unpreserved
        slots = [{'attrib_byte': 0x80} for _ in range(8)]
        slots[2]['attrib_byte'] = 0x00
        res = DropProbeShadowEngine.allocate_slot_eviction_probe(
            slots, candidate_attrib=0x00, dist_checker=lambda idx: False
        )
        self.assertEqual(res, ('EVICT', 2, 0x0694 + 2 * 9))

        # All 8 slots preserved (bit 7 set)
        slots_full = [{'attrib_byte': 0x80} for _ in range(8)]
        # Ordinary item drop fails cleanly
        res_fail = DropProbeShadowEngine.allocate_slot_eviction_probe(
            slots_full, candidate_attrib=0x00, dist_checker=lambda idx: False
        )
        self.assertEqual(res_fail, ('FAIL_DROP',))
        # Key item (bit 5 set) triggers panic trap
        res_panic = DropProbeShadowEngine.allocate_slot_eviction_probe(
            slots_full, candidate_attrib=0x20, dist_checker=lambda idx: False
        )
        self.assertEqual(res_panic, ('PANIC_TRAP',))

    def test_shadow_particle_spawner(self):
        parent = {'pos_x': 100, 'pos_y': 200, 'elevation': 15, 'flag_36': 0}
        child = DropProbeShadowEngine.spawn_ground_shadow(parent, allocator_fn=lambda: {})
        self.assertIsNotNone(child)
        self.assertEqual(child['entity_type'], 0x006A)
        self.assertEqual(child['flag_37'] & 0x60, 0x60)
        self.assertEqual(child['flag_38'] & 0x04, 0x00)
        self.assertEqual(child['pos_x'], 100)
        self.assertEqual(parent['flag_36'] & 0x02, 0x02)
        self.assertEqual(parent['elevation_priority'], 0x7D00)

        # Failure test: allocator returns None
        parent2 = {'flag_36': 0}
        failed_child = DropProbeShadowEngine.spawn_ground_shadow(parent2, allocator_fn=lambda: None)
        self.assertIsNone(failed_child)
        self.assertEqual(parent2['flag_36'], 0)

    def test_kill_credit_and_special_drop(self):
        state = {'special_drop_flag_dbb': True, 'ali_kill_counter': 10}
        enemy = {'drop_field_8c': 0, 'attacker_ptr': DropProbeShadowEngine.ALI_ENTITY_ADDR, 'flag_3a': 0}
        DropProbeShadowEngine.credit_ali_kill_and_special_drop(enemy, state)
        self.assertFalse(state['special_drop_flag_dbb'])
        self.assertEqual(enemy['drop_field_8c'], 0x1101)
        self.assertEqual(state['ali_kill_counter'], 11)

        # Saturating clamp test: counter at 0xFFFF should not wrap to 0
        state['ali_kill_counter'] = 0xFFFF
        DropProbeShadowEngine.credit_ali_kill_and_special_drop(enemy, state)
        self.assertEqual(state['ali_kill_counter'], 0xFFFF)

if __name__ == '__main__':
    unittest.main()
