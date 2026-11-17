#!/usr/bin/env python3
"""
rom_entity_dispatcher_test.py — Unit tests for ROM Master Group 2 Entity Tick Dispatcher,
Post-Tick Isometric Bounding Box Calculator & 89-Entry Action Function Pointer Table
(0x00DEEC..0x00E0BA).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_dispatcher_m15_446.json"
)


class TestRomEntityDispatcher(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)
        cls.insns_by_addr = {
            insn["address"]: insn for insn in cls.evidence["instructions"]
        }
        cls.table_by_type = {
            entry["entity_type"]: entry for entry in cls.evidence["jump_table"]
        }

    def test_01_json_evidence_integrity(self):
        self.assertEqual(self.evidence["start_address"], "0x00DEEC")
        self.assertEqual(self.evidence["end_address"], "0x00E0BA")
        self.assertEqual(self.evidence["byte_count"], 462)
        self.assertEqual(self.evidence["table_entry_count"], 89)
        self.assertEqual(self.evidence["table_min_type"], "0x16")
        self.assertEqual(self.evidence["table_max_type"], "0xC6")
        self.assertEqual(self.evidence["pool"]["base_address"], "$FF1AA4")
        self.assertEqual(self.evidence["pool"]["entity_count"], 20)

    def test_02_dispatcher_loop_and_pause_check(self):
        # Pause test
        self.assertEqual(self.insns_by_addr["0x00DEEC"]["mnemonic"], "btst.b")
        self.assertIn("#$1, $ff17b8", self.insns_by_addr["0x00DEEC"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DEF4"]["mnemonic"], "bne.w")
        self.assertIn("$df52", self.insns_by_addr["0x00DEF4"]["op_str"])

        # Pool setup: 20 slots at $FF1AA4
        self.assertEqual(self.insns_by_addr["0x00DEF8"]["mnemonic"], "lea.l")
        self.assertIn("$ff1aa4", self.insns_by_addr["0x00DEF8"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DEFE"]["mnemonic"], "moveq")
        self.assertIn("#$13, d7", self.insns_by_addr["0x00DEFE"]["op_str"])

        # Type filter: ble, cmpi.w #$16, bcs
        self.assertEqual(self.insns_by_addr["0x00DF04"]["mnemonic"], "ble.w")
        self.assertEqual(self.insns_by_addr["0x00DF08"]["mnemonic"], "cmpi.w")
        self.assertIn("#$16, d6", self.insns_by_addr["0x00DF08"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DF0C"]["mnemonic"], "bcs.w")

        # Stride and loop
        self.assertEqual(self.insns_by_addr["0x00DF4A"]["mnemonic"], "lea.l")
        self.assertIn("$bc(a6), a6", self.insns_by_addr["0x00DF4A"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DF4E"]["mnemonic"], "dbra")
        self.assertIn("d7, $df00", self.insns_by_addr["0x00DF4E"]["op_str"])

    def test_03_jump_table_indexing_and_targets(self):
        # Dispatch instruction
        self.assertEqual(self.insns_by_addr["0x00DF10"]["mnemonic"], "add.w")
        self.assertIn("d6, d6", self.insns_by_addr["0x00DF10"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DF12"]["mnemonic"], "movea.l")
        self.assertIn("$df28(pc, d6.w), a0", self.insns_by_addr["0x00DF12"]["op_str"])

        # Check known spirit and NPC entity targets
        self.assertEqual(self.table_by_type["0x16"]["handler_addr"], "0x0115C8")  # Bow
        self.assertEqual(self.table_by_type["0x18"]["handler_addr"], "0x011B4E")  # Ditte
        self.assertEqual(self.table_by_type["0x1A"]["handler_addr"], "0x01254C")  # Efreet
        self.assertEqual(self.table_by_type["0x1C"]["handler_addr"], "0x012F1E")  # Shadow
        self.assertEqual(self.table_by_type["0x2E"]["handler_addr"], "0x00E25C")  # NPC

        # Check dummy targets
        self.assertTrue(self.table_by_type["0x2A"]["is_dummy"])
        self.assertEqual(self.table_by_type["0x2A"]["handler_addr"], "0x00E0B8")

    def test_04_post_tick_isometric_box_projection(self):
        # Read raw extents
        self.assertEqual(self.insns_by_addr["0x00DF24"]["mnemonic"], "move.b")
        self.assertIn("$48(a6), d1", self.insns_by_addr["0x00DF24"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DF28"]["mnemonic"], "move.b")
        self.assertIn("$49(a6), d2", self.insns_by_addr["0x00DF28"]["op_str"])

        # Orientation test and swap
        self.assertEqual(self.insns_by_addr["0x00DF2C"]["mnemonic"], "btst.b")
        self.assertIn("#$0, $17(a6)", self.insns_by_addr["0x00DF2C"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DF36"]["mnemonic"], "exg.l")
        self.assertIn("d1, d2", self.insns_by_addr["0x00DF36"]["op_str"])

        # Python projection simulation
        def compute_footprint(raw_w, raw_h, swap_flag):
            d1, d2 = raw_w, raw_h
            if swap_flag:
                d1, d2 = d2, d1
            eff_w = d1
            step1 = (d2 + 1) >> 1
            step2 = step1 >> 1
            eff_h = step1 + step2
            return eff_w, eff_h

        # 16x16 standard box
        w, h = compute_footprint(16, 16, False)
        self.assertEqual((w, h), (16, 12))  # 8 + 4 = 12 (3/4 projection)

        # 24x32 swapped
        w, h = compute_footprint(24, 32, True)
        self.assertEqual((w, h), (32, 18))  # d2 was 24 -> (25 >> 1) = 12 -> 12 + 6 = 18

    def test_05_dummy_handler_instruction(self):
        self.assertEqual(self.insns_by_addr["0x00E0B8"]["mnemonic"], "rts")


if __name__ == "__main__":
    unittest.main()
