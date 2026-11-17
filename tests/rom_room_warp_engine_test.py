#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Room Warp Controller,
Camera Centering, Cutscene Locks & Player Ali Spawn Reset Engine (0x00C9B2..0x00CC12, 608 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C9B2
END_ADDR = 0x00CC12
ROUTINE_SIZE = END_ADDR - START_ADDR  # 608 bytes


class RoomWarpState:
    """Emulates room warp, cutscene locks, camera centering, and player spawn."""
    def __init__(self):
        # Lock flags
        self.cutscene_active = False   # $FF17B9
        self.transition_busy = False   # $FF1987
        self.pause_menu = False        # $FF1985
        self.cutscene_lock_bit2 = False # $FF17B8 bit 2
        self.lock_83 = False           # $FF1983
        self.lock_82 = False           # $FF1982

        # Warp mode flags
        self.transition_mode = 0       # $FF19FE
        self.immediate_warp = False    # $FF17BE
        self.fade_active = False       # $FF17BC
        self.blackout = False          # $FF17BB
        self.room_loaded = False       # $FF17BA

        # Stored coordinates
        self.camera_x = 0
        self.camera_y = 0
        self.spawn_x = 0
        self.spawn_y = 0

        # Player Ali slot ($FF19E8)
        self.ali_x = 0
        self.ali_y = 0
        self.ali_elev = 0
        self.ali_mode = 0
        self.ali_facing = 0
        self.ali_idle_frame = 0
        self.ali_visible = 0
        self.ali_layer = 0

    def check_cutscene_locks(self):
        """Emulates 0x00C9B2..0x00C9EA."""
        if (self.cutscene_active or
            self.transition_busy or
            self.pause_menu or
            self.cutscene_lock_bit2 or
            self.lock_83 or
            self.lock_82):
            return False  # Locked -> Abort caller
        return True  # Clean -> Proceed

    def setup_warp_coords(self, target_x, target_y, mode, immediate=True):
        """Emulates 0x00C9EC or 0x00CA10."""
        self.transition_mode = mode
        if immediate:
            self.immediate_warp = True
            self.fade_active = False
            self.blackout = False
        else:
            self.immediate_warp = False
            self.fade_active = True
            self.blackout = True

        # Center camera: target - (160, 112)
        self.camera_x = (target_x - 160) & 0xFFFF
        self.camera_y = (target_y - 112) & 0xFFFF
        self.spawn_x = target_x
        self.spawn_y = target_y

    def execute_room_transition(self, room_id):
        """Emulates 0x00CA3E..0x00CC12."""
        if room_id == 4:
            room_id = 0x0611  # Special override

        self.room_loaded = True
        self.cutscene_active = False
        self.transition_busy = False

        # Spawn Player Ali ($FF19E8)
        self.ali_x = self.spawn_x
        self.ali_y = self.spawn_y
        self.ali_idle_frame = 0x003F
        self.ali_visible = 0x01
        self.ali_layer = 0x10

        # Check falling spawn condition
        if self.fade_active or self.blackout:
            self.ali_elev = 240  # 0x00F0 px from sky!
            self.ali_mode = 6     # Falling pose
            self.ali_facing = 6
        else:
            self.ali_elev = 0
            self.ali_mode = 0
            self.ali_facing = 0
        return room_id


class TestRomRoomWarpEngine(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(ROM_PATH):
            raise unittest.SkipTest(f"ROM not found at {ROM_PATH}")
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_01_binary_bounds_and_opcodes(self):
        """Verify binary size, opcodes, and entrypoint signatures."""
        self.assertGreaterEqual(len(self.rom), END_ADDR)
        code = self.rom[START_ADDR:END_ADDR]
        self.assertEqual(len(code), ROUTINE_SIZE)

        # 0x00C9B2: tst.b $FF17B9.l
        self.assertEqual(code[0:6], bytes([0x4A, 0x39, 0x00, 0xFF, 0x17, 0xB9]))
        # 0x00C9E8: addq.l #4, a7; rts
        self.assertEqual(code[0x36:0x3A], bytes([0x58, 0x8F, 0x4E, 0x75]))
        # 0x00C9EC: move.w d5, $FF19FE.l
        self.assertEqual(code[0x3A:0x40], bytes([0x33, 0xC5, 0x00, 0xFF, 0x19, 0xFE]))
        # 0x00CA10: st.b $FF17BC.l
        self.assertEqual(code[0x5E:0x64], bytes([0x50, 0xF9, 0x00, 0xFF, 0x17, 0xBC]))
        # 0x00CA3E: cmpi.w #4, d0
        self.assertEqual(code[0x8C:0x90], bytes([0x0C, 0x40, 0x00, 0x04]))
        # 0x00CA44: move.w #$611, d0
        self.assertEqual(code[0x92:0x96], bytes([0x30, 0x3C, 0x06, 0x11]))
        # 0x00CBE6: move.w #$F0, $10(a0) (falling elevation 240)
        self.assertEqual(code[0x0234:0x023A], bytes([0x31, 0x7C, 0x00, 0xF0, 0x00, 0x10]))

    def test_02_cutscene_lock_abort_behavior(self):
        """Verify lock checks abort actions when any lock flag is active."""
        state = RoomWarpState()
        self.assertTrue(state.check_cutscene_locks())

        # Test each lock flag independently
        state.cutscene_active = True
        self.assertFalse(state.check_cutscene_locks())
        state.cutscene_active = False

        state.transition_busy = True
        self.assertFalse(state.check_cutscene_locks())
        state.transition_busy = False

        state.pause_menu = True
        self.assertFalse(state.check_cutscene_locks())
        state.pause_menu = False

        state.cutscene_lock_bit2 = True
        self.assertFalse(state.check_cutscene_locks())
        state.cutscene_lock_bit2 = False

        state.lock_83 = True
        self.assertFalse(state.check_cutscene_locks())
        state.lock_83 = False

        state.lock_82 = True
        self.assertFalse(state.check_cutscene_locks())

    def test_03_camera_centering_coordinates(self):
        """Verify camera is centered on (target - 160, target - 112)."""
        state = RoomWarpState()
        # Spawn at (400, 300)
        state.setup_warp_coords(400, 300, mode=1, immediate=True)
        self.assertEqual(state.camera_x, 400 - 160)  # 240
        self.assertEqual(state.camera_y, 300 - 112)  # 188
        self.assertEqual(state.spawn_x, 400)
        self.assertEqual(state.spawn_y, 300)
        self.assertTrue(state.immediate_warp)
        self.assertFalse(state.fade_active)

    def test_04_special_room_override(self):
        """Verify room ID 4 is remapped to 0x0611."""
        state = RoomWarpState()
        state.setup_warp_coords(200, 200, mode=0, immediate=True)
        resolved_id = state.execute_room_transition(4)
        self.assertEqual(resolved_id, 0x0611)

        resolved_id2 = state.execute_room_transition(0x0100)
        self.assertEqual(resolved_id2, 0x0100)

    def test_05_player_spawn_and_falling_sky_drop(self):
        """Verify Player Ali spawn initialization for normal vs falling drop."""
        # 1. Immediate warp (normal ground spawn)
        state_ground = RoomWarpState()
        state_ground.setup_warp_coords(500, 600, mode=1, immediate=True)
        state_ground.execute_room_transition(0x0200)
        self.assertEqual(state_ground.ali_x, 500)
        self.assertEqual(state_ground.ali_y, 600)
        self.assertEqual(state_ground.ali_elev, 0)
        self.assertEqual(state_ground.ali_mode, 0)
        self.assertEqual(state_ground.ali_idle_frame, 0x003F)

        # 2. Fade warp (sky drop: elevation 240, falling pose 6)
        state_drop = RoomWarpState()
        state_drop.setup_warp_coords(500, 600, mode=2, immediate=False)
        state_drop.execute_room_transition(0x0200)
        self.assertEqual(state_drop.ali_x, 500)
        self.assertEqual(state_drop.ali_y, 600)
        self.assertEqual(state_drop.ali_elev, 240)
        self.assertEqual(state_drop.ali_mode, 6)
        self.assertEqual(state_drop.ali_facing, 6)


if __name__ == "__main__":
    unittest.main()
