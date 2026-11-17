#!/usr/bin/env python3
"""
rom_projectile_physics_test.py — Unit tests for ROM Spark Effect Dispatcher,
Projectile Gravity Fall, Lifetime Blink & Despawn Subsystem (0x00E0BA..0x00E25C).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_projectile_physics_m15_447.json"
)


class TestRomProjectilePhysics(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)
        cls.insns_by_addr = {
            insn["address"]: insn for insn in cls.evidence["instructions"]
        }

    def test_01_json_evidence_integrity(self):
        self.assertEqual(self.evidence["start_address"], "0x00E0BA")
        self.assertEqual(self.evidence["end_address"], "0x00E25C")
        self.assertEqual(self.evidence["byte_count"], 418)
        self.assertEqual(self.evidence["constants"]["spark_slots"], 6)
        self.assertEqual(self.evidence["constants"]["projectile_slots"], 8)
        self.assertEqual(self.evidence["constants"]["slot_stride"], 90)
        self.assertEqual(self.evidence["constants"]["gravity_subtrahend"], "0x0000C000")

    def test_02_spark_effect_dispatch_loop(self):
        # Pause test
        self.assertEqual(self.insns_by_addr["0x00E0BA"]["mnemonic"], "btst.b")
        self.assertIn("#$1, $ff17b8", self.insns_by_addr["0x00E0BA"]["op_str"])

        # Spark pool setup: 6 slots at $FF2D8C
        self.assertEqual(self.insns_by_addr["0x00E0C6"]["mnemonic"], "lea.l")
        self.assertIn("$ff2d8c", self.insns_by_addr["0x00E0C6"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E0CC"]["mnemonic"], "moveq")
        self.assertIn("#$5, d7", self.insns_by_addr["0x00E0CC"]["op_str"])

        # Active check and callback call
        self.assertEqual(self.insns_by_addr["0x00E0D6"]["mnemonic"], "move.l")
        self.assertIn("$56(a6), d6", self.insns_by_addr["0x00E0D6"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E0E4"]["mnemonic"], "jsr")

        # Stride and loop
        self.assertEqual(self.insns_by_addr["0x00E0EA"]["mnemonic"], "lea.l")
        self.assertIn("$5a(a6), a6", self.insns_by_addr["0x00E0EA"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E0EE"]["mnemonic"], "dbra")
        self.assertIn("d7, $e0ce", self.insns_by_addr["0x00E0EE"]["op_str"])

    def test_03_projectile_dispatch_and_jump_table(self):
        # Projectile pool setup: 8 slots at $FF2ABC
        self.assertEqual(self.insns_by_addr["0x00E112"]["mnemonic"], "lea.l")
        self.assertIn("$ff2abc", self.insns_by_addr["0x00E112"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E118"]["mnemonic"], "moveq")
        self.assertIn("#$7, d7", self.insns_by_addr["0x00E118"]["op_str"])

        # Jump table targets
        jt = self.evidence["subroutines"]["projectile_jump_table"]
        self.assertEqual(jt["state_0_target"], "0x00E150")
        self.assertEqual(jt["state_2_target"], "0x00E192")

    def test_04_projectile_gravity_and_landing(self):
        # Gravity subtraction: subi.l #$c000, d0
        self.assertEqual(self.insns_by_addr["0x00E15A"]["mnemonic"], "subi.l")
        self.assertIn("#$c000, d0", self.insns_by_addr["0x00E15A"]["op_str"])

        # Elevation integration: add.l d0, $10(a6)
        self.assertEqual(self.insns_by_addr["0x00E164"]["mnemonic"], "add.l")
        self.assertIn("d0, $10(a6)", self.insns_by_addr["0x00E164"]["op_str"])

        # Landing clamp: clr.l $10(a6)
        self.assertEqual(self.insns_by_addr["0x00E16A"]["mnemonic"], "clr.l")
        self.assertIn("$10(a6)", self.insns_by_addr["0x00E16A"]["op_str"])

        # Parent unlatch: bclr.b #$1, $36(a4)
        self.assertEqual(self.insns_by_addr["0x00E180"]["mnemonic"], "bclr.b")
        self.assertIn("#$1, $36(a4)", self.insns_by_addr["0x00E180"]["op_str"])

        # Slot free: clr.w $0(a6)
        self.assertEqual(self.insns_by_addr["0x00E18C"]["mnemonic"], "clr.w")
        self.assertIn("$0(a6)", self.insns_by_addr["0x00E18C"]["op_str"])

    def test_05_projectile_lifetime_blink_and_despawn(self):
        # Counter increment: addq.w #$1, d0
        self.assertEqual(self.insns_by_addr["0x00E1D6"]["mnemonic"], "addq.w")
        self.assertIn("#$1, d0", self.insns_by_addr["0x00E1D6"]["op_str"])

        # Blink range: cmpi.w #$f0, d0 and cmpi.w #$12c, d0
        self.assertEqual(self.insns_by_addr["0x00E1D8"]["mnemonic"], "cmpi.w")
        self.assertIn("#$f0, d0", self.insns_by_addr["0x00E1D8"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E1E0"]["mnemonic"], "cmpi.w")
        self.assertIn("#$12c, d0", self.insns_by_addr["0x00E1E0"]["op_str"])

        # Blink toggle: bset/bclr #1 on $36(a6)
        self.assertEqual(self.insns_by_addr["0x00E1F0"]["mnemonic"], "bset.b")
        self.assertIn("#$1, $36(a6)", self.insns_by_addr["0x00E1F0"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E1F8"]["mnemonic"], "bclr.b")
        self.assertIn("#$1, $36(a6)", self.insns_by_addr["0x00E1F8"]["op_str"])

        # Despawn at > 300 frames (0x12C): clr.w $0(a6)
        self.assertEqual(self.insns_by_addr["0x00E204"]["mnemonic"], "clr.w")
        self.assertIn("$0(a6)", self.insns_by_addr["0x00E204"]["op_str"])


if __name__ == "__main__":
    unittest.main()
