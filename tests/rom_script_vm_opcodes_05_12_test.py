#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Opcodes $05 & $12: Unconditional Jump &
Conditional Branch Engine (0x00FAAC..0x00FC6C, Milestone M15.465).
"""

import unittest

class ScriptVmOpcodes0512:
    @staticmethod
    def op_05_unconditional_branch(pc, displacement):
        """0x00FAAC: adda.w $2(a0), a0; rts"""
        return pc + displacement

    @staticmethod
    def op_12_conditional_branch(pc, mode, op, comp_val, branch_disp, env):
        """
        0x00FAB2: Evaluates condition and branches by branch_disp if true,
        or skips 8 bytes to pc + 8 if false.
        """
        # 1. Mode dispatch: resolve condition or value
        condition_met = False

        if mode == 0:  # Quest variable
            val = env.get('quest_var', 0)
            condition_met = ScriptVmOpcodes0512._eval_relational(op, val, comp_val)
        elif mode in (1, 2, 3):  # Persistent bitmask banks
            bank_key = f'bank{mode}'
            bit = comp_val & 7
            byte_idx = comp_val >> 3
            bit_is_set = (env.get(bank_key, {}).get(byte_idx, 0) & (1 << bit)) != 0
            # op == 0: branch if set; op != 0: branch if clear
            condition_met = bit_is_set if op == 0 else not bit_is_set
        elif mode == 7:  # Spirit status
            spirit_status = env.get('spirit_status', 0)
            # op == 0: branch if equal; op != 0: branch if not equal
            condition_met = (spirit_status == comp_val) if op == 0 else (spirit_status != comp_val)
        elif mode == 8:  # Ali X
            val = env.get('ali_x', 0)
            condition_met = ScriptVmOpcodes0512._eval_relational(op, val, comp_val)
        elif mode == 9:  # Ali Y
            val = env.get('ali_y', 0)
            condition_met = ScriptVmOpcodes0512._eval_relational(op, val, comp_val)
        elif mode == 10:  # Spirit X
            if env.get('spirit_status', 0) != 0x1A:
                condition_met = False  # Branches directly to not taken: 0x00FBA6
            else:
                val = env.get('spirit_x', 0)
                condition_met = ScriptVmOpcodes0512._eval_relational(op, val, comp_val)
        elif mode == 11:  # Spirit Y
            if env.get('spirit_status', 0) != 0x1A:
                condition_met = False
            else:
                val = env.get('spirit_y', 0)
                condition_met = ScriptVmOpcodes0512._eval_relational(op, val, comp_val)

        # 2. Branch Taken vs Not Taken
        if condition_met:
            # 0x00FBA0: adda.w $6(a0), a0
            return pc + 6 + branch_disp
        else:
            # 0x00FBA6: lea.l $8(a0), a0
            return pc + 8

    @staticmethod
    def _eval_relational(op, val, comp_val):
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


class TestScriptVmOpcodes0512(unittest.TestCase):
    def test_op_05_unconditional_branch(self):
        pc = 0x1000
        new_pc = ScriptVmOpcodes0512.op_05_unconditional_branch(pc, 0x0040)
        self.assertEqual(new_pc, 0x1040)

        # Negative branch backwards
        new_pc_back = ScriptVmOpcodes0512.op_05_unconditional_branch(pc, -0x0020)
        self.assertEqual(new_pc_back, 0x0FE0)

    def test_op_12_relational_quest_var(self):
        pc = 0x2000
        env = {'quest_var': 5}
        # Mode 0, op 0 (== 5): True -> branch by 0x30
        taken_pc = ScriptVmOpcodes0512.op_12_conditional_branch(
            pc, mode=0, op=0, comp_val=5, branch_disp=0x30, env=env
        )
        self.assertEqual(taken_pc, pc + 6 + 0x30)

        # Mode 0, op 0 (== 6): False -> not taken (pc + 8)
        not_taken_pc = ScriptVmOpcodes0512.op_12_conditional_branch(
            pc, mode=0, op=0, comp_val=6, branch_disp=0x30, env=env
        )
        self.assertEqual(not_taken_pc, pc + 8)

    def test_op_12_persistent_bitmasks(self):
        pc = 0x3000
        env = {'bank2': {1: 0x04}}  # Byte 1, bit 2 is set (bit index 10)
        # Check bit 10 set in Bank 2 (mode=2, op=0) -> True
        taken_pc = ScriptVmOpcodes0512.op_12_conditional_branch(
            pc, mode=2, op=0, comp_val=10, branch_disp=0x50, env=env
        )
        self.assertEqual(taken_pc, pc + 6 + 0x50)

        # Check bit 10 clear (op=1) -> False
        not_taken_pc = ScriptVmOpcodes0512.op_12_conditional_branch(
            pc, mode=2, op=1, comp_val=10, branch_disp=0x50, env=env
        )
        self.assertEqual(not_taken_pc, pc + 8)

    def test_op_12_spirit_status_and_coords(self):
        pc = 0x4000
        env_active = {'spirit_status': 0x1A, 'spirit_x': 320}
        env_inactive = {'spirit_status': 0x00, 'spirit_x': 320}

        # Active spirit: X > 300 (mode=10, op=2) -> True
        taken_pc = ScriptVmOpcodes0512.op_12_conditional_branch(
            pc, mode=10, op=2, comp_val=300, branch_disp=0x20, env=env_active
        )
        self.assertEqual(taken_pc, pc + 6 + 0x20)

        # Inactive spirit: always fails branch to pc + 8
        not_taken_pc = ScriptVmOpcodes0512.op_12_conditional_branch(
            pc, mode=10, op=2, comp_val=300, branch_disp=0x20, env=env_inactive
        )
        self.assertEqual(not_taken_pc, pc + 8)

if __name__ == '__main__':
    unittest.main()
