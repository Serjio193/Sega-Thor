#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Dialogue Window 4-Line Circular Scroll Engine,
VDP Direct Clearer & Glyph DMA Streamer (0x00C566..0x00C756, 496 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C566
END_ADDR = 0x00C756
ROUTINE_SIZE = END_ADDR - START_ADDR  # 496 bytes


class ScrollDMAState:
    """Emulates scroll and DMA queuing state."""
    def __init__(self, top_base=0x2000, bottom_base=0x3000):
        self.plane_top_base = top_base
        self.plane_bottom_base = bottom_base
        self.row_offsets = [12, 24, 36, 48]
        self.cursor_x = 0
        self.line_counter = 0
        self.dma_queue = []
        self.blank_clears = []

    def advance_line(self):
        """Emulates 0x00C566."""
        self.cursor_x = 0
        self.line_counter += 1
        if self.line_counter <= 3:
            return False  # No scroll

        # Scroll: decrement all offsets by 2 until one reaches 0
        scrolled = False
        while True:
            for i in range(4):
                self.row_offsets[i] -= 2

            # Check if any reached 0
            if self.row_offsets[0] == 0:
                self.blank_clears.append(self.plane_top_base)
                self.row_offsets[0] = 0x30
                break
            elif self.row_offsets[1] == 0:
                self.blank_clears.append(self.plane_top_base + 0x400)
                self.row_offsets[1] = 0x30
                break
            elif self.row_offsets[2] == 0:
                self.blank_clears.append(self.plane_bottom_base)
                self.row_offsets[2] = 0x30
                break
            elif self.row_offsets[3] == 0:
                self.blank_clears.append(self.plane_bottom_base + 0x400)
                self.row_offsets[3] = 0x30
                break
        return True

    def queue_glyph_dma(self, char_code):
        """Emulates 0x00C6BE."""
        glyph_idx = char_code - 0x20
        src_byte_addr = 0xFF2FA8 + glyph_idx * 32
        src_word_addr = src_byte_addr >> 1

        active_line = self.line_counter & 3
        if active_line == 0:
            dest_base = self.plane_top_base
        elif active_line == 1:
            dest_base = self.plane_top_base + 0x400
        elif active_line == 2:
            dest_base = self.plane_bottom_base
        else:
            dest_base = self.plane_bottom_base + 0x400

        dest_vram = dest_base + (self.cursor_x << 5)
        entry = {
            "src_word": src_word_addr,
            "dest_vram": dest_vram,
            "word_count": 0x10,  # 16 words = 32 bytes
        }
        self.dma_queue.append(entry)
        self.cursor_x += 1
        return entry

    def queue_blank_dma(self, dest_vram):
        """Emulates 0x00C734."""
        entry = {
            "src_addr": 0x007F9DD4,
            "dest_vram": dest_vram,
            "word_count": 0x200,  # 512 words = 1024 bytes
        }
        self.dma_queue.append(entry)
        return entry


class TestRomDialogueScrollDMA(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(ROM_PATH):
            raise unittest.SkipTest(f"ROM not found at {ROM_PATH}")
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_01_binary_bounds_and_entrypoints(self):
        """Verify binary size, opcodes, and entrypoint signatures."""
        self.assertGreaterEqual(len(self.rom), END_ADDR)
        code = self.rom[START_ADDR:END_ADDR]
        self.assertEqual(len(code), ROUTINE_SIZE)

        # 0x00C566: clr.w $FF187E.l
        self.assertEqual(code[0:6], bytes([0x42, 0x79, 0x00, 0xFF, 0x18, 0x7E]))
        # 0x00C580: rts (fast exit when line_idx <= 3)
        self.assertEqual(code[0x1A:0x1C], bytes([0x4E, 0x75]))
        # 0x00C668: rts (end of scroll helper)
        self.assertEqual(code[0x0102:0x0104], bytes([0x4E, 0x75]))
        # 0x00C66A: cmpi.w #8, d2 (start of VDP line clearer)
        self.assertEqual(code[0x0104:0x0108], bytes([0x0C, 0x42, 0x00, 0x08]))
        # 0x00C6BC: rts (end of VDP line clearer)
        self.assertEqual(code[0x0156:0x0158], bytes([0x4E, 0x75]))
        # 0x00C6BE: movem.l a0, -(a7) (start of glyph DMA queue)
        self.assertEqual(code[0x0158:0x015C], bytes([0x48, 0xE7, 0x00, 0x80]))
        # 0x00C732: rts (end of glyph DMA queue)
        self.assertEqual(code[0x01CC:0x01CE], bytes([0x4E, 0x75]))
        # 0x00C734: ori.w #$700, sr (start of blank DMA queue helper)
        self.assertEqual(code[0x01CE:0x01D2], bytes([0x00, 0x7C, 0x07, 0x00]))
        # 0x00C754: rts (end of blank DMA queue helper)
        self.assertEqual(code[0x01EE:0x01F0], bytes([0x4E, 0x75]))

    def test_02_line_advance_without_scrolling(self):
        """Verify lines 1, 2, 3 advance without triggering window scrolling."""
        state = ScrollDMAState()
        # Line 0 -> 1
        scrolled = state.advance_line()
        self.assertFalse(scrolled)
        self.assertEqual(state.line_counter, 1)
        self.assertEqual(state.row_offsets, [12, 24, 36, 48])

        # Line 1 -> 2
        scrolled = state.advance_line()
        self.assertFalse(scrolled)
        self.assertEqual(state.line_counter, 2)

        # Line 2 -> 3
        scrolled = state.advance_line()
        self.assertFalse(scrolled)
        self.assertEqual(state.line_counter, 3)
        self.assertEqual(len(state.blank_clears), 0)

    def test_03_scroll_and_circular_buffer_recycling(self):
        """Verify line 4 triggers scrolling and row offset recycling to 0x30."""
        state = ScrollDMAState(top_base=0x2000, bottom_base=0x3000)
        # Advance to line 3
        for _ in range(3):
            state.advance_line()

        # Advance to line 4 (triggers scroll)
        scrolled = state.advance_line()
        self.assertTrue(scrolled)
        self.assertEqual(state.line_counter, 4)
        # Row 1 (was 12) decremented 6 times by 2 -> reached 0, recycled to 0x30 (48)!
        self.assertEqual(state.row_offsets[0], 0x30)
        self.assertEqual(state.blank_clears, [0x2000])

    def test_04_glyph_dma_queuing(self):
        """Verify glyph DMA queue entry construction for ASCII characters."""
        state = ScrollDMAState(top_base=0x4000, bottom_base=0x5000)
        # Queue character 'A' (ASCII 0x41) on line 0 at cursor_x = 0
        entry0 = state.queue_glyph_dma(0x41)
        expected_src = (0xFF2FA8 + (0x41 - 0x20) * 32) >> 1
        self.assertEqual(entry0["src_word"], expected_src)
        self.assertEqual(entry0["dest_vram"], 0x4000)
        self.assertEqual(entry0["word_count"], 0x10)
        self.assertEqual(state.cursor_x, 1)

        # Queue character 'B' (ASCII 0x42) at cursor_x = 1
        entry1 = state.queue_glyph_dma(0x42)
        self.assertEqual(entry1["dest_vram"], 0x4000 + 32)
        self.assertEqual(state.cursor_x, 2)

    def test_05_blank_tile_dma_helper(self):
        """Verify blank tile DMA transfer helper 0x00C734 queues 512 words from 0x7F9DD4."""
        state = ScrollDMAState()
        entry = state.queue_blank_dma(0x6000)
        self.assertEqual(entry["src_addr"], 0x007F9DD4)
        self.assertEqual(entry["dest_vram"], 0x6000)
        self.assertEqual(entry["word_count"], 0x200)


if __name__ == "__main__":
    unittest.main()
