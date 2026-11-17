#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Camera Deadzone Tracking,
Boundary Clamp & Screen Shake Controller (0x00CEEA..0x00D146, 604 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00CEEA
END_ADDR = 0x00D146
ROUTINE_SIZE = END_ADDR - START_ADDR  # 604 bytes


class CameraControllerState:
    """Emulates ROM camera tracking, deadzone evaluation, boundary clamping, and shake."""

    def __init__(self, cam_x=0, cam_y=0, max_x=1000, max_y=800):
        self.cam_x = cam_x
        self.cam_y = cam_y
        self.max_x = max_x
        self.max_y = max_y
        self.offset_x = 0
        self.offset_y = 0

        self.player_x = 160
        self.player_y = 120
        self.player_elev_z = 0

        self.warp_active_17bb = 0
        self.instant_snap_17ba = 0
        self.room_flag_16f1 = 0
        self.shake_timer_17c4 = 0
        self.scroll_bias_1859 = 0
        self.scroll_bias_target_185a = 0

    def clamp_viewport_bounds(self, map_width_tiles, map_height_tiles):
        """Emulates 0x00D118..0x00D146: clamps cam_x and cam_y to map bounds."""
        max_x = (map_width_tiles - 40) * 8
        max_y = (map_height_tiles - 28) * 8

        self.cam_x = max(0, min(self.cam_x, max(0, max_x)))
        self.cam_y = max(0, min(self.cam_y, max(0, max_y)))
        return self.cam_x, self.cam_y

    def update_camera(self):
        """Emulates 0x00CEEA..0x00D118: full camera tracker update."""
        target_x = self.player_x
        target_y = self.player_y
        if self.warp_active_17bb == 0:
            target_y -= self.player_elev_z

        d1 = self.cam_x
        d2 = self.cam_y
        d5 = self.instant_snap_17ba
        self.instant_snap_17ba = 0

        # Check direct follow mode (room flag bit 3)
        if (self.room_flag_16f1 & 0x08) != 0:
            # Direct follow mode (0x00D0C4)
            d1 = target_x + self.offset_x - 160
            d2 = target_y + self.offset_y - 120

            # Screen shake in direct mode
            if self.shake_timer_17c4 != 0:
                self.shake_timer_17c4 -= 1
                t = self.shake_timer_17c4
                shake_offset = t if (t & 1) != 0 else -t
                d2 += shake_offset

            d1 = max(0, min(d1, self.max_x))
            d2 = max(0, min(d2, self.max_y))
            self.cam_x = d1
            self.cam_y = d2
            return

        # Deadzone tracking (0x00CF26)
        rel_x = target_x - d1

        # Bias checks
        if rel_x <= 0x50:
            self.scroll_bias_1859 = 0xFF
            if d5 != 0:
                self.scroll_bias_target_185a = 0x0138
        elif rel_x >= 0xF0:
            self.scroll_bias_1859 = 0
            if d5 != 0:
                self.scroll_bias_target_185a = 0x20

        # Horizontal panning
        if rel_x < 0x78:
            d0 = rel_x - 0x78
            if d5 == 0:
                if d0 < -3:
                    if d0 >= -32:
                        d0 = -3
                    else:
                        d0 += 30
            d1 += d0
        d1 = max(0, d1)

        if rel_x > 0xC8:
            d0 = rel_x - 0xC8
            if d5 == 0:
                if d0 > 3:
                    if d0 <= 32:
                        d0 = 3
                    else:
                        d0 -= 30
            d1 += d0
        d1 = min(d1, self.max_x)

        # Vertical panning (0x00CFE8)
        rel_y = target_y - d2
        if rel_y < 0x68:
            d0 = rel_y - 0x68
            if d5 == 0:
                if d0 < -3:
                    if d0 >= -32:
                        d0 = -3
                    else:
                        d0 += 30
                        if d0 < -6:
                            d0 = -6
            d2 += d0
        d2 = max(0, d2)

        if rel_y > 0x80:
            d0 = rel_y - 0x80
            if d5 == 0:
                if d0 > 3:
                    if d0 <= 32:
                        d0 = 3
                    else:
                        d0 -= 30
            d2 += d0
        d2 = min(d2, self.max_y)

        # Screen shake oscillation (0x00D068)
        if self.shake_timer_17c4 != 0:
            self.shake_timer_17c4 -= 1
            t = self.shake_timer_17c4
            shake_offset = t if (t & 1) != 0 else -t
            d2 += shake_offset
            d2 = max(0, min(d2, self.max_y))

        # Directional motion bias update
        if d5 == 0:
            diff_x = d1 - self.cam_x
            if diff_x < 0:
                self.scroll_bias_1859 = 0xFF
            elif diff_x > 0:
                self.scroll_bias_1859 = 0

        self.cam_x = d1
        self.cam_y = d2


class TestRomCameraController(unittest.TestCase):
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

        # 0x00CEEA: move.w $FF19F0.l, d3 -> 36 39 00 FF 19 F0
        self.assertEqual(self.routine_bytes[0:6], bytes([0x36, 0x39, 0x00, 0xFF, 0x19, 0xF0]))

        # 0x00D118: tst.w d1 -> 4A 41
        off_clamp = 0x00D118 - START_ADDR
        self.assertEqual(self.routine_bytes[off_clamp:off_clamp + 2], bytes([0x4A, 0x41]))

        # 0x00D144: rts -> 4E 75
        off_rts = 0x00D144 - START_ADDR
        self.assertEqual(self.routine_bytes[off_rts:off_rts + 2], bytes([0x4E, 0x75]))

    def test_02_horizontal_deadzone_and_panning(self):
        """Verify horizontal deadzone [120..200] and left/right panning speeds."""
        # Player at 160 -> rel_x = 160 (inside deadzone 120..200), cam should not move
        state = CameraControllerState(cam_x=0, cam_y=0, max_x=1000, max_y=1000)
        state.player_x = 160
        state.player_y = 100
        state.update_camera()
        self.assertEqual(state.cam_x, 0)

        # Player moves right past 200 (e.g. 210 -> d0 = 10 -> clamped to 3)
        state.player_x = 210
        state.update_camera()
        self.assertEqual(state.cam_x, 3)

        # Fast catchup: rel_x = 250 (rel_x - 200 = 50 > 32 -> d0 = 50 - 30 = 20)
        state.cam_x = 0
        state.player_x = 250
        state.update_camera()
        self.assertEqual(state.cam_x, 20)

    def test_03_vertical_deadzone_and_panning(self):
        """Verify vertical deadzone [104..128] and top/bottom panning speeds."""
        # Inside deadzone
        state = CameraControllerState(cam_x=0, cam_y=0, max_x=1000, max_y=1000)
        state.player_x = 160
        state.player_y = 110
        state.update_camera()
        self.assertEqual(state.cam_y, 0)

        # Player moves down past 128 (e.g. 140 -> d0 = 12 -> clamped to 3)
        state.player_y = 140
        state.update_camera()
        self.assertEqual(state.cam_y, 3)

        # Max negative vertical catch-up clamped to -6
        state.cam_y = 100
        state.player_y = 20  # rel_y = -80 < 104 -> large negative offset
        state.update_camera()
        self.assertEqual(state.cam_y, 94)  # 100 - 6 = 94

    def test_04_screen_shake_oscillation(self):
        """Verify screen shake timer decrements and alternates sign."""
        state = CameraControllerState(cam_x=0, cam_y=50, max_x=1000, max_y=1000)
        state.player_x = 160
        state.player_y = 160  # inside deadzone relative to cam_y 50
        state.shake_timer_17c4 = 4

        # Frame 1: timer becomes 3 (odd) -> offset = +3
        state.update_camera()
        self.assertEqual(state.shake_timer_17c4, 3)
        self.assertEqual(state.cam_y, 53)

        # Frame 2: timer becomes 2 (even) -> offset = -2
        state.update_camera()
        self.assertEqual(state.shake_timer_17c4, 2)
        self.assertEqual(state.cam_y, 51)

    def test_05_direct_follow_and_viewport_clamp(self):
        """Verify direct follow mode and tile map viewport boundary clamp."""
        state = CameraControllerState(cam_x=0, cam_y=0, max_x=1000, max_y=1000)
        state.room_flag_16f1 = 0x08  # bit 3 set (direct follow)
        state.player_x = 300
        state.player_y = 250
        state.offset_x = 10
        state.offset_y = 20

        state.update_camera()
        # cam_x = 300 + 10 - 160 = 150
        # cam_y = 250 + 20 - 120 = 150
        self.assertEqual(state.cam_x, 150)
        self.assertEqual(state.cam_y, 150)

        # Test viewport clamp (width=60 tiles -> max_x = (60-40)*8 = 160)
        state.cam_x = 200
        state.cam_y = 300
        cx, cy = state.clamp_viewport_bounds(map_width_tiles=60, map_height_tiles=40)
        self.assertEqual(cx, 160)
        self.assertEqual(cy, (40 - 28) * 8)  # 12 * 8 = 96


if __name__ == "__main__":
    unittest.main()
