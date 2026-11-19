#!/usr/bin/env python3
"""
Unit tests for ROM Interactive World Objects: Moving Bridge/Dust Streamer,
Falling Stalactite Ceiling Trap, Floor Pressure Switch & Hit-Toggle Crystal Switch
(0x016A64..0x016CBE, Milestone M15.518).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_world_traps_switches_m15_518.json"
)


def step_dust_streamer(obj, prng_val=0):
    """Emulate Object_MovingBridge_DustStreamer (0x016A64..0x016ADE)."""
    mode = obj.get("mode_9e", 0)
    if mode == 0:
        # X-axis
        obj["z"] = obj.get("z", 0) + ((prng_val & 0xFFFF) << 1)
        vx = obj.get("vx_4e", 0)
        obj["x"] = obj.get("x", 0) + vx
        bound = obj.get("bound_a0", 0)
        if vx >= 0:
            if obj["x"] >= bound:
                obj["active_0"] = 0
                return "DESPAWNED"
        else:
            if obj["x"] <= bound:
                obj["active_0"] = 0
                return "DESPAWNED"
        return "ACTIVE_X"

    elif mode == 1:
        # Y-axis
        obj["z"] = obj.get("z", 0) + ((prng_val & 0xFFFF) << 2)
        vy = obj.get("vy_52", 0)
        obj["y"] = obj.get("y", 0) + vy
        bound = obj.get("bound_a0", 0)
        if vy >= 0:
            if obj["y"] >= bound:
                obj["active_0"] = 0
                return "DESPAWNED"
        else:
            if obj["y"] <= bound:
                obj["active_0"] = 0
                return "DESPAWNED"
        return "ACTIVE_Y"

    return "UNKNOWN_MODE"


def step_stalactite_trap(obj, ali=None, is_visible=True, prng_seq=None, global_bit_1971=0):
    """Emulate Object_FallingStalactite_Trap (0x016ADE..0x016BD0)."""
    state = obj.get("state_9e", 0)

    if state == 0:
        obj["z"] = 0xF0  # 240
        p = prng_seq.pop(0) if prng_seq else 0
        obj["timer"] = (p & 0x3F) + 4
        obj["vz"] = 0
        if obj.get("bound_a0", 0) != 0:
            obj["timer"] = 1
            obj["vz"] = -0x18
        obj["flags_36"] = obj.get("flags_36", 0) | 0x02
        obj["state_9e"] = 1
        return "INITIALIZED"

    elif state == 1:
        if obj.get("bound_a0", 0) == 0:
            if not is_visible:
                obj["z"] = 0xF0
                return "CULLED_OFFSCREEN"

        obj["timer"] = obj.get("timer", 1) - 1
        if obj["timer"] != 0:
            obj["z"] = 0xF0
            return "WAITING_COUNTDOWN"

        obj["anim_id"] = 4 if (global_bit_1971 & 1) != 0 else 0
        obj["flags_36"] = obj.get("flags_36", 0) & ~0x02
        obj["state_9e"] = 2
        obj["z"] = 0xF0
        return "START_FALL"

    elif state == 2:
        obj["vz"] = obj.get("vz", 0) - 0xC000
        obj["z"] = obj.get("z", 0) + obj["vz"]

        if obj["z"] < 0:
            obj["z"] = 1
            obj["anim_id"] = obj.get("anim_id", 0) + 2
            obj["state_9e"] = 3
            return "FLOOR_IMPACT"

        # Check collision with Ali
        if ali is not None:
            # Check within [-1..+1, -1..+1]
            dx = abs(obj.get("x", 0) - ali.get("x", 0))
            dy = abs(obj.get("y", 0) - ali.get("y", 0))
            if dx <= 1 and dy <= 1:
                # Target is grounded if bit 2 of flags_38 is 0
                if (ali.get("flags_38", 0) & 0x04) == 0:
                    obj["anim_id"] = obj.get("anim_id", 0) + 2
                    obj["state_9e"] = 3
                    return "ALI_IMPACT"

        return "FALLING"

    elif state == 3:
        if (obj.get("flags_37", 0) & 0x02) != 0:
            if obj.get("bound_a0", 0) == 0:
                obj["state_9e"] = 0
                return "RESPAWNED"
            else:
                obj["active_0"] = 0
                return "DESPAWNED"
        return "SHATTERING"


def step_floor_switch(obj, ali_pos, memory_byte, bit_idx):
    """Emulate Object_FloorPressureSwitch (0x016BD0..0x016C5C)."""
    obj["z"] = -16
    mode = obj.get("mode_9e", 0)

    # AABB check vs Ali: [-6..+10, +6..+22]
    # dx = ali_x - obj_x in [-6..+10], dy = ali_y - obj_y in [+6..+22]
    ali_x, ali_y = ali_pos
    dx = ali_x - obj.get("x", 0)
    dy = ali_y - obj.get("y", 0)
    is_on_switch = (-6 <= dx <= 10) and (6 <= dy <= 22)

    if mode == 0:
        # Momentary
        if is_on_switch:
            memory_byte[0] |= (1 << bit_idx)
            obj["anim_id"] = 2
            return "PRESSED_MOMENTARY", False
        else:
            memory_byte[0] &= ~(1 << bit_idx)
            obj["anim_id"] = 0
            return "RELEASED_MOMENTARY", False

    elif mode == 1:
        # One-shot latch
        is_already_set = (memory_byte[0] & (1 << bit_idx)) != 0
        if is_already_set:
            obj["anim_id"] = 2
            return "ALREADY_LATCHED", False

        if is_on_switch:
            memory_byte[0] |= (1 << bit_idx)
            obj["anim_id"] = 2
            return "LATCHED_NOW", True  # Triggers SFX 0x900229
        else:
            obj["anim_id"] = 0
            return "UNLATCHED", False


def step_crystal_switch(obj, memory_byte, bit_idx):
    """Emulate Object_HitToggleCrystalSwitch (0x016C5C..0x016CBE)."""
    cooldown = obj.get("timer_2", 0)
    sfx_played = False

    if cooldown != 0:
        cooldown -= 1
        obj["timer_2"] = cooldown
        if cooldown == 0:
            obj["guard_82"] = 0
    else:
        # Check incoming hit
        if obj.get("damage_86", 0) != 0:
            obj["damage_86"] = 0
            obj["flinch_83"] = 0
            obj["guard_82"] = 0xFF
            obj["timer_2"] = 10
            # Toggle bit
            memory_byte[0] ^= (1 << bit_idx)
            sfx_played = True

    # Update visual state based on bit
    is_active = (memory_byte[0] & (1 << bit_idx)) != 0
    obj["anim_id"] = 2 if is_active else 0
    return is_active, sfx_played


class TestROMWorldTrapsSwitches(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        self_dir = os.path.dirname(__file__)
        ev_file = os.path.join(self_dir, "..", "data", "thor-evidence", "rom_world_traps_switches_m15_518.json")
        with open(ev_file, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        self.assertEqual(self.evidence["milestone"], "M15.518")
        self.assertEqual(self.evidence["range"]["start"], "0x016A64")
        self.assertEqual(self.evidence["range"]["end"], "0x016CBE")
        self.assertEqual(self.evidence["range"]["size_bytes"], 602)
        self.assertEqual(self.evidence["instruction_count"], 174)
        self.assertEqual(len(self.evidence["routines"]), 4)

    def test_dust_streamer_x_axis(self):
        # Moving right (vx > 0)
        obj_right = {"mode_9e": 0, "x": 100, "vx_4e": 4, "bound_a0": 110, "z": 0, "active_0": 1}
        res = step_dust_streamer(obj_right, prng_val=3)
        self.assertEqual(res, "ACTIVE_X")
        self.assertEqual(obj_right["x"], 104)
        self.assertEqual(obj_right["z"], 6)  # 3 << 1

        # Reaches bound -> despawns
        step_dust_streamer(obj_right, prng_val=0)  # x: 108
        res = step_dust_streamer(obj_right, prng_val=0)  # x: 112 >= 110
        self.assertEqual(res, "DESPAWNED")
        self.assertEqual(obj_right["active_0"], 0)

        # Moving left (vx < 0)
        obj_left = {"mode_9e": 0, "x": 100, "vx_4e": -4, "bound_a0": 90, "z": 0, "active_0": 1}
        step_dust_streamer(obj_left, prng_val=0)  # 96
        step_dust_streamer(obj_left, prng_val=0)  # 92
        res = step_dust_streamer(obj_left, prng_val=0)  # 88 <= 90 -> despawns
        self.assertEqual(res, "DESPAWNED")

    def test_dust_streamer_y_axis(self):
        obj_y = {"mode_9e": 1, "y": 200, "vy_52": 5, "bound_a0": 210, "z": 0, "active_0": 1}
        res = step_dust_streamer(obj_y, prng_val=2)
        self.assertEqual(res, "ACTIVE_Y")
        self.assertEqual(obj_y["y"], 205)
        self.assertEqual(obj_y["z"], 8)  # 2 << 2

        res = step_dust_streamer(obj_y, prng_val=0)  # y: 210 >= 210
        self.assertEqual(res, "DESPAWNED")

    def test_falling_stalactite_ceiling_init_and_countdown(self):
        # Mode 0: Normal ceiling wait
        obj = {"state_9e": 0, "bound_a0": 0}
        res = step_stalactite_trap(obj, prng_seq=[10])
        self.assertEqual(res, "INITIALIZED")
        self.assertEqual(obj["z"], 240)
        self.assertEqual(obj["timer"], 14)  # (10 & 0x3F) + 4
        self.assertEqual(obj["vz"], 0)
        self.assertEqual(obj["state_9e"], 1)

        # Viewport culled offscreen
        res = step_stalactite_trap(obj, is_visible=False)
        self.assertEqual(res, "CULLED_OFFSCREEN")
        self.assertEqual(obj["z"], 240)

        # Countdown
        obj["timer"] = 2
        res = step_stalactite_trap(obj, is_visible=True)
        self.assertEqual(res, "WAITING_COUNTDOWN")
        self.assertEqual(obj["timer"], 1)

        # Timer reaches 0 -> start fall with random anim (bit 0 = 1 -> anim 4)
        res = step_stalactite_trap(obj, is_visible=True, global_bit_1971=1)
        self.assertEqual(res, "START_FALL")
        self.assertEqual(obj["state_9e"], 2)
        self.assertEqual(obj["anim_id"], 4)

        # Proximity drop mode (bound_a0 != 0)
        obj_prox = {"state_9e": 0, "bound_a0": 1}
        res = step_stalactite_trap(obj_prox, prng_seq=[50])
        self.assertEqual(res, "INITIALIZED")
        self.assertEqual(obj_prox["timer"], 1)
        self.assertEqual(obj_prox["vz"], -0x18)

    def test_falling_stalactite_fall_impact_and_respawn(self):
        obj = {"state_9e": 2, "z": 0x100000, "vz": 0, "x": 100, "y": 100, "anim_id": 0}
        ali = {"x": 100, "y": 100, "flags_38": 0}

        # Falling in air
        res = step_stalactite_trap(obj, ali=None)
        self.assertEqual(res, "FALLING")
        self.assertEqual(obj["vz"], -0xC000)
        self.assertEqual(obj["z"], 0xF4000)

        # Impact on Ali
        res = step_stalactite_trap(obj, ali=ali)
        self.assertEqual(res, "ALI_IMPACT")
        self.assertEqual(obj["state_9e"], 3)
        self.assertEqual(obj["anim_id"], 2)  # anim + 2

        # Floor impact
        obj_floor = {"state_9e": 2, "z": 0x4000, "vz": 0, "anim_id": 4}
        res = step_stalactite_trap(obj_floor, ali=None)
        self.assertEqual(res, "FLOOR_IMPACT")
        self.assertEqual(obj_floor["z"], 1)
        self.assertEqual(obj_floor["anim_id"], 6)  # 4 + 2
        self.assertEqual(obj_floor["state_9e"], 3)

        # Respawn test (bound_a0 == 0)
        obj_floor["flags_37"] = 2
        obj_floor["bound_a0"] = 0
        res = step_stalactite_trap(obj_floor)
        self.assertEqual(res, "RESPAWNED")
        self.assertEqual(obj_floor["state_9e"], 0)

        # Despawn test (bound_a0 != 0)
        obj_floor["state_9e"] = 3
        obj_floor["bound_a0"] = 1
        obj_floor["flags_37"] = 2
        res = step_stalactite_trap(obj_floor)
        self.assertEqual(res, "DESPAWNED")
        self.assertEqual(obj_floor["active_0"], 0)

    def test_floor_pressure_switch_momentary(self):
        obj = {"mode_9e": 0, "x": 100, "y": 100}
        mem = bytearray([0])

        # Ali on switch: dx=0, dy=10 -> within [-6..+10, +6..+22]
        res, sfx = step_floor_switch(obj, (100, 110), mem, bit_idx=3)
        self.assertEqual(res, "PRESSED_MOMENTARY")
        self.assertEqual(obj["z"], -16)
        self.assertEqual(obj["anim_id"], 2)
        self.assertEqual(mem[0], 0x08)
        self.assertFalse(sfx)

        # Ali steps off switch
        res, sfx = step_floor_switch(obj, (100, 150), mem, bit_idx=3)
        self.assertEqual(res, "RELEASED_MOMENTARY")
        self.assertEqual(obj["anim_id"], 0)
        self.assertEqual(mem[0], 0x00)

    def test_floor_pressure_switch_latched(self):
        obj = {"mode_9e": 1, "x": 100, "y": 100}
        mem = bytearray([0])

        # Step on switch -> Latches!
        res, sfx = step_floor_switch(obj, (100, 110), mem, bit_idx=2)
        self.assertEqual(res, "LATCHED_NOW")
        self.assertTrue(sfx)
        self.assertEqual(mem[0], 0x04)
        self.assertEqual(obj["anim_id"], 2)

        # Step off switch -> Stays latched!
        res, sfx = step_floor_switch(obj, (100, 150), mem, bit_idx=2)
        self.assertEqual(res, "ALREADY_LATCHED")
        self.assertFalse(sfx)
        self.assertEqual(mem[0], 0x04)
        self.assertEqual(obj["anim_id"], 2)

    def test_hit_toggle_crystal_switch(self):
        obj = {"timer_2": 0, "damage_86": 0}
        mem = bytearray([0])

        # No hit -> inactive
        is_active, sfx = step_crystal_switch(obj, mem, bit_idx=5)
        self.assertFalse(is_active)
        self.assertFalse(sfx)
        self.assertEqual(obj["anim_id"], 0)

        # Hit by weapon -> toggles ON!
        obj["damage_86"] = 10
        is_active, sfx = step_crystal_switch(obj, mem, bit_idx=5)
        self.assertTrue(is_active)
        self.assertTrue(sfx)
        self.assertEqual(mem[0], 0x20)
        self.assertEqual(obj["anim_id"], 2)
        self.assertEqual(obj["damage_86"], 0)
        self.assertEqual(obj["guard_82"], 0xFF)
        self.assertEqual(obj["timer_2"], 10)

        # In cooldown -> timer decrements
        for i in range(9):
            step_crystal_switch(obj, mem, bit_idx=5)
            self.assertEqual(obj["guard_82"], 0xFF)

        # 10th frame: cooldown expires, guard cleared
        step_crystal_switch(obj, mem, bit_idx=5)
        self.assertEqual(obj["timer_2"], 0)
        self.assertEqual(obj["guard_82"], 0)

        # Second weapon hit -> toggles OFF!
        obj["damage_86"] = 15
        is_active, sfx = step_crystal_switch(obj, mem, bit_idx=5)
        self.assertFalse(is_active)
        self.assertTrue(sfx)
        self.assertEqual(mem[0], 0x00)
        self.assertEqual(obj["anim_id"], 0)


if __name__ == "__main__":
    unittest.main()
