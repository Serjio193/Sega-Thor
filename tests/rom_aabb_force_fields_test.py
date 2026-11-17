#!/usr/bin/env python3
"""
rom_aabb_force_fields_test.py — Unit tests for ROM 3D AABB Entity Force Vector Field,
Conveyor Belt & Elevation Current Displacement Subsystem (0x00DDA2..0x00DEEC).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_aabb_force_fields_m15_445.json"
)


class TestRomAABBForceFields(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)
        cls.insns_by_addr = {
            insn["address"]: insn for insn in cls.evidence["instructions"]
        }

    def test_01_json_evidence_integrity(self):
        self.assertEqual(self.evidence["start_address"], "0x00DDA2")
        self.assertEqual(self.evidence["end_address"], "0x00DEEC")
        self.assertEqual(self.evidence["byte_count"], 330)
        self.assertEqual(self.evidence["instruction_count"], 102)
        self.assertIn("entity_apply_box_force_vector", self.evidence["entrypoints"])
        self.assertIn("entity_apply_box_force_vector_with_elevation_boost", self.evidence["entrypoints"])
        self.assertIn("entity_apply_conveyor_current_force", self.evidence["entrypoints"])

    def test_02_entity_pool_traversal_and_status_checks(self):
        # Entry 1 at 0x00DDA2: lea $ff19e8, a0; moveq #$14, d0
        self.assertEqual(self.insns_by_addr["0x00DDA2"]["mnemonic"], "lea.l")
        self.assertIn("$ff19e8", self.insns_by_addr["0x00DDA2"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DDA8"]["mnemonic"], "moveq")
        self.assertIn("#$14, d0", self.insns_by_addr["0x00DDA8"]["op_str"])

        # Stride at 0x00DDF6: lea $bc(a0), a0
        self.assertEqual(self.insns_by_addr["0x00DDF6"]["mnemonic"], "lea.l")
        self.assertIn("$bc(a0), a0", self.insns_by_addr["0x00DDF6"]["op_str"])

        # Loop at 0x00DDFA: dbra d0, $ddaa
        self.assertEqual(self.insns_by_addr["0x00DDFA"]["mnemonic"], "dbra")
        self.assertIn("d0, $ddaa", self.insns_by_addr["0x00DDFA"]["op_str"])

        # Status check at 0x00DDAA: tst.w $0(a0); ble.w $ddf6
        self.assertEqual(self.insns_by_addr["0x00DDAA"]["mnemonic"], "tst.w")
        self.assertEqual(self.insns_by_addr["0x00DDAE"]["mnemonic"], "ble.w")

    def test_03_3d_aabb_bounding_box_logic(self):
        # Y axis checks: $c(a0) against d3 (min) and d4 (max)
        self.assertEqual(self.insns_by_addr["0x00DDB2"]["mnemonic"], "move.w")
        self.assertIn("$c(a0), d6", self.insns_by_addr["0x00DDB2"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DDB6"]["mnemonic"], "cmp.w")
        self.assertIn("d3, d6", self.insns_by_addr["0x00DDB6"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DDBC"]["mnemonic"], "cmp.w")
        self.assertIn("d4, d6", self.insns_by_addr["0x00DDBC"]["op_str"])

        # X axis checks: swap d3/d4, then $8(a0) against d3 (min) and d4 (max)
        self.assertEqual(self.insns_by_addr["0x00DDC2"]["mnemonic"], "swap")
        self.assertEqual(self.insns_by_addr["0x00DDC4"]["mnemonic"], "swap")
        self.assertEqual(self.insns_by_addr["0x00DDC6"]["mnemonic"], "move.w")
        self.assertIn("$8(a0), d6", self.insns_by_addr["0x00DDC6"]["op_str"])

        # Z axis checks: $10(a0) against d5 (max), swap d5, then d5 (min)
        self.assertEqual(self.insns_by_addr["0x00DDD6"]["mnemonic"], "move.w")
        self.assertIn("$10(a0), d6", self.insns_by_addr["0x00DDD6"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DDDA"]["mnemonic"], "cmp.w")
        self.assertIn("d5, d6", self.insns_by_addr["0x00DDDA"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DDE0"]["mnemonic"], "swap")
        self.assertEqual(self.insns_by_addr["0x00DDE2"]["mnemonic"], "cmp.w")

        # Python simulation of 3D AABB test
        def test_aabb(x, y, z, min_x, max_x, min_y, max_y, min_z, max_z):
            if y < min_y or y > max_y:
                return False
            if x < min_x or x > max_x:
                return False
            if z > max_z or z < min_z:
                return False
            return True

        self.assertTrue(test_aabb(100, 200, 10, 50, 150, 150, 250, 0, 20))
        self.assertFalse(test_aabb(40, 200, 10, 50, 150, 150, 250, 0, 20))   # Out of X
        self.assertFalse(test_aabb(100, 300, 10, 50, 150, 150, 250, 0, 20))  # Out of Y
        self.assertFalse(test_aabb(100, 200, 25, 50, 150, 150, 250, 0, 20))  # Out of Z

    def test_04_force_application_and_elevation_boost(self):
        # Entry 1: single application
        self.assertEqual(self.insns_by_addr["0x00DDE8"]["mnemonic"], "add.l")
        self.assertIn("d1, $72(a0)", self.insns_by_addr["0x00DDE8"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DDEC"]["mnemonic"], "add.l")
        self.assertIn("d2, $76(a0)", self.insns_by_addr["0x00DDEC"]["op_str"])

        # Entry 2 at 0x00DE00: elevation boost check
        self.assertEqual(self.insns_by_addr["0x00DE4E"]["mnemonic"], "tst.l")
        self.assertIn("$10(a0)", self.insns_by_addr["0x00DE4E"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DE52"]["mnemonic"], "beq.b")

        # Second application at 0x00DE54 and 0x00DE58 when z != 0
        self.assertEqual(self.insns_by_addr["0x00DE54"]["mnemonic"], "add.l")
        self.assertIn("d1, $72(a0)", self.insns_by_addr["0x00DE54"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DE58"]["mnemonic"], "add.l")
        self.assertIn("d2, $76(a0)", self.insns_by_addr["0x00DE58"]["op_str"])

    def test_05_conveyor_immunity_flags(self):
        # Entry 3 at 0x00DE6C: immunity checks
        self.assertEqual(self.insns_by_addr["0x00DE7C"]["mnemonic"], "btst.b")
        self.assertIn("#$0, $3a(a0)", self.insns_by_addr["0x00DE7C"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DE82"]["mnemonic"], "bne.w")

        self.assertEqual(self.insns_by_addr["0x00DE86"]["mnemonic"], "btst.b")
        self.assertIn("#$3, $3a(a0)", self.insns_by_addr["0x00DE86"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DE8C"]["mnemonic"], "bne.w")


if __name__ == "__main__":
    unittest.main()
