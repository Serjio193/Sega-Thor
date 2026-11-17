#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Map & Scene Script Dispatcher,
21-Area Script Table & State Manager (0x00C88C..0x00C9B2, 294 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C88C
END_ADDR = 0x00C9B2
ROUTINE_SIZE = END_ADDR - START_ADDR  # 294 bytes

AREA_TABLE_ADDR = 0x00C92C
NUM_AREAS = 21


class MapDispatcherState:
    """Emulates map & scene script state manager in $FF0E40, $FF0E2C, $FF0E1C, $FF0DB3."""
    def __init__(self, initial_id=0):
        self.current_id = initial_id
        self.area_flags = [0xFF] * 16  # $FF0E2C..$FF0E3C
        self.map_flags = [0xFF] * 16   # $FF0E1C..$FF0E2C
        self.trigger_flags = [0xFF] * 3 # $FF0DB3..$FF0DB6
        self.script_ptr = 0
        self.cleared_area_flags = False
        self.cleared_map_flags = False

    def load_scene(self, target_id, area_table_ptrs, rom):
        """Emulates 0x00C8AA."""
        target_area = (target_id >> 8) & 0xFF
        target_scene = target_id & 0xFF
        current_area = (self.current_id >> 8) & 0xFF

        if target_id != self.current_id:
            # Clear 16-byte area state flags ($FF0E2C)
            self.area_flags = [0] * 16
            self.cleared_area_flags = True

            if target_area != current_area:
                # Clear 16-byte map state flags ($FF0E1C) and 3 trigger bytes ($FF0DB3)
                self.map_flags = [0] * 16
                self.trigger_flags = [0] * 3
                self.cleared_map_flags = True

            self.current_id = target_id

        # Resolve script address
        area_base = area_table_ptrs[target_area]
        scene_off = struct.unpack_from(">h", rom, area_base + target_scene * 2)[0]
        self.script_ptr = area_base + target_scene * 2 + scene_off
        init_routine_addr = self.script_ptr + 0x1A
        return init_routine_addr


class TestRomMapSceneDispatcher(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(ROM_PATH):
            raise unittest.SkipTest(f"ROM not found at {ROM_PATH}")
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

        cls.area_ptrs = []
        for i in range(NUM_AREAS):
            ptr = struct.unpack_from(">I", cls.rom, AREA_TABLE_ADDR + i * 4)[0]
            cls.area_ptrs.append(ptr)

    def test_01_binary_bounds_and_opcodes(self):
        """Verify binary size, opcodes, and subroutine returns."""
        self.assertGreaterEqual(len(self.rom), END_ADDR)
        code = self.rom[START_ADDR:END_ADDR]
        self.assertEqual(len(code), ROUTINE_SIZE)

        # 0x00C88C: move.l $FF1882.l, d6
        self.assertEqual(code[0:6], bytes([0x2C, 0x39, 0x00, 0xFF, 0x18, 0x82]))
        # 0x00C8A8: rts
        self.assertEqual(code[0x1C:0x1E], bytes([0x4E, 0x75]))
        # 0x00C8AA: moveq #0, d7
        self.assertEqual(code[0x1E:0x20], bytes([0x7E, 0x00]))
        # 0x00C8AC: cmp.w $FF0E40.l, d0
        self.assertEqual(code[0x20:0x26], bytes([0xB0, 0x79, 0x00, 0xFF, 0x0E, 0x40]))
        # 0x00C90C: jmp (a0)
        self.assertEqual(code[0x80:0x82], bytes([0x4E, 0xD0]))
        # 0x00C92A: rts
        self.assertEqual(code[0x9E:0xA0], bytes([0x4E, 0x75]))
        # 0x00C996: rts (end of tick)
        self.assertEqual(code[0x010A:0x010C], bytes([0x4E, 0x75]))
        # 0x00C9B0: rts (end of subtick)
        self.assertEqual(code[ROUTINE_SIZE - 2:ROUTINE_SIZE], bytes([0x4E, 0x75]))

    def test_02_area_table_validity(self):
        """Verify 21 32-bit area table pointers are within valid ROM code/data ranges."""
        self.assertEqual(len(self.area_ptrs), NUM_AREAS)
        expected_ptrs = [
            0x0002C6B8, 0x0002E56A, 0x0002F1A4, 0x0002F9B2, 0x0002FF80,
            0x00030218, 0x00030E56, 0x00031B50, 0x000328D6, 0x0003361C,
            0x000341A4, 0x00035528, 0x00036286, 0x00037B42, 0x00038AAC,
            0x0002C6F2, 0x00038FBA, 0x0003957C, 0x00039F7A, 0x00039F7C,
            0x00039F7E,
        ]
        self.assertEqual(self.area_ptrs, expected_ptrs)
        for ptr in self.area_ptrs:
            self.assertGreater(ptr, 0x00020000)
            self.assertLess(ptr, len(self.rom) - 100)

    def test_03_scene_script_resolution(self):
        """Verify scene script offset calculation and init address."""
        state = MapDispatcherState(initial_id=0x0100)
        # Load Area 01, Scene 00 (packed ID 0x0100)
        init_addr = state.load_scene(0x0100, self.area_ptrs, self.rom)
        # Script pointer for Area 1 Scene 0
        area1_base = self.area_ptrs[1]
        scene0_off = struct.unpack_from(">h", self.rom, area1_base)[0]
        expected_script = area1_base + scene0_off
        self.assertEqual(state.script_ptr, expected_script)
        self.assertEqual(init_addr, expected_script + 0x1A)

    def test_04_state_flag_clearing_on_transitions(self):
        """Verify area vs room transition flag clearing."""
        # 1. Same area, different room: ID 0x0100 -> 0x0101
        state = MapDispatcherState(initial_id=0x0100)
        state.load_scene(0x0101, self.area_ptrs, self.rom)
        self.assertTrue(state.cleared_area_flags)
        self.assertFalse(state.cleared_map_flags)  # Area did not change!
        self.assertEqual(state.area_flags, [0] * 16)
        self.assertEqual(state.map_flags, [0xFF] * 16)  # Preserved!

        # 2. Area change: ID 0x0101 -> 0x0200
        state2 = MapDispatcherState(initial_id=0x0101)
        state2.load_scene(0x0200, self.area_ptrs, self.rom)
        self.assertTrue(state2.cleared_area_flags)
        self.assertTrue(state2.cleared_map_flags)  # Area changed!
        self.assertEqual(state2.area_flags, [0] * 16)
        self.assertEqual(state2.map_flags, [0] * 16)
        self.assertEqual(state2.trigger_flags, [0] * 3)

        # 3. Reloading exact same ID: 0x0200 -> 0x0200
        state3 = MapDispatcherState(initial_id=0x0200)
        state3.load_scene(0x0200, self.area_ptrs, self.rom)
        self.assertFalse(state3.cleared_area_flags)
        self.assertFalse(state3.cleared_map_flags)

    def test_05_script_tick_and_subtick_targets(self):
        """Verify tick (+0) and subtick (+2) jump target calculation."""
        # Check script header at Area 1 Scene 0
        area1_base = self.area_ptrs[1]
        scene0_off = struct.unpack_from(">h", self.rom, area1_base)[0]
        script_addr = area1_base + scene0_off

        tick_off = struct.unpack_from(">h", self.rom, script_addr)[0]
        subtick_off = struct.unpack_from(">h", self.rom, script_addr + 2)[0]

        tick_tgt = script_addr + tick_off
        subtick_tgt = script_addr + 2 + subtick_off

        self.assertGreater(tick_tgt, 0x00020000)
        self.assertLess(tick_tgt, len(self.rom))
        self.assertGreater(subtick_tgt, 0x00020000)
        self.assertLess(subtick_tgt, len(self.rom))


if __name__ == "__main__":
    unittest.main()
