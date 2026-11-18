#!/usr/bin/env python3
"""
Unit tests for ROM Script Virtual Machine Opcodes $0F..$11, $13, $14 & State Query Subroutines:
Persistent Modifiers, GOSUB & Ali Vital Metrics (0x010140..0x01027C, Milestone M15.470).
"""

import unittest

class ScriptControlStateEngine:
    def __init__(self):
        # 3 persistent banks (32 bytes each)
        self.bank1 = [0] * 32  # $FF0DBC
        self.bank2 = [0] * 32  # $FF0E1C
        self.bank3 = [0] * 32  # $FF0E2C
        self.return_pc = 0     # $FF1836
        self.script_pc = 0x1000

        # Pool 2 entity slots (17 slots, each with type and flags)
        self.pool2_slots = [{'type': 0, 'flags_3a': 0} for _ in range(17)]

        # Ali vitals
        self.ali_max_hp = 100  # $FF1A70
        self.ali_cur_hp = 100  # $FF1A72
        self.ali_max_sp = 100  # $FF1A74
        self.ali_cur_sp = 100  # $FF1A76

    def _modify_bit(self, bank, bit_idx, mode):
        byte_idx = bit_idx >> 3
        bit_num = bit_idx & 7
        if byte_idx < len(bank):
            if mode == 0:  # Set
                bank[byte_idx] |= (1 << bit_num)
            elif mode == 1:  # Clear
                bank[byte_idx] &= ~(1 << bit_num)
            else:  # Toggle (bchg)
                bank[byte_idx] ^= (1 << bit_num)

    def op_0f_bank1_bit(self, mode, bit_idx):
        self._modify_bit(self.bank1, bit_idx, mode)
        self.script_pc += 4

    def op_10_bank2_bit(self, mode, bit_idx):
        self._modify_bit(self.bank2, bit_idx, mode)
        self.script_pc += 4

    def op_11_bank3_bit(self, mode, bit_idx):
        self._modify_bit(self.bank3, bit_idx, mode)
        self.script_pc += 4

    def op_13_gosub(self, target_address):
        # In M68K: instruction is 4 bytes (+0=op, +1..+3=target 24-bit).
        # a0 is advanced to return address (+4), then saved to $FF1836.
        self.return_pc = self.script_pc + 4
        self.script_pc = target_address

    def op_14_return(self):
        self.script_pc = self.return_pc

    def count_living_enemies(self):
        count = 0
        for slot in self.pool2_slots:
            if slot['type'] > 0 and (slot['flags_3a'] & 1) == 0:
                count += 1
        return count

    def count_active_entities(self):
        count = 0
        for slot in self.pool2_slots:
            if slot['type'] > 0:
                count += 1
        return count

    def find_entity_type(self, target_type):
        for slot in self.pool2_slots:
            if slot['type'] == target_type:
                return True
        return False

    def calc_ali_hp_percent(self):
        if self.ali_max_hp == 0:
            return 0
        return (self.ali_cur_hp * 100) // self.ali_max_hp

    def calc_ali_sp_percent(self):
        if self.ali_max_sp == 0:
            return 0
        return (self.ali_cur_sp * 100) // self.ali_max_sp


class TestScriptControlStateEngine(unittest.TestCase):
    def test_op_0f_10_11_bit_modifiers(self):
        engine = ScriptControlStateEngine()

        # Bank 1: set bit 19 (byte 2, bit 3)
        engine.op_0f_bank1_bit(mode=0, bit_idx=19)
        self.assertEqual(engine.bank1[2], 1 << 3)
        self.assertEqual(engine.script_pc, 0x1004)

        # Bank 1: toggle bit 19 (should clear)
        engine.op_0f_bank1_bit(mode=2, bit_idx=19)
        self.assertEqual(engine.bank1[2], 0)
        self.assertEqual(engine.script_pc, 0x1008)

        # Bank 2: set and clear bit 7
        engine.op_10_bank2_bit(mode=0, bit_idx=7)
        self.assertEqual(engine.bank2[0], 0x80)
        engine.op_10_bank2_bit(mode=1, bit_idx=7)
        self.assertEqual(engine.bank2[0], 0x00)

        # Bank 3: toggle bit 0
        engine.op_11_bank3_bit(mode=2, bit_idx=0)
        self.assertEqual(engine.bank3[0], 1)
        engine.op_11_bank3_bit(mode=2, bit_idx=0)
        self.assertEqual(engine.bank3[0], 0)

    def test_op_13_14_gosub_return(self):
        engine = ScriptControlStateEngine()
        engine.script_pc = 0x2000

        # Gosub to subroutine 0x3500
        engine.op_13_gosub(0x3500)
        self.assertEqual(engine.return_pc, 0x2004)
        self.assertEqual(engine.script_pc, 0x3500)

        # Simulate execution in subroutine...
        engine.script_pc = 0x3520

        # Return from subroutine
        engine.op_14_return()
        self.assertEqual(engine.script_pc, 0x2004)

    def test_entity_counters_and_search(self):
        engine = ScriptControlStateEngine()
        # Slot 0: living enemy type 0x16
        engine.pool2_slots[0] = {'type': 0x16, 'flags_3a': 0}
        # Slot 1: dying enemy type 0x18 (flag bit 0 set)
        engine.pool2_slots[1] = {'type': 0x18, 'flags_3a': 1}
        # Slot 2: living enemy type 0x20
        engine.pool2_slots[2] = {'type': 0x20, 'flags_3a': 0}
        # Remaining slots empty (type 0)

        self.assertEqual(engine.count_living_enemies(), 2)
        self.assertEqual(engine.count_active_entities(), 3)
        self.assertTrue(engine.find_entity_type(0x18))
        self.assertFalse(engine.find_entity_type(0x55))

    def test_ali_vital_percentage(self):
        engine = ScriptControlStateEngine()
        engine.ali_max_hp = 120
        engine.ali_cur_hp = 60
        self.assertEqual(engine.calc_ali_hp_percent(), 50)

        engine.ali_cur_hp = 90
        self.assertEqual(engine.calc_ali_hp_percent(), 75)

        engine.ali_max_sp = 200
        engine.ali_cur_sp = 50
        self.assertEqual(engine.calc_ali_sp_percent(), 25)

if __name__ == '__main__':
    unittest.main()
