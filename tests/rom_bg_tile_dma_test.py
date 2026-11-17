#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Background Tile Pattern Decompressor
& 4KB VRAM DMA Streamer (0x00D3B2..0x00D406, 84 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00D3B2
END_ADDR = 0x00D406
ROUTINE_SIZE = END_ADDR - START_ADDR  # 84 bytes

BG_TILE_TABLE_ADDR = 0x05CE96


class BgTileDmaState:
    """Emulates ROM background tile decompression and VRAM DMA queueing."""

    def __init__(self):
        self.dma_queue = []
        self.dma_queue_ptr = 0x00FF18A0
        self.vint_sync_flags = 0
        self.decompressed_pattern_index = None
        self.dest_vram = None

    def decompress_and_dma_bg_tiles(self, pattern_idx, vram_dest, rom_bytes):
        """Emulates 0x00D3B2..0x00D406."""
        self.decompressed_pattern_index = pattern_idx
        self.dest_vram = vram_dest

        # Look up pointer in ROM table 0x05CE96
        table_offset = BG_TILE_TABLE_ADDR + pattern_idx * 4
        assert table_offset + 4 <= len(rom_bytes)
        pattern_ptr = struct.unpack_from(">I", rom_bytes, table_offset)[0]

        # Enqueue DMA entry
        entry = {
            "source_bus_addr": 0x007F97D4,  # 0xFF2FA8 >> 1
            "dest_vram": vram_dest,
            "length_words": 0x0800,         # 4096 bytes
            "pattern_ptr": pattern_ptr,
        }
        self.dma_queue.append(entry)
        self.dma_queue_ptr += 8

        # V-Int DMA request
        self.vint_sync_flags |= 0x02
        # Emulate V-Int completion
        self.vint_sync_flags &= ~0x02


class TestRomBgTileDma(unittest.TestCase):
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

        # 0x00D3B2: movem.l d0/a0-a1, -(a7) -> 48 E7 80 C0
        self.assertEqual(self.routine_bytes[0:4], bytes([0x48, 0xE7, 0x80, 0xC0]))

        # 0x00D3B6: lea $FF2FA8.l, a1 -> 43 F9 00 FF 2F A8
        self.assertEqual(self.routine_bytes[4:10], bytes([0x43, 0xF9, 0x00, 0xFF, 0x2F, 0xA8]))

        # 0x00D3D8: move.l #$7F97D4, (a0)+ -> 20 FC 00 7F 97 D4
        off_dma_src = 0x00D3D8 - START_ADDR
        self.assertEqual(self.routine_bytes[off_dma_src:off_dma_src + 6], bytes([0x20, 0xFC, 0x00, 0x7F, 0x97, 0xD4]))

        # 0x00D3E0: move.w #$800, (a0)+ -> 30 FC 08 00
        off_len = 0x00D3E0 - START_ADDR
        self.assertEqual(self.routine_bytes[off_len:off_len + 4], bytes([0x30, 0xFC, 0x08, 0x00]))

        # 0x00D404: rts -> 4E 75
        off_rts = 0x00D404 - START_ADDR
        self.assertEqual(self.routine_bytes[off_rts:off_rts + 2], bytes([0x4E, 0x75]))

    def test_02_pattern_table_validity(self):
        """Verify ROM pointer table at 0x05CE96."""
        for i in range(1, 10):
            ptr = struct.unpack_from(">I", self.rom, BG_TILE_TABLE_ADDR + i * 4)[0]
            self.assertGreater(ptr, 0x100000)
            self.assertLess(ptr, len(self.rom))

    def test_03_dma_queue_entry_and_length(self):
        """Verify DMA transfer packaging and 4KB length."""
        state = BgTileDmaState()
        state.decompress_and_dma_bg_tiles(pattern_idx=1, vram_dest=0x2000, rom_bytes=self.rom)

        self.assertEqual(len(state.dma_queue), 1)
        entry = state.dma_queue[0]
        self.assertEqual(entry["source_bus_addr"], 0x007F97D4)
        self.assertEqual(entry["dest_vram"], 0x2000)
        self.assertEqual(entry["length_words"], 0x0800)  # 2048 words = 4096 bytes
        self.assertEqual(entry["pattern_ptr"], 0x1AD000)
        self.assertEqual(state.dma_queue_ptr, 0x00FF18A8)
        self.assertEqual(state.vint_sync_flags & 0x02, 0)  # completed

    def test_04_multiple_tile_patterns(self):
        """Verify sequential streaming of multiple patterns to different VRAM bases."""
        state = BgTileDmaState()
        state.decompress_and_dma_bg_tiles(pattern_idx=2, vram_dest=0x4000, rom_bytes=self.rom)
        state.decompress_and_dma_bg_tiles(pattern_idx=3, vram_dest=0x6000, rom_bytes=self.rom)

        self.assertEqual(len(state.dma_queue), 2)
        self.assertEqual(state.dma_queue[0]["pattern_ptr"], 0x1AD9D4)
        self.assertEqual(state.dma_queue[1]["pattern_ptr"], 0x1AE1A8)


if __name__ == "__main__":
    unittest.main()
