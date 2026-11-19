#!/usr/bin/env python3
"""
Unit tests for ROM Serpent Boss Phase 1 Intro Cutscene, Palette Crossfades, Active Master Tick,
5-Segment Hit-Flash, Death Transition & 13-Entry State Jump Table (0x016294..0x0164E2).
Milestone M15.515.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

ROOT_DIR = Path(__file__).resolve().parent.parent
EVIDENCE_PATH = (
    ROOT_DIR
    / "data"
    / "thor-evidence"
    / "rom_serpent_boss_phase1_intro_damage_m15_515.json"
)


def step_intro_cutscene(
    boss: dict,
    body_slots: list[dict],
    cram: dict,
    sfx_callback=None,
    event_callback=None,
) -> str:
    """
    Emulates 0x016294..0x0163A2 Phase 1 Intro Cutscene Sequencer.
    """
    if boss.get("flag_ff0e1d_bit0", 0) != 0:
        return "active_tick"

    t = boss.get("timer_b0", 0) + 1
    boss["timer_b0"] = t

    if t < 0x5A:  # < 90
        return "standby_15afc"

    elif t < 0x7B:  # 90..122
        step = t - 0x5A  # 0..32 (0x00..0x20)
        cram["ff136c_crossfade_step"] = step
        if step == 0 and sfx_callback:
            sfx_callback(0x700219)
        if step == 0x20:
            boss["ff20ae"] = 0x12
            if event_callback:
                event_callback("dae6")
        return f"fade_ff136c_step_{step}"

    elif t < 0x96:  # 123..149
        return "standby_15afc"

    elif t < 0xB7:  # 150..182
        step = t - 0x96  # 0..32 (0x00..0x20)
        cram["ff13ac_crossfade_step"] = step
        if step == 0x20:
            boss["tile_palette"] = 0x0200
            for slot in body_slots[:4]:
                slot["tile_palette"] = 0x0200
            if sfx_callback:
                sfx_callback(0x700219)
        return f"fade_ff13ac_step_{step}"

    elif t < 0xD3:  # 183..210
        return "standby_15afc"

    else:  # >= 211 (0xD3)
        # Finalization
        # 1. 0x015C4A Sub-Mode 3 init
        for i, slot in enumerate(body_slots[:4], start=1):
            slot["anim_id"] = 0x10
        if len(body_slots) >= 5:
            body_slots[4]["anim_id"] = 0x12
            body_slots[4]["flag_36"] = 0
            body_slots[4]["flag_38"] = 0
        for slot in body_slots[5:9]:
            slot["flag_36"] = 0
        boss["flag_36"] = 0
        boss["flag_38"] = 0
        boss["timer_2"] = 0

        # 2. Copy 32 bytes from $5D126 to $FF13AC
        cram["ff13ac_loaded_palette"] = "5D126_32bytes"

        # 3. Assert $FF164D:0, set event $102, set $FF0E1D:0
        boss["flag_ff164d_bit0"] = 1
        if event_callback:
            event_callback("dac2_102")
        boss["flag_ff0e1d_bit0"] = 1
        return "intro_complete_combat_active"


def step_phase1_active_master_tick(
    boss: dict,
    body_slots: list[dict],
    child_death_callback=None,
) -> str:
    """
    Emulates 0x0163A4..0x0164C6 Phase 1 Active Master Tick.
    """
    boss["pos_72"] = 0
    boss["pos_76"] = 0
    boss["hitstun_83"] = 0
    boss["reaction_9c"] = 0

    # Invulnerability countdown +$AE
    ae = boss.get("timer_ae", 0)
    if ae != 0:
        ae -= 1
        boss["timer_ae"] = ae
        if ae != 0:
            boss["flag_82"] = 0xFF
        else:
            boss["flag_82"] = 0
    else:
        boss["flag_82"] = 0

    # Hit-flash palette restoration +$AF
    af = boss.get("timer_af", 0)
    if af != 0:
        af -= 1
        boss["timer_af"] = af
        if af == 0:
            boss["tile_palette"] = 0x0200
            if boss.get("hp", 0) != 0:
                for slot in body_slots[:4]:
                    slot["tile_palette"] = 0x0200

    # Damage handling
    incoming = boss.get("damage_in", 0)
    if incoming != 0:
        boss["tile_palette"] = 0x4200
        for slot in body_slots[:4]:
            slot["tile_palette"] = 0x4200
        boss["timer_af"] = 2

        d7 = incoming
        boss["damage_in"] = 0

        # Heavy hit check (bits 14/15)
        if (d7 & 0xC000) != 0:
            boss["timer_ae"] = 2
            boss["flag_82"] = 0xFF

        # Defense mitigation
        masked = incoming & 0x0FFF
        defense = boss.get("defense", 0)
        mitigated = masked - defense
        if mitigated <= 0:
            mitigated = 1

        new_hp = boss.get("hp", 0) - mitigated
        if new_hp < 0:
            new_hp = 0
        boss["hp"] = new_hp

        if new_hp == 0:
            # Death sequence
            boss["state"] = 0x16
            boss["anim_id"] = 8
            boss["vz"] = 0x00020000
            boss["flag_38"] = boss.get("flag_38", 0) | 0x0C
            boss["ff210a"] = 0x0FFF
            if child_death_callback:
                for slot in body_slots[:4]:
                    child_death_callback(slot, mode=10)
            return "death_triggered"

    return f"dispatched_state_{boss.get('state', 0)}"


class TestROMSerpentBossPhase1IntroDamage(unittest.TestCase):
    def setUp(self):
        self.evidence = json.loads(EVIDENCE_PATH.read_text())

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.515")
        self.assertEqual(meta["range_start"], "0x016294")
        self.assertEqual(meta["range_end"], "0x0164E2")
        self.assertEqual(meta["range_size_bytes"], 590)
        self.assertEqual(meta["code_size_bytes"], 564)
        self.assertEqual(meta["table_size_bytes"], 26)
        self.assertEqual(meta["instruction_count"], 142)
        self.assertEqual(meta["jump_table_entry_count"], 13)

        routines = self.evidence["routines"]
        self.assertEqual(len(routines), 3)
        self.assertEqual(routines[0]["instruction_count"], 62)
        self.assertEqual(routines[1]["instruction_count"], 80)
        self.assertEqual(routines[2]["entry_count"], 13)

    def test_state_jump_table_targets(self):
        table = self.evidence["jump_table"]
        self.assertEqual(len(table), 13)

        expected_targets = [
            ("0x00", 0x0164E2, +26),
            ("0x02", 0x01652E, +100),
            ("0x04", 0x01658A, +190),
            ("0x06", 0x0165AE, +224),
            ("0x08", 0x0165C4, +244),
            ("0x0A", 0x0165D0, +254),
            ("0x0C", 0x0165E2, +270),
            ("0x0E", 0x01663A, +356),
            ("0x10", 0x016690, +440),
            ("0x12", 0x0166D2, +504),
            ("0x14", 0x0166F6, +538),
            ("0x16", 0x0164F2, +20),
            ("0x18", 0x01651C, +60),
        ]

        for entry, (exp_state, exp_target, exp_offset) in zip(table, expected_targets):
            self.assertEqual(entry["state_id"], exp_state)
            self.assertEqual(entry["target_address"], f"0x{exp_target:06X}")
            self.assertEqual(entry["relative_offset"], exp_offset)

    def test_intro_cutscene_timer_sequence(self):
        sfx_events = []
        app_events = []
        boss = {"flag_ff0e1d_bit0": 0, "timer_b0": 0}
        body = [{} for _ in range(9)]
        cram = {}

        # 1. Early stage: timer < 90
        boss["timer_b0"] = 10
        res = step_intro_cutscene(boss, body, cram)
        self.assertEqual(res, "standby_15afc")
        self.assertEqual(boss["timer_b0"], 11)

        # 2. Stage 2: timer 90 (step 0 of FF136C crossfade -> SFX $700219)
        boss["timer_b0"] = 89  # next will be 90
        res2 = step_intro_cutscene(
            boss, body, cram,
            sfx_callback=lambda s: sfx_events.append(s),
            event_callback=lambda e: app_events.append(e),
        )
        self.assertEqual(res2, "fade_ff136c_step_0")
        self.assertEqual(cram["ff136c_crossfade_step"], 0)
        self.assertEqual(sfx_events, [0x700219])

        # 3. Stage 2 end: timer 122 (step 32 -> FF20AE=0x12 and event dae6)
        boss["timer_b0"] = 121
        res3 = step_intro_cutscene(
            boss, body, cram,
            event_callback=lambda e: app_events.append(e),
        )
        self.assertEqual(res3, "fade_ff136c_step_32")
        self.assertEqual(boss["ff20ae"], 0x12)
        self.assertIn("dae6", app_events)

        # 4. Stage 3 standby: timer 123..149
        boss["timer_b0"] = 125
        self.assertEqual(step_intro_cutscene(boss, body, cram), "standby_15afc")

        # 5. Stage 4 end: timer 182 (step 32 -> sets palette $200 across 5 segments & SFX $700219)
        boss["timer_b0"] = 181
        sfx_events.clear()
        res5 = step_intro_cutscene(
            boss, body, cram,
            sfx_callback=lambda s: sfx_events.append(s),
        )
        self.assertEqual(res5, "fade_ff13ac_step_32")
        self.assertEqual(boss["tile_palette"], 0x0200)
        for slot in body[:4]:
            self.assertEqual(slot["tile_palette"], 0x0200)
        self.assertEqual(sfx_events, [0x700219])

        # 6. Stage 6 finalization: timer >= 211 (0xD3)
        boss["timer_b0"] = 210
        app_events.clear()
        res_fin = step_intro_cutscene(
            boss, body, cram,
            event_callback=lambda e: app_events.append(e),
        )
        self.assertEqual(res_fin, "intro_complete_combat_active")
        self.assertEqual(boss["flag_ff164d_bit0"], 1)
        self.assertEqual(boss["flag_ff0e1d_bit0"], 1)
        self.assertIn("dac2_102", app_events)
        self.assertEqual(cram["ff13ac_loaded_palette"], "5D126_32bytes")

    def test_active_master_tick_initial_resets(self):
        boss = {"pos_72": 999, "pos_76": 888, "hitstun_83": 7, "reaction_9c": 5, "hp": 50}
        body = [{} for _ in range(4)]
        step_phase1_active_master_tick(boss, body)
        self.assertEqual(boss["pos_72"], 0)
        self.assertEqual(boss["pos_76"], 0)
        self.assertEqual(boss["hitstun_83"], 0)
        self.assertEqual(boss["reaction_9c"], 0)

    def test_invulnerability_countdown(self):
        boss = {"timer_ae": 2, "hp": 50}
        body = [{} for _ in range(4)]

        # Tick 1: 2 -> 1, invulnerable (flag_82 = 0xFF)
        step_phase1_active_master_tick(boss, body)
        self.assertEqual(boss["timer_ae"], 1)
        self.assertEqual(boss["flag_82"], 0xFF)

        # Tick 2: 1 -> 0, invulnerability expires (flag_82 = 0)
        step_phase1_active_master_tick(boss, body)
        self.assertEqual(boss["timer_ae"], 0)
        self.assertEqual(boss["flag_82"], 0)

    def test_hit_flash_palette_toggle(self):
        boss = {"timer_af": 1, "tile_palette": 0x4200, "hp": 50}
        body = [{"tile_palette": 0x4200} for _ in range(4)]

        # Timer 1 -> 0: restores 0x0200 across root + 4 body slots
        step_phase1_active_master_tick(boss, body)
        self.assertEqual(boss["timer_af"], 0)
        self.assertEqual(boss["tile_palette"], 0x0200)
        for slot in body:
            self.assertEqual(slot["tile_palette"], 0x0200)

        # If HP == 0 when flash expires, body segments are NOT restored
        boss_dead = {"timer_af": 1, "tile_palette": 0x4200, "hp": 0}
        body_dead = [{"tile_palette": 0x4200} for _ in range(4)]
        step_phase1_active_master_tick(boss_dead, body_dead)
        self.assertEqual(boss_dead["tile_palette"], 0x0200)
        for slot in body_dead:
            self.assertEqual(slot["tile_palette"], 0x4200)

    def test_heavy_hit_invulnerability_trigger(self):
        # 1. Normal hit: no invulnerability
        boss = {"damage_in": 10, "hp": 50, "defense": 2}
        body = [{} for _ in range(4)]
        step_phase1_active_master_tick(boss, body)
        self.assertEqual(boss.get("timer_ae", 0), 0)
        self.assertEqual(boss.get("flag_82", 0), 0)

        # 2. Heavy hit with bit 15 set (0x8010): sets timer_ae = 2, flag_82 = 0xFF
        boss2 = {"damage_in": 0x8010, "hp": 50, "defense": 2}
        step_phase1_active_master_tick(boss2, body)
        self.assertEqual(boss2["timer_ae"], 2)
        self.assertEqual(boss2["flag_82"], 0xFF)
        self.assertEqual(boss2["hp"], 36)  # 50 - (16 - 2)

    def test_defense_mitigated_damage(self):
        # Damage <= defense: clamped to 1 damage
        boss = {"damage_in": 5, "hp": 50, "defense": 10}
        body = [{} for _ in range(4)]
        step_phase1_active_master_tick(boss, body)
        self.assertEqual(boss["hp"], 49)

    def test_death_sequence_on_zero_hp(self):
        killed_slots = []
        boss = {"damage_in": 50, "hp": 20, "defense": 0, "flag_38": 0}
        body = [{"id": i} for i in range(4)]

        res = step_phase1_active_master_tick(
            boss, body,
            child_death_callback=lambda s, mode: killed_slots.append((s["id"], mode)),
        )
        self.assertEqual(res, "death_triggered")
        self.assertEqual(boss["hp"], 0)
        self.assertEqual(boss["state"], 0x16)
        self.assertEqual(boss["anim_id"], 8)
        self.assertEqual(boss["vz"], 0x00020000)
        self.assertEqual(boss["flag_38"], 0x0C)
        self.assertEqual(boss["ff210a"], 0x0FFF)
        self.assertEqual(killed_slots, [(0, 10), (1, 10), (2, 10), (3, 10)])


if __name__ == "__main__":
    unittest.main()
