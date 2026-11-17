#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Scene Script Header Unpacker,
Dual-Plane Camera Initializer & 16KB VRAM DMA Upload Engine (0x00D406..0x00D542, 316 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00D406
END_ADDR = 0x00D542
ROUTINE_SIZE = END_ADDR - START_ADDR  # 316 bytes


class SceneHeaderDmaState:
    """Emulates ROM scene header unpacking, camera setup, and 16KB VRAM DMA."""

    def __init__(self):
        # RAM variables
        self.bgm_track_16f4 = 0
        self.bgm_tempo_16f6 = 0
        self.bgm_mode_16f8 = 0
        self.scroll_flags_d6 = 0
        self.vdp_scroll_mode_16f0 = 0
        self.bg_patterns_16fa = [0, 0, 0, 0]

        # Elevation tables
        self.unpacked_elevation_16fe = [0, 0, 0, 0]
        self.height_bands_1706 = [0 for _ in range(8)]

        # Camera structs (20 words each)
        self.cam_plane_a_1716 = [0 for _ in range(20)]
        self.cam_plane_b_173e = [0 for _ in range(20)]

        # DMA parameters
        self.dma_queue = []
        self.vint_sync_flags = 0

    def unpack_scene_header(self, header_bytes, initial_coord_d1, initial_coord_d2):
        """Emulates 0x00D406..0x00D542."""
        # Unpack header fields
        self.bgm_track_16f4 = struct.unpack_from(">H", header_bytes, 0x10)[0]
        self.bgm_tempo_16f6 = struct.unpack_from(">H", header_bytes, 0x12)[0]
        self.bgm_mode_16f8 = struct.unpack_from(">H", header_bytes, 0x14)[0]
        self.scroll_flags_d6 = struct.unpack_from(">H", header_bytes, 0x16)[0]
        self.vdp_scroll_mode_16f0 = struct.unpack_from(">H", header_bytes, 0x18)[0]

        for i in range(4):
            self.bg_patterns_16fa[i] = header_bytes[0x08 + i]

        # Elevation bands (0x00D44A..0x00D466)
        d5 = 0
        a2_idx = 0
        for i in range(4):
            raw_b = header_bytes[0x0C + i]
            # Sign extend and shift << 3
            if raw_b >= 128:
                d3 = ((raw_b - 256) << 3) & 0xFFFF
            else:
                d3 = (raw_b << 3) & 0xFFFF
            self.unpacked_elevation_16fe[i] = d3

            self.height_bands_1706[a2_idx] = d5
            a2_idx += 1

            d4 = (d3 >> 1) + d5
            self.height_bands_1706[a2_idx] = d4
            a2_idx += 1

            d5 = (d5 + d3) & 0xFFFF

        # Set sentinel at -2(a2)
        self.height_bands_1706[7] = 0xF000

        # Clear 20 words for Plane A and Plane B (0x00D46C..0x00D480)
        self.cam_plane_a_1716 = [0 for _ in range(20)]
        self.cam_plane_b_173e = [0 for _ in range(20)]

        # Setup initial Plane A coordinates (0x00D48A..0x00D4A4)
        aligned_d1 = initial_coord_d1 & 0xFFF8FFF8
        y1 = aligned_d1 & 0xFFFF
        x1 = (aligned_d1 >> 16) & 0xFFFF
        self.cam_plane_a_1716[2] = max(0, y1)  # offset +4: cam_y
        self.cam_plane_a_1716[0] = max(0, x1)  # offset +0: cam_x

        # Setup initial Plane B coordinates (0x00D4AC..0x00D4C6)
        aligned_d2 = initial_coord_d2 & 0xFFF8FFF8
        y2 = aligned_d2 & 0xFFFF
        x2 = (aligned_d2 >> 16) & 0xFFFF
        self.cam_plane_b_173e[2] = max(0, y2)
        self.cam_plane_b_173e[0] = max(0, x2)

        # 16KB VRAM DMA upload (0x00D50E..0x00D53C)
        self.dma_queue.append({
            "bus_source": 0x007F9FD4,  # 0xFF3FA8 >> 1
            "vram_dest": 0x0000,
            "length_words": 0x2000,     # 8192 words = 16384 bytes
        })
        self.vint_sync_flags |= 0x02
        self.vint_sync_flags &= ~0x02


class TestRomSceneHeaderDma(unittest.TestCase):
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

        # 0x00D406: move.w $10(a1), $FF16F4.l -> 33 E9 00 10 00 FF 16 F4
        self.assertEqual(self.routine_bytes[0:8], bytes([0x33, 0xE9, 0x00, 0x10, 0x00, 0xFF, 0x16, 0xF4]))

        # 0x00D466: move.w #$F000, -$2(a2) -> 35 7C F0 00 FF FE
        off_sentinel = 0x00D466 - START_ADDR
        self.assertEqual(self.routine_bytes[off_sentinel:off_sentinel + 6], bytes([0x35, 0x7C, 0xF0, 0x00, 0xFF, 0xFE]))

        # 0x00D514: move.l #$7F9FD4, (a0)+ -> 20 FC 00 7F 9F D4
        off_dma = 0x00D514 - START_ADDR
        self.assertEqual(self.routine_bytes[off_dma:off_dma + 6], bytes([0x20, 0xFC, 0x00, 0x7F, 0x9F, 0xD4]))

        # 0x00D51E: move.w #$2000, (a0)+ -> 30 FC 20 00
        off_dma_len = 0x00D51E - START_ADDR
        self.assertEqual(self.routine_bytes[off_dma_len:off_dma_len + 4], bytes([0x30, 0xFC, 0x20, 0x00]))

    def test_02_script_header_unpacking(self):
        """Verify unpacking of BGM, scroll mode, and pattern indices."""
        # Construct synthetic 32-byte header
        header = bytearray(32)
        # 0x08: patterns
        header[0x08:0x0C] = bytes([1, 2, 3, 4])
        # 0x0C: elevation slices
        header[0x0C:0x10] = bytes([10, 15, 20, 25])
        # 0x10: BGM track
        struct.pack_into(">H", header, 0x10, 0x0012)
        # 0x12: BGM tempo
        struct.pack_into(">H", header, 0x12, 0x0080)
        # 0x14: BGM mode
        struct.pack_into(">H", header, 0x14, 0x0001)
        # 0x16: scroll flags
        struct.pack_into(">H", header, 0x16, 0x00AA)
        # 0x18: VDP scroll mode
        struct.pack_into(">H", header, 0x18, 0x0003)

        state = SceneHeaderDmaState()
        state.unpack_scene_header(header, initial_coord_d1=0x00640032, initial_coord_d2=0x00C80096)

        self.assertEqual(state.bgm_track_16f4, 0x0012)
        self.assertEqual(state.bgm_tempo_16f6, 0x0080)
        self.assertEqual(state.bgm_mode_16f8, 0x0001)
        self.assertEqual(state.scroll_flags_d6, 0x00AA)
        self.assertEqual(state.vdp_scroll_mode_16f0, 0x0003)
        self.assertEqual(state.bg_patterns_16fa, [1, 2, 3, 4])

    def test_03_elevation_bands_and_sentinel(self):
        """Verify elevation threshold shifts and sentinel $F000."""
        header = bytearray(32)
        header[0x0C:0x10] = bytes([2, 4, 6, 8])

        state = SceneHeaderDmaState()
        state.unpack_scene_header(header, 0, 0)

        # 2 << 3 = 16, 4 << 3 = 32, 6 << 3 = 48, 8 << 3 = 64
        self.assertEqual(state.unpacked_elevation_16fe, [16, 32, 48, 64])
        # Band 0: start 0, mid 8
        self.assertEqual(state.height_bands_1706[0], 0)
        self.assertEqual(state.height_bands_1706[1], 8)
        # Sentinel at end
        self.assertEqual(state.height_bands_1706[7], 0xF000)

    def test_04_camera_coords_grid_alignment(self):
        """Verify 8-pixel alignment of initial camera coordinates."""
        header = bytearray(32)
        state = SceneHeaderDmaState()
        # d1 = X: 103 (0x67) -> aligned 96 (0x60), Y: 55 (0x37) -> aligned 48 (0x30)
        coord_d1 = (103 << 16) | 55
        state.unpack_scene_header(header, initial_coord_d1=coord_d1, initial_coord_d2=0)

        self.assertEqual(state.cam_plane_a_1716[0], 96)
        self.assertEqual(state.cam_plane_a_1716[2], 48)

    def test_05_16kb_vram_dma_upload(self):
        """Verify 16KB VRAM DMA transfer queueing."""
        header = bytearray(32)
        state = SceneHeaderDmaState()
        state.unpack_scene_header(header, 0, 0)

        self.assertEqual(len(state.dma_queue), 1)
        entry = state.dma_queue[0]
        self.assertEqual(entry["bus_source"], 0x007F9FD4)
        self.assertEqual(entry["vram_dest"], 0x0000)
        self.assertEqual(entry["length_words"], 0x2000)  # 8192 words = 16KB


if __name__ == "__main__":
    unittest.main()
