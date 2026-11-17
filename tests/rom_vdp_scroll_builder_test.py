#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM VDP Dual-Plane H/V Scroll Table Builder,
Scanline/Strip Mode Dispatcher & V-Int DMA Sync Trigger (0x00D2B4..0x00D3B0, 252 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00D2B4
END_ADDR = 0x00D3B2
ROUTINE_SIZE = END_ADDR - START_ADDR  # 254 bytes


class VdpScrollState:
    """Emulates ROM VDP scroll table generator and V-Int DMA sync logic."""

    def __init__(self, is_plane_a=True):
        self.is_plane_a = is_plane_a

        # Camera struct registers
        self.hscroll_value = 0x0123  # +0x0E(a0)
        self.vscroll_value = 0x0045  # +0x10(a0)
        self.dirty_flags = 0x0C      # +0x26(a0): bits 2 and 3 dirty

        # Hardware & VDP registers
        self.scroll_mode_16f0 = 0
        self.vint_sync_164e = 0

        # Destination buffers in RAM
        self.fullscreen_hscroll_plane_a_1344 = 0
        self.fullscreen_hscroll_plane_b_1346 = 0
        self.fullscreen_vscroll_plane_a_1348 = 0
        self.fullscreen_vscroll_plane_b_134a = 0

        # Table buffers in RAM
        self.hscroll_table_ram = [0 for _ in range(512)]  # words at 0xFFFB5C
        self.vscroll_table_ram = [0 for _ in range(40)]   # words at 0xFFFF5C

    def update_vscroll_table(self):
        """Emulates 0x00D344..0x00D396."""
        if (self.dirty_flags & 0x08) == 0:
            return  # bit 3 clean

        self.dirty_flags &= ~0x08
        self.scroll_mode_16f0 |= 0x10  # set bit 4 (vscroll pending)
        d0 = self.vscroll_value

        if (self.scroll_mode_16f0 & 0x04) != 0:
            # 2-cell column mode (bit 2 set)
            # 20 iterations (stride 4 bytes = 2 words)
            offset = 0 if self.is_plane_a else 1
            for i in range(20):
                self.vscroll_table_ram[offset + i * 2] = d0
        else:
            # Fullscreen mode
            if self.is_plane_a:
                self.fullscreen_vscroll_plane_a_1348 = d0
            else:
                self.fullscreen_vscroll_plane_b_134a = d0

    def update_hscroll_table(self):
        """Emulates 0x00D2B4..0x00D342."""
        # Calls vertical scroll update first
        self.update_vscroll_table()

        if (self.dirty_flags & 0x04) == 0:
            return  # bit 2 clean

        self.dirty_flags &= ~0x04
        self.scroll_mode_16f0 |= 0x08  # set bit 3 (hscroll pending)
        d0 = self.hscroll_value

        if (self.scroll_mode_16f0 & 0x02) != 0:
            # Line or Strip scroll mode
            offset = 0 if self.is_plane_a else 1
            if (self.scroll_mode_16f0 & 0x01) != 0:
                # 1-line scanline mode: 58 loops * 4 words = 232 words
                idx = offset
                for _ in range(58):
                    for _ in range(4):
                        self.hscroll_table_ram[idx] = d0
                        idx += 2
            else:
                # 16-pixel tile strip mode: 29 entries, stride 32 bytes (16 words)
                idx = offset
                for _ in range(29):
                    self.hscroll_table_ram[idx] = d0
                    idx += 16
        else:
            # Fullscreen mode
            if self.is_plane_a:
                self.fullscreen_hscroll_plane_a_1344 = d0
            else:
                self.fullscreen_hscroll_plane_b_1346 = d0

    def commit_scroll_dma_flags(self):
        """Emulates 0x00D398..0x00D3B0."""
        pending = self.scroll_mode_16f0 & 0x18  # bits 3 and 4
        self.scroll_mode_16f0 &= ~0x18
        self.vint_sync_164e |= pending


class TestRomVdpScrollBuilder(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(ROM_PATH):
            raise unittest.SkipTest(f"ROM not found at {ROM_PATH}")
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()
        cls.routine_bytes = cls.rom[START_ADDR:END_ADDR]

    def test_01_rom_slice_and_header(self):
        """Verify byte span and key instruction opcodes."""
        self.assertEqual(len(self.routine_bytes), ROUTINE_SIZE)

        # 0x00D2B4: bsr.w $D344 -> 61 00 00 8E
        self.assertEqual(self.routine_bytes[0:4], bytes([0x61, 0x00, 0x00, 0x8E]))

        # 0x00D344: bclr.b #$3, $26(a0) -> 08 A8 00 03 00 26
        off_vscroll = 0x00D344 - START_ADDR
        self.assertEqual(self.routine_bytes[off_vscroll:off_vscroll + 6], bytes([0x08, 0xA8, 0x00, 0x03, 0x00, 0x26]))

        # 0x00D398: move.b $FF16F0.l, d0 -> 10 39 00 FF 16 F0
        off_commit = 0x00D398 - START_ADDR
        self.assertEqual(self.routine_bytes[off_commit:off_commit + 6], bytes([0x10, 0x39, 0x00, 0xFF, 0x16, 0xF0]))

        # 0x00D3B0: rts -> 4E 75
        off_rts = 0x00D3B0 - START_ADDR
        self.assertEqual(self.routine_bytes[off_rts:off_rts + 2], bytes([0x4E, 0x75]))

    def test_02_fullscreen_scroll_modes(self):
        """Verify fullscreen H/V scroll writes to 1344/1348 (Plane A) and 1346/134A (Plane B)."""
        # Plane A
        state_a = VdpScrollState(is_plane_a=True)
        state_a.scroll_mode_16f0 = 0  # fullscreen mode
        state_a.hscroll_value = 0x0111
        state_a.vscroll_value = 0x0222
        state_a.update_hscroll_table()

        self.assertEqual(state_a.fullscreen_hscroll_plane_a_1344, 0x0111)
        self.assertEqual(state_a.fullscreen_vscroll_plane_a_1348, 0x0222)
        self.assertEqual(state_a.scroll_mode_16f0 & 0x18, 0x18)  # bits 3 and 4 pending
        self.assertEqual(state_a.dirty_flags, 0)  # bits 2 and 3 cleared

        # Plane B
        state_b = VdpScrollState(is_plane_a=False)
        state_b.scroll_mode_16f0 = 0
        state_b.hscroll_value = 0x0333
        state_b.vscroll_value = 0x0444
        state_b.update_hscroll_table()

        self.assertEqual(state_b.fullscreen_hscroll_plane_b_1346, 0x0333)
        self.assertEqual(state_b.fullscreen_vscroll_plane_b_134a, 0x0444)

    def test_03_tile_strip_16px_hscroll(self):
        """Verify 16-pixel tile strip mode writes 29 entries at stride 32 bytes."""
        state = VdpScrollState(is_plane_a=True)
        state.scroll_mode_16f0 = 0x02  # bit 1 set, bit 0 clear
        state.hscroll_value = 0x0555
        state.update_hscroll_table()

        # Should write 29 entries at indices 0, 16, 32, ...
        for i in range(29):
            self.assertEqual(state.hscroll_table_ram[i * 16], 0x0555)

    def test_04_scanline_1line_hscroll(self):
        """Verify 1-line scanline mode writes 232 words at stride 2 words."""
        state = VdpScrollState(is_plane_a=True)
        state.scroll_mode_16f0 = 0x03  # bit 1 set, bit 0 set
        state.hscroll_value = 0x0777
        state.update_hscroll_table()

        # Should write 232 entries (58 * 4)
        for i in range(232):
            self.assertEqual(state.hscroll_table_ram[i * 2], 0x0777)

    def test_05_column_2cell_vscroll_and_commit(self):
        """Verify 2-cell column V-scroll and commit to V-Int sync register."""
        state = VdpScrollState(is_plane_a=True)
        state.scroll_mode_16f0 = 0x04  # bit 2 set
        state.vscroll_value = 0x0888
        state.update_vscroll_table()

        # 20 columns
        for i in range(20):
            self.assertEqual(state.vscroll_table_ram[i * 2], 0x0888)

        # Commit flags
        state.commit_scroll_dma_flags()
        self.assertEqual(state.scroll_mode_16f0 & 0x18, 0)
        self.assertEqual(state.vint_sync_164e & 0x10, 0x10)  # V-Int bit 4 set


if __name__ == "__main__":
    unittest.main()
