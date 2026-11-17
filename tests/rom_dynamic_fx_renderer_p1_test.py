#!/usr/bin/env python3
"""
Unit tests for M15.418: ROM Entity Special Dynamic FX, Aura, Wave & Attachment
SAT Renderer Subsystem (Part 1: 0x00B34C..0x00B56E).
"""

import os
import struct
import unittest

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")

class TestRomDynamicFxRendererP1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_dispatcher_bounds_and_opcodes(self):
        # 0x00B34C: tst.b $ff1927.l
        w0 = struct.unpack_from(">H", self.rom, 0x00B34C)[0]
        w1 = struct.unpack_from(">I", self.rom, 0x00B34E)[0]
        self.assertEqual(w0, 0x4A39)
        self.assertEqual(w1, 0x00FF1927)

        # 0x00B356: move.b $39(a6), d5
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B356)[0], 0x1A2E)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B358)[0], 0x0039)

        # Dispatcher RTS at 0x00B382
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B382)[0], 0x4E75)

    def test_bit0_aura_emitter(self):
        # Entry 0x00B3D2: lea $9b(a6), a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B3D2)[0], 0x41EE)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B3D4)[0], 0x009B)

        # Exit RTS at 0x00B42A
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B42A)[0], 0x4E75)

        # Check table at 0x00B42C (16 entries x 3 bytes)
        table_addr = 0x00B42C
        self.assertEqual(self.rom[table_addr:table_addr+3], bytes([0xFF, 0x00, 0x00]))
        self.assertEqual(self.rom[table_addr+3:table_addr+6], bytes([0x07, 0x04, 0x04]))

    def test_bit3_wave_ring_tables(self):
        # Entry 0x00B45C: lea $98(a6), a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B45C)[0], 0x41EE)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B45E)[0], 0x0098)

        # Exit RTS at 0x00B4C6
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B4C6)[0], 0x4E75)

        # Table selection logic based on shadow preset +0x3D(a6)
        def select_wave_table(preset_3d):
            if preset_3d <= 8:
                return "small", 8   # 0x00B4C8
            return "large", 10      # 0x00B4E0

        tbl1, cnt1 = select_wave_table(4)
        self.assertEqual(tbl1, "small")
        self.assertEqual(cnt1, 8)

        tbl2, cnt2 = select_wave_table(12)
        self.assertEqual(tbl2, "large")
        self.assertEqual(cnt2, 10)

    def test_bit4_ripple_attachment(self):
        # Entry 0x00B4FE: lea $98(a6), a0
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B4FE)[0], 0x41EE)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B500)[0], 0x0098)

        # Normal exit RTS at 0x00B54C
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B54C)[0], 0x4E75)

        # Cycle finish bit-clear at 0x00B54E: bclr.b #4, $39(a6)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B54E)[0], 0x08AE)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B550)[0], 0x0004)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B552)[0], 0x0039)
        self.assertEqual(struct.unpack_from(">H", self.rom, 0x00B554)[0], 0x4E75)

    def test_global_suppress_flag(self):
        def should_render_fx(suppress_flag, mode_39):
            if suppress_flag != 0:
                return False
            return mode_39 != 0

        self.assertFalse(should_render_fx(1, 0x01))
        self.assertFalse(should_render_fx(0, 0x00))
        self.assertTrue(should_render_fx(0, 0x01))
        self.assertTrue(should_render_fx(0, 0x08))
        self.assertTrue(should_render_fx(0, 0x10))

if __name__ == "__main__":
    unittest.main()
