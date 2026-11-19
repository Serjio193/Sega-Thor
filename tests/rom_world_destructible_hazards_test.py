#!/usr/bin/env python3
"""
Unit tests for ROM Interactive World Objects: Timed Switch, Destructible Block,
Periodic Pop-Up Hazard & Periodic Toggle/Vent Hazard
(0x016EBC..0x0171C4, Milestone M15.520).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_world_destructible_hazards_m15_520.json"
)


def step_timed_switch(obj, memory_byte, bit_idx):
    """Emulate AI_WorldObject_TimedSwitch (0x016EBC..0x016F10)."""
    if not obj.get("init_done_a6", False):
        obj["init_done_a6"] = True
        obj["z"] = obj.get("z_8c", 0)
        mode = obj.get("mode_9e", 0)
        if mode == 0:
            obj["height_4a"] = 16
        elif mode == 1:
            obj["flags_3c"] = obj.get("flags_3c", 0) | 0x01
        return "INITIALIZED"

    if obj.get("state_66", 0) == 0:
        return "IDLE_UNTRIGGERED"

    obj["timer_2"] = obj.get("timer_2", 0) + 1
    if obj["timer_2"] >= 6:
        obj["active_0"] = 0
        memory_byte[0] |= (1 << bit_idx)
        return "DESPAWNED_TRIGGERED"

    return "COUNTING"


def step_destructible_block(obj, memory_byte, bit_idx):
    """Emulate AI_WorldObject_Destructible (0x016F10..0x016F7C)."""
    if not obj.get("init_done_a6", False):
        obj["init_done_a6"] = True
        obj["z"] = obj.get("z_8c", 0)
        hp = obj.get("max_hp_a0", 1)
        obj["hp_8a"] = hp
        obj["max_hp_88"] = hp
        return "INITIALIZED", False

    # Cooldown check
    cooldown = obj.get("guard_82", 0)
    if cooldown != 0:
        obj["guard_82"] = cooldown - 1
        return "IN_COOLDOWN", False

    damage = obj.get("damage_86", 0)
    if damage == 0:
        return "NO_DAMAGE", False

    obj["damage_86"] = 0
    obj["guard_82"] = 6
    masked_dmg = damage & 0x0FFF
    obj["hp_8a"] = obj.get("hp_8a", 0) - masked_dmg

    if obj["hp_8a"] > 0:
        return "SURVIVED_HIT", False

    # Destroyed!
    memory_byte[0] |= (1 << bit_idx)
    obj["active_0"] = 0
    shatter_effect = False
    if obj.get("mode_9e", 0) != 0:
        obj["z"] = obj.get("z", 0) + 24
        shatter_effect = True
    return "DESTROYED", shatter_effect


def step_pop_up_hazard(obj, memory_byte, bit_idx, ali=None):
    """Emulate AI_WorldObject_PeriodicPopUpHazard (0x016F7C..0x0170E0)."""
    state = obj.get("state_66", 0)

    if state == 0:
        obj["state_66"] = 1
        obj["timer_2"] = obj.get("timer_a0", 30)
        return "INITIALIZED", None

    elif state == 1:
        # Hidden countdown
        timer = obj.get("timer_2", 0)
        if timer != 0:
            timer -= 1
            obj["timer_2"] = timer
            if timer != 0:
                return "HIDDEN_WAIT", None

        # Timer expired: check persistent bit
        if (memory_byte[0] & (1 << bit_idx)) != 0:
            return "SUPPRESSED_BY_BIT", None

        # Pop up!
        obj["state_66"] = 2
        obj["timer_2"] = obj.get("timer_9e", 20)
        obj["anim_id"] = 2
        obj["flags_3c"] = obj.get("flags_3c", 0) | 0x01
        obj["flags_38"] = obj.get("flags_38", 0) & ~0x04
        obj["height_4a"] = 48
        return "POPPED_UP", "PLAY_SFX_6004DF_500260"

    elif state == 2:
        # Active hazard
        is_suppressed = (memory_byte[0] & (1 << bit_idx)) != 0
        timer = obj.get("timer_2", 0)
        if is_suppressed or timer == 0 or (timer - 1 == 0):
            # Retract
            obj["state_66"] = 1
            obj["timer_2"] = obj.get("timer_a0", 30)
            obj["anim_id"] = 0
            obj["flags_3c"] = obj.get("flags_3c", 0) & ~0x01
            obj["flags_38"] = obj.get("flags_38", 0) | 0x04
            obj["height_4a"] = 1
            return "RETRACTED", None

        obj["timer_2"] = timer - 1

        # Check collision with Ali
        hit_result = None
        if ali is not None:
            dx = ali.get("x", 0) - obj.get("x", 0)
            dy = ali.get("y", 0) - obj.get("y", 0)
            z_ali = ali.get("z", 0)
            if (-8 <= dx <= 8) and (-2 <= dy <= 2) and (z_ali < 47):
                is_immune = (ali.get("flags_38", 0) & 0x08) != 0 or ali.get("guard_82", False)
                reaction = ali.get("reaction_83", 0)
                if not is_immune and reaction < 2:
                    ali["attacker"] = obj
                    ali["guard_82"] = False
                    ali["field_9d"] = False
                    ali["damage_86"] = 10
                    ali["knockback_x"] = 0x400
                    ali["knockback_y"] = 0x3C0
                    ali["vz"] = 0x20000
                    if reaction != 0 or z_ali != 0:
                        ali["damage_86"] |= 0x8000
                    ali["reaction_83"] = 8
                    hit_result = "HIT_ALI_DEALT_DAMAGE"

        return "ACTIVE_HAZARD", hit_result


def step_toggle_vent_hazard(obj, memory_byte, bit_idx):
    """Emulate AI_WorldObject_PeriodicToggleVentHazard (0x0170E0..0x0171C4)."""
    state = obj.get("state_66", 0)

    if state == 0:
        obj["state_66"] = 1
        obj["timer_2"] = obj.get("timer_a0", 40)
        obj["z"] = obj.get("z_8c", 0)
        return "INITIALIZED", None

    elif state == 1:
        # Inactive vent wait
        if obj.get("anim_id", 0) != 0 and (obj.get("flags_37", 0) & 0x02) != 0:
            obj["anim_id"] = 0

        timer = obj.get("timer_2", 0)
        if timer != 0:
            timer -= 1
            obj["timer_2"] = timer
            if timer != 0:
                return "INACTIVE_WAIT", None

        # Timer expired: check persistent bit
        if (memory_byte[0] & (1 << bit_idx)) != 0:
            return "SUPPRESSED_BY_BIT", None

        # Open vent!
        obj["state_66"] = 2
        obj["timer_2"] = obj.get("timer_9e", 30)
        obj["anim_id"] = 2
        obj["flags_3c"] = obj.get("flags_3c", 0) | 0x02
        obj["depth_49"] = 56
        return "OPENED_VENT", "PLAY_SFX_80034F"

    elif state == 2:
        # Active venting
        is_suppressed = (memory_byte[0] & (1 << bit_idx)) != 0
        if not is_suppressed:
            if obj.get("anim_id", 0) != 4 and (obj.get("flags_37", 0) & 0x02) != 0:
                obj["anim_id"] = 4

        timer = obj.get("timer_2", 0)
        if is_suppressed or timer == 0 or (timer - 1 == 0):
            # Close vent
            obj["state_66"] = 1
            obj["timer_2"] = obj.get("timer_a0", 40)
            obj["anim_id"] = 6
            obj["flags_3c"] = obj.get("flags_3c", 0) & ~0x02
            obj["depth_49"] = 0
            return "CLOSED_VENT", None

        obj["timer_2"] = timer - 1
        return "VENTING_ACTIVE", None


class TestROMWorldDestructibleHazards(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(__file__)
        ev_file = os.path.join(
            self_dir, "..", "data", "thor-evidence", "rom_world_destructible_hazards_m15_520.json"
        )
        with open(ev_file, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        self.assertEqual(self.evidence["milestone"], "M15.520")
        self.assertEqual(self.evidence["range"]["start"], "0x016EBC")
        self.assertEqual(self.evidence["range"]["end"], "0x0171C4")
        self.assertEqual(self.evidence["range"]["size_bytes"], 776)
        self.assertEqual(self.evidence["instruction_count"], 203)
        self.assertEqual(len(self.evidence["routines"]), 4)

    def test_timed_switch_init_and_countdown(self):
        mem = bytearray([0])
        # Mode 0 init: height 16
        obj = {"z_8c": 20, "mode_9e": 0, "state_66": 0}
        res = step_timed_switch(obj, mem, bit_idx=2)
        self.assertEqual(res, "INITIALIZED")
        self.assertEqual(obj["z"], 20)
        self.assertEqual(obj["height_4a"], 16)

        # Triggered state 66 = 1 -> counts 6 frames then despawns
        obj["state_66"] = 1
        for i in range(1, 6):
            res = step_timed_switch(obj, mem, bit_idx=2)
            self.assertEqual(res, "COUNTING")
            self.assertEqual(obj["timer_2"], i)
            self.assertEqual(mem[0], 0)

        # 6th frame -> despawns and sets bit 2
        res = step_timed_switch(obj, mem, bit_idx=2)
        self.assertEqual(res, "DESPAWNED_TRIGGERED")
        self.assertEqual(obj["active_0"], 0)
        self.assertEqual(mem[0], 0x04)

        # Mode 1 init: flags_3c bit 0 set
        obj_mode1 = {"z_8c": 10, "mode_9e": 1}
        step_timed_switch(obj_mode1, mem, bit_idx=0)
        self.assertTrue(obj_mode1["flags_3c"] & 0x01)

    def test_destructible_object_damage_and_destruction(self):
        mem = bytearray([0])
        obj = {"z_8c": 30, "max_hp_a0": 50, "mode_9e": 1}
        res, shatter = step_destructible_block(obj, mem, bit_idx=4)
        self.assertEqual(res, "INITIALIZED")
        self.assertEqual(obj["z"], 30)
        self.assertEqual(obj["hp_8a"], 50)
        self.assertEqual(obj["max_hp_88"], 50)

        # Hit 1: 20 damage -> survives with 30 HP
        obj["damage_86"] = 20
        res, shatter = step_destructible_block(obj, mem, bit_idx=4)
        self.assertEqual(res, "SURVIVED_HIT")
        self.assertEqual(obj["hp_8a"], 30)
        self.assertEqual(obj["guard_82"], 6)

        # In cooldown -> ignores hits
        obj["damage_86"] = 100
        res, shatter = step_destructible_block(obj, mem, bit_idx=4)
        self.assertEqual(res, "IN_COOLDOWN")
        self.assertEqual(obj["hp_8a"], 30)
        self.assertEqual(obj["guard_82"], 5)

        # Decrement cooldown down to 0
        for _ in range(5):
            step_destructible_block(obj, mem, bit_idx=4)
        self.assertEqual(obj["guard_82"], 0)

        # Fatal hit: 40 damage -> 30 - 40 <= 0 -> Destroyed!
        obj["damage_86"] = 40
        res, shatter = step_destructible_block(obj, mem, bit_idx=4)
        self.assertEqual(res, "DESTROYED")
        self.assertTrue(shatter)
        self.assertEqual(obj["z"], 30 + 24)
        self.assertEqual(obj["active_0"], 0)
        self.assertEqual(mem[0], 0x10)

    def test_pop_up_hazard_states(self):
        mem = bytearray([0])
        obj = {"timer_a0": 2, "timer_9e": 3}

        # State 0 -> Init
        res, sfx = step_pop_up_hazard(obj, mem, bit_idx=1)
        self.assertEqual(res, "INITIALIZED")
        self.assertEqual(obj["state_66"], 1)
        self.assertEqual(obj["timer_2"], 2)

        # State 1 -> Countdown
        res, sfx = step_pop_up_hazard(obj, mem, bit_idx=1)
        self.assertEqual(res, "HIDDEN_WAIT")
        self.assertEqual(obj["timer_2"], 1)

        # Pops up to State 2
        res, sfx = step_pop_up_hazard(obj, mem, bit_idx=1)
        self.assertEqual(res, "POPPED_UP")
        self.assertEqual(sfx, "PLAY_SFX_6004DF_500260")
        self.assertEqual(obj["state_66"], 2)
        self.assertEqual(obj["timer_2"], 3)
        self.assertEqual(obj["anim_id"], 2)
        self.assertEqual(obj["height_4a"], 48)

        # State 2 -> Active countdown
        res, sfx = step_pop_up_hazard(obj, mem, bit_idx=1)
        self.assertEqual(res, "ACTIVE_HAZARD")
        self.assertEqual(obj["timer_2"], 2)

        # Retracts to State 1
        step_pop_up_hazard(obj, mem, bit_idx=1)  # timer 1
        res, sfx = step_pop_up_hazard(obj, mem, bit_idx=1)  # timer 0 -> retracts
        self.assertEqual(res, "RETRACTED")
        self.assertEqual(obj["state_66"], 1)
        self.assertEqual(obj["anim_id"], 0)
        self.assertEqual(obj["height_4a"], 1)

    def test_pop_up_hazard_damage_collision(self):
        mem = bytearray([0])
        obj = {"state_66": 2, "timer_2": 10, "x": 100, "y": 100}
        ali_vuln = {"x": 100, "y": 100, "z": 0, "flags_38": 0, "guard_82": False, "reaction_83": 0}

        # Grounded vulnerable hit
        res, hit = step_pop_up_hazard(obj, mem, bit_idx=1, ali=ali_vuln)
        self.assertEqual(hit, "HIT_ALI_DEALT_DAMAGE")
        self.assertEqual(ali_vuln["damage_86"], 10)
        self.assertEqual(ali_vuln["vz"], 0x20000)
        self.assertEqual(ali_vuln["reaction_83"], 8)

        # Airborne hit: heavy damage bit 0x8000 added
        ali_air = {"x": 100, "y": 100, "z": 10, "flags_38": 0, "guard_82": False, "reaction_83": 0}
        res, hit = step_pop_up_hazard(obj, mem, bit_idx=1, ali=ali_air)
        self.assertEqual(hit, "HIT_ALI_DEALT_DAMAGE")
        self.assertEqual(ali_air["damage_86"], 10 | 0x8000)

        # Invulnerable hit ignored
        ali_immune = {"x": 100, "y": 100, "z": 0, "flags_38": 0x08, "guard_82": True}
        res, hit = step_pop_up_hazard(obj, mem, bit_idx=1, ali=ali_immune)
        self.assertIsNone(hit)

    def test_toggle_vent_hazard_states(self):
        mem = bytearray([0])
        obj = {"timer_a0": 40, "timer_9e": 30, "z_8c": 15}

        # State 0 -> Init
        res, sfx = step_toggle_vent_hazard(obj, mem, bit_idx=3)
        self.assertEqual(res, "INITIALIZED")
        self.assertEqual(obj["state_66"], 1)
        self.assertEqual(obj["timer_2"], 40)
        self.assertEqual(obj["z"], 15)

        # Set timer to 1 and step -> opens vent!
        obj["timer_2"] = 1
        res, sfx = step_toggle_vent_hazard(obj, mem, bit_idx=3)
        self.assertEqual(res, "OPENED_VENT")
        self.assertEqual(sfx, "PLAY_SFX_80034F")
        self.assertEqual(obj["state_66"], 2)
        self.assertEqual(obj["timer_2"], 30)
        self.assertEqual(obj["anim_id"], 2)
        self.assertEqual(obj["depth_49"], 56)

        # Active venting: loops anim 4 when finished
        obj["flags_37"] = 2
        step_toggle_vent_hazard(obj, mem, bit_idx=3)
        self.assertEqual(obj["anim_id"], 4)

        # Timer countdown to close
        obj["timer_2"] = 1
        res, sfx = step_toggle_vent_hazard(obj, mem, bit_idx=3)
        self.assertEqual(res, "CLOSED_VENT")
        self.assertEqual(obj["state_66"], 1)
        self.assertEqual(obj["anim_id"], 6)
        self.assertEqual(obj["depth_49"], 0)


if __name__ == "__main__":
    unittest.main()
