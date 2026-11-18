#!/usr/bin/env python3
"""
rom_entity_stun_lookup_test.py - M15.458 Test Suite
Tests Pool 2 Entity Knockback Stun Broadcast & Global Type Scanner.
"""

import json
import os
import unittest

EVIDENCE_JSON = "data/thor-evidence/rom_entity_stun_lookup_m15_458.json"

def simulate_knockback_broadcast(slots, self_idx: int):
    """Simulates 0x00F1B2."""
    if self_idx < len(slots) and slots[self_idx].get("type", 0) != 0:
        return False, None
    
    affected = []
    for i, s in enumerate(slots):
        if s.get("type", 0) <= 0:
            continue
        if i == self_idx:
            continue
        if s.get("flags_38", 0) & 0x08:
            continue
        if s.get("flags_3a", 0) & 0x01:
            continue
        if s.get("hitstun_82", 0) != 0:
            continue
        
        # Invert facing bit 1
        s["facing_16"] ^= 0x0002
        s["target_facing_84"] = s["facing_16"]
        s["damage_86"] = 0xCFFF
        if s.get("timer_8c", 0) == 0:
            s["timer_8c"] = 0xFFFF
        affected.append(i)
        
    script_event = 0x0004382E
    carry = True
    return carry, script_event, affected

def simulate_find_by_type_id(slots, target_type: int):
    """Simulates 0x00F234."""
    for i, s in enumerate(slots[:21]):
        if s.get("type", 0) == target_type:
            return True, i
    return False, None

class TestRomEntityStunLookup(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.abspath(os.path.join(self_dir, ".."))
        json_path = os.path.join(repo_root, EVIDENCE_JSON)
        with open(json_path, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.458")
        self.assertEqual(self.evidence["rom_range"]["start"], "0x00F1B2")
        self.assertEqual(self.evidence["rom_range"]["end"], "0x00F258")
        self.assertEqual(self.evidence["rom_range"]["size_bytes"], 166)
        self.assertEqual(self.evidence["rom_range"]["total_instructions"], 44)
        self.assertEqual(len(self.evidence["subroutines"]), 2)

    def test_pool2_knockback_occupied_slot_abort(self):
        # If slot at self_idx is already occupied, routine aborts with Carry=False
        slots = [{"type": 0x10} for _ in range(17)]
        carry, event = simulate_knockback_broadcast(slots, self_idx=0)
        self.assertFalse(carry)
        self.assertIsNone(event)

    def test_pool2_knockback_broadcast_effects(self):
        slots = [{"type": 0} for _ in range(17)]
        # Target entity at slot 1
        slots[1] = {
            "type": 0x22,
            "flags_38": 0,
            "flags_3a": 0,
            "hitstun_82": 0,
            "facing_16": 0x0001,
            "timer_8c": 0
        }
        # Immune entity at slot 2 (bit 3 of flags_38 set)
        slots[2] = {
            "type": 0x24,
            "flags_38": 0x08,
            "flags_3a": 0,
            "hitstun_82": 0,
            "facing_16": 0x0000
        }
        # Hitstunned entity at slot 3
        slots[3] = {
            "type": 0x26,
            "flags_38": 0,
            "flags_3a": 0,
            "hitstun_82": 10,
            "facing_16": 0x0000
        }

        carry, event, affected = simulate_knockback_broadcast(slots, self_idx=0)
        self.assertTrue(carry)
        self.assertEqual(event, 0x0004382E)
        self.assertEqual(affected, [1])

        # Slot 1 should have facing flipped (1 ^ 2 = 3), damage 0xCFFF, timer 0xFFFF
        self.assertEqual(slots[1]["facing_16"], 3)
        self.assertEqual(slots[1]["target_facing_84"], 3)
        self.assertEqual(slots[1]["damage_86"], 0xCFFF)
        self.assertEqual(slots[1]["timer_8c"], 0xFFFF)

    def test_find_by_type_id_present(self):
        slots = [{"type": 0} for _ in range(21)]
        slots[5]["type"] = 0x003A # Colossus Boss
        found, idx = simulate_find_by_type_id(slots, 0x003A)
        self.assertTrue(found)
        self.assertEqual(idx, 5)

    def test_find_by_type_id_absent(self):
        slots = [{"type": 0x0010} for _ in range(21)]
        found, idx = simulate_find_by_type_id(slots, 0x003A)
        self.assertFalse(found)
        self.assertIsNone(idx)

if __name__ == "__main__":
    unittest.main()
