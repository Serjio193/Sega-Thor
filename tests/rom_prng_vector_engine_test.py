#!/usr/bin/env python3
"""
rom_prng_vector_engine_test.py — Unit tests for ROM Core PRNG Subsystem,
8-Way Directional Velocity Projector & Cardinal Angle Resolver (0x00E25C..0x00E398).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_prng_vector_engine_m15_448.json"
)


class TestRomPrngVectorEngine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)
        cls.insns_by_addr = {
            insn["address"]: insn for insn in cls.evidence["instructions"]
        }

    def test_01_json_evidence_integrity(self):
        self.assertEqual(self.evidence["start_address"], "0x00E25C")
        self.assertEqual(self.evidence["end_address"], "0x00E398")
        self.assertEqual(self.evidence["byte_count"], 316)
        self.assertIn("prng_step_16bit_lfsr", self.evidence["subroutines"])
        self.assertIn("prng_range_word", self.evidence["subroutines"])
        self.assertIn("vector_directional_projection_8way", self.evidence["subroutines"])
        self.assertIn("vector_to_cardinal_direction_4way", self.evidence["subroutines"])

    def test_02_prng_step_algorithm_simulation(self):
        # Disassembly checks
        self.assertEqual(self.insns_by_addr["0x00E26A"]["mnemonic"], "move.w")
        self.assertIn("$ff1928", self.insns_by_addr["0x00E26A"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E298"]["mnemonic"], "move.w")
        self.assertIn("$ff1928", self.insns_by_addr["0x00E298"]["op_str"])

        # Python exact simulation of the 4-round XOR-shift LFSR
        def prng_step(seed):
            d0 = seed & 0xFFFF
            for _ in range(4):
                d1 = ((d0 << 2) | (d0 >> 14)) & 0xFFFF  # rol.w #2, d1
                d1 = d1 ^ d0                            # eor.w d0, d1
                x_flag = d1 & 1                         # lsr.w #1, d1 (bit 0 -> X)
                d0 = ((d0 << 1) | x_flag) & 0xFFFF      # addx.w d0, d0
            return d0

        # Step through multiple seeds and ensure validity
        seed = 0x1234
        outputs = []
        for _ in range(10):
            seed = prng_step(seed)
            outputs.append(seed)
            self.assertTrue(0 <= seed <= 0xFFFF)
        # Verify sequence is non-trivial and deterministic
        self.assertEqual(len(set(outputs)), 10)

    def test_03_prng_range_word_and_byte(self):
        # Instruction checks
        self.assertEqual(self.insns_by_addr["0x00E2A2"]["mnemonic"], "tst.w")
        self.assertEqual(self.insns_by_addr["0x00E2B0"]["mnemonic"], "mulu.w")
        self.assertEqual(self.insns_by_addr["0x00E2B2"]["mnemonic"], "swap")

        # Range scaling simulation: (rand_word * N) >> 16
        def prng_range(rand_val, n):
            if n == 0:
                return 0
            return (rand_val * n) >> 16

        self.assertEqual(prng_range(0, 100), 0)
        self.assertEqual(prng_range(0xFFFF, 100), 99)
        self.assertEqual(prng_range(0x8000, 100), 50)
        self.assertEqual(prng_range(0x4000, 100), 25)

    def test_04_8way_vector_projection_math(self):
        # Diagonal projection formula: d = (d >> 1) + (d >> 2) (0.75x)
        self.assertEqual(self.insns_by_addr["0x00E322"]["mnemonic"], "asr.l")
        self.assertEqual(self.insns_by_addr["0x00E326"]["mnemonic"], "asr.l")
        self.assertEqual(self.insns_by_addr["0x00E328"]["mnemonic"], "add.l")

        def project_8way(vx, vy, direction):
            direction = direction & 7
            if direction == 0:   # Up
                return 0, -vy
            elif direction == 2: # Right
                return vx, 0
            elif direction == 4: # Down
                return 0, vy
            elif direction == 6: # Left
                return -vx, 0
            else: # Diagonals: 1, 3, 5, 7
                dx, dy = vx, vy
                if direction == 1:   # Up-Right
                    dy = -dy
                elif direction == 5: # Down-Left
                    dx = -dx
                elif direction == 7: # Up-Left
                    dx = -dx
                    dy = -dy
                # 0.75x scaling
                dx = (dx >> 1) + (dx >> 2)
                dy = (dy >> 1) + (dy >> 2)
                return dx, dy

        self.assertEqual(project_8way(100, 100, 0), (0, -100))
        self.assertEqual(project_8way(100, 100, 2), (100, 0))
        self.assertEqual(project_8way(100, 100, 4), (0, 100))
        self.assertEqual(project_8way(100, 100, 6), (-100, 0))
        self.assertEqual(project_8way(100, 100, 3), (75, 75))
        self.assertEqual(project_8way(100, 100, 1), (75, -75))

    def test_05_vector_to_cardinal_direction_and_water_probe(self):
        # Cardinal direction resolver simulation
        def vector_to_cardinal(dx, dy):
            abs_x = abs(dx)
            abs_y = abs(dy)
            if abs_x > abs_y:
                return 3 if dx < 0 else 1
            else:
                return 0 if dy < 0 else 2

        self.assertEqual(vector_to_cardinal(100, 20), 1)   # Right
        self.assertEqual(vector_to_cardinal(-100, 20), 3)  # Left
        self.assertEqual(vector_to_cardinal(10, 50), 2)    # Up/South in this table
        self.assertEqual(vector_to_cardinal(10, -50), 0)   # Down/North in this table

        # Water probe bit checks
        self.assertEqual(self.insns_by_addr["0x00E372"]["mnemonic"], "bclr.b")
        self.assertIn("#$3, d0", self.insns_by_addr["0x00E372"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E37E"]["mnemonic"], "cmpi.b")
        self.assertIn("#$80, d5", self.insns_by_addr["0x00E37E"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00E38E"]["mnemonic"], "bset.b")
        self.assertIn("#$3, d0", self.insns_by_addr["0x00E38E"]["op_str"])


if __name__ == "__main__":
    unittest.main()
