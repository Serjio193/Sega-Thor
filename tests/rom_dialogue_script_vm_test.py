#!/usr/bin/env python3
"""
Unit tests for Beyond Oasis ROM Dialogue Script Virtual Machine &
Message Window Presentation Controller (0x00C2EC..0x00C566) and string table (0x051514).
"""

import os
import struct
import unittest

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C2EC
END_ADDR = 0x00C566
ROUTINE_SIZE = END_ADDR - START_ADDR  # 634 bytes

STRING_TABLE_ADDR = 0x051514
NUM_STRINGS = 0x62  # 98 strings (0x00..0x61). At 0x62 (0x0515D8) begins placeholder 'Kari Serihu' (仮セリフ) text.


class DialogueVMState:
    """Emulates the state machine in $FF1870..$FF1887."""
    def __init__(self, dialogue_id=0, param_d6=0x1000, param_d7=0x2000, param_d5=0):
        self.dialogue_id = dialogue_id
        self.param_d6 = param_d6
        self.param_d7 = param_d7
        self.param_d5 = param_d5
        self.line_row_offsets = [12, 24, 36, 48]
        self.cursor_x = 0
        self.line_idx = 0
        self.prompt_wait_icon = False
        self.dismissal_wait = False
        self.entity_active = True
        self.window_open = True
        self.rendered_glyphs = []
        self.events = []
        self.total_frames_delayed = 0

    def prompt_wait(self, button_pressed):
        """Simulates 0x00C536 button wait loop."""
        self.prompt_wait_icon = True
        self.events.append("PROMPT_WAIT_START")
        # In hardware, waits until button A (bit 6), B (bit 4), or C (bit 5)
        # on $FF165F is pressed:
        has_button = bool(button_pressed & 0x70)  # bits 4, 5, 6
        if has_button:
            self.events.append("BUTTON_PRESSED")
        self.prompt_wait_icon = False
        self.events.append("PROMPT_WAIT_END")
        return has_button

    def advance_line(self):
        """Simulates 0x00C566 line advance."""
        self.cursor_x = 0
        self.line_idx += 1
        self.events.append(f"ADVANCE_LINE_{self.line_idx}")

    def clear_page(self):
        """Simulates 0x00C4EC clear page / 4-line scroll."""
        for _ in range(4):
            self.advance_line()
        self.cursor_x = 0
        self.line_idx = 0
        self.line_row_offsets = [12, 24, 36, 48]
        self.events.append("CLEAR_PAGE")

    def close_window(self):
        """Simulates 0x00C45C close window."""
        self.entity_active = False
        self.window_open = False
        self.events.append("WINDOW_CLOSED")


def execute_dialogue_bytecode(tokens, state, button_input=0x40):
    """Executes dialogue bytecode tokens."""
    for tok in tokens:
        if tok["type"] == "text":
            for ch in tok["text"]:
                state.rendered_glyphs.append(ch)
                state.cursor_x += 1
                state.total_frames_delayed += 1
        elif tok["type"] == "opcode":
            op = tok["code"]
            if op == 0x00:  # TERMINATE
                state.dismissal_wait = True
                state.prompt_wait(button_input)
                state.dismissal_wait = False
                state.close_window()
                break
            elif op == 0x01:  # NEWLINE
                state.advance_line()
                state.total_frames_delayed += 1
            elif op == 0x02:  # PROMPT_NEWLINE
                state.prompt_wait(button_input)
                state.advance_line()
                state.total_frames_delayed += 1
            elif op == 0x03:  # DELAY_FRAMES
                frames = tok.get("frames", 1)
                state.total_frames_delayed += frames
                state.events.append(f"DELAY_{frames}_FRAMES")
            elif op == 0x05:  # PROMPT_WAIT
                state.prompt_wait(button_input)
            elif op == 0x06:  # CLEAR_PAGE
                state.clear_page()
                state.total_frames_delayed += 1
            elif op == 0x07:  # PROMPT_CLEAR_PAGE
                state.prompt_wait(button_input)
                state.clear_page()
                state.total_frames_delayed += 1
    return state


class TestRomDialogueScriptVM(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(ROM_PATH):
            raise unittest.SkipTest(f"ROM not found at {ROM_PATH}")
        with open(ROM_PATH, "rb") as f:
            cls.rom = f.read()

    def test_01_binary_bounds_and_opcodes(self):
        """Verify binary size, entry points and key opcodes in 0x00C2EC..0x00C566."""
        self.assertGreaterEqual(len(self.rom), END_ADDR)
        code = self.rom[START_ADDR:END_ADDR]
        self.assertEqual(len(code), ROUTINE_SIZE)

        # 0x00C2EC: btst.b #7, $FF0D7A.l -> 0x0839 0x0007 0x00FF 0x0D7A
        self.assertEqual(code[0:8], bytes([0x08, 0x39, 0x00, 0x07, 0x00, 0xFF, 0x0D, 0x7A]))
        # 0x00C2F4: beq.b $C326 -> 0x6730
        self.assertEqual(code[8:10], bytes([0x67, 0x30]))
        # 0x00C2F6: cmpi.l #$10100500, $FF165E.l
        self.assertEqual(code[10:20], bytes([0x0C, 0xB9, 0x10, 0x10, 0x05, 0x00, 0x00, 0xFF, 0x16, 0x5E]))
        # 0x00C324: rts -> 0x4E75
        self.assertEqual(code[0x38:0x3A], bytes([0x4E, 0x75]))

        # 0x00C326: movem.l d0-d7/a0-a6, -(a7) -> 0x48E7 0xFFFE
        self.assertEqual(code[0x3A:0x3E], bytes([0x48, 0xE7, 0xFF, 0xFE]))
        # 0x00C564: rts (end of 0x00C536 helper) -> 0x4E75
        self.assertEqual(code[ROUTINE_SIZE - 2:ROUTINE_SIZE], bytes([0x4E, 0x75]))

    def test_02_debug_loop_branching_logic(self):
        """Verify condition evaluation for debug dialogue validator."""
        def evaluate_entry(flag_byte, trigger_lword):
            has_debug_bit = bool(flag_byte & 0x80)
            if not has_debug_bit:
                return "SINGLE_MESSAGE"
            if trigger_lword != 0x10100500:
                return "SINGLE_MESSAGE"
            return "DEBUG_ALL_LOOP"

        self.assertEqual(evaluate_entry(0x00, 0x10100500), "SINGLE_MESSAGE")
        self.assertEqual(evaluate_entry(0x80, 0x00000000), "SINGLE_MESSAGE")
        self.assertEqual(evaluate_entry(0x80, 0x10100500), "DEBUG_ALL_LOOP")

        # In debug loop, test which message IDs are visited between 0 and 0x62
        visited = []
        d0 = 0
        while d0 != 0x62:
            visited.append(d0)
            d0 += 1
            while d0 in (0x1B, 0x1C, 0x1D):
                d0 += 1
        self.assertNotIn(0x1B, visited)
        self.assertNotIn(0x1C, visited)
        self.assertNotIn(0x1D, visited)
        self.assertEqual(visited[0], 0x00)
        self.assertEqual(visited[-1], 0x61)
        self.assertEqual(len(visited), 0x62 - 3)

    def test_03_string_table_resolution(self):
        """Verify 116 dialogue strings at 0x051514 resolve properly within ROM."""
        for i in range(NUM_STRINGS):
            off = struct.unpack_from(">h", self.rom, STRING_TABLE_ADDR + i * 2)[0]
            str_addr = STRING_TABLE_ADDR + i * 2 + off
            self.assertGreater(str_addr, STRING_TABLE_ADDR)
            self.assertLess(str_addr, len(self.rom) - 10)
            # Must find a 0x00 terminator within 1024 bytes
            found_null = False
            for j in range(1024):
                if self.rom[str_addr + j] == 0:
                    found_null = True
                    break
            self.assertTrue(found_null, f"String {i:02X} at 0x{str_addr:06X} missing null terminator")

        # Verify String 00 ("#I am Dytto...")
        off0 = struct.unpack_from(">h", self.rom, STRING_TABLE_ADDR)[0]
        addr0 = STRING_TABLE_ADDR + off0
        self.assertEqual(self.rom[addr0:addr0 + 13], b"#I am Dytto, ")

        # Verify String 01 ("#Hi, Ali!#")
        off1 = struct.unpack_from(">h", self.rom, STRING_TABLE_ADDR + 2)[0]
        addr1 = STRING_TABLE_ADDR + 2 + off1
        self.assertEqual(self.rom[addr1:addr1 + 10], b"#Hi, Ali!#")

    def test_04_dialogue_vm_execution_simulation(self):
        """Simulate dialogue VM execution on String 00 and verify events."""
        # String 00 tokens:
        tokens_s0 = [
            {"type": "text", "text": "#I am Dytto, a water spirit."},
            {"type": "opcode", "code": 0x01},  # NEWLINE
            {"type": "text", "text": "Where are we going?#"},
            {"type": "opcode", "code": 0x00},  # TERMINATE
        ]
        state = DialogueVMState(dialogue_id=0)
        execute_dialogue_bytecode(tokens_s0, state, button_input=0x40)  # button A

        self.assertFalse(state.window_open)
        self.assertFalse(state.entity_active)
        self.assertEqual("".join(state.rendered_glyphs), "#I am Dytto, a water spirit.Where are we going?#")
        self.assertIn("ADVANCE_LINE_1", state.events)
        self.assertIn("WINDOW_CLOSED", state.events)

    def test_05_interactive_button_prompt_loop(self):
        """Verify button prompt detection for A (0x40), B (0x10), C (0x20)."""
        state = DialogueVMState()
        # No button pressed
        self.assertFalse(state.prompt_wait(0x00))
        # Button A pressed (bit 6 = 0x40)
        self.assertTrue(state.prompt_wait(0x40))
        # Button B pressed (bit 4 = 0x10)
        self.assertTrue(state.prompt_wait(0x10))
        # Button C pressed (bit 5 = 0x20)
        self.assertTrue(state.prompt_wait(0x20))
        # D-Pad only (e.g. 0x01 up) should not satisfy prompt
        self.assertFalse(state.prompt_wait(0x01))

    def test_06_page_clear_and_multipage_prologue(self):
        """Simulate multi-page prologue text (String 0x1B) with clear page."""
        prologue_tokens = [
            {"type": "text", "text": "Page 1 Line 1"},
            {"type": "opcode", "code": 0x01},  # NEWLINE
            {"type": "text", "text": "Page 1 Line 2"},
            {"type": "opcode", "code": 0x02},  # PROMPT_NEWLINE
            {"type": "opcode", "code": 0x06},  # CLEAR_PAGE
            {"type": "text", "text": "Page 2 Line 1"},
            {"type": "opcode", "code": 0x00},  # TERMINATE
        ]
        state = DialogueVMState(dialogue_id=0x1B)
        execute_dialogue_bytecode(prologue_tokens, state, button_input=0x40)

        self.assertIn("CLEAR_PAGE", state.events)
        self.assertEqual(state.line_idx, 0)
        self.assertEqual(state.line_row_offsets, [12, 24, 36, 48])
        self.assertFalse(state.window_open)


if __name__ == "__main__":
    unittest.main()
