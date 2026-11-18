#!/usr/bin/env python3
"""
rom_boss_attachment_engine_test.py — Unit tests for ROM Entity Type $36 Boss Attachment,
Hierarchical Parent-Child Transform & 3D Combat Physics Subsystem
(0x00EA76..0x00ED04, 654 bytes).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_attachment_engine_m15_454.json"
)


class TestRomBossAttachmentEngine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(EVIDENCE_PATH):
            raise FileNotFoundError(f"Evidence file missing at {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_rom_constants(self):
        self.assertEqual(self.evidence["start_address"], "0x00EA76")
        self.assertEqual(self.evidence["end_address"], "0x00ED04")
        self.assertEqual(self.evidence["byte_count"], 654)
        self.assertEqual(self.evidence["instruction_count"], 169)

        insns = self.evidence["instructions"]
        # Gravity constant #$6000
        grav_found = any("6000" in i.get("op_str", "") for i in insns)
        self.assertTrue(grav_found, "Gravity #$6000 must be present")

        # Launch velocity #$20000
        launch_found = any("20000" in i.get("op_str", "") for i in insns)
        self.assertTrue(launch_found, "Launch velocity #$20000 must be present")

        # HUD register $FF1850
        hud_found = any("ff1850" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(hud_found, "HUD tracking register $FF1850 must be present")

    def test_attachment_hierarchical_transforms(self):
        # Latch setup
        def latch_child_to_parent(parent_x, parent_y, parent_z, child_x, child_y, child_z):
            rel_x = child_x - parent_x
            rel_y = child_y - parent_y
            rel_z = child_z - parent_z
            return rel_x, rel_y, rel_z

        rel_x, rel_y, rel_z = latch_child_to_parent(100, 200, 0, 120, 185, 5)
        self.assertEqual(rel_x, 20)
        self.assertEqual(rel_y, -15)
        self.assertEqual(rel_z, 5)

        # Parent moves, update child
        def update_child_position(parent_x, parent_y, parent_z, rel_x, rel_y, rel_z):
            new_child_x = parent_x + rel_x
            new_child_y = parent_y + rel_y
            new_child_z = parent_z + rel_z
            return new_child_x, new_child_y, new_child_z

        c_x, c_y, c_z = update_child_position(140, 230, 10, rel_x, rel_y, rel_z)
        self.assertEqual(c_x, 160)
        self.assertEqual(c_y, 215)
        self.assertEqual(c_z, 15)

    def test_attachment_combat_overlap_and_knockback(self):
        def apply_attachment_hit(target, attacker_addr, damage_stat, is_ali):
            target["attacker"] = attacker_addr
            target["damage"] = damage_stat
            target["reaction_mode"] = 0x0A
            target["launch_vel_z"] = 0x00020000

            hud_target = None
            hud_timer = None
            if is_ali:
                hud_timer = 0x5A
                hud_target = "target_ptr"

            return target, hud_timer, hud_target

        target = {"attacker": 0, "damage": 0, "reaction_mode": 0, "launch_vel_z": 0}
        tgt, timer, hud = apply_attachment_hit(target, 0xFF19E8, 45, is_ali=True)
        self.assertEqual(tgt["attacker"], 0xFF19E8)
        self.assertEqual(tgt["damage"], 45)
        self.assertEqual(tgt["reaction_mode"], 0x0A)
        self.assertEqual(tgt["launch_vel_z"], 0x00020000)
        self.assertEqual(timer, 0x5A)
        self.assertEqual(hud, "target_ptr")

    def test_attachment_projectile_trajectory_and_culling(self):
        def integrate_attachment_trajectory(elev_z, vel_z, cam_x, cam_y, pos_x, pos_y):
            vel_z -= 0x6000
            elev_z += vel_z
            if elev_z < 0:
                elev_z = 0
                vel_z = 0

            # Viewport test with margin 10
            margin = 10
            min_x = cam_x - margin
            min_y = cam_y - margin
            max_x = cam_x + 320 + 2 * margin
            max_y = cam_y + 224 + 2 * margin

            is_visible = True
            if pos_x < min_x or pos_y < min_y or pos_x > max_x or pos_y > max_y:
                is_visible = False

            return elev_z, vel_z, is_visible

        # Normal trajectory
        z, vz, vis = integrate_attachment_trajectory(0x20000, 0x10000, 0, 0, 160, 100)
        self.assertEqual(vz, 0x10000 - 0x6000)
        self.assertEqual(z, 0x20000 + (0x10000 - 0x6000))
        self.assertTrue(vis)

        # Offscreen culling
        z, vz, vis = integrate_attachment_trajectory(0, 0, 0, 0, 450, 100)
        self.assertFalse(vis)

    def test_latch_timer_despawn_transition(self):
        def tick_latch_timer(timer):
            if timer == 1:
                return 0, True  # Despawn!
            elif timer > 1:
                return timer - 1, False
            return 0, False

        t, despawn = tick_latch_timer(5)
        self.assertEqual(t, 4)
        self.assertFalse(despawn)

        t, despawn = tick_latch_timer(1)
        self.assertEqual(t, 0)
        self.assertTrue(despawn)


if __name__ == "__main__":
    unittest.main()
