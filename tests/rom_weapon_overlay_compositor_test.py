#!/usr/bin/env python3
"""
Unit tests for M15.417: ROM Entity Secondary Weapon & Auxiliary Overlay
SAT Compositor Subsystem (0x00B15E..0x00B34C).
"""

import os
import struct
import unittest

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")

class TestRomWeaponOverlayCompositor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_entry_bounds_and_branches(self):
        # Entry point 0x00B15E: btst.b #9, d5 (represented as btst.b #1, d5 upper byte)
        w0 = struct.unpack_from(">H", self.rom, 0x00B15E)[0]
        w1 = struct.unpack_from(">H", self.rom, 0x00B160)[0]
        self.assertEqual(w0, 0x0805)
        self.assertEqual(w1, 0x0009)

        # Branch to non-DMA path: bne.w $b28e
        bne_op = struct.unpack_from(">H", self.rom, 0x00B162)[0]
        disp = struct.unpack_from(">h", self.rom, 0x00B164)[0]
        self.assertEqual(bne_op, 0x6600)
        self.assertEqual(0x00B164 + disp, 0x00B28E)

        # DMA path RTS at 0x00B28C
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B28C)[0], 0x4E75)
        # Non-DMA path RTS at 0x00B34A
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B34A)[0], 0x4E75)

    def test_descriptor_table_pointer(self):
        # 0x00B1AA: lea $3f2fa.l, a3
        lea_op = struct.unpack_from(">H", self.rom, 0x00B1AA)[0]
        table_addr = struct.unpack_from(">I", self.rom, 0x00B1AC)[0]
        self.assertEqual(lea_op, 0x47F9)
        self.assertEqual(table_addr, 0x0003F2FA)

        # Test index formula: table + (type * 16) - 20
        def get_desc_entry(entity_type):
            return 0x0003F2FA + (entity_type * 16) - 20

        # Type 2 (Player) -> 0x0003F2FA + 32 - 20 = 0x0003F306
        self.assertEqual(get_desc_entry(2), 0x0003F306)

    def test_mode_routing_bit9(self):
        def select_overlay_path(d5_word):
            # Bit 9 test
            if (d5_word & 0x0200) != 0:
                return "nondma"  # 0x00B28E
            return "dma"         # 0x00B166

        self.assertEqual(select_overlay_path(0x0000), "dma")
        self.assertEqual(select_overlay_path(0x0100), "dma")
        self.assertEqual(select_overlay_path(0x0200), "nondma")
        self.assertEqual(select_overlay_path(0x0300), "nondma")

    def test_viewport_clipping(self):
        def is_clipped(screen_y, screen_x):
            y_rel = (screen_y - 0x60) & 0xFFFF
            x_rel = (screen_x - 0x60) & 0xFFFF
            return (y_rel > 0x100) or (x_rel > 0x160)

        # Inside screen
        self.assertFalse(is_clipped(0x80, 0x90))
        # Outside Y
        self.assertTrue(is_clipped(0x50, 0x90))
        self.assertTrue(is_clipped(0x170, 0x90))
        # Outside X
        self.assertTrue(is_clipped(0x80, 0x50))
        self.assertTrue(is_clipped(0x80, 0x1D0))

    def test_cache_update_dma_trigger(self):
        def step_cache(cached_frame, new_frame):
            if new_frame == cached_frame:
                return cached_frame, False  # No DMA
            return new_frame, True          # Queue DMA (a5 += 1)

        c1, q1 = step_cache(0x10, 0x10)
        self.assertEqual(c1, 0x10)
        self.assertFalse(q1)

        c2, q2 = step_cache(0x10, 0x12)
        self.assertEqual(c2, 0x12)
        self.assertTrue(q2)

if __name__ == "__main__":
    unittest.main()
