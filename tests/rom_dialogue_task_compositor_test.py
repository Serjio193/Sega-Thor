#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Dialogue Entity Sprite Compositor,
Prompt Arrow Indicator & Frame Sync Engine (0x00C756..0x00C88C, 310 bytes).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C756
END_ADDR = 0x00C88C
ROUTINE_SIZE = END_ADDR - START_ADDR  # 310 bytes


class SatCompositorState:
    """Emulates dialogue entity sprite composition into SAT buffer."""
    def __init__(self, top_base=0x2000, bottom_base=0x3000, initial_link=0):
        self.plane_top_base = top_base
        self.plane_bottom_base = bottom_base
        self.row_offsets = [12, 24, 36, 48]
        self.line_counter = 0
        self.prompt_active = False
        self.prompt_dismiss = False
        self.anim_timer_bit4 = True  # Flashing visibility bit
        self.sat_pieces = []
        self.sat_link = initial_link

    def emit_sat_piece(self, y, size_code, tile_attr, x):
        """Emulates 0x00C838."""
        if self.sat_link >= 0x50:  # 80 sprites hardware limit
            return False
        self.sat_link += 1
        link_size = (self.sat_link + size_code) & 0xFFFF
        piece = {
            "y": y,
            "link_size": link_size,
            "tile": tile_attr,
            "x": x,
        }
        self.sat_pieces.append(piece)
        return True

    def emit_line_sprites(self, base_tile, row_offset):
        """Emulates 0x00C81C."""
        x = 0x00A0  # 160
        y = row_offset + 0x0110  # row + 272
        tile = base_tile
        for _ in range(8):
            self.emit_sat_piece(y, 0x0C00, tile, x)
            x += 0x20  # +32 px
            tile = (tile + 4) & 0xFFFF

    def composite_dialogue_sprites(self):
        """Emulates 0x00C756."""
        # 1. Prompt arrow
        if self.prompt_active and self.anim_timer_bit4:
            tile = 0x86DC
            if self.prompt_dismiss:
                tile |= 0x1000  # bset #12 (0x96DC)
            x = 0x011C  # 284
            y = 0x014C  # 332
            if self.line_counter < 3:
                y -= 0x0C
                if self.line_counter < 2:
                    y -= 0x0C
                    if self.line_counter < 1:
                        y -= 0x0C
            self.emit_sat_piece(y, 0x0000, tile, x)

        # 2. Window text lines
        top_tile = ((self.plane_top_base >> 5) | 0x8000) & 0xFFFF
        self.emit_line_sprites(top_tile, self.row_offsets[0])
        self.emit_line_sprites(top_tile, self.row_offsets[1])

        bottom_tile = ((self.plane_bottom_base >> 5) | 0x8000) & 0xFFFF
        self.emit_line_sprites(bottom_tile, self.row_offsets[2])
        self.emit_line_sprites(bottom_tile, self.row_offsets[3])


class TestRomDialogueTaskCompositor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(ROM_PATH):
            raise unittest.SkipTest(f"ROM not found at {ROM_PATH}")
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_01_binary_bounds_and_opcodes(self):
        """Verify binary size, opcodes, and subroutine returns."""
        self.assertGreaterEqual(len(self.rom), END_ADDR)
        code = self.rom[START_ADDR:END_ADDR]
        self.assertEqual(len(code), ROUTINE_SIZE)

        # 0x00C756: movem.l d7/a6, -(a7)
        self.assertEqual(code[0:4], bytes([0x48, 0xE7, 0x01, 0x02]))
        # 0x00C77E: move.w #$86DC, d3
        self.assertEqual(code[0x28:0x2C], bytes([0x36, 0x3C, 0x86, 0xDC]))
        # 0x00C7C0: move.w #$0C00, d4
        self.assertEqual(code[0x6A:0x6E], bytes([0x38, 0x3C, 0x0C, 0x00]))
        # 0x00C81A: rts
        self.assertEqual(code[0xC4:0xC6], bytes([0x4E, 0x75]))
        # 0x00C836: rts
        self.assertEqual(code[0xE0:0xE2], bytes([0x4E, 0x75]))
        # 0x00C84C: rts
        self.assertEqual(code[0xF6:0xF8], bytes([0x4E, 0x75]))
        # 0x00C88A: rts
        self.assertEqual(code[ROUTINE_SIZE - 2:ROUTINE_SIZE], bytes([0x4E, 0x75]))

    def test_02_prompt_arrow_flashing_and_dismiss_tiles(self):
        """Verify prompt arrow visibility toggle and continue vs dismiss tile."""
        # Inactive prompt
        state_inactive = SatCompositorState()
        state_inactive.prompt_active = False
        state_inactive.composite_dialogue_sprites()
        self.assertEqual(len(state_inactive.sat_pieces), 32)  # only the 32 line pieces

        # Active prompt, flashing ON, continue prompt
        state_continue = SatCompositorState()
        state_continue.prompt_active = True
        state_continue.prompt_dismiss = False
        state_continue.anim_timer_bit4 = True
        state_continue.composite_dialogue_sprites()
        self.assertEqual(len(state_continue.sat_pieces), 33)
        arrow_piece = state_continue.sat_pieces[0]
        self.assertEqual(arrow_piece["tile"], 0x86DC)
        self.assertEqual(arrow_piece["x"], 284)

        # Active prompt, dismiss mode
        state_dismiss = SatCompositorState()
        state_dismiss.prompt_active = True
        state_dismiss.prompt_dismiss = True
        state_dismiss.anim_timer_bit4 = True
        state_dismiss.composite_dialogue_sprites()
        self.assertEqual(state_dismiss.sat_pieces[0]["tile"], 0x96DC)

        # Active prompt, flashing OFF (timer bit 4 is 0)
        state_off = SatCompositorState()
        state_off.prompt_active = True
        state_off.anim_timer_bit4 = False
        state_off.composite_dialogue_sprites()
        self.assertEqual(len(state_off.sat_pieces), 32)

    def test_03_prompt_arrow_dynamic_y_coordinates(self):
        """Verify prompt arrow Y position scales with active line counter."""
        expected_y = {
            3: 332,
            2: 320,  # 332 - 12
            1: 308,  # 332 - 24
            0: 296,  # 332 - 36
        }
        for line, exp_y in expected_y.items():
            state = SatCompositorState()
            state.prompt_active = True
            state.line_counter = line
            state.composite_dialogue_sprites()
            self.assertEqual(state.sat_pieces[0]["y"], exp_y, f"Mismatch for line {line}")

    def test_04_window_lines_piece_layout(self):
        """Verify 32 pieces emitted across 4 lines with correct step offsets."""
        state = SatCompositorState(top_base=0x2000, bottom_base=0x4000)
        state.composite_dialogue_sprites()
        self.assertEqual(len(state.sat_pieces), 32)

        # Check line 0 pieces (first 8 pieces)
        line0 = state.sat_pieces[0:8]
        top_tile_base = ((0x2000 >> 5) | 0x8000) & 0xFFFF
        for i, piece in enumerate(line0):
            self.assertEqual(piece["y"], 12 + 272)
            self.assertEqual(piece["x"], 160 + i * 32)
            self.assertEqual(piece["tile"], top_tile_base + i * 4)

    def test_05_sat_overflow_protection(self):
        """Verify SAT emitter drops sprites when hardware link reaches 80."""
        state = SatCompositorState(initial_link=78)
        state.composite_dialogue_sprites()
        # Should only emit 2 pieces (79, 80) then drop the remaining 30 pieces
        self.assertEqual(len(state.sat_pieces), 2)
        self.assertEqual(state.sat_link, 80)


if __name__ == "__main__":
    unittest.main()
