#!/usr/bin/env python3
"""
Unit tests for ROM Bow Spirit Spawn Initializer, Boss/Flying Enemy AI Master Tick,
Hit-Damage/Death Handler, 14-Entry State Jump Table, State 0 PRNG Action Selector
& State 4 Wander Stepper (0x0147E0..0x014A72).
Milestone M15.503.
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_ai_master_state0_m15_503.json"
)

EXPECTED_TABLE_WORDS = [30, 26, 324, 418, 462, 562, 1018, 1476, 334, 1268, 1252, 594, 652, 1586]
EXPECTED_TARGETS = [
    "0x014932", "0x014930", "0x014A5C", "0x014ABC", "0x014AEA", "0x014B50",
    "0x014D1A", "0x014EE6", "0x014A72", "0x014E1A", "0x014E0C", "0x014B7C",
    "0x014BB8", "0x014F60"
]


def init_bow_spirit(spawn_x, spawn_y, facing):
    """Emulate 0x0147E0..0x014846 Bow Spirit spawn initialization."""
    return {
        "entity_type": 0x1E,
        "vram_tile": 0x04F8,
        "max_hp": 100,
        "hp": 100,
        "max_sp": 10,
        "sp": 10,
        "x": spawn_x,
        "y": spawn_y,
        "z": 0,
        "facing": facing,
        "state": 2,
        "anim": 0x14,
        "flags_38": 0x04,  # bit 2 set (flying)
        "leash_flag_ff1c1c": 0,
        "sfx": 0x80037E
    }


def process_damage_and_tick(entity, incoming_damage, cutscene_b4=False):
    """
    Emulate 0x014848..0x014908 hit-damage and death logic.
    Returns: (event, next_state)
    """
    if entity.get("invuln_a2", False) or cutscene_b4:
        return "NO_DAMAGE", entity.get("state", 0)

    raw_dmg = incoming_damage
    if raw_dmg == 0:
        return "NO_DAMAGE", entity.get("state", 0)

    # Bit 13 check: if bit 13 set, damage is ignored
    if (raw_dmg & 0x2000) != 0:
        effective_dmg = 0
    else:
        effective_dmg = raw_dmg & 0x0FFF

    entity["hit_counter"] = entity.get("hit_counter", 0) + 1
    entity["anim"] = 0x10
    entity["state"] = 0x0E
    entity["flags_36"] = entity.get("flags_36", 0) | 0x40  # bit 6 set

    entity["hp"] = max(0, entity["hp"] - effective_dmg)
    entity["hit_sfx"] = 0xA00250

    if entity["hp"] > 0:
        return "SURVIVED", entity["state"]

    # Death sequence
    entity["anim"] = 0x18
    entity["state"] = 0x1A
    entity["flags_38"] = entity.get("flags_38", 0) | 0x08  # bit 3 set
    entity["invuln_a2"] = True
    entity["timer"] = 0
    entity["death_particle"] = {
        "type": 0x7F,
        "x": entity["x"],
        "y": entity["y"] + 0x20,
        "z": entity["z"] + 0x20
    }
    entity["boss_lock_ff1983"] = True
    return "DIED", 0x1A


def select_state0_action(prng_val, cutscene_lock_ff1987, boss_pos, ali_pos, jitter=(0, 0)):
    """
    Emulate 0x014932..0x014A5A State 0x00 Action Selector.
    Returns dict describing action chosen.
    """
    if not cutscene_lock_ff1987:
        if prng_val <= 0x2000:
            return {"action": "ACTION_1", "anim": 0x02, "state": 0x06, "sfx": 0x800251}
        elif prng_val <= 0x6000:
            return {"action": "ACTION_2", "anim": 0x12, "state": 0x10}
        elif prng_val <= 0x9000:
            return {
                "action": "ACTION_3",
                "anim": 0x18,
                "state": 0x16,
                "speed": 0x40000,
                "timer": 24,
                "sfx": 0x800251
            }

    # Default: Wander Setup (0x014958)
    origin_x, origin_y = 0x100, 0x140
    dx = (ali_pos[0] - origin_x) // 2
    dy = (ali_pos[1] - origin_y) // 2
    mid_x = origin_x + dx + jitter[0]
    mid_y = origin_y + dy + jitter[1]

    # Chebyshev distance
    cdx = abs(mid_x - boss_pos[0])
    cdy = abs(mid_y - boss_pos[1])
    chebyshev = max(cdx, cdy)

    if chebyshev > 0x20:
        speed = 0x00020000
    elif chebyshev > 0x10:
        speed = 0x00010000
    else:
        speed = 0x00008000

    return {
        "action": "WANDER",
        "target_x": mid_x,
        "target_y": mid_y,
        "speed": speed,
        "timer": 10,
        "state": 0x04
    }


def step_state4_wander(timer):
    """Emulate 0x014A5C..0x014A70 State 0x04 Wander Stepper."""
    new_timer = timer - 1
    if new_timer == 0:
        return True, 0x00  # Transition back to State 0
    return False, 0x04


class TestROMBossAIMasterState0(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.503")
        self.assertEqual(self.evidence["range"]["start"], "0x0147E0")
        self.assertEqual(self.evidence["range"]["end"], "0x014A72")
        self.assertEqual(self.evidence["range"]["size_bytes"], 658)
        self.assertEqual(self.evidence["jump_table"]["entry_count"], 14)
        self.assertEqual(self.evidence["jump_table"]["offsets"], EXPECTED_TABLE_WORDS)
        targets = [e["target_address"] for e in self.evidence["jump_table"]["entries"]]
        self.assertEqual(targets, EXPECTED_TARGETS)
        self.assertEqual(len(self.evidence["routines"]), 4)
        self.assertEqual(self.evidence["instruction_count"], 150)

    def test_bow_spirit_spawn_init(self):
        spirit = init_bow_spirit(spawn_x=120, spawn_y=160, facing=2)
        self.assertEqual(spirit["entity_type"], 0x1E)
        self.assertEqual(spirit["vram_tile"], 0x04F8)
        self.assertEqual(spirit["max_hp"], 100)
        self.assertEqual(spirit["hp"], 100)
        self.assertEqual(spirit["max_sp"], 10)
        self.assertEqual(spirit["sp"], 10)
        self.assertEqual(spirit["state"], 2)
        self.assertEqual(spirit["anim"], 0x14)
        self.assertEqual(spirit["flags_38"], 0x04)
        self.assertEqual(spirit["sfx"], 0x80037E)

    def test_hit_damage_and_death(self):
        boss = {"hp": 50, "x": 100, "y": 100, "z": 0, "state": 4}

        # Regular hit: 20 damage -> 30 HP, flinch anim 0x10, state 0x0E
        event, state = process_damage_and_tick(boss, incoming_damage=20)
        self.assertEqual(event, "SURVIVED")
        self.assertEqual(boss["hp"], 30)
        self.assertEqual(state, 0x0E)
        self.assertEqual(boss["anim"], 0x10)
        self.assertEqual(boss["hit_counter"], 1)

        # Invulnerable hit (bit 13 set: 0x2010) -> 0 damage applied, counter still increments
        event, state = process_damage_and_tick(boss, incoming_damage=0x2010)
        self.assertEqual(event, "SURVIVED")
        self.assertEqual(boss["hp"], 30)
        self.assertEqual(boss["hit_counter"], 2)

        # Fatal hit: 50 damage -> HP clamped to 0, enters death state 0x1A
        event, state = process_damage_and_tick(boss, incoming_damage=50)
        self.assertEqual(event, "DIED")
        self.assertEqual(boss["hp"], 0)
        self.assertEqual(state, 0x1A)
        self.assertEqual(boss["anim"], 0x18)
        self.assertTrue(boss["invuln_a2"])
        self.assertEqual(boss["death_particle"]["type"], 0x7F)
        self.assertEqual(boss["death_particle"]["y"], 132)  # 100 + 0x20 (32) = 132
        self.assertTrue(boss["boss_lock_ff1983"])

    def test_jump_table_targets(self):
        entries = self.evidence["jump_table"]["entries"]
        self.assertEqual(entries[0]["target_address"], "0x014932")  # State 0x00 Action Selector
        self.assertEqual(entries[1]["target_address"], "0x014930")  # State 0x02 Dummy RTS
        self.assertEqual(entries[2]["target_address"], "0x014A5C")  # State 0x04 Wander Stepper
        self.assertEqual(entries[7]["target_address"], "0x014EE6")  # State 0x0E Flinch Recovery
        self.assertEqual(entries[13]["target_address"], "0x014F60") # State 0x1A Death State

    def test_state0_prng_action_selector(self):
        boss_pos = (200, 200)
        ali_pos = (300, 400)

        # 1. P <= 0x2000 -> Action 1
        res1 = select_state0_action(0x1000, False, boss_pos, ali_pos)
        self.assertEqual(res1["action"], "ACTION_1")
        self.assertEqual(res1["state"], 0x06)
        self.assertEqual(res1["anim"], 0x02)

        # 2. 0x2000 < P <= 0x6000 -> Action 2
        res2 = select_state0_action(0x4000, False, boss_pos, ali_pos)
        self.assertEqual(res2["action"], "ACTION_2")
        self.assertEqual(res2["state"], 0x10)
        self.assertEqual(res2["anim"], 0x12)

        # 3. 0x6000 < P <= 0x9000 -> Action 3
        res3 = select_state0_action(0x7500, False, boss_pos, ali_pos)
        self.assertEqual(res3["action"], "ACTION_3")
        self.assertEqual(res3["state"], 0x16)
        self.assertEqual(res3["speed"], 0x40000)
        self.assertEqual(res3["timer"], 24)

        # 4. P > 0x9000 -> Wander
        res4 = select_state0_action(0x9500, False, boss_pos, ali_pos)
        self.assertEqual(res4["action"], "WANDER")
        self.assertEqual(res4["state"], 0x04)
        # Midpoint: x = 256 + (300 - 256)/2 = 278, y = 320 + (400 - 320)/2 = 360
        self.assertEqual(res4["target_x"], 278)
        self.assertEqual(res4["target_y"], 360)
        # Chebyshev dist to (200, 200): max(|278-200|, |360-200|) = max(78, 160) = 160 > 0x20 -> fast speed
        self.assertEqual(res4["speed"], 0x00020000)

        # 5. Cutscene lock forces wander even if PRNG <= 0x2000
        res5 = select_state0_action(0x0500, True, boss_pos, ali_pos)
        self.assertEqual(res5["action"], "WANDER")

    def test_chebyshev_speed_thresholds(self):
        origin_mid = (0x100, 0x140)
        ali_pos = (0x100, 0x140)  # midpoint will be (0x100, 0x140) = (256, 320)

        # Case A: Boss at (256, 360) -> chebyshev = 40 (> 32) -> 0x20000
        res_fast = select_state0_action(0xA000, False, (256, 360), ali_pos)
        self.assertEqual(res_fast["speed"], 0x00020000)

        # Case B: Boss at (256, 340) -> chebyshev = 20 (17..32) -> 0x10000
        res_med = select_state0_action(0xA000, False, (256, 340), ali_pos)
        self.assertEqual(res_med["speed"], 0x00010000)

        # Case C: Boss at (256, 328) -> chebyshev = 8 (<= 16) -> 0x08000
        res_slow = select_state0_action(0xA000, False, (256, 328), ali_pos)
        self.assertEqual(res_slow["speed"], 0x00008000)

    def test_state4_wander_stepper(self):
        # Frame 1: timer 2 -> decrements to 1, stays in State 4
        transition, state = step_state4_wander(2)
        self.assertFalse(transition)
        self.assertEqual(state, 0x04)

        # Frame 2: timer 1 -> decrements to 0, transitions to State 0
        transition, state = step_state4_wander(1)
        self.assertTrue(transition)
        self.assertEqual(state, 0x00)


if __name__ == "__main__":
    unittest.main()
