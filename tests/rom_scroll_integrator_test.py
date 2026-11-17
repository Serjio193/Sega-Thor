#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Plane A 512x256 Ring-Buffer Scroll Integrator,
Tile Strip Dirty Marker & Toroidal Modulo Normalizer (0x00D146..0x00D2B4, 366 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00D146
END_ADDR = 0x00D2B4
ROUTINE_SIZE = END_ADDR - START_ADDR  # 366 bytes


class TilemapCameraState:
    """Emulates ROM Camera/Tilemap Struct (a0) and scroll delta integration."""

    def __init__(self, cam_x=100, cam_y=80, width_tiles=60, height_tiles=40):
        # 16-bit coordinates and subpixel
        self.cam_x = cam_x
        self.cam_sub_x = 0
        self.cam_y = cam_y
        self.cam_sub_y = 0

        # Plane A VDP scroll accumulators
        self.vdp_hscroll = 0  # 0..511 ring buffer (0x0E)
        self.vdp_vscroll = 0  # 0..255 ring buffer (0x10)

        # Delta outputs
        self.delta_x = 0  # (0x12)
        self.delta_y = 0  # (0x14)
        self.tile_column_step = 0  # (0x1C)
        self.tile_row_step = 0     # (0x1E)

        # Map dimensions in 8x8 tiles
        self.map_width_tiles = width_tiles   # (0x20)
        self.map_height_tiles = height_tiles # (0x22)

        # Status and control flags
        self.dirty_flags = 0   # (0x26)
        self.control_flags = 1  # (0x27): bit 0 enables integration

    def accumulate_long_and_integrate(self, delta_long_x, delta_long_y):
        """Emulates 0x00D146..0x00D164."""
        if (self.control_flags & 0x01) == 0:
            return  # early return (0x00D264)

        # Add 32-bit fixed point: 16.16
        old_fixed_x = (self.cam_x << 16) | (self.cam_sub_x & 0xFFFF)
        new_fixed_x = (old_fixed_x + delta_long_x) & 0xFFFFFFFF
        self.cam_sub_x = new_fixed_x & 0xFFFF
        new_cam_x = (new_fixed_x >> 16) & 0xFFFF
        if new_cam_x >= 0x8000:
            new_cam_x -= 0x10000

        old_fixed_y = (self.cam_y << 16) | (self.cam_sub_y & 0xFFFF)
        new_fixed_y = (old_fixed_y + delta_long_y) & 0xFFFFFFFF
        self.cam_sub_y = new_fixed_y & 0xFFFF
        new_cam_y = (new_fixed_y >> 16) & 0xFFFF
        if new_cam_y >= 0x8000:
            new_cam_y -= 0x10000

        self.integrate_scroll_deltas(new_cam_x, new_cam_y)

    def accumulate_word_and_integrate(self, delta_word_x, delta_word_y):
        """Emulates 0x00D168..0x00D17A."""
        if (self.control_flags & 0x01) == 0:
            return
        new_cam_x = self.cam_x + delta_word_x
        new_cam_y = self.cam_y + delta_word_y
        self.integrate_scroll_deltas(new_cam_x, new_cam_y)

    def integrate_scroll_deltas(self, new_x, new_y):
        """Emulates 0x00D188..0x00D264."""
        status_d0 = 0
        self.delta_x = 0
        self.delta_y = 0

        # Horizontal integration
        old_x = self.cam_x
        if new_x != old_x:
            self.cam_x = new_x
            d5 = new_x - old_x

            # Check tile shift
            if (self.control_flags & 0x02) == 0:
                old_tile_x = old_x >> 3
                new_tile_x = new_x >> 3
                tile_shift_x = new_tile_x - old_tile_x
                if tile_shift_x != 0:
                    self.tile_column_step = tile_shift_x
                    status_d0 |= 0x01  # bit 0: h tile strip dirty

            self.delta_x = d5
            # VDP Plane A horizontal scroll: sub delta from scroll reg, mask 0x1FF
            self.vdp_hscroll = (self.vdp_hscroll - d5) & 0x1FF
            status_d0 |= 0x04  # bit 2: vdp hscroll dirty

            # Wrapping bound checks
            map_w_px = self.map_width_tiles << 3
            if self.cam_x <= -10000:
                self.cam_x += map_w_px * 2
            elif self.cam_x >= 10000:
                self.cam_x -= map_w_px * 2

        # Vertical integration
        old_y = self.cam_y
        if new_y != old_y:
            self.cam_y = new_y
            d6 = new_y - old_y

            # Check tile shift
            if (self.control_flags & 0x02) == 0:
                old_tile_y = old_y >> 3
                new_tile_y = new_y >> 3
                tile_shift_y = new_tile_y - old_tile_y
                if tile_shift_y != 0:
                    self.tile_row_step = tile_shift_y
                    status_d0 |= 0x02  # bit 1: v tile strip dirty

            self.delta_y = d6
            # VDP Plane A vertical scroll: add delta to scroll reg, mask 0xFF
            self.vdp_vscroll = (self.vdp_vscroll + d6) & 0xFF
            status_d0 |= 0x08  # bit 3: vdp vscroll dirty

            # Wrapping bound checks
            map_h_px = self.map_height_tiles << 3
            if self.cam_y <= -10000:
                self.cam_y += map_h_px * 2
            elif self.cam_y >= 10000:
                self.cam_y -= map_h_px * 2

        self.dirty_flags = status_d0

    def normalize_toroidal_coords(self):
        """Emulates 0x00D266..0x00D2B4: modulo wrapping into [0..size_px)."""
        w_px = self.map_width_tiles << 3
        while self.cam_x < 0:
            self.cam_x += w_px
        while self.cam_x >= w_px:
            self.cam_x -= w_px

        h_px = self.map_height_tiles << 3
        while self.cam_y < 0:
            self.cam_y += h_px
        while self.cam_y >= h_px:
            self.cam_y -= h_px


class TestRomScrollIntegrator(unittest.TestCase):
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

        # 0x00D146: btst.b #$0, $27(a0) -> 08 28 00 00 00 27
        self.assertEqual(self.routine_bytes[0:6], bytes([0x08, 0x28, 0x00, 0x00, 0x00, 0x27]))

        # 0x00D188: clr.w d0 -> 42 40
        off_integrate = 0x00D188 - START_ADDR
        self.assertEqual(self.routine_bytes[off_integrate:off_integrate + 2], bytes([0x42, 0x40]))

        # 0x00D264: rts -> 4E 75
        off_rts1 = 0x00D264 - START_ADDR
        self.assertEqual(self.routine_bytes[off_rts1:off_rts1 + 2], bytes([0x4E, 0x75]))

        # 0x00D2B2: rts -> 4E 75
        off_rts2 = 0x00D2B2 - START_ADDR
        self.assertEqual(self.routine_bytes[off_rts2:off_rts2 + 2], bytes([0x4E, 0x75]))

    def test_02_accumulate_long_and_word_deltas(self):
        """Verify 16.16 fixed-point accumulation and word accumulation."""
        state = TilemapCameraState(cam_x=10, cam_y=20)
        # Add 32-bit: 2 pixels + 0x8000 (0.5 pixel) to X
        state.accumulate_long_and_integrate(delta_long_x=0x00028000, delta_long_y=0x00034000)
        self.assertEqual(state.cam_x, 12)
        self.assertEqual(state.cam_sub_x, 0x8000)
        self.assertEqual(state.cam_y, 23)
        self.assertEqual(state.cam_sub_y, 0x4000)

        # Word accumulation
        state.accumulate_word_and_integrate(delta_word_x=4, delta_word_y=5)
        self.assertEqual(state.cam_x, 16)
        self.assertEqual(state.cam_y, 28)

    def test_03_horizontal_delta_and_vdp_ring_buffer(self):
        """Verify horizontal delta, 512-px VDP ring buffer, and column strip dirty flag."""
        state = TilemapCameraState(cam_x=10, cam_y=20)
        state.vdp_hscroll = 50

        # Move right by 10 pixels (from 10 to 20): crossing tile 1 (8) and tile 2 (16)
        # old_tile = 10 >> 3 = 1; new_tile = 20 >> 3 = 2 -> tile_shift = +1
        state.integrate_scroll_deltas(new_x=20, new_y=20)

        self.assertEqual(state.delta_x, 10)
        self.assertEqual(state.tile_column_step, 1)
        # VDP hscroll subtracts delta: (50 - 10) & 0x1FF = 40
        self.assertEqual(state.vdp_hscroll, 40)
        # Dirty flags: bit 0 (0x01) and bit 2 (0x04) set -> 0x05
        self.assertEqual(state.dirty_flags, 0x05)

        # Move left by 50 pixels (wrapping backwards in 512-px buffer)
        state.integrate_scroll_deltas(new_x=-30, new_y=20)
        # (40 - (-50)) & 0x1FF = 90
        self.assertEqual(state.vdp_hscroll, 90)

    def test_04_vertical_delta_and_vdp_ring_buffer(self):
        """Verify vertical delta, 256-px VDP ring buffer, and row strip dirty flag."""
        state = TilemapCameraState(cam_x=10, cam_y=20)
        state.vdp_vscroll = 250

        # Move down by 16 pixels (crossing 2 tiles)
        state.integrate_scroll_deltas(new_x=10, new_y=36)

        self.assertEqual(state.delta_y, 16)
        self.assertEqual(state.tile_row_step, 2)
        # VDP vscroll adds delta: (250 + 16) & 0xFF = 266 & 0xFF = 10
        self.assertEqual(state.vdp_vscroll, 10)
        # Dirty flags: bit 1 (0x02) and bit 3 (0x08) set -> 0x0A
        self.assertEqual(state.dirty_flags, 0x0A)

    def test_05_toroidal_modulo_normalizer(self):
        """Verify toroidal modulo coordinate normalization (0x00D266)."""
        # width = 60 tiles (480 px), height = 40 tiles (320 px)
        state = TilemapCameraState(cam_x=-50, cam_y=-30, width_tiles=60, height_tiles=40)
        state.normalize_toroidal_coords()
        self.assertEqual(state.cam_x, 430)  # -50 + 480 = 430
        self.assertEqual(state.cam_y, 290)  # -30 + 320 = 290

        # Positive overshoot
        state.cam_x = 500  # 500 - 480 = 20
        state.cam_y = 350  # 350 - 320 = 30
        state.normalize_toroidal_coords()
        self.assertEqual(state.cam_x, 20)
        self.assertEqual(state.cam_y, 30)


if __name__ == "__main__":
    unittest.main()
