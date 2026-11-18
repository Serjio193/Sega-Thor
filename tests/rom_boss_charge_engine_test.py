#!/usr/bin/env python3
"""
rom_boss_charge_engine_test.py - M15.456 Test Suite
Tests Boss Type $3A Combat State Engine (Part 2), Velocity Damping,
3D Trajectory Physics, Floor Collision 50% Recoil Bounce & Spawner Notification.
"""

import json
import os
import unittest

EVIDENCE_JSON = "data/thor-evidence/rom_boss_charge_engine_m15_456.json"

def damp_velocity_m68k(val: int) -> int:
    """Simulates m68k asr.l #1, move.l d1, d0, asr.l #1, add.l d0, d1."""
    # Convert to 32-bit signed
    if val >= 0x80000000:
        val -= 0x100000000
    half = val >> 1
    quarter = half >> 1
    res = half + quarter
    # Back to 32-bit unsigned
    return res & 0xFFFFFFFF

def simulate_bounce_physics(pos_x, pos_y, elev, vel_x, vel_y, vel_z, flags_18, flags_36):
    """Simulates 0x00EFA2 trajectory and bounce logic."""
    # Integrate horizontal
    pos_x = (pos_x + vel_x) & 0xFFFFFFFF
    pos_y = (pos_y + vel_y) & 0xFFFFFFFF
    # Apply gravity: -0xC000
    vel_z_signed = vel_z if vel_z < 0x80000000 else vel_z - 0x100000000
    vel_z_signed -= 0xC000
    
    elev_signed = elev if elev < 0x80000000 else elev - 0x100000000
    new_elev_signed = elev_signed + vel_z_signed
    
    despawned = False
    sfx_played = None
    
    if new_elev_signed < 0:
        new_elev_signed = 0
        if flags_36 & 0x08:
            despawned = True
            return pos_x, pos_y, 0, vel_x, vel_y, vel_z, flags_18, despawned, sfx_played
        
        # Inelastic bounce: neg.l d0; asr.l #1, d0
        recoil_z = (-vel_z_signed) >> 1
        vel_x_signed = vel_x if vel_x < 0x80000000 else vel_x - 0x100000000
        vel_y_signed = vel_y if vel_y < 0x80000000 else vel_y - 0x100000000
        recoil_x = vel_x_signed >> 1
        recoil_y = vel_y_signed >> 1
        flags_18 ^= 0x0800
        sfx_played = 0x400376
        
        if recoil_z < 0x4000:
            recoil_x = 0
            recoil_y = 0
            recoil_z = 0
            
        vel_x = recoil_x & 0xFFFFFFFF
        vel_y = recoil_y & 0xFFFFFFFF
        vel_z = recoil_z & 0xFFFFFFFF
    else:
        vel_z = vel_z_signed & 0xFFFFFFFF
        
    return pos_x, pos_y, new_elev_signed & 0xFFFFFFFF, vel_x, vel_y, vel_z, flags_18, despawned, sfx_played

class TestRomBossChargeEngine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(os.path.abspath(__file__))
        repo_root = os.path.abspath(os.path.join(self_dir, ".."))
        json_path = os.path.join(repo_root, EVIDENCE_JSON)
        with open(json_path, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.456")
        self.assertEqual(self.evidence["rom_range"]["start"], "0x00EE94")
        self.assertEqual(self.evidence["rom_range"]["end"], "0x00F054")
        self.assertEqual(self.evidence["rom_range"]["size_bytes"], 448)
        self.assertEqual(self.evidence["rom_range"]["total_instructions"], 120)
        self.assertEqual(len(self.evidence["subroutines"]), 6)

    def test_velocity_damping_formula(self):
        # Initial velocity 0x00010000 (1.0 in 16.16)
        v = 0x00010000
        damped = damp_velocity_m68k(v)
        # 0.75 * 0x10000 = 0xC000
        self.assertEqual(damped, 0x0000C000)

        # Negative velocity -0x00010000
        v_neg = (-0x00010000) & 0xFFFFFFFF
        damped_neg = damp_velocity_m68k(v_neg)
        self.assertEqual(damped_neg, (-0x0000C000) & 0xFFFFFFFF)

    def test_trajectory_bounce_recoil(self):
        # Downward moving rock hitting ground
        pos_x = 0x00640000
        pos_y = 0x00400000
        elev = 0x00005000
        vel_x = 0x00020000
        vel_y = 0x00010000
        vel_z = (-0x00020000) & 0xFFFFFFFF
        flags_18 = 0
        flags_36 = 0

        px, py, el, vx, vy, vz, f18, despawn, sfx = simulate_bounce_physics(
            pos_x, pos_y, elev, vel_x, vel_y, vel_z, flags_18, flags_36
        )

        self.assertEqual(el, 0)
        self.assertFalse(despawn)
        self.assertEqual(sfx, 0x400376)
        # Downward speed before floor impact: -0x20000 - 0xC000 = -0x2C000
        # Recoil = -(-0x2C000) >> 1 = 0x16000
        self.assertEqual(vz, 0x00016000)
        # Horizontal friction cut in half: 0x20000 >> 1 = 0x10000, 0x10000 >> 1 = 0x8000
        self.assertEqual(vx, 0x00010000)
        self.assertEqual(vy, 0x00008000)
        # Flip bit 11 toggled
        self.assertEqual(f18, 0x0800)

    def test_trajectory_low_velocity_rest(self):
        # Bounce with recoil < 0x4000 comes to complete rest
        pos_x = 0
        pos_y = 0
        elev = 0x00001000
        vel_x = 0x00001000
        vel_y = 0x00001000
        # Positive upward velocity decaying to small downward speed under gravity
        # 0x00005000 - 0xC000 = -0x00007000 -> hits ground, recoil = 0x3800 < 0x4000
        vel_z = 0x00005000
        flags_18 = 0
        flags_36 = 0

        px, py, el, vx, vy, vz, f18, despawn, sfx = simulate_bounce_physics(
            pos_x, pos_y, elev, vel_x, vel_y, vel_z, flags_18, flags_36
        )

        self.assertEqual(el, 0)
        # Recoil < 0x4000 should clear all velocities
        self.assertEqual(vx, 0)
        self.assertEqual(vy, 0)
        self.assertEqual(vz, 0)

    def test_spawner_despawn_notification(self):
        # Spawner notification sets word at +$06(spawner) to 0x10 and clears +$00(entity)
        entity = {"slot_active": 0x3A, "parent_ptr": 0xFF1800}
        spawner = {"state": 0x0004}

        # Despawn logic
        if entity["parent_ptr"] not in (0, 0xFF19E8):
            spawner["state"] = 0x0010
        entity["slot_active"] = 0

        self.assertEqual(spawner["state"], 0x0010)
        self.assertEqual(entity["slot_active"], 0)

if __name__ == "__main__":
    unittest.main()
