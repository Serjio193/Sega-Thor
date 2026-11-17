#!/usr/bin/env python3
"""
Unit tests for M15.421: Weapon Attack Frame 3D Hitbox Streamer &
Dual-Pool Scanner Engine (0x00BAAC..0x00BC8E).
"""

import os
import struct
import unittest

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")

class TestRomWeaponHitboxStreamer(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_entrypoints_and_opcodes(self):
        # 0x00BAAC: lea $ff19e8.l, a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00BAAC)[0], 0x41F9)
        self.assertEqual(struct.unpack_from(">I", self.rom, 0x00BAAE)[0], 0x00FF19E8)

        # 0x00BAB8: lea $ff1cd8.l, a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00BAB8)[0], 0x41F9)
        self.assertEqual(struct.unpack_from(">I", self.rom, 0x00BABA)[0], 0x00FF1CD8)

        # 0x00BBA6: rts
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00BBA6)[0], 0x4E75)
        # 0x00BC8C: rts
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00BC8C)[0], 0x4E75)

    def test_hitbox_stream_unpacking(self):
        # Mock 12-byte stream entry: z_min=0, z_max=24, x_min=-10, y_min=-8, x_max=25, y_max=12
        raw_stream = struct.pack(">6h", 0, 24, -10, -8, 25, 12)
        z_min, z_max, x_min, y_min, x_max, y_max = struct.unpack(">6h", raw_stream)
        self.assertEqual((z_min, z_max), (0, 24))
        self.assertEqual((x_min, y_min, x_max, y_max), (-10, -8, 25, 12))

    def test_horizontal_mirroring_logic(self):
        def mirror_hitbox(x_min, x_max):
            n_min = -x_min
            n_max = -x_max
            return n_max, n_min  # exg

        # Facing right: [0, 24] -> facing left: [-24, 0]
        self.assertEqual(mirror_hitbox(0, 24), (-24, 0))
        # Facing right: [-10, 25] -> facing left: [-25, 10]
        self.assertEqual(mirror_hitbox(-10, 25), (-25, 10))

    def test_world_space_projection(self):
        world_x = 300
        world_y = 400
        elev_z = 15
        ground_h = 10

        z_min, z_max = 0, 20
        x_min, x_max = -8, 24
        y_min, y_max = -6, 12

        # 3D projection:
        # z += elev_z + ground_h
        # x += world_x
        # y += world_y + ground_h
        proj_z_min = z_min + elev_z + ground_h
        proj_z_max = z_max + elev_z + ground_h
        proj_x_min = x_min + world_x
        proj_x_max = x_max + world_x
        proj_y_min = y_min + world_y + ground_h
        proj_y_max = y_max + world_y + ground_h

        self.assertEqual((proj_z_min, proj_z_max), (25, 45))
        self.assertEqual((proj_x_min, proj_x_max), (292, 324))
        self.assertEqual((proj_y_min, proj_y_max), (404, 422))

    def test_target_filter_checks(self):
        def is_valid_target(slot_type, target_addr, self_addr, flags_38, flags_3a):
            if slot_type <= 0:
                return False
            if target_addr == self_addr:
                return False
            if (flags_38 & 0x08) != 0:
                return False
            if (flags_3a & 0x01) != 0:
                return False
            return True

        self.assertTrue(is_valid_target(5, 0x2000, 0x1000, 0x00, 0x00))
        self.assertFalse(is_valid_target(0, 0x2000, 0x1000, 0x00, 0x00))
        self.assertFalse(is_valid_target(5, 0x1000, 0x1000, 0x00, 0x00))
        self.assertFalse(is_valid_target(5, 0x2000, 0x1000, 0x08, 0x00))
        self.assertFalse(is_valid_target(5, 0x2000, 0x1000, 0x00, 0x01))

    def test_3d_aabb_hit_evaluation(self):
        def test_hit(box, target):
            z_hit = (box["z_max"] >= target["z_min"]) and (box["z_min"] <= target["z_max"])
            x_hit = (box["x_max"] >= target["x_min"]) and (box["x_min"] <= target["x_max"])
            y_hit = (box["y_max"] >= target["y_min"]) and (box["y_min"] <= target["y_max"])
            return z_hit and x_hit and y_hit

        box = {"x_min": 280, "x_max": 320, "y_min": 390, "y_max": 420, "z_min": 20, "z_max": 40}
        tgt_hit = {"x_min": 300, "x_max": 330, "y_min": 400, "y_max": 430, "z_min": 25, "z_max": 45}
        tgt_miss = {"x_min": 330, "x_max": 360, "y_min": 400, "y_max": 430, "z_min": 25, "z_max": 45}

        self.assertTrue(test_hit(box, tgt_hit))
        self.assertFalse(test_hit(box, tgt_miss))

if __name__ == "__main__":
    unittest.main()
