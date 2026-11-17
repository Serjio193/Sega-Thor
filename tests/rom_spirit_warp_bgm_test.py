#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Companion Spirit Warp Repositioning,
Room Collision Table Loader & BGM Engine Synchronizer (0x00CC12..0x00CD64, 338 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00CC12
END_ADDR = 0x00CD64
ROUTINE_SIZE = END_ADDR - START_ADDR  # 338 bytes

COLLISION_TABLE_BASE = 0x04371E
NUM_COLLISION_AREAS = 32


class SpiritWarpState:
    """Emulates companion spirit repositioning, collision table loading, and BGM sync."""
    def __init__(self, ali_x=300, ali_y=400, spirit_type=0x001A):
        self.ali_x = ali_x
        self.ali_y = ali_y
        self.spirit_active = True
        self.spirit_type = spirit_type
        self.spirit_x = 0
        self.spirit_y = 0
        self.spirit_mode = 5
        self.spirit_facing = 3
        self.spirit_counter_9e = 10
        self.spirit_counter_a2 = 20

        self.collision_table_ptr = 0
        self.collision_tick_cadence = 0
        self.current_bgm = 0x10
        self.target_bgm = 0x10
        self.sent_sound_cmd = None
        self.latched_entities = []

    def reposition_spirit(self, room_forbids_spirit=False):
        """Emulates 0x00CC12..0x00CCA4 and 0x00CCF2..0x00CD02."""
        if not self.spirit_active or self.spirit_type == 0:
            return

        # Check type
        if self.spirit_type == 0x0016:  # Bow
            self.spirit_type = 0
            self.spirit_active = False
            return

        # Position spirit at (Ali_X, Ali_Y - 8)
        self.spirit_x = self.ali_x
        self.spirit_y = self.ali_y - 8

        if self.spirit_type == 0x001A:
            self.spirit_mode = 0
            self.spirit_facing = 0
            self.spirit_counter_9e = 0
            self.spirit_counter_a2 = 0

        # Room spirit forbid flag ($FF16F1 bit 0)
        if room_forbids_spirit:
            self.spirit_type = 0
            self.spirit_active = False

    def load_collision_table(self, room_id, collision_ptrs, rom):
        """Emulates 0x00CCA4..0x00CCD4."""
        area = (room_id >> 8) & 0xFF
        scene = room_id & 0xFF
        area_base = collision_ptrs[area]
        scene_off = struct.unpack_from(">h", rom, area_base + scene * 2)[0]
        self.collision_table_ptr = area_base + scene * 2 + scene_off
        self.collision_tick_cadence = 10  # 0x0A

    def sync_bgm_track(self):
        """Emulates 0x00CD2C..0x00CD44."""
        if self.target_bgm == self.current_bgm:
            self.sent_sound_cmd = 3  # Continue track
        else:
            self.current_bgm = self.target_bgm
            self.sent_sound_cmd = self.target_bgm

    def sweep_ground_surface(self, entity_slots):
        """Emulates 0x00CD4A..0x00CD64."""
        for slot_idx, slot in enumerate(entity_slots[:21]):
            if slot.get("active", False):
                self.latched_entities.append(slot_idx)


class TestRomSpiritWarpBGM(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(ROM_PATH):
            raise unittest.SkipTest(f"ROM not found at {ROM_PATH}")
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

        cls.collision_ptrs = []
        for i in range(NUM_COLLISION_AREAS):
            ptr = struct.unpack_from(">I", cls.rom, COLLISION_TABLE_BASE + i * 4)[0]
            cls.collision_ptrs.append(ptr)

    def test_01_binary_bounds_and_opcodes(self):
        """Verify binary size, opcodes, and entrypoint signatures."""
        self.assertGreaterEqual(len(self.rom), END_ADDR)
        code = self.rom[START_ADDR:END_ADDR]
        self.assertEqual(len(code), ROUTINE_SIZE)

        # 0x00CC12: lea $FF1AA4.l, a1
        self.assertEqual(code[0:6], bytes([0x43, 0xF9, 0x00, 0xFF, 0x1A, 0xA4]))
        # 0x00CC1C: cmpi.w #$1C, $0(a1)
        self.assertEqual(code[10:16], bytes([0x0C, 0x69, 0x00, 0x1C, 0x00, 0x00]))
        # 0x00CC40: subq.w #8, $c(a1)
        self.assertEqual(code[0x2E:0x32], bytes([0x51, 0x69, 0x00, 0x0C]))
        # 0x00CCB0: lea $4371E.l, a0
        self.assertEqual(code[0x9E:0xA4], bytes([0x41, 0xF9, 0x00, 0x04, 0x37, 0x1E]))
        # 0x00CCDA: move.b #$A, $FF17B7.l
        self.assertEqual(code[0xC8:0xD0], bytes([0x13, 0xFC, 0x00, 0x0A, 0x00, 0xFF, 0x17, 0xB7]))
        # 0x00CD3A: moveq #3, d0
        self.assertEqual(code[0x0128:0x012A], bytes([0x70, 0x03]))
        # 0x00CD44: jsr $60004.l
        self.assertEqual(code[0x0132:0x0138], bytes([0x4E, 0xB9, 0x00, 0x06, 0x00, 0x04]))

    def test_02_collision_area_table_validity(self):
        """Verify 32 collision table pointers resolve to valid ROM locations."""
        self.assertEqual(len(self.collision_ptrs), NUM_COLLISION_AREAS)
        expected_ptrs = [
            0x04379E, 0x046332, 0x046B9A, 0x04734E, 0x04735C,
            0x047E98, 0x049046, 0x049DB0, 0x04A8CE, 0x04B6CE,
        ]
        self.assertEqual(self.collision_ptrs[:10], expected_ptrs)
        for ptr in self.collision_ptrs:
            self.assertGreater(ptr, 0x00040000)
            self.assertLess(ptr, len(self.rom) - 100)

    def test_03_spirit_repositioning_and_offsets(self):
        """Verify spirit positioning at Ali_Y - 8 and type-specific handling."""
        # Type 0x001A: repositioned to Y - 8, counters reset
        state = SpiritWarpState(ali_x=200, ali_y=350, spirit_type=0x001A)
        state.reposition_spirit()
        self.assertEqual(state.spirit_x, 200)
        self.assertEqual(state.spirit_y, 350 - 8)
        self.assertEqual(state.spirit_mode, 0)
        self.assertEqual(state.spirit_counter_9e, 0)
        self.assertEqual(state.spirit_counter_a2, 0)

        # Type 0x0016 (Bow): unsummoned
        state_bow = SpiritWarpState(ali_x=200, ali_y=350, spirit_type=0x0016)
        state_bow.reposition_spirit()
        self.assertFalse(state_bow.spirit_active)
        self.assertEqual(state_bow.spirit_type, 0)

        # Room spirit forbid flag
        state_forbid = SpiritWarpState(ali_x=200, ali_y=350, spirit_type=0x001C)
        state_forbid.reposition_spirit(room_forbids_spirit=True)
        self.assertFalse(state_forbid.spirit_active)

    def test_04_bgm_track_synchronization(self):
        """Verify BGM track continue (command 3) vs track change command."""
        # Same track: continue command 3
        state = SpiritWarpState()
        state.current_bgm = 0x12
        state.target_bgm = 0x12
        state.sync_bgm_track()
        self.assertEqual(state.sent_sound_cmd, 3)

        # Different track: change command
        state.target_bgm = 0x25
        state.sync_bgm_track()
        self.assertEqual(state.sent_sound_cmd, 0x25)
        self.assertEqual(state.current_bgm, 0x25)

    def test_05_ground_surface_latch_sweep(self):
        """Verify 21-entity pool sweep only latches active entities."""
        state = SpiritWarpState()
        entities = [{"active": (i % 2 == 0)} for i in range(25)]
        state.sweep_ground_surface(entities)
        # Only active entities within the first 21 slots (0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20)
        expected = [0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20]
        self.assertEqual(state.latched_entities, expected)


if __name__ == "__main__":
    unittest.main()
