#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Opcodes $02..$04: Parameter Initializers &
Relational Wait-Condition Engine (0x00F920..0x00FAAC, Milestone M15.464).
"""

import unittest

class ScriptVmOpcodes0204:
    @staticmethod
    def op_02_set_param_buffer(stream):
        """0x00F920: Copies up to 16 words into $FF17C6 until 0xFFFF."""
        buffer = []
        for w in stream:
            if w == 0xFFFF:
                break
            if len(buffer) < 16:
                buffer.append(w)
        return buffer

    @staticmethod
    def op_03_set_coord_table(stream):
        """0x00F942: Copies up to 16 word pairs into $FF17E6. Fills rest with 0xFFFF."""
        table = []
        i = 0
        while i < len(stream) and len(table) < 16:
            w1 = stream[i]
            if w1 == 0xFFFF:
                break
            w2 = stream[i + 1] if i + 1 < len(stream) else 0xFFFF
            table.append((w1, w2))
            i += 2
        # Fill remaining with 0xFFFF
        while len(table) < 16:
            table.append((0xFFFF, 0xFFFF))
        return table

    @staticmethod
    def op_04_wait_condition(var_src, op, comp_val, state):
        """
        0x00F96A: Evaluates wait condition.
        Returns True if condition met (advances), False if yields.
        """
        # Resolve variable value
        val = 0
        if var_src == 1:  # Bank 1 bits: $FF0DBC
            bit = comp_val & 7
            byte_idx = comp_val >> 3
            bit_is_set = (state.get('bank1', {}).get(byte_idx, 0) & (1 << bit)) != 0
            # Polarity: op == 0 expects bit set; op != 0 expects bit clear
            return bit_is_set if op == 0 else not bit_is_set
        elif var_src == 2:  # Bank 2 bits: $FF0E1C
            bit = comp_val & 7
            byte_idx = comp_val >> 3
            bit_is_set = (state.get('bank2', {}).get(byte_idx, 0) & (1 << bit)) != 0
            return bit_is_set if op == 0 else not bit_is_set
        elif var_src == 3:  # Bank 3 bits: $FF0E2C
            bit = comp_val & 7
            byte_idx = comp_val >> 3
            bit_is_set = (state.get('bank3', {}).get(byte_idx, 0) & (1 << bit)) != 0
            return bit_is_set if op == 0 else not bit_is_set
        elif var_src == 4:
            val = state.get('ali_x', 0)
        elif var_src == 5:
            val = state.get('ali_y', 0)
        elif var_src == 6:
            if state.get('spirit_status', 0) == 0x1A:
                val = state.get('spirit_x', 0)
        elif var_src == 7:
            if state.get('spirit_status', 0) == 0x1A:
                val = state.get('spirit_y', 0)

        # Relational operators against comp_val
        if op == 0:    # ==
            return val == comp_val
        elif op == 1:  # !=
            return val != comp_val
        elif op == 2:  # >
            return val > comp_val
        elif op == 3:  # >=
            return val >= comp_val
        elif op == 4:  # <=
            return val <= comp_val
        elif op == 5:  # <
            return val < comp_val
        return False


class TestScriptVmOpcodes0204(unittest.TestCase):
    def test_op_02_parameter_buffer(self):
        stream = [10, 20, 30, 0xFFFF, 40]
        buf = ScriptVmOpcodes0204.op_02_set_param_buffer(stream)
        self.assertEqual(buf, [10, 20, 30])

        stream_long = list(range(20)) + [0xFFFF]
        buf_long = ScriptVmOpcodes0204.op_02_set_param_buffer(stream_long)
        self.assertEqual(len(buf_long), 16)

    def test_op_03_coordinate_table(self):
        stream = [100, 200, 300, 400, 0xFFFF]
        table = ScriptVmOpcodes0204.op_03_set_coord_table(stream)
        self.assertEqual(len(table), 16)
        self.assertEqual(table[0], (100, 200))
        self.assertEqual(table[1], (300, 400))
        self.assertEqual(table[2], (0xFFFF, 0xFFFF))
        self.assertEqual(table[15], (0xFFFF, 0xFFFF))

    def test_op_04_persistent_bitmask_conditions(self):
        state = {'bank1': {2: 0x08}}  # Byte 2, bit 3 is set (bit index 2*8 + 3 = 19)
        # Check bit 19 is set (polarity op = 0)
        self.assertTrue(ScriptVmOpcodes0204.op_04_wait_condition(1, 0, 19, state))
        # Check bit 19 is clear (polarity op = 1) -> False
        self.assertFalse(ScriptVmOpcodes0204.op_04_wait_condition(1, 1, 19, state))
        # Check bit 18 is set -> False
        self.assertFalse(ScriptVmOpcodes0204.op_04_wait_condition(1, 0, 18, state))

    def test_op_04_relational_comparisons(self):
        state = {'ali_x': 250, 'ali_y': 180, 'spirit_status': 0x1A, 'spirit_x': 260}
        # Ali X >= 200 -> True
        self.assertTrue(ScriptVmOpcodes0204.op_04_wait_condition(4, 3, 200, state))
        # Ali X == 250 -> True
        self.assertTrue(ScriptVmOpcodes0204.op_04_wait_condition(4, 0, 250, state))
        # Ali X < 250 -> False (yields)
        self.assertFalse(ScriptVmOpcodes0204.op_04_wait_condition(4, 5, 250, state))

        # Spirit X > 250 (spirit active) -> True
        self.assertTrue(ScriptVmOpcodes0204.op_04_wait_condition(6, 2, 250, state))
        # Inactive spirit -> val = 0, spirit X > 250 -> False
        state_inactive = {'ali_x': 250, 'spirit_status': 0x00, 'spirit_x': 260}
        self.assertFalse(ScriptVmOpcodes0204.op_04_wait_condition(6, 2, 250, state_inactive))

if __name__ == '__main__':
    unittest.main()
