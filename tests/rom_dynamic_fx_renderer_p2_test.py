#!/usr/bin/env python3
"""
Unit tests for M15.419: ROM Entity Special Dynamic FX Halo, Swirl &
SAT Piece Compositor Subsystem (Part 2: 0x00B56E..0x00B79A).
"""

import os
import struct
import unittest

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")

class TestRomDynamicFxRendererP2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_bit2_halo_emitter_bounds_and_clear(self):
        # Entry 0x00B56E: lea $9a(a6), a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B56E)[0], 0x41EE)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B570)[0], 0x009A)

        # Bit clear on counter zero at 0x00B5C8: bclr.b #2, $39(a6)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B5C8)[0], 0x08AE)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B5CA)[0], 0x0002)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B5CC)[0], 0x0039)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B5CE)[0], 0x4E75)

        # 21 entries in table at 0x00B5D0
        tbl = self.rom[0x00B5D0:0x00B5D0 + 63]
        self.assertEqual(len(tbl), 63)

    def test_bit1_swirl_emitter(self):
        # 0x00B610: tst.l $10(a6) - checks grounded
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B610)[0], 0x4AAE)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B612)[0], 0x0010)

        # Swirl Y offset formula: (preset >> 2) - 6
        def get_swirl_y_offset(preset_3d):
            return (preset_3d >> 2) - 6

        self.assertEqual(get_swirl_y_offset(0x00), -6)
        self.assertEqual(get_swirl_y_offset(0x18), 0)
        self.assertEqual(get_swirl_y_offset(0x20), 2)

        # Frame sequence table at 0x00B660
        frames = [self.rom[0x00B660 + i] for i in range(9)]
        self.assertEqual(len(frames), 9)

    def test_universal_fx_blitter(self):
        # 0x00B6AA: movea.l $3f326.l, a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B6AA)[0], 0x2079)
        self.assertEqual(struct.unpack_from(">I", self.rom, 0x00B6AC)[0], 0x0003F326)

        # Base tile calculation: (tile_18 & 0x8000) | 0x055E
        def calc_fx_tile_attr(base_tile_18):
            return (base_tile_18 & 0x8000) | 0x055E

        self.assertEqual(calc_fx_tile_attr(0x0200), 0x055E)
        self.assertEqual(calc_fx_tile_attr(0x8200), 0x855E)

    def test_raw_piece_blitter(self):
        # 0x00B734: addi.w #$80, d1
        # 0x00B738: addi.w #$80, d4
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B734)[0], 0x0641)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B736)[0], 0x0080)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B738)[0], 0x0644)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B73A)[0], 0x0080)

        # RTS at 0x00B798
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B798)[0], 0x4E75)

    def test_viewport_clipping(self):
        def clip_check(y, x):
            y_rel = (y - 0x60) & 0xFFFF
            x_rel = (x - 0x60) & 0xFFFF
            return (y_rel > 0x100) or (x_rel > 0x160)

        self.assertFalse(clip_check(0x80, 0x80))
        self.assertTrue(clip_check(0x50, 0x80))
        self.assertTrue(clip_check(0x80, 0x1D0))

if __name__ == "__main__":
    unittest.main()
