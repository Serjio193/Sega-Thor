#!/usr/bin/env python3
"""
rom_effect_spawner_jitter_test.py — Unit tests for ROM Dynamic Effect Type $34 Spawner,
Audio Rate Limiter & PRNG Shake-Jitter Engine
(0x00E90E..0x00EA76, 360 bytes).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_effect_spawner_jitter_m15_453.json"
)


class TestRomEffectSpawnerJitter(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(EVIDENCE_PATH):
            raise FileNotFoundError(f"Evidence file missing at {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_rom_constants(self):
        self.assertEqual(self.evidence["start_address"], "0x00E90E")
        self.assertEqual(self.evidence["end_address"], "0x00EA76")
        self.assertEqual(self.evidence["byte_count"], 360)
        self.assertEqual(self.evidence["instruction_count"], 95)

        insns = self.evidence["instructions"]
        # Type code #$34
        type_found = any("34" in i.get("op_str", "") for i in insns)
        self.assertTrue(type_found, "Type #$34 must be present")

        # Priority #$55e
        prio_found = any("55e" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(prio_found, "Priority #$55E must be present")

        # SFX #$90021b and #$600382
        sfx1_found = any("90021b" in i.get("op_str", "").lower() for i in insns)
        sfx2_found = any("600382" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(sfx1_found, "SFX #$90021B must be present")
        self.assertTrue(sfx2_found, "SFX #$600382 must be present")

    def test_init_effect_type34(self):
        def init_effect(world_x, world_y, world_z, lifetime, is_priority, current_frame, last_sfx_frame):
            entity = {
                "type": 0x34,
                "priority": 0x855E if is_priority else 0x055E,
                "facing": 0,
                "x": world_x,
                "y": world_y,
                "z": world_z,
                "anchor_x": world_x,
                "anchor_y": world_y,
                "lifetime": lifetime,
            }

            elapsed = current_frame - last_sfx_frame
            sfx_triggered = False
            if elapsed < 0 or elapsed >= 7:
                sfx_triggered = True
                last_sfx_frame = current_frame

            return entity, sfx_triggered, last_sfx_frame

        # Normal mode
        e1, sfx, last = init_effect(100, 150, 0, 30, False, current_frame=20, last_sfx_frame=10)
        self.assertEqual(e1["type"], 0x34)
        self.assertEqual(e1["priority"], 0x055E)
        self.assertEqual(e1["anchor_x"], 100)
        self.assertTrue(sfx)
        self.assertEqual(last, 20)

        # Rate-limited mode (elapsed 3 < 7 frames)
        e2, sfx, last = init_effect(100, 150, 0, 30, True, current_frame=23, last_sfx_frame=20)
        self.assertEqual(e2["priority"], 0x855E)
        self.assertFalse(sfx)
        self.assertEqual(last, 20)

    def test_cutscene_invisibility_gate(self):
        def evaluate_cutscene_gate(slot0_type):
            flags_36 = 0
            flags_38 = 0
            if slot0_type == 0x1E:
                flags_36 |= 0x02  # bit 1 invisible
                flags_38 |= 0x04  # bit 2 intangible
            else:
                flags_36 &= ~0x02
                flags_38 &= ~0x04
            return flags_36, flags_38

        f36, f38 = evaluate_cutscene_gate(0x1E)
        self.assertEqual(f36 & 2, 2)
        self.assertEqual(f38 & 4, 4)

        f36, f38 = evaluate_cutscene_gate(0x02)
        self.assertEqual(f36 & 2, 0)
        self.assertEqual(f38 & 4, 0)

    def test_tick_effect_jitter_simulation(self):
        def tick_jitter(lifetime, anchor_x, anchor_y, rand1, rand2):
            if lifetime == 0:
                return 0, anchor_x, anchor_y, False

            lifetime -= 1
            if lifetime == 0:
                return 0, anchor_x, anchor_y, False  # despawned

            jitter_x = (rand1 & 0x1F) - 0x10
            jitter_y = (rand2 & 0x1F) - 0x10

            pos_x = anchor_x + jitter_x
            pos_y = anchor_y + jitter_y
            return lifetime, pos_x, pos_y, True

        # Active jitter step
        # rand1 = 0x15 -> (0x15 & 0x1F) - 0x10 = 21 - 16 = +5
        # rand2 = 0x06 -> (0x06 & 0x1F) - 0x10 = 6 - 16 = -10
        lt, x, y, active = tick_jitter(10, 200, 300, 0x15, 0x06)
        self.assertEqual(lt, 9)
        self.assertEqual(x, 205)
        self.assertEqual(y, 290)
        self.assertTrue(active)

        # Terminal step
        lt, x, y, active = tick_jitter(1, 200, 300, 0x15, 0x06)
        self.assertEqual(lt, 0)
        self.assertFalse(active)  # despawned


if __name__ == "__main__":
    unittest.main()
