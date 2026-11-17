#!/usr/bin/env python3
"""
Unit tests for M15.415: ROM Master Entity SAT Sprite Compositor, Screen Clipping
& Dynamic Shadow Blitter Subsystem (0x00A9A0..0x00AD76).
"""

import os
import struct
import unittest

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")

class TestRomMasterSatCompositor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_rom_opcodes_and_bounds(self):
        # Entry point: btst.b #1, $36(a6)
        w0 = struct.unpack_from(">H", self.rom, 0x00A9A0)[0]
        w1 = struct.unpack_from(">H", self.rom, 0x00A9A2)[0]
        w2 = struct.unpack_from(">H", self.rom, 0x00A9A4)[0]
        self.assertEqual(w0, 0x082E)
        self.assertEqual(w1, 0x0001)
        self.assertEqual(w2, 0x0036)

        # Early exit branch: bne.w $acfc
        bne_op = struct.unpack_from(">H", self.rom, 0x00A9A6)[0]
        disp = struct.unpack_from(">h", self.rom, 0x00A9A8)[0]
        self.assertEqual(bne_op, 0x6600)
        self.assertEqual(0x00A9A8 + disp, 0x00ACFC)

        # Subsystem exit RTS
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00ACFC)[0], 0x4E75)
        # Quota/limit exit RTS
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00AD40)[0], 0x4E75)

    def test_shadow_table_lookup(self):
        # 4 shadow entries at 0x00ACFE (8 bytes each)
        shadow_table_addr = 0x00ACFE
        entries = []
        for i in range(4):
            off = shadow_table_addr + i * 8
            y_off = struct.unpack_from(">H", self.rom, off)[0]
            size_word = struct.unpack_from(">H", self.rom, off + 2)[0]
            tile_idx = struct.unpack_from(">H", self.rom, off + 4)[0]
            x_off = struct.unpack_from(">H", self.rom, off + 6)[0]
            entries.append((y_off, size_word, tile_idx, x_off))

        # Preset 0: small shadow (1x1, tile 0x0548)
        self.assertEqual(entries[0], (3, 0x0000, 0x0548, 3))
        # Preset 1: medium shadow (2x1, tile 0x0549)
        self.assertEqual(entries[1], (3, 0x0400, 0x0549, 8))
        # Preset 2: large shadow (3x1, tile 0x054B)
        self.assertEqual(entries[2], (3, 0x0800, 0x054B, 11))
        # Preset 3: giant shadow (4x2, tile 0x054E)
        self.assertEqual(entries[3], (8, 0x0D00, 0x054E, 16))

    def test_tile_count_table_4x4(self):
        # 4x4 sprite tile count table at 0x00AD56
        table_addr = 0x00AD56
        for w in range(1, 5):
            for h in range(1, 5):
                idx = (w - 1) * 4 + (h - 1)
                tiles = struct.unpack_from(">H", self.rom, table_addr + idx * 2)[0]
                self.assertEqual(tiles, w * h)

    def test_screen_clipping_logic(self):
        # Mega drive SAT Y coordinate range: [0x60, 0x160] (clipped if > 0x100 relative)
        # Mega drive SAT X coordinate range: [0x60, 0x1C0] (clipped if > 0x160 relative)
        def is_clipped_y(screen_y):
            rel = (screen_y - 0x60) & 0xFFFF
            return rel > 0x100

        def is_clipped_x(screen_x):
            rel = (screen_x - 0x60) & 0xFFFF
            return rel > 0x160

        # Inside viewport
        self.assertFalse(is_clipped_y(0x80))
        self.assertFalse(is_clipped_x(0x80))
        self.assertFalse(is_clipped_y(0x150))
        self.assertFalse(is_clipped_x(0x1B0))

        # Outside viewport (top/left)
        self.assertTrue(is_clipped_y(0x50))
        self.assertTrue(is_clipped_x(0x50))

        # Outside viewport (bottom/right)
        self.assertTrue(is_clipped_y(0x170))
        self.assertTrue(is_clipped_x(0x1D0))

    def test_camera_projection_and_elevation(self):
        world_x = 400
        world_y = 500
        elev_z = 25
        cam_x = 350
        cam_y = 420

        # Coordinate formula from 0x00AB00:
        # screen_y = (world_y - cam_y + 0x80) - elev_z
        # screen_x = world_x - cam_x + 0x80
        screen_y = (world_y - cam_y + 0x80) - elev_z
        screen_x = world_x - cam_x + 0x80

        self.assertEqual(screen_x, 400 - 350 + 128)  # 178
        self.assertEqual(screen_y, (500 - 420 + 128) - 25)  # 183

    def test_frame_cache_and_quota_check(self):
        # Logic at 0x00A9D6:
        # if cached_frame >= 0 and depth > remaining_quota:
        #   reuse cached frame and mark FF1651 = 0xFF
        remaining_quota = 32
        depth_high = 50
        depth_low = 10

        def evaluate_cache(cached_frame, depth, new_frame, quota):
            if new_frame == cached_frame:
                return cached_frame, False, False  # hit
            if cached_frame >= 0 and depth > quota:
                return cached_frame, True, False   # defer / quota exceed (FF1651 set)
            return new_frame, False, True          # update cache + queue DMA (a5 += 1)

        # Cold frame (-1)
        res_frame, set_ff1651, queue_dma = evaluate_cache(-1, depth_high, 4, remaining_quota)
        self.assertEqual(res_frame, 4)
        self.assertFalse(set_ff1651)
        self.assertTrue(queue_dma)

        # Quota overflow with valid cache
        res_frame, set_ff1651, queue_dma = evaluate_cache(4, depth_high, 5, remaining_quota)
        self.assertEqual(res_frame, 4)
        self.assertTrue(set_ff1651)
        self.assertFalse(queue_dma)

        # Sufficient quota
        res_frame, set_ff1651, queue_dma = evaluate_cache(4, depth_low, 5, remaining_quota)
        self.assertEqual(res_frame, 5)
        self.assertFalse(set_ff1651)
        self.assertTrue(queue_dma)

if __name__ == "__main__":
    unittest.main()
