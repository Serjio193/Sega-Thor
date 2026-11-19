#!/usr/bin/env python3
"""
Unit tests for ROM Composite Serpent/Multi-Segment Boss Sub-Modes 2/3, Hit-Flash Palette
Toggle, Defense-Mitigated HP Subtraction, Knockback Trigger & 12-Entry State Jump Table
(0x015C4A..0x015E0E).
Milestone M15.512.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

EVIDENCE_PATH = (
    Path(__file__).resolve().parent.parent
    / "data"
    / "thor-evidence"
    / "rom_serpent_boss_master_damage_m15_512.json"
)


def step_submode3_activation(slots: list[dict], root_idx: int = 0):
    """
    Emulates 0x015C4A..0x015CB4 Sub-Mode 3 Cutscene/Intro Activation across 9 child slots + root.
    """
    root = slots[root_idx]
    # slots 1..4: anim $10
    for i in range(1, 5):
        slots[root_idx + i]["anim_id"] = 0x10

    # slot 5 (head): anim $12, clear +$36, +$38
    head = slots[root_idx + 5]
    head["anim_id"] = 0x12
    head["flag_36"] = 0
    head["flag_38"] = 0

    # slots 6..9: clear +$36
    for i in range(6, 10):
        slots[root_idx + i]["flag_36"] = 0

    # root restored
    root["flag_36"] = 0
    root["flag_38"] = 0
    root["timer"] = 0


def step_submode2_countdown(entity: dict, death_callback=None):
    """
    Emulates 0x015CB6..0x015CC6 Sub-Mode 2 Countdown & Death Trigger.
    """
    t = entity.get("timer", 0) - 1
    if t == 0:
        if death_callback:
            death_callback(mode=2)
        return "death_triggered"
    entity["timer"] = t
    return "running"


def step_serpent_master_tick(
    root: dict,
    body_slots: list[dict],
    ali: dict,
    game_active_bit: int = 1,
    cutscene_flag_ff1982: int = 0,
):
    """
    Emulates 0x015CCA..0x015DF2 Sub-Mode 2 Master Tick & Combat Handler.
    Returns: action string ("inactive_branch", "jump_state12", "dispatched_state")
    """
    if game_active_bit == 0:
        return "inactive_branch"  # beq.w 0x015AFC

    root["hitstun_timer"] = 0     # sf.b $83
    root["invuln_flag_82"] = 0    # sf.b $82
    root["reaction_9c"] = 0       # clr.w $9c

    if root.get("state", 0) == 0x12:
        return "jump_state12"     # beq.w 0x0160E2

    # Palette restoration check
    flash_timer = root.get("flash_timer", 0)
    if flash_timer != 0:
        flash_timer -= 1
        root["flash_timer"] = flash_timer
        if flash_timer == 0:
            # Restore normal palette 0x0200 across root + 4 body slots
            root["tile_palette"] = 0x0200
            for slot in body_slots[:4]:
                slot["tile_palette"] = 0x0200

    pending_damage = root.get("damage_in", 0)
    if pending_damage != 0:
        # Damage Flash Palette across 5 segments
        root["tile_palette"] = 0x4200
        for slot in body_slots[:4]:
            slot["tile_palette"] = 0x4200
        root["flash_timer"] = 2

        # State 2 reflection check
        if root.get("state", 0) == 2:
            root["state"] = 4
            root["anim_id"] = 0x34
            reflected_timer = 17 - root.get("timer", 0)
            if reflected_timer == 0:
                reflected_timer = 1
            root["timer"] = reflected_timer
            root["vel_a6"] = -root.get("vel_a6", 0)
            root["vel_aa"] = -root.get("vel_aa", 0)

        # Clear pending damage
        root["damage_in"] = 0

        # Defense-mitigated damage
        masked_damage = pending_damage & 0x0FFF
        defense = root.get("defense", 0)
        mitigated = masked_damage - defense
        if mitigated <= 0:
            mitigated = 1

        new_hp = root.get("hp", 0) - mitigated
        if new_hp < 0:
            new_hp = 0
        root["hp"] = new_hp

        if new_hp == 0:
            root["state"] = 0x10
            root["vz"] = 0x00020000
            root["flag_38"] = root.get("flag_38", 0) | (1 << 3)

            if cutscene_flag_ff1982 != 0:
                ali["damage_in"] = 0x800A
                ali["vx"] = 0
                ali["vy"] = 0x00030000
                ali["field_16"] = 0
                ali["reaction_mode"] = 2
                root["anim_id"] = 0x34

    return "dispatched_state"


class TestROMSerpentBossMasterDamage(unittest.TestCase):
    def setUp(self):
        self.evidence = json.loads(EVIDENCE_PATH.read_text())

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.512")
        self.assertEqual(meta["range_start"], "0x015C4A")
        self.assertEqual(meta["range_end"], "0x015E0E")
        self.assertEqual(meta["range_size_bytes"], 452)
        self.assertEqual(meta["code_size_bytes"], 426)
        self.assertEqual(meta["table_size_bytes"], 26)
        self.assertEqual(meta["instruction_count"], 106)
        self.assertEqual(meta["jump_table_entry_count"], 13)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 4)

    def test_state_jump_table_targets(self):
        table = self.evidence["jump_table"]
        self.assertEqual(len(table), 13)

        expected_targets = [
            ("0x00", 0x015E0E, +26),
            ("0x02", 0x015E56, +96),
            ("0x04", 0x015EEA, +242),
            ("0x06", 0x015F04, +266),
            ("0x08", 0x015E3C, +64),
            ("0x0A", 0x015F34, +310),
            ("0x0C", 0x015F52, +338),
            ("0x0E", 0x015FC4, +450),
            ("0x10", 0x01608C, +648),
            ("0x12", 0x0160E2, +732),
            ("0x14", 0x015FE0, +472),
            ("0x16", 0x015FFE, +500),
            ("0x18", 0x016070, +612),
        ]

        for entry, (exp_state, exp_target, exp_offset) in zip(table, expected_targets):
            self.assertEqual(entry["state_id"], exp_state)
            self.assertEqual(entry["target_address"], f"0x{exp_target:06X}")
            self.assertEqual(entry["relative_offset"], exp_offset)

    def test_submode3_cutscene_activation(self):
        # 1 root + 9 children = 10 slots
        slots = [{"flag_36": 0xFF, "flag_38": 0xFF, "anim_id": 0, "timer": 99} for _ in range(10)]
        step_submode3_activation(slots, root_idx=0)

        # slots 1..4: anim $10
        for i in range(1, 5):
            self.assertEqual(slots[i]["anim_id"], 0x10)

        # slot 5 (head): anim $12, clear flag_36, flag_38
        self.assertEqual(slots[5]["anim_id"], 0x12)
        self.assertEqual(slots[5]["flag_36"], 0)
        self.assertEqual(slots[5]["flag_38"], 0)

        # slots 6..9: clear flag_36
        for i in range(6, 10):
            self.assertEqual(slots[i]["flag_36"], 0)

        # root: clear flag_36, flag_38, timer 0
        self.assertEqual(slots[0]["flag_36"], 0)
        self.assertEqual(slots[0]["flag_38"], 0)
        self.assertEqual(slots[0]["timer"], 0)

    def test_submode2_countdown_and_death(self):
        death_modes = []
        entity = {"timer": 3}
        res1 = step_submode2_countdown(entity, death_callback=lambda mode: death_modes.append(mode))
        self.assertEqual(res1, "running")
        self.assertEqual(entity["timer"], 2)
        self.assertEqual(death_modes, [])

        step_submode2_countdown(entity, death_callback=lambda mode: death_modes.append(mode))
        self.assertEqual(entity["timer"], 1)

        res_final = step_submode2_countdown(entity, death_callback=lambda mode: death_modes.append(mode))
        self.assertEqual(res_final, "death_triggered")
        self.assertEqual(death_modes, [2])

    def test_submode2_active_guard_and_state12_bypass(self):
        root = {"state": 0x12, "hitstun_timer": 5, "invuln_flag_82": 1, "reaction_9c": 9}
        body = [{} for _ in range(4)]
        ali = {}

        # 1. Inactive game pause flag bit 0 == 0
        res_paused = step_serpent_master_tick(root, body, ali, game_active_bit=0)
        self.assertEqual(res_paused, "inactive_branch")

        # 2. State 0x12 jumps directly to 0x0160E2
        res_state12 = step_serpent_master_tick(root, body, ali, game_active_bit=1)
        self.assertEqual(res_state12, "jump_state12")
        self.assertEqual(root["hitstun_timer"], 0)
        self.assertEqual(root["invuln_flag_82"], 0)
        self.assertEqual(root["reaction_9c"], 0)

    def test_hit_flash_palette_toggle(self):
        root = {"state": 0, "flash_timer": 2, "tile_palette": 0x4200, "damage_in": 0}
        body = [{"tile_palette": 0x4200} for _ in range(4)]
        ali = {}

        # Tick 1: flash_timer 2 -> 1, palette remains 0x4200
        step_serpent_master_tick(root, body, ali)
        self.assertEqual(root["flash_timer"], 1)
        self.assertEqual(root["tile_palette"], 0x4200)

        # Tick 2: flash_timer 1 -> 0, palette restored to 0x0200 across 5 segments
        step_serpent_master_tick(root, body, ali)
        self.assertEqual(root["flash_timer"], 0)
        self.assertEqual(root["tile_palette"], 0x0200)
        for slot in body:
            self.assertEqual(slot["tile_palette"], 0x0200)

        # Tick 3: Incoming damage sets palette 0x4200 and flash_timer = 2
        root["damage_in"] = 10
        root["hp"] = 50
        step_serpent_master_tick(root, body, ali)
        self.assertEqual(root["tile_palette"], 0x4200)
        for slot in body:
            self.assertEqual(slot["tile_palette"], 0x4200)
        self.assertEqual(root["flash_timer"], 2)

    def test_state2_hit_rebound_reflection(self):
        root = {
            "state": 2,
            "timer": 5,
            "vel_a6": 0x00018000,
            "vel_aa": -0x00020000,
            "damage_in": 8,
            "defense": 2,
            "hp": 50,
        }
        body = [{} for _ in range(4)]
        ali = {}

        step_serpent_master_tick(root, body, ali)
        self.assertEqual(root["state"], 4)
        self.assertEqual(root["anim_id"], 0x34)
        self.assertEqual(root["timer"], 12)  # 17 - 5 = 12
        self.assertEqual(root["vel_a6"], -0x00018000)
        self.assertEqual(root["vel_aa"], 0x00020000)

        # Edge case: timer == 17 -> 17 - 17 = 0 clamped to 1
        root2 = {"state": 2, "timer": 17, "damage_in": 5, "hp": 50}
        step_serpent_master_tick(root2, body, ali)
        self.assertEqual(root2["timer"], 1)

    def test_defense_mitigated_damage(self):
        # 1. Damage > defense
        root = {"state": 0, "hp": 50, "defense": 6, "damage_in": 0x8014}  # 0x8014 & 0xFFF = 20
        body = [{} for _ in range(4)]
        ali = {}
        step_serpent_master_tick(root, body, ali)
        # Mitigated: 20 - 6 = 14; HP = 50 - 14 = 36
        self.assertEqual(root["hp"], 36)

        # 2. Damage <= defense clamps to 1 damage
        root2 = {"state": 0, "hp": 36, "defense": 10, "damage_in": 5}
        step_serpent_master_tick(root2, body, ali)
        # Mitigated: 5 <= 10 -> clamp 1; HP = 36 - 1 = 35
        self.assertEqual(root2["hp"], 35)

    def test_death_state_transition_without_cutscene(self):
        root = {"state": 0, "hp": 5, "defense": 0, "damage_in": 10, "flag_38": 0}
        body = [{} for _ in range(4)]
        ali = {}
        step_serpent_master_tick(root, body, ali, cutscene_flag_ff1982=0)

        self.assertEqual(root["hp"], 0)
        self.assertEqual(root["state"], 0x10)
        self.assertEqual(root["vz"], 0x00020000)
        self.assertEqual(root["flag_38"], 1 << 3)
        self.assertNotIn("damage_in", ali)

    def test_death_state_transition_with_cutscene(self):
        root = {"state": 0, "hp": 5, "defense": 0, "damage_in": 10, "flag_38": 0}
        body = [{} for _ in range(4)]
        ali = {}
        step_serpent_master_tick(root, body, ali, cutscene_flag_ff1982=1)

        self.assertEqual(root["hp"], 0)
        self.assertEqual(root["state"], 0x10)
        self.assertEqual(root["vz"], 0x00020000)
        self.assertEqual(root["flag_38"], 1 << 3)
        self.assertEqual(root["anim_id"], 0x34)
        self.assertEqual(ali["damage_in"], 0x800A)
        self.assertEqual(ali["vx"], 0)
        self.assertEqual(ali["vy"], 0x00030000)
        self.assertEqual(ali["reaction_mode"], 2)


if __name__ == "__main__":
    unittest.main()
