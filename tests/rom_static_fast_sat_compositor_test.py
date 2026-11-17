#!/usr/bin/env python3
"""
Unit tests for M15.416: ROM Static Entity & Shadowless Fast SAT Sprite Compositors
(0x00AD76..0x00B15E).
"""

import os
import struct
import unittest

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")

class TestRomStaticFastSatCompositor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_static_compositor_bounds_and_opcodes(self):
        # Entry point 0x00AD76: btst.b #1, $36(a6)
        w0 = struct.unpack_from(">H", self.rom, 0x00AD76)[0]
        w1 = struct.unpack_from(">H", self.rom, 0x00AD78)[0]
        w2 = struct.unpack_from(">H", self.rom, 0x00AD7A)[0]
        self.assertEqual(w0, 0x082E)
        self.assertEqual(w1, 0x0001)
        self.assertEqual(w2, 0x0036)

        # Early exit: bne.w $b010
        bne_op = struct.unpack_from(">H", self.rom, 0x00AD7C)[0]
        disp = struct.unpack_from(">h", self.rom, 0x00AD7E)[0]
        self.assertEqual(bne_op, 0x6600)
        self.assertEqual(0x00AD7E + disp, 0x00B010)

        # RTS at 0x00B010
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B010)[0], 0x4E75)

    def test_dynamic_tails(self):
        # 0x00B012: bsr.w $b34c; bra.w $aee6
        bsr1 = struct.unpack_from(">H", self.rom, 0x00B012)[0]
        disp1 = struct.unpack_from(">h", self.rom, 0x00B014)[0]
        self.assertEqual(bsr1, 0x6100)
        self.assertEqual(0x00B014 + disp1, 0x00B34C)

        bra1 = struct.unpack_from(">H", self.rom, 0x00B016)[0]
        disp_bra1 = struct.unpack_from(">h", self.rom, 0x00B018)[0]
        self.assertEqual(bra1, 0x6000)
        self.assertEqual(0x00B018 + disp_bra1, 0x00AEE6)

    def test_fast_compositor_bounds_and_opcodes(self):
        # Entry point 0x00B022: moveq #0, d0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B022)[0], 0x7000)

        # Buffer base subtract at 0x00B14A: suba.l #$ff13cc, a1
        suba_op = struct.unpack_from(">H", self.rom, 0x00B14A)[0]
        base_val = struct.unpack_from(">I", self.rom, 0x00B14C)[0]
        self.assertEqual(suba_op, 0x93FC)
        self.assertEqual(base_val, 0x00FF13CC)

        # RTS at 0x00B15C
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B15C)[0], 0x4E75)

    def test_dispatch_mode_selection(self):
        # In SAT passes, btst.b #0, $36(a6) determines static (0) vs dynamic (1)
        def select_sat_compositor(flags_36):
            if (flags_36 & 0x01) == 0:
                return 0x00AD76  # Static compositor
            return 0x00A9A0      # Dynamic DMA compositor

        self.assertEqual(select_sat_compositor(0x00), 0x00AD76)
        self.assertEqual(select_sat_compositor(0x02), 0x00AD76)
        self.assertEqual(select_sat_compositor(0x01), 0x00A9A0)
        self.assertEqual(select_sat_compositor(0x07), 0x00A9A0)

    def test_viewport_clipping(self):
        # Y clipping: rel = (y - 0x60) & 0xFFFF > 0x100
        # X clipping: rel = (x - 0x60) & 0xFFFF > 0x160
        def clip_check(y, x):
            y_rel = (y - 0x60) & 0xFFFF
            x_rel = (x - 0x60) & 0xFFFF
            return (y_rel > 0x100) or (x_rel > 0x160)

        # Valid center coordinates
        self.assertFalse(clip_check(0x100, 0x100))
        # Top-left boundary
        self.assertFalse(clip_check(0x60, 0x60))
        # Bottom-right boundary
        self.assertFalse(clip_check(0x160, 0x1C0))
        # Off-screen top
        self.assertTrue(clip_check(0x5F, 0x100))
        # Off-screen bottom
        self.assertTrue(clip_check(0x161, 0x100))
        # Off-screen left
        self.assertTrue(clip_check(0x100, 0x5F))
        # Off-screen right
        self.assertTrue(clip_check(0x100, 0x1C1))

if __name__ == "__main__":
    unittest.main()
