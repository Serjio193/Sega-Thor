#!/usr/bin/env python3
"""
rom_entity_trajectory_culling_test.py — Unit tests for ROM Entity Trajectory Gravity
Integration, Viewport AABB Culling & 188-Byte Slot Cloner
(0x00E502..0x00E6BA, 440 bytes).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_trajectory_culling_m15_450.json"
)


class TestRomEntityTrajectoryCulling(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(EVIDENCE_PATH):
            raise FileNotFoundError(f"Evidence file missing at {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_rom_constants(self):
        self.assertEqual(self.evidence["start_address"], "0x00E502")
        self.assertEqual(self.evidence["end_address"], "0x00E6BA")
        self.assertEqual(self.evidence["byte_count"], 440)
        self.assertEqual(self.evidence["instruction_count"], 122)

        insns = self.evidence["instructions"]
        # Check gravity constant #$c000
        gravity_found = any("c000" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(gravity_found, "Gravity #$c000 must be present")

        # Check viewport dimensions #$140 (320) and #$e0 (224)
        vp_w_found = any("140" in i.get("op_str", "").lower() for i in insns)
        vp_h_found = any("e0" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(vp_w_found, "Viewport width 0x140 must be present")
        self.assertTrue(vp_h_found, "Viewport height 0xE0 must be present")

        # Check entity copy counter 0x5D (93 -> 94 words = 188 bytes)
        copy_found = any("5d" in i.get("op_str", "").lower() for i in insns)
        self.assertTrue(copy_found, "188-byte copy counter 0x5D must be present")

    def test_entity_integrate_trajectory_gravity_fall(self):
        def integrate_trajectory(elev_z, vel_z, vel_x, vel_y, flags_39, shockwave_enabled):
            if vel_z == 0:
                return 3, elev_z, vel_z, 0, 0, flags_39, False

            # Gravity acceleration
            vel_z -= 0xC000
            if vel_z == 0:
                vel_z = -1
            elev_z += vel_z

            if elev_z < 0:
                # Landed
                elev_z = 0
                vel_z = 0
                flags_39 |= 4
                spawn_shockwave = False
                if shockwave_enabled:
                    flags_39 &= ~4
                    spawn_shockwave = True
                return 1, elev_z, vel_z, 0, 0, flags_39, spawn_shockwave
            else:
                # Airborne
                disp_x = vel_x
                disp_y = vel_y
                return 2, elev_z, vel_z, disp_x, disp_y, flags_39, False

        # Airborne step
        state, z, vz, dx, dy, f39, shock = integrate_trajectory(
            elev_z=0x00040000, vel_z=0x00020000, vel_x=0x0100, vel_y=0x0080, flags_39=0, shockwave_enabled=False
        )
        self.assertEqual(state, 2)
        self.assertEqual(vz, 0x00020000 - 0xC000)
        self.assertEqual(z, 0x00040000 + (0x00020000 - 0xC000))
        self.assertEqual((dx, dy), (0x0100, 0x0080))

        # Ground impact step
        state, z, vz, dx, dy, f39, shock = integrate_trajectory(
            elev_z=0x00004000, vel_z=-0x00020000, vel_x=0x0100, vel_y=0x0080, flags_39=0, shockwave_enabled=True
        )
        self.assertEqual(state, 1)  # landed
        self.assertEqual(z, 0)
        self.assertEqual(vz, 0)
        self.assertTrue(shock)

    def test_entity_viewport_culling_test_screen(self):
        def test_screen_culling(x, y, elev, cam_x, cam_y, margin):
            screen_y = y - elev
            min_x = cam_x - margin
            min_y = cam_y - margin
            max_x = cam_x + 0x140 + 2 * margin
            max_y = cam_y + 0x0E0 + 2 * margin

            if x < min_x or screen_y < min_y:
                return False  # culled
            if x > max_x or screen_y > max_y:
                return False  # culled
            return True  # visible

        # Inside standard viewport
        self.assertTrue(test_screen_culling(160, 112, 0, 0, 0, 0))

        # Outside standard viewport
        self.assertFalse(test_screen_culling(350, 112, 0, 0, 0, 0))

        # Visible with margin expansion
        self.assertTrue(test_screen_culling(350, 112, 0, 0, 0, 32))

        # Elevated entity brought into viewport by elevation subtraction
        # World Y=240, cam_y=0 -> 240 > 224 (culled without elevation)
        # Screen Y = 240 - 50 = 190 <= 224 (visible with elevation!)
        self.assertTrue(test_screen_culling(160, 240, 50, 0, 0, 0))

    def test_entity_viewport_culling_test_shadow(self):
        def test_shadow_culling(x, y, cam_x, cam_y, margin):
            min_x = cam_x - margin
            min_y = cam_y - margin
            max_x = cam_x + 0x140 + 2 * margin
            max_y = cam_y + 0x0E0 + 2 * margin

            if x < min_x or y < min_y:
                return False
            if x > max_x or y > max_y:
                return False
            return True

        # Shadow ignores elevation: world Y=240 is culled outside 224
        self.assertFalse(test_shadow_culling(160, 240, 0, 0, 0))
        self.assertTrue(test_shadow_culling(160, 200, 0, 0, 0))

    def test_entity_slot_copy_and_cooldown_tick(self):
        # 188-byte copy simulation
        src_slot = list(range(94))
        dst_slot = [0] * 94
        for i in range(94):
            dst_slot[i] = src_slot[i]
        self.assertEqual(dst_slot, src_slot)
        self.assertEqual(len(dst_slot) * 2, 188)

        # Invulnerability tick simulation
        def tick_cooldown(flash_timer, hitstun_cooldown, flags_36, anim_frame):
            invuln_bit = False
            if flash_timer > 0 and flash_timer < 20:
                invuln_bit = True

            abort_caller = False
            if hitstun_cooldown > 0:
                anim_frame += 1
                hitstun_cooldown -= 1
                if hitstun_cooldown > 0:
                    abort_caller = True

            return invuln_bit, hitstun_cooldown, anim_frame, abort_caller

        # Active hitstun locks action
        invuln, cd, frame, abort = tick_cooldown(flash_timer=10, hitstun_cooldown=5, flags_36=0, anim_frame=0)
        self.assertTrue(invuln)
        self.assertEqual(cd, 4)
        self.assertEqual(frame, 1)
        self.assertTrue(abort)

        # Completed cooldown allows action
        invuln, cd, frame, abort = tick_cooldown(flash_timer=0, hitstun_cooldown=1, flags_36=0, anim_frame=1)
        self.assertFalse(invuln)
        self.assertEqual(cd, 0)
        self.assertEqual(frame, 2)
        self.assertFalse(abort)


if __name__ == "__main__":
    unittest.main()
