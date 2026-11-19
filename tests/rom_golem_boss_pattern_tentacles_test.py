#!/usr/bin/env python3
"""
Deterministic unit tests for ROM Multi-Part Boss/Golem (0x9A) Master Tick,
4-State Dispatcher, 5x8 PRNG Pattern Sequencer & Tentacle/Child Initializer (0x015316..0x0154E8).
Milestone M15.508.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

ROOT_DIR = Path(__file__).resolve().parent.parent
EVIDENCE_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_golem_boss_pattern_tentacles_m15_508.json"

GOLEM_ENTITY_TYPE = 0x009A
TENTACLE_CHILD_COUNT = 7
TENTACLE_SLOT_STRIDE = 0xBC

EXPECTED_PATTERNS = [
    [5, 5, 5, 5, 0, 3, 2, 1],
    [4, 1, 2, 3, 0, 3, 1, 0],
    [0, 5, 3, 1, 2, 0, 2, 0],
    [4, 2, 2, 0, 3, 3, 2, 2],
    [1, 1, 1, 5, 1, 3, 2, 0],
]


def route_sub_mode(sub_mode: int) -> str | None:
    """Emulates 0x015316..0x015336 sub-mode router."""
    routes = {
        0: "0x015758",
        1: "0x015338",
        2: "0x0159E2",
        3: "0x0159B4",
        4: "0x01573C",
    }
    return routes.get(sub_mode)


def check_golem_body_contact(ali_dx: int, ali_dy: int) -> bool:
    """Emulates 0x015338..0x01534E body AABB contact test."""
    # dx in [-56, +56], dy in [-120, +120]
    return (-56 <= ali_dx <= 56) and (-120 <= ali_dy <= 120)


class MockGolemBoss:
    def __init__(self):
        self.state_4 = 0
        self.timer_2 = 0
        self.pattern_index_a8 = 0
        self.step_cursor_aa = 0
        self.action_param_a6 = 0
        self.arm_slot: dict[str, int] = {"type": 0, "state": 0, "timer": 0, "flags_38": 0, "flags_36": 0, "offset_1e": 0}
        self.tentacle_slots: list[dict[str, int]] = [{"type": 0, "tile": 0, "vram_art": 0, "sub_mode": 0, "substate": 0} for _ in range(7)]
        self.contact_callback_called = False
        self.idle_reset_called = False

    def step_pattern_sequencer(self, prng_val: int) -> int:
        """Emulates 0x015368..0x0153AA."""
        self.timer_2 -= 1
        if self.timer_2 >= 0:
            return -1  # timer not expired

        d0 = self.pattern_index_a8
        d1 = self.step_cursor_aa

        if d1 >= 8:
            d1 = 0
            self.step_cursor_aa = 0
            d0 = (prng_val & 3) + 1
            self.pattern_index_a8 = d0

        self.step_cursor_aa += 1
        action_id = EXPECTED_PATTERNS[d0][d1]
        self.execute_action(action_id)
        return action_id

    def execute_action(self, action_id: int) -> None:
        """Emulates 0x0153E0..0x0154A6 action execution."""
        if action_id == 0:
            self.action_param_a6 = 12
            self.state_4 = 2
        elif action_id == 1:
            self.action_param_a6 = 2
            self.state_4 = 2
        elif action_id == 2:
            self.timer_2 = 90  # 0x5A
            self.state_4 = 4
        elif action_id == 3:
            self.activate_tentacles()
        elif action_id == 4:
            self.timer_2 = 30  # 0x1E
        elif action_id == 5:
            self.timer_2 = 50  # 0x32

    def activate_tentacles(self, vram_base: int = 0x00010000) -> None:
        """Emulates 0x01541A..0x0154A6."""
        self.arm_slot["type"] = GOLEM_ENTITY_TYPE
        self.arm_slot["state"] = 1
        self.arm_slot["timer"] = 40
        self.arm_slot["flags_38"] &= ~0x08
        self.arm_slot["flags_36"] &= ~0x02
        self.arm_slot["offset_1e"] = -32
        self.arm_slot["substate"] = 8
        self.state_4 = 6  # Parent state -> 6

        # Activate 7 child slots
        for slot in self.tentacle_slots:
            slot["type"] = GOLEM_ENTITY_TYPE
            slot["tile"] = 0x0300
            slot["vram_art"] = vram_base + 0x19A0
            slot["sub_mode"] = 4
            slot["substate"] = 0x0012

    def step_state_06_retract(self) -> bool:
        """Emulates 0x0154A8..0x0154E8."""
        if self.arm_slot["type"] == GOLEM_ENTITY_TYPE and self.arm_slot["state"] == 0:
            # Arm retracted
            for slot in self.tentacle_slots:
                slot["type"] = 0  # cleared
            self.arm_slot["flags_38"] |= 0x08
            self.arm_slot["flags_36"] |= 0x02
            self.arm_slot["offset_1e"] = 0x80
            self.idle_reset_called = True
            return True
        return False


class TestROMGolemBossPatternTentacles(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure_and_counts(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.508")
        self.assertEqual(self.evidence["range"]["start"], "0x015316")
        self.assertEqual(self.evidence["range"]["end"], "0x0154E8")
        self.assertEqual(self.evidence["range"]["size_bytes"], 466)
        self.assertEqual(self.evidence["instruction_count"], 102)
        self.assertEqual(len(self.evidence["code_segments"]), 5)

    def test_state_jump_table_entries(self) -> None:
        entries = self.evidence["state_jump_table"]["entries"]
        self.assertEqual(len(entries), 4)
        self.assertEqual(entries[0]["target_address"], "0x015368")  # State 0
        self.assertEqual(entries[1]["target_address"], "0x01552E")  # State 2
        self.assertEqual(entries[2]["target_address"], "0x0154E8")  # State 4
        self.assertEqual(entries[3]["target_address"], "0x0154A8")  # State 6

    def test_action_jump_table_entries(self) -> None:
        entries = self.evidence["action_jump_table"]["entries"]
        self.assertEqual(len(entries), 6)
        self.assertEqual(entries[0]["target_address"], "0x0153F0")
        self.assertEqual(entries[1]["target_address"], "0x0153FE")
        self.assertEqual(entries[2]["target_address"], "0x01540C")
        self.assertEqual(entries[3]["target_address"], "0x01541A")
        self.assertEqual(entries[4]["target_address"], "0x0153E0")
        self.assertEqual(entries[5]["target_address"], "0x0153E8")

    def test_pattern_table_values(self) -> None:
        patterns = self.evidence["pattern_table_5x8"]["patterns"]
        self.assertEqual(len(patterns), 5)
        for idx, row in enumerate(patterns):
            self.assertEqual(row["sequence"], EXPECTED_PATTERNS[idx])

    def test_sub_mode_router(self) -> None:
        self.assertEqual(route_sub_mode(0), "0x015758")
        self.assertEqual(route_sub_mode(1), "0x015338")
        self.assertEqual(route_sub_mode(2), "0x0159E2")
        self.assertEqual(route_sub_mode(3), "0x0159B4")
        self.assertEqual(route_sub_mode(4), "0x01573C")
        self.assertIsNone(route_sub_mode(5))

    def test_body_contact_aabb(self) -> None:
        # Inside [-56, +56] x [-120, +120]
        self.assertTrue(check_golem_body_contact(0, 0))
        self.assertTrue(check_golem_body_contact(56, 120))
        self.assertTrue(check_golem_body_contact(-56, -120))

        # Outside
        self.assertFalse(check_golem_body_contact(57, 0))
        self.assertFalse(check_golem_body_contact(0, 121))
        self.assertFalse(check_golem_body_contact(-57, 0))

    def test_pattern_sequencer_and_prng_wrap(self) -> None:
        golem = MockGolemBoss()
        golem.pattern_index_a8 = 0
        golem.step_cursor_aa = 0
        golem.timer_2 = 0

        # Initial pattern 0 sequence: [5, 5, 5, 5, 0, 3, 2, 1]
        actions = []
        for _ in range(8):
            golem.timer_2 = 0
            act = golem.step_pattern_sequencer(prng_val=0)
            actions.append(act)
        self.assertEqual(actions, [5, 5, 5, 5, 0, 3, 2, 1])
        self.assertEqual(golem.step_cursor_aa, 8)

        # 9th step triggers wrap: prng=2 -> (2 & 3) + 1 = 3 -> Pattern 3
        # Pattern 3 is [4, 2, 2, 0, 3, 3, 2, 2]
        golem.timer_2 = 0
        act = golem.step_pattern_sequencer(prng_val=2)
        self.assertEqual(golem.pattern_index_a8, 3)
        self.assertEqual(golem.step_cursor_aa, 1)
        self.assertEqual(act, 4)  # First action of Pattern 3 is 4!

    def test_pattern_actions(self) -> None:
        golem = MockGolemBoss()

        # Action 0 -> param 12, state 2
        golem.execute_action(0)
        self.assertEqual(golem.action_param_a6, 12)
        self.assertEqual(golem.state_4, 2)

        # Action 1 -> param 2, state 2
        golem.execute_action(1)
        self.assertEqual(golem.action_param_a6, 2)
        self.assertEqual(golem.state_4, 2)

        # Action 2 -> timer 90, state 4
        golem.execute_action(2)
        self.assertEqual(golem.timer_2, 90)
        self.assertEqual(golem.state_4, 4)

        # Action 4 -> timer 30
        golem.execute_action(4)
        self.assertEqual(golem.timer_2, 30)

        # Action 5 -> timer 50
        golem.execute_action(5)
        self.assertEqual(golem.timer_2, 50)

    def test_tentacle_arm_activation(self) -> None:
        golem = MockGolemBoss()
        golem.activate_tentacles(vram_base=0x00020000)

        # Parent state becomes 6
        self.assertEqual(golem.state_4, 6)
        self.assertEqual(golem.arm_slot["type"], GOLEM_ENTITY_TYPE)
        self.assertEqual(golem.arm_slot["state"], 1)
        self.assertEqual(golem.arm_slot["timer"], 40)
        self.assertEqual(golem.arm_slot["offset_1e"], -32)

        # 7 child slots initialized
        self.assertEqual(len(golem.tentacle_slots), 7)
        for slot in golem.tentacle_slots:
            self.assertEqual(slot["type"], GOLEM_ENTITY_TYPE)
            self.assertEqual(slot["tile"], 0x0300)
            self.assertEqual(slot["vram_art"], 0x00020000 + 0x19A0)
            self.assertEqual(slot["sub_mode"], 4)
            self.assertEqual(slot["substate"], 0x0012)

    def test_state_06_retract_reset(self) -> None:
        golem = MockGolemBoss()
        golem.activate_tentacles()

        # Arm still active (state == 1) -> does not retract yet
        retracted = golem.step_state_06_retract()
        self.assertFalse(retracted)
        self.assertFalse(golem.idle_reset_called)

        # Arm finished retracting (state == 0)
        golem.arm_slot["state"] = 0
        retracted = golem.step_state_06_retract()
        self.assertTrue(retracted)
        self.assertTrue(golem.idle_reset_called)
        # All 7 child slots cleared
        for slot in golem.tentacle_slots:
            self.assertEqual(slot["type"], 0)
        self.assertEqual(golem.arm_slot["offset_1e"], 0x80)
        self.assertEqual(golem.arm_slot["flags_38"] & 0x08, 0x08)
        self.assertEqual(golem.arm_slot["flags_36"] & 0x02, 0x02)


if __name__ == "__main__":
    unittest.main()
