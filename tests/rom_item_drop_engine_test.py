#!/usr/bin/env python3
"""
rom_item_drop_engine_test.py - M15.461 Test Suite
Tests Enemy Defeat Loot Drop & Chest Item Spawner Subsystem.
"""

import json
import os
import unittest

EVIDENCE_JSON = "data/thor-evidence/rom_item_drop_engine_m15_461.json"

def evaluate_enemy_drop(drop_table, prng_val1, prng_val2, flag_6f):
    """Simulates 0x00F484..0x00F4C6."""
    if flag_6f & 0x80:
        return None # drop disabled / already dropped
        
    prob1, item1, prob2, item2 = drop_table
    if prob1 != 0 and prng_val1 <= prob1:
        return item1
    if prob2 != 0 and prng_val2 <= prob2:
        return item2
    return None

def evaluate_chest_record(record_bytes, banks_ram):
    """Simulates 0x00F578..0x00F5D6."""
    cat = record_bytes[0]
    item_id = record_bytes[1]
    bank_idx = record_bytes[3]
    bit_idx = (record_bytes[4] << 8) | record_bytes[5]
    
    if bank_idx == 0:
        return True, cat, item_id # no persistence tracking
        
    bank = banks_ram.get(bank_idx, bytearray(32))
    byte_offset = bit_idx >> 3
    bit_pos = bit_idx & 7
    
    if byte_offset < len(bank):
        is_collected = (bank[byte_offset] & (1 << bit_pos)) != 0
        if is_collected:
            return False, None, None # already collected
            
    return True, cat, item_id

class TestRomItemDropEngine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.abspath(os.path.join(self_dir, ".."))
        json_path = os.path.join(repo_root, EVIDENCE_JSON)
        with open(json_path, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.461")
        self.assertEqual(self.evidence["rom_range"]["start"], "0x00F476")
        self.assertEqual(self.evidence["rom_range"]["end"], "0x00F64C")
        self.assertEqual(self.evidence["rom_range"]["size_bytes"], 470)
        self.assertEqual(self.evidence["rom_range"]["total_instructions"], 118)
        self.assertEqual(len(self.evidence["subroutines"]), 2)
        self.assertGreaterEqual(len(self.evidence["sample_chest_records"]), 10)

    def test_enemy_drop_evaluation(self):
        # Table: Primary 50% (0x8000) item 0x10, Secondary 25% (0x4000) item 0x20
        table = (0x8000, 0x0010, 0x4000, 0x0020)

        # Primary roll succeeds (0x4000 <= 0x8000)
        item = evaluate_enemy_drop(table, prng_val1=0x4000, prng_val2=0x9000, flag_6f=0)
        self.assertEqual(item, 0x0010)

        # Primary roll fails (0x9000 > 0x8000), secondary succeeds (0x2000 <= 0x4000)
        item = evaluate_enemy_drop(table, prng_val1=0x9000, prng_val2=0x2000, flag_6f=0)
        self.assertEqual(item, 0x0020)

        # Both fail
        item = evaluate_enemy_drop(table, prng_val1=0x9000, prng_val2=0x5000, flag_6f=0)
        self.assertIsNone(item)

        # Disabled via bit 7 of flag_6f
        item = evaluate_enemy_drop(table, prng_val1=0x1000, prng_val2=0x1000, flag_6f=0x80)
        self.assertIsNone(item)

    def test_chest_drop_uncollected_and_collected(self):
        # Record 0 from table $43830: 10 01 00 02 00 0D (cat 0x10, item 0x01, bank 2, bit 13)
        rec = bytes([0x10, 0x01, 0x00, 0x02, 0x00, 0x0D])
        banks = {
            1: bytearray(16),
            2: bytearray(16), # bit 13 is byte 1, bit 5
            3: bytearray(16)
        }

        # Initially uncollected
        spawn, cat, item = evaluate_chest_record(rec, banks)
        self.assertTrue(spawn)
        self.assertEqual(cat, 0x10)
        self.assertEqual(item, 0x01)

        # Mark collected: bit 13 -> byte 1, bit 5 (1 << 5 = 0x20)
        banks[2][1] |= (1 << 5)

        # Evaluates as already collected -> no spawn
        spawn, cat, item = evaluate_chest_record(rec, banks)
        self.assertFalse(spawn)
        self.assertIsNone(cat)
        self.assertIsNone(item)

    def test_rom_constants(self):
        c = self.evidence["constants"]
        self.assertEqual(c["item_entity_type"], 8)
        self.assertEqual(c["elevation_launch_velocity"], "0x00080000")
        self.assertEqual(c["chest_table_addr"], "0x043830")
        self.assertEqual(c["chest_table_record_size"], 6)

if __name__ == "__main__":
    unittest.main()
