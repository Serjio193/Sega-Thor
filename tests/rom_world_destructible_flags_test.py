#!/usr/bin/env python3
"""
Unit tests for ROM Interactive/Destructible Enemy/Object & Core Persistent Flag
Bitfield Helpers (0x017E28..0x018072, Milestone M15.525).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_world_destructible_flags_m15_525.json"
)

BANK_BASE_ADDRS = {
    1: 0xFF0DBC,
    2: 0xFF0E1C,
    3: 0xFF0E2C,
}


def persistent_flag_set_bit(bank_id: int, flag_index: int, ram_banks: dict[int, bytearray]) -> bool:
    """Emulate PersistentFlag_SetBit (0x017FF2..0x018030)."""
    if bank_id not in BANK_BASE_ADDRS:
        return False
    base_addr = BANK_BASE_ADDRS[bank_id]
    byte_offset = (flag_index & 0xFFFF) >> 3
    bit_index = flag_index & 0x07
    ram_banks[base_addr][byte_offset] |= (1 << bit_index)
    return True


def persistent_flag_resolve_bit(bank_id: int, flag_index: int) -> tuple[int | None, int | None, bool]:
    """
    Emulate PersistentFlag_ResolveBit (0x018030..0x018072).
    Returns (byte_address, bit_index, carry_set).
    """
    if bank_id not in BANK_BASE_ADDRS:
        return None, None, True
    base_addr = BANK_BASE_ADDRS[bank_id]
    byte_offset = (flag_index & 0xFFFF) >> 3
    bit_index = flag_index & 0x07
    return base_addr + byte_offset, bit_index, False


def select_next_action(obj: dict, target_dist: int, screen_shake: int, prng_val: int) -> str:
    """Emulate action selector 0x017EF0..0x017F40."""
    if (target_dist & 0xFFFF) < 0x30:
        obj["state_4"] = 6
        obj["anim_2a"] = 6
        return "ENTER_STATE_6_ATTACK"

    if screen_shake != 0 or (prng_val & 0xFFFF) < 0x1A00:
        obj["state_4"] = 4
        obj["anim_2a"] = 4
        return "ENTER_STATE_4_REACT"

    obj["timer_2"] = 4
    obj["state_4"] = 0
    obj["anim_2a"] = 0
    return "ENTER_STATE_0_IDLE"


def attack_hit_callback_017fc0(target: dict) -> None:
    """Emulate attack hit callback 0x017FC0..0x017FD2."""
    target["beba_applied"] = True
    target["knockback_mode_84"] = 2
    target["vy_52"] = -target.get("vy_52", 0)


def step_destructible_enemy_object(
    obj: dict,
    target_dist: int = 100,
    screen_shake: int = 0,
    prng_val: int = 0x4000,
    hit_target: dict | None = None,
) -> str:
    """Emulate AI_WorldObject_DestructibleEnemyObject (0x017E28..0x017FF2)."""
    # First-tick initialization check (+$66)
    if obj.get("init_flag_66", 0) != 0:
        obj["facing_16"] = 0
        obj["init_flag_66"] = 0

    obj["called_e682"] = True

    # Incoming damage handling (+$86)
    dmg_word = obj.get("damage_86", 0) & 0xFFFF
    if dmg_word != 0:
        obj["damage_86"] = 0
        if (dmg_word & (1 << 13)) != 0:
            obj["flags_39"] = obj.get("flags_39", 0) | 0x08
            obj["sfx"] = 0x8004D9

        raw_dmg = dmg_word & 0x0FFF
        eff_dmg = raw_dmg - obj.get("defense_92", 0)
        if eff_dmg <= 0:
            eff_dmg = 1

        new_hp = obj.get("hp_8a", 0) - eff_dmg
        if new_hp < 0:
            new_hp = 0
        obj["hp_8a"] = new_hp

        if new_hp == 0:
            obj["state_4"] = 2
            obj["anim_8ef8"] = 2
            obj["timer_2"] = 0x10
            obj["flags_38"] = obj.get("flags_38", 0) | 0x08
            obj["voice_sfx"] = 0x900232
        else:
            obj["state_4"] = 8
            obj["anim_8ef8"] = 0x0A

    # Gravity integration when z > 0 (+$10)
    z_32 = obj.get("z_10", 0)
    if z_32 != 0:
        vz = obj.get("vz_56", 0) - 0xC000
        obj["vz_56"] = vz
        z_32 += vz
        if z_32 < 0:
            z_32 = 0
        obj["z_10"] = z_32

    # State dispatch via jump table 0x017EE6 (states 0, 2, 4, 6, 8)
    state = obj.get("state_4", 0)

    if state == 0:  # 0x017F40: Idle Wait
        if screen_shake != 0:
            return select_next_action(obj, target_dist, screen_shake, prng_val)
        obj["timer_2"] = obj.get("timer_2", 0) - 1
        if obj["timer_2"] <= 0:
            return select_next_action(obj, target_dist, screen_shake, prng_val)
        return "STATE_0_WAITING"

    elif state == 2:  # 0x017F50: Death Blink & Loot Drop
        obj["flags_36"] = obj.get("flags_36", 0) ^ 0x02  # bchg #1, $36(a6)
        obj["timer_2"] = obj.get("timer_2", 0) - 1
        if obj["timer_2"] > 0:
            return "STATE_2_BLINKING"
        obj["loot_table_addr"] = 0x017F6C
        obj["loot_spawned_f476"] = True
        obj["active_0"] = 0
        return "STATE_2_DESPAWNED_WITH_LOOT"

    elif state == 4:  # 0x017F74: Secondary Idle / Taunt Anim Wait
        if (obj.get("flags_37", 0) & 0x02) != 0:
            return select_next_action(obj, target_dist, screen_shake, prng_val)
        return "STATE_4_ANIMATING"

    elif state == 6:  # 0x017F80: Close-Range Attack
        frame_30 = obj.get("anim_frame_30", 0)
        if (frame_30 == 0x0C or frame_30 == 0x14) and obj.get("subtick_2e", 0) == 0:
            obj["voice_sfx"] = 0x800202

        obj["callback_ff1940"] = 0x017FC0
        obj["called_bce2"] = True
        if hit_target is not None:
            attack_hit_callback_017fc0(hit_target)

        if (obj.get("flags_37", 0) & 0x02) != 0:
            return select_next_action(obj, target_dist, screen_shake, prng_val)
        return "STATE_6_ATTACKING"

    elif state == 8:  # 0x017FD2: Hit / Flinch Recovery
        if (obj.get("flags_37", 0) & 0x02) != 0:
            obj["flags_36"] = obj.get("flags_36", 0) & ~0x40  # bclr #6
            obj["flags_39"] = obj.get("flags_39", 0) & ~0x08  # bclr #3
            return select_next_action(obj, target_dist, screen_shake, prng_val)
        obj["flags_36"] = obj.get("flags_36", 0) | 0x40  # bset #6
        return "STATE_8_FLINCHING"

    return "UNKNOWN_STATE"


class TestRomWorldDestructibleFlags(unittest.TestCase):
    """Test suite for Milestone M15.525 destructible enemy/object & persistent flag helpers."""

    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence file: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            self.evidence = json.load(f)

    def test_evidence_structure_and_embedded_tables(self):
        """Verify M15.525 metadata, routine boundaries, instruction counts, and embedded tables."""
        self.assertEqual(self.evidence["milestone"], "M15.525")
        self.assertEqual(self.evidence["range"]["start"], "0x017E28")
        self.assertEqual(self.evidence["range"]["end"], "0x018072")
        self.assertEqual(self.evidence["range"]["size_bytes"], 586)
        self.assertEqual(self.evidence["instruction_count"], 146)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 3)

        expected_routines = [
            ("AI_WorldObject_DestructibleEnemyObject", "0x017E28", "0x017FF2", 458, 107),
            ("PersistentFlag_SetBit", "0x017FF2", "0x018030", 62, 19),
            ("PersistentFlag_ResolveBit", "0x018030", "0x018072", 66, 20),
        ]

        for r, (name, start, end, sz, icnt) in zip(routines, expected_routines):
            self.assertEqual(r["name"], name)
            self.assertEqual(r["start"], start)
            self.assertEqual(r["end"], end)
            self.assertEqual(r["size_bytes"], sz)
            self.assertEqual(r["instruction_count"], icnt)

        # Verify jump table 0x017EE6
        jt = self.evidence["embedded_tables"]["state_jump_table_0x017ee6"]
        self.assertEqual(len(jt), 5)
        expected_targets = ["0x017F40", "0x017F50", "0x017F74", "0x017F80", "0x017FD2"]
        for idx, entry in enumerate(jt):
            self.assertEqual(entry["state_value"], idx * 2)
            self.assertEqual(entry["target_address"], expected_targets[idx])

        # Verify loot drop table 0x017F6C
        loot = self.evidence["embedded_tables"]["loot_drop_table_0x017f6c"]
        self.assertEqual(loot, ["0x00", "0x00", "0x1E", "0x01", "0x00", "0x00", "0x1E", "0x01"])

    def test_first_tick_init_and_gravity_clamping(self):
        """Verify first-tick +$66 reset and Z-axis gravity integration clamping at z = 0."""
        obj = {
            "init_flag_66": 1,
            "facing_16": 6,
            "state_4": 0,
            "timer_2": 5,
            "z_10": 0x10000,
            "vz_56": 0,
        }

        res = step_destructible_enemy_object(obj)
        self.assertEqual(res, "STATE_0_WAITING")
        self.assertEqual(obj["init_flag_66"], 0)
        self.assertEqual(obj["facing_16"], 0)
        self.assertEqual(obj["vz_56"], -0xC000)
        self.assertEqual(obj["z_10"], 0x4000)

        # Second step drops below 0 and clamps to 0
        res = step_destructible_enemy_object(obj)
        self.assertEqual(obj["z_10"], 0)
        self.assertEqual(obj["vz_56"], -0x18000)

    def test_damage_defense_clamping_and_flinch_state_8(self):
        """Test damage bit 13 flag/SFX, minimum 1 damage clamping against high defense, and State 8 recovery."""
        obj = {
            "hp_8a": 10,
            "defense_92": 25,
            "damage_86": 0x2000 | 10,  # Bit 13 set, raw damage 10 < defense 25 -> clamped to 1
            "state_4": 0,
            "flags_37": 0,
        }

        res = step_destructible_enemy_object(obj)
        self.assertEqual(res, "STATE_8_FLINCHING")
        self.assertEqual(obj["hp_8a"], 9)
        self.assertEqual(obj["state_4"], 8)
        self.assertEqual(obj["anim_8ef8"], 0x0A)
        self.assertEqual(obj["sfx"], 0x8004D9)
        self.assertNotEqual(obj["flags_39"] & 0x08, 0)
        self.assertNotEqual(obj["flags_36"] & 0x40, 0)

        # Finish flinch animation (+$37:1 set) -> clears flags and re-evaluates action
        obj["flags_37"] = 0x02
        res = step_destructible_enemy_object(obj, target_dist=80, screen_shake=0, prng_val=0x5000)
        self.assertEqual(res, "ENTER_STATE_0_IDLE")
        self.assertEqual(obj["flags_36"] & 0x40, 0)
        self.assertEqual(obj["flags_39"] & 0x08, 0)
        self.assertEqual(obj["state_4"], 0)
        self.assertEqual(obj["timer_2"], 4)

    def test_lethal_damage_and_state_2_death_loot_drop(self):
        """Test lethal hit transition to State 2, 16-frame visibility blink, loot spawn, and despawn."""
        obj = {
            "active_0": 1,
            "hp_8a": 15,
            "defense_92": 5,
            "damage_86": 0x8000 | 25,  # 25 - 5 = 20 >= 15 -> 0 HP
            "state_4": 0,
            "flags_36": 0,
        }

        # Frame 1: takes lethal damage, enters State 2, and executes first blink tick (16 -> 15)
        res = step_destructible_enemy_object(obj)
        self.assertEqual(res, "STATE_2_BLINKING")
        self.assertEqual(obj["hp_8a"], 0)
        self.assertEqual(obj["state_4"], 2)
        self.assertEqual(obj["anim_8ef8"], 2)
        self.assertEqual(obj["timer_2"], 15)
        self.assertNotEqual(obj["flags_38"] & 0x08, 0)
        self.assertEqual(obj["voice_sfx"], 0x900232)
        self.assertEqual(obj["flags_36"] & 0x02, 0x02)

        # Tick remaining 14 frames of blinking
        for _ in range(14):
            res = step_destructible_enemy_object(obj)
            self.assertEqual(res, "STATE_2_BLINKING")

        self.assertEqual(obj["timer_2"], 1)

        # 16th frame: timer reaches 0 -> calls 0x00F476 with 0x017F6C and despawns
        res = step_destructible_enemy_object(obj)
        self.assertEqual(res, "STATE_2_DESPAWNED_WITH_LOOT")
        self.assertTrue(obj["loot_spawned_f476"])
        self.assertEqual(obj["loot_table_addr"], 0x017F6C)
        self.assertEqual(obj["active_0"], 0)

    def test_action_selection_and_state_6_attack_callback(self):
        """Test action selector transitions to State 4 and State 6, attack SFX, and 0x017FC0 hit callback."""
        obj = {"state_4": 0, "timer_2": 10}

        # Screen shake interrupts State 0 immediately; distance >= 48 enters State 4
        res = step_destructible_enemy_object(obj, target_dist=60, screen_shake=4)
        self.assertEqual(res, "ENTER_STATE_4_REACT")
        self.assertEqual(obj["state_4"], 4)
        self.assertEqual(obj["anim_2a"], 4)

        # State 4 waits until animation finishes, then close distance (< 48) enters State 6
        obj["flags_37"] = 0x02
        res = step_destructible_enemy_object(obj, target_dist=32, screen_shake=0)
        self.assertEqual(res, "ENTER_STATE_6_ATTACK")
        self.assertEqual(obj["state_4"], 6)
        self.assertEqual(obj["anim_2a"], 6)

        # State 6 on frame 12 (0x0C) with subtick 0 plays SFX #$800202 and invokes hit callback 0x017FC0
        obj["flags_37"] = 0x00
        obj["anim_frame_30"] = 0x0C
        obj["subtick_2e"] = 0
        target = {"vy_52": 0x2800, "knockback_mode_84": 0}

        res = step_destructible_enemy_object(obj, hit_target=target)
        self.assertEqual(res, "STATE_6_ATTACKING")
        self.assertEqual(obj["voice_sfx"], 0x800202)
        self.assertEqual(obj["callback_ff1940"], 0x017FC0)
        self.assertTrue(target["beba_applied"])
        self.assertEqual(target["knockback_mode_84"], 2)
        self.assertEqual(target["vy_52"], -0x2800)

    def test_persistent_flag_set_and_resolve_helpers(self):
        """Test PersistentFlag_SetBit (0x017FF2) and PersistentFlag_ResolveBit (0x018030) across all banks."""
        ram_banks = {
            0xFF0DBC: bytearray(32),
            0xFF0E1C: bytearray(16),
            0xFF0E2C: bytearray(16),
        }

        # Valid banks 1, 2, 3
        test_cases = [
            (1, 0x0015, 0xFF0DBC + 2, 5),  # byte 2, bit 5
            (2, 0x002F, 0xFF0E1C + 5, 7),  # byte 5, bit 7
            (3, 0x0008, 0xFF0E2C + 1, 0),  # byte 1, bit 0
        ]

        for bank_id, flag_idx, exp_addr, exp_bit in test_cases:
            addr, bit, carry = persistent_flag_resolve_bit(bank_id, flag_idx)
            self.assertFalse(carry)
            self.assertEqual(addr, exp_addr)
            self.assertEqual(bit, exp_bit)

            base = BANK_BASE_ADDRS[bank_id]
            offset = exp_addr - base
            self.assertEqual(ram_banks[base][offset] & (1 << exp_bit), 0)
            self.assertTrue(persistent_flag_set_bit(bank_id, flag_idx, ram_banks))
            self.assertNotEqual(ram_banks[base][offset] & (1 << exp_bit), 0)

        # Invalid banks (0 and >= 4) must set Carry (C = 1) and no-op on set
        for invalid_bank in (0, 4, 5, 255):
            addr, bit, carry = persistent_flag_resolve_bit(invalid_bank, 0x0010)
            self.assertTrue(carry)
            self.assertIsNone(addr)
            self.assertIsNone(bit)
            self.assertFalse(persistent_flag_set_bit(invalid_bank, 0x0010, ram_banks))


if __name__ == "__main__":
    unittest.main()
