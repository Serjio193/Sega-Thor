#!/usr/bin/env python3
"""
rom_elemental_blade_test.py - M15.459 Test Suite
Tests Elemental Sword Wave Projectile Spawner, Parameter Evaluator
& 8-Way Directional Trajectory Subsystem.
"""

import json
import os
import unittest

EVIDENCE_JSON = "data/thor-evidence/rom_elemental_blade_m15_459.json"

def decode_sword_parameters(param_word: int):
    """Simulates 0x00F2D6..0x00F330 parameter decoding."""
    elev = param_word & 0xFFFF
    if elev >= 0x8000:
        elev -= 0x10000
        
    damage = 3
    anim = 2
    flag_3c_bit1 = False
    omega_pierce = False
    
    if param_word & 0x80000000: # Fire Sword
        damage = 0x2004
        anim = 4
        flag_3c_bit1 = True
        
    if param_word & 0x40000000: # Omega Sword
        omega_pierce = True
        damage = 0x8005
        speed_c = 0x1000
        speed_d = 0x0F00
    elif param_word & 0x20000000: # Hyper Sword
        damage = 0x0000
        anim = 4
        flag_3c_bit1 = True
        speed_c = 0x0E00
        speed_d = 0x0D00
    else: # Normal / Fire Sword
        speed_c = 0x0C00
        speed_d = 0x0BC0
        
    return {
        "elevation": elev,
        "damage": damage,
        "anim": anim,
        "flag_3c_bit1": flag_3c_bit1,
        "omega_pierce": omega_pierce,
        "speed_c": speed_c,
        "speed_d": speed_d
    }

def calculate_anchor(facing: int, elevation: int, is_water: bool, scale_bit: bool):
    """Simulates 0x00F34E..0x00F37C anchor offset computation."""
    if is_water:
        offsets = [(3, -32), (20, -17), (-3, 4), (-20, -17)]
    else:
        offsets = [(1, -40), (16, -26), (-3, 0), (-16, -26)]
        
    dx, dy = offsets[facing & 3]
    dy += elevation
    
    anchor_x = dx << 16
    anchor_y = dy << 16
    
    if scale_bit:
        anchor_x = (anchor_x * 2) & 0xFFFFFFFF
        anchor_y = (anchor_y * 2) & 0xFFFFFFFF
    else:
        anchor_x &= 0xFFFFFFFF
        anchor_y &= 0xFFFFFFFF
        
    return anchor_x, anchor_y

class TestRomElementalBlade(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.abspath(os.path.join(self_dir, ".."))
        json_path = os.path.join(repo_root, EVIDENCE_JSON)
        with open(json_path, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.459")
        self.assertEqual(self.evidence["rom_range"]["start"], "0x00F258")
        self.assertEqual(self.evidence["rom_range"]["end"], "0x00F396")
        self.assertEqual(self.evidence["rom_range"]["size_bytes"], 318)
        self.assertEqual(len(self.evidence["ground_facing_offsets"]), 4)
        self.assertEqual(len(self.evidence["water_facing_offsets"]), 4)

    def test_facing_offset_tables(self):
        g = self.evidence["ground_facing_offsets"]
        self.assertEqual((g[0]["offset_dx"], g[0]["offset_dy"]), (1, -40))
        self.assertEqual((g[1]["offset_dx"], g[1]["offset_dy"]), (16, -26))
        self.assertEqual((g[2]["offset_dx"], g[2]["offset_dy"]), (-3, 0))
        self.assertEqual((g[3]["offset_dx"], g[3]["offset_dy"]), (-16, -26))

        w = self.evidence["water_facing_offsets"]
        self.assertEqual((w[0]["offset_dx"], w[0]["offset_dy"]), (3, -32))
        self.assertEqual((w[1]["offset_dx"], w[1]["offset_dy"]), (20, -17))
        self.assertEqual((w[2]["offset_dx"], w[2]["offset_dy"]), (-3, 4))
        self.assertEqual((w[3]["offset_dx"], w[3]["offset_dy"]), (-20, -17))

    def test_sword_type_decoding(self):
        # Fire Sword (bit 31 set)
        p_fire = decode_sword_parameters(0x8000001C)
        self.assertEqual(p_fire["elevation"], 28)
        self.assertEqual(p_fire["damage"], 0x2004)
        self.assertEqual(p_fire["anim"], 4)
        self.assertTrue(p_fire["flag_3c_bit1"])
        self.assertEqual(p_fire["speed_c"], 0x0C00)

        # Omega Sword (bit 30 set)
        p_omega = decode_sword_parameters(0x4000001C)
        self.assertEqual(p_omega["damage"], 0x8005)
        self.assertTrue(p_omega["omega_pierce"])
        self.assertEqual(p_omega["speed_c"], 0x1000)

        # Hyper Sword (bit 29 set)
        p_hyper = decode_sword_parameters(0x2000001C)
        self.assertEqual(p_hyper["damage"], 0x0000)
        self.assertEqual(p_hyper["anim"], 4)
        self.assertEqual(p_hyper["speed_c"], 0x0E00)

    def test_anchor_offset_computation(self):
        # Ground facing Up (dx=1, dy=-40), elev=10
        ax, ay = calculate_anchor(facing=0, elevation=10, is_water=False, scale_bit=False)
        self.assertEqual(ax, 0x00010000)
        self.assertEqual(ay, (-30 << 16) & 0xFFFFFFFF)

        # Water facing Right (dx=20, dy=-17), elev=0, scale_bit=True (doubled)
        ax_w, ay_w = calculate_anchor(facing=1, elevation=0, is_water=True, scale_bit=True)
        self.assertEqual(ax_w, (40 << 16) & 0xFFFFFFFF)
        self.assertEqual(ay_w, (-34 << 16) & 0xFFFFFFFF)

    def test_rom_constants(self):
        c = self.evidence["constants"]
        self.assertEqual(c["projectile_slot_ram"], "0x00FF1B60")
        self.assertEqual(c["projectile_type"], "0x0034")
        self.assertEqual(c["sprite_priority"], "0x055E")
        self.assertEqual(c["elevation_float_velocity"], "0x0000C000")
        self.assertEqual(c["lifetime_frames"], 240)

if __name__ == "__main__":
    unittest.main()
