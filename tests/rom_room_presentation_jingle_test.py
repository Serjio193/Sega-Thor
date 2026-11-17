#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Room Entry Presentation Manager,
Palette Crossfader & Footstep Audio Jingle (0x00CD64..0x00CEEA, 390 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00CD64
END_ADDR = 0x00CEEA
ROUTINE_SIZE = END_ADDR - START_ADDR  # 390 bytes

TARGET_WHITE_PALETTE_BASE = 0x05D686
NUM_WHITE_PALETTE_WORDS = 64


class RoomPresentationState:
    """Emulates ROM presentation manager, palette effects, and audio jingle."""

    def __init__(self):
        # Hardware / RAM flags
        self.dma_busy_164e = 0
        self.cram_dirty_164d = 0
        self.room_flag_16f1 = 0
        self.room_flag_164f = 0
        self.special_fade_mode_17bf = 0
        self.warp_type_17be = 0
        self.sfx_trigger_17bb = 0
        self.sfx_suppress_17bc = 0
        self.fade_timer_1896 = 0
        self.saved_sp_1930 = 0x00FFFE00
        self.current_sp = 0x00FFFD80

        # Memory buffers
        self.cram_buffer_134c = [0x0222 for _ in range(64)]  # 64 words (128 bytes)
        self.palette_scratch_18a6 = [0 for _ in range(64)]
        self.source_palette = [0x0444 + i for i in range(32)]
        self.dest_palette_line1_138c = [0 for _ in range(32)]

        # Execution tracking
        self.sound_commands = []
        self.wait_frame_delays = []
        self.crossfade_steps = []
        self.called_instant_commit_2db6 = False
        self.called_standard_fade_2d84 = False
        self.jumped_to_gameplay_8aa6 = False

    def play_footsteps_jingle(self):
        """Emulates 0x00CE72..0x00CECC: 4-step footstep audio jingle."""
        sequence = [
            (0x0090038E, 14),
            (0x0090038F, 14),
            (0x0090038E, 14),
            (0x0090038F, 14),
        ]
        for snd_id, frames in sequence:
            self.sound_commands.append(snd_id)
            self.wait_frame_delays.append(frames)

    def copy_cram_palette_line1(self, source_words):
        """Emulates 0x00CECC..0x00CEEA: copies 32 words from source to $FF138C."""
        assert len(source_words) >= 32
        for i in range(32):
            self.dest_palette_line1_138c[i] = source_words[i]

    def run_presentation_manager(self):
        """Emulates 0x00CD64..0x00CE72: room entry presentation director."""
        # Polling loop: wait until DMA/fade flag 164E clears
        while self.dma_busy_164e != 0:
            self.dma_busy_164e = 0

        # Mode branch on special_fade_mode_17bf
        if self.special_fade_mode_17bf != 0:
            if self.special_fade_mode_17bf == 1:
                # Whiteout flash (0x00CDEC)
                self.special_fade_mode_17bf = 0
                for i in range(64):
                    self.cram_buffer_134c[i] = 0x0EEE
                self.cram_dirty_164d |= 0x01
                self.cram_dirty_164d |= 0x02
                # Acknowledge / clear bit 1
                self.cram_dirty_164d &= ~0x02
            else:
                # Smooth 33-step crossfade (0x00CDA8)
                self.special_fade_mode_17bf = 0
                self.palette_scratch_18a6 = list(self.cram_buffer_134c)
                for step in range(33):
                    self.crossfade_steps.append(step)
                    # jsr $302E: interpolate scratch buffer toward target white palette
                    alpha = step / 32.0
                    for c in range(64):
                        # Genesis RGB interpolation mock
                        orig = self.palette_scratch_18a6[c]
                        self.cram_buffer_134c[c] = orig  # placeholder for mock
        else:
            # Check warp type 17BE
            if self.warp_type_17be == 0:
                # Standard fade-in (0x00CE22)
                self.fade_timer_1896 = 0
                self.called_standard_fade_2d84 = True
            else:
                # Instant warp with footstep jingle (0x00CD7E)
                self.fade_timer_1896 = 4
                self.called_instant_commit_2db6 = True
                self.play_footsteps_jingle()
                # Wait until bit 4 of 164D clears
                self.cram_dirty_164d &= ~0x10

        # Post-fade setup (0x00CE2E)
        if (self.room_flag_16f1 & 0x02) != 0:
            self.room_flag_164f = 0xFF

        # Trigger warp chime SFX if requested and not suppressed
        if self.sfx_trigger_17bb != 0 and self.sfx_suppress_17bc == 0:
            self.sound_commands.append(0x00800230)

        # Clear suppress flag if set
        if self.sfx_suppress_17bc != 0:
            self.sfx_suppress_17bc = 0

        # Restore stack pointer and branch to gameplay loop
        self.current_sp = self.saved_sp_1930
        self.jumped_to_gameplay_8aa6 = True


class TestRomRoomPresentationJingle(unittest.TestCase):
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

        # 0x00CD64: tst.b $FF164E -> 4A 39 00 FF 16 4E
        self.assertEqual(self.routine_bytes[0:6], bytes([0x4A, 0x39, 0x00, 0xFF, 0x16, 0x4E]))

        # 0x00CE68: movea.l $FF1930, a7 -> 2E 79 00 FF 19 30
        off_sp = 0x00CE68 - START_ADDR
        self.assertEqual(self.routine_bytes[off_sp:off_sp + 6], bytes([0x2E, 0x79, 0x00, 0xFF, 0x19, 0x30]))

        # 0x00CE6E: bra.w $8AA6 -> 60 00 BC 36
        off_bra = 0x00CE6E - START_ADDR
        self.assertEqual(self.routine_bytes[off_bra:off_bra + 4], bytes([0x60, 0x00, 0xBC, 0x36]))

        # 0x00CE72: movem.l d0, -(a7) -> 48 E7 80 00
        off_jingle = 0x00CE72 - START_ADDR
        self.assertEqual(self.routine_bytes[off_jingle:off_jingle + 4], bytes([0x48, 0xE7, 0x80, 0x00]))

        # 0x00CECC: movem.l d0/a0-a1, -(a7) -> 48 E7 80 C0
        off_copy = 0x00CECC - START_ADDR
        self.assertEqual(self.routine_bytes[off_copy:off_copy + 4], bytes([0x48, 0xE7, 0x80, 0xC0]))

    def test_02_whiteout_flash_mode(self):
        """Verify whiteout flash ($FF17BF == 1) sets CRAM buffer to pure white."""
        state = RoomPresentationState()
        state.special_fade_mode_17bf = 1

        state.run_presentation_manager()

        self.assertEqual(state.special_fade_mode_17bf, 0)
        self.assertTrue(all(c == 0x0EEE for c in state.cram_buffer_134c))
        self.assertEqual(state.cram_dirty_164d & 0x01, 1)
        self.assertTrue(state.jumped_to_gameplay_8aa6)
        self.assertEqual(state.current_sp, state.saved_sp_1930)

    def test_03_palette_crossfade_mode(self):
        """Verify 33-step palette crossfade mode ($FF17BF > 1)."""
        state = RoomPresentationState()
        state.special_fade_mode_17bf = 2

        state.run_presentation_manager()

        self.assertEqual(state.special_fade_mode_17bf, 0)
        self.assertEqual(len(state.crossfade_steps), 33)
        self.assertEqual(state.crossfade_steps[0], 0)
        self.assertEqual(state.crossfade_steps[-1], 32)
        self.assertTrue(state.jumped_to_gameplay_8aa6)

    def test_04_standard_fade_and_instant_warp(self):
        """Verify standard fade vs instant warp with footsteps."""
        # 4a: Standard fade
        s1 = RoomPresentationState()
        s1.special_fade_mode_17bf = 0
        s1.warp_type_17be = 0
        s1.run_presentation_manager()
        self.assertTrue(s1.called_standard_fade_2d84)
        self.assertFalse(s1.called_instant_commit_2db6)
        self.assertEqual(len(s1.sound_commands), 0)

        # 4b: Instant warp with footsteps
        s2 = RoomPresentationState()
        s2.special_fade_mode_17bf = 0
        s2.warp_type_17be = 1
        s2.sfx_trigger_17bb = 1
        s2.room_flag_16f1 = 0x02  # bit 1 set
        s2.run_presentation_manager()
        self.assertTrue(s2.called_instant_commit_2db6)
        self.assertEqual(s2.fade_timer_1896, 4)
        self.assertEqual(s2.room_flag_164f, 0xFF)
        # Should have 4 footsteps + 1 exit chime (0x00800230)
        self.assertEqual(len(s2.sound_commands), 5)
        self.assertEqual(s2.sound_commands[-1], 0x00800230)

    def test_05_footstep_audio_jingle_and_cram_copy(self):
        """Verify footstep audio sequence and CRAM palette line copy."""
        state = RoomPresentationState()
        state.play_footsteps_jingle()

        # 4 steps alternating $8E and $8F, each with 14 frame delay
        expected_sounds = [0x0090038E, 0x0090038F, 0x0090038E, 0x0090038F]
        self.assertEqual(state.sound_commands, expected_sounds)
        self.assertEqual(state.wait_frame_delays, [14, 14, 14, 14])

        # Test CRAM palette copy
        test_words = [0x0123 + i for i in range(32)]
        state.copy_cram_palette_line1(test_words)
        self.assertEqual(state.dest_palette_line1_138c, test_words)


if __name__ == "__main__":
    unittest.main()
