#!/usr/bin/env python3
"""
Unit tests for ROM Spirit Button A Command Decoder, Shared SP Guard &
Dytto Master Tick / SP Drain (0x011A4C..0x011BA0, 340 bytes).
Milestone M15.485.
"""

from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import unittest

EVIDENCE_PATH = (
    Path(__file__).resolve().parent.parent
    / "data"
    / "thor-evidence"
    / "rom_spirit_command_sp_drain_m15_485.json"
)

BUTTON_A_MASK = 0x40
HOLD_THRESHOLD_FRAMES = 0x14  # 20 frames
DOUBLE_TAP_WINDOW_FRAMES = 6
DYTTO_SP_DRAIN_INTERVAL = 0x2D  # 45 frames
LOW_SP_BLINK_BIT_MASK = 0x02


def to_s16(val: int) -> int:
    val &= 0xFFFF
    return val - 0x10000 if val & 0x8000 else val


@dataclass
class SpiritInputState:
    lock_ff1982: int = 0
    lock_ff1983: int = 0
    lock_ff19a7: int = 0
    input_ff165e: int = 0
    player_facing_ff19fe: int = 0
    hold_counter_ff199e: int = 0
    prev_button_a_ff19a0: int = 0
    double_tap_timer_ff19a1: int = 0
    command_out_ff19a2: int = 0
    latched_input_ff19a3: int = 0
    latched_facing_ff19a4: int = 0


def spirit_poll_button_a_command_input(state: SpiritInputState) -> int:
    """Faithful 68K simulation of Spirit_PollButtonACommandInput (0x011A4C..0x011B28)."""
    d0 = 0

    if (
        (state.lock_ff1982 & 0xFF) != 0
        or (state.lock_ff1983 & 0xFF) != 0
        or (state.lock_ff19a7 & 0xFF) != 0
    ):
        # 0x011B0C: lock suppression reset
        state.hold_counter_ff199e = 0
        state.double_tap_timer_ff19a1 = 0
        state.prev_button_a_ff19a0 = 0xFF
        state.command_out_ff19a2 = d0
        return d0

    d7 = state.input_ff165e & BUTTON_A_MASK
    d6 = state.prev_button_a_ff19a0 & 0xFF
    if d6 & 0x80:  # bpl.w $11a88 fails when negative (e.g. 0xFF)
        if d7 != 0:
            state.command_out_ff19a2 = d0
            return d0
        d6 = d7  # 0x00
    state.prev_button_a_ff19a0 = d7

    d5 = state.double_tap_timer_ff19a1 & 0xFF
    if d5 != 0:
        d5 = (d5 - 1) & 0xFF
        if d5 == 0:
            # 0x011B0A -> single tap command (d0 = 1)
            d0 = 1
            state.hold_counter_ff199e = 0
            state.double_tap_timer_ff19a1 = 0
            state.prev_button_a_ff19a0 = 0xFF
            state.command_out_ff19a2 = d0
            return d0
        state.double_tap_timer_ff19a1 = d5

    if d7 != 0:
        state.hold_counter_ff199e = (state.hold_counter_ff199e + 1) & 0xFFFF
        if d6 != 0:
            state.command_out_ff19a2 = d0
            return d0
        if (state.double_tap_timer_ff19a1 & 0xFF) != 0:
            # 0x011B08 -> double tap command (d0 = 2)
            d0 = 2
            state.hold_counter_ff199e = 0
            state.double_tap_timer_ff19a1 = 0
            state.prev_button_a_ff19a0 = 0xFF
            state.command_out_ff19a2 = d0
            return d0
        # 0x011AC0..0x011ACA: initial press latches controller byte & facing
        state.latched_input_ff19a3 = state.input_ff165e & 0xFF
        state.latched_facing_ff19a4 = state.player_facing_ff19fe & 0xFFFF
        state.command_out_ff19a2 = d0
        return d0

    if d6 == 0:
        state.hold_counter_ff199e = d0
        state.command_out_ff19a2 = d0
        return d0

    # 0x011AE8: release edge check against 0x14 (20 frames)
    if (state.hold_counter_ff199e & 0xFFFF) >= HOLD_THRESHOLD_FRAMES:
        # 0x011B06 -> 20+ frame hold release command (d0 = 3)
        d0 = 3
        state.hold_counter_ff199e = 0
        state.double_tap_timer_ff19a1 = 0
        state.prev_button_a_ff19a0 = 0xFF
        state.command_out_ff19a2 = d0
        return d0

    state.hold_counter_ff199e = d0
    state.double_tap_timer_ff19a1 = DOUBLE_TAP_WINDOW_FRAMES
    state.command_out_ff19a2 = d0
    return d0


def spirit_try_consume_player_sp(player_sp: int, cost: int) -> tuple[int, bool]:
    """
    Faithful 68K simulation of Spirit_TryConsumePlayerSP (0x011B28..0x011B4E).
    Returns (updated_player_sp, carry_flag).
    Carry is False (0) on success (result >= 1), True (1) on rejection (< 1).
    """
    remaining = to_s16(to_s16(player_sp) - to_s16(cost))
    if remaining < 1:
        return player_sp & 0xFFFF, True
    return remaining & 0xFFFF, False


@dataclass
class DyttoMasterState:
    sub_entity_flag_a2: int = 0
    drain_timer_8e: int = DYTTO_SP_DRAIN_INTERVAL
    player_sp_8e: int = 100
    render_flags_36: int = 0
    input_state: SpiritInputState | None = None
    polled_input_count: int = 0
    collision_helper_count: int = 0


def ai_dytto_master_tick_and_sp_drain(state: DyttoMasterState) -> str:
    """
    Faithful 68K simulation of AI_Dytto_MasterTickAndSpDrain (0x011B4E..0x011BA0).
    Returns 'DISMISS_012314' if branched to spirit dismissal, else 'DISPATCH_011BA0'.
    """
    if (state.sub_entity_flag_a2 & 0xFF) != 0:
        return "DISPATCH_011BA0"

    if state.input_state is not None:
        spirit_poll_button_a_command_input(state.input_state)
    state.polled_input_count += 1
    state.collision_helper_count += 1  # bsr.w $00E36E

    if (state.drain_timer_8e & 0xFFFF) != 0:
        state.drain_timer_8e = (state.drain_timer_8e - 1) & 0xFFFF
    else:
        state.drain_timer_8e = DYTTO_SP_DRAIN_INTERVAL
        sp = state.player_sp_8e & 0xFFFF
        if sp == 0:
            return "DISMISS_012314"
        sp = (sp - 1) & 0xFFFF
        state.player_sp_8e = sp
        if sp == 0:
            return "DISMISS_012314"

    if (state.player_sp_8e & 0xFFFF) > 1:
        state.render_flags_36 = (state.render_flags_36 & ~LOW_SP_BLINK_BIT_MASK) & 0xFF
    else:
        state.render_flags_36 = (state.render_flags_36 ^ LOW_SP_BLINK_BIT_MASK) & 0xFF

    return "DISPATCH_011BA0"


class TestROMSpiritCommandSpDrain(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Missing evidence JSON: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.485")
        self.assertEqual(self.evidence["range"]["start"], "0x011A4C")
        self.assertEqual(self.evidence["range"]["end"], "0x011BA0")
        self.assertEqual(self.evidence["range"]["size_bytes"], 340)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertEqual(
            list(routines.keys()),
            [
                "Spirit_PollButtonACommandInput",
                "Spirit_TryConsumePlayerSP",
                "AI_Dytto_MasterTickAndSpDrain",
            ],
        )
        self.assertEqual(routines["Spirit_PollButtonACommandInput"]["size_bytes"], 220)
        self.assertEqual(routines["Spirit_PollButtonACommandInput"]["instruction_count"], 47)
        self.assertEqual(routines["Spirit_TryConsumePlayerSP"]["size_bytes"], 38)
        self.assertEqual(routines["Spirit_TryConsumePlayerSP"]["instruction_count"], 11)
        self.assertEqual(routines["AI_Dytto_MasterTickAndSpDrain"]["size_bytes"], 82)
        self.assertEqual(routines["AI_Dytto_MasterTickAndSpDrain"]["instruction_count"], 21)

        insns = self.evidence["instructions"]
        self.assertEqual(self.evidence["instruction_count"], 79)
        self.assertEqual(len(insns), 79)
        self.assertEqual(sum(i["size"] for i in insns), 340)

        # Verify contiguous addresses and key decoded instructions
        cursor = 0x011A4C
        by_addr = {}
        for insn in insns:
            addr = int(insn["address"], 16)
            self.assertEqual(addr, cursor)
            cursor += insn["size"]
            by_addr[addr] = insn
        self.assertEqual(cursor, 0x011BA0)

        self.assertEqual(by_addr[0x011A72]["op_str"], "#$40, d7")
        self.assertEqual(by_addr[0x011AE8]["op_str"], "#$14, $ff199e.l")
        self.assertEqual(by_addr[0x011AFA]["op_str"], "#$6, $ff19a1.l")
        self.assertEqual(by_addr[0x011B30]["op_str"], "#$1, d1")
        self.assertEqual(by_addr[0x011B3C]["op_str"], "#$fe, ccr")
        self.assertEqual(by_addr[0x011B44]["op_str"], "#$1, ccr")
        self.assertEqual(by_addr[0x011B4E]["op_str"], "$ff19e8.l, a5")
        self.assertEqual(by_addr[0x011B72]["op_str"], "#$2d, $8e(a6)")
        self.assertEqual(by_addr[0x011B92]["op_str"], "#$1, $36(a6)")

    def test_lock_flag_suppression_of_button_a_polling(self) -> None:
        for lock_field in ("lock_ff1982", "lock_ff1983", "lock_ff19a7"):
            state = SpiritInputState(
                input_ff165e=0x4F,
                player_facing_ff19fe=2,
                hold_counter_ff199e=12,
                prev_button_a_ff19a0=0x40,
                double_tap_timer_ff19a1=4,
                command_out_ff19a2=9,
                latched_input_ff19a3=0x11,
                latched_facing_ff19a4=0x03,
            )
            setattr(state, lock_field, 1)

            cmd = spirit_poll_button_a_command_input(state)
            self.assertEqual(cmd, 0)
            self.assertEqual(state.command_out_ff19a2, 0)
            self.assertEqual(state.hold_counter_ff199e, 0)
            self.assertEqual(state.double_tap_timer_ff19a1, 0)
            self.assertEqual(state.prev_button_a_ff19a0, 0xFF)
            self.assertEqual(state.latched_input_ff19a3, 0x11)
            self.assertEqual(state.latched_facing_ff19a4, 0x03)

            # Unlock while Button A remains held: 0xFF sentinel must still suppress
            setattr(state, lock_field, 0)
            cmd_still_held = spirit_poll_button_a_command_input(state)
            self.assertEqual(cmd_still_held, 0)
            self.assertEqual(state.prev_button_a_ff19a0, 0xFF)
            self.assertEqual(state.hold_counter_ff199e, 0)

            # Releasing Button A clears the 0xFF suppression sentinel cleanly
            state.input_ff165e = 0x00
            cmd_released = spirit_poll_button_a_command_input(state)
            self.assertEqual(cmd_released, 0)
            self.assertEqual(state.prev_button_a_ff19a0, 0x00)
            self.assertEqual(state.double_tap_timer_ff19a1, 0)

    def test_button_a_single_double_hold_command_recognition(self) -> None:
        # 1. Single tap + initial-press latch check
        state = SpiritInputState(player_facing_ff19fe=3, input_ff165e=0x45)
        self.assertEqual(spirit_poll_button_a_command_input(state), 0)
        self.assertEqual(state.hold_counter_ff199e, 1)
        self.assertEqual(state.latched_input_ff19a3, 0x45)
        self.assertEqual(state.latched_facing_ff19a4, 3)

        # Hold 2 more frames with changed facing/dpad: latched fields must stay from initial press
        state.input_ff165e = 0x4A
        state.player_facing_ff19fe = 1
        self.assertEqual(spirit_poll_button_a_command_input(state), 0)
        self.assertEqual(state.hold_counter_ff199e, 2)
        self.assertEqual(state.latched_input_ff19a3, 0x45)
        self.assertEqual(state.latched_facing_ff19a4, 3)

        # Release before 20 frames -> starts 6-frame double-tap window
        state.input_ff165e = 0x00
        self.assertEqual(spirit_poll_button_a_command_input(state), 0)
        self.assertEqual(state.hold_counter_ff199e, 0)
        self.assertEqual(state.double_tap_timer_ff19a1, 6)

        for expected_timer in (5, 4, 3, 2, 1):
            self.assertEqual(spirit_poll_button_a_command_input(state), 0)
            self.assertEqual(state.double_tap_timer_ff19a1, expected_timer)

        # 6th release frame decrements timer 1 -> 0 and emits Single Tap (1)
        self.assertEqual(spirit_poll_button_a_command_input(state), 1)
        self.assertEqual(state.command_out_ff19a2, 1)
        self.assertEqual(state.double_tap_timer_ff19a1, 0)
        self.assertEqual(state.prev_button_a_ff19a0, 0xFF)

        # 2. Double tap within window (second press while timer > 1 before decrement)
        dt_state = SpiritInputState(player_facing_ff19fe=2, input_ff165e=0x40)
        spirit_poll_button_a_command_input(dt_state)  # press 1
        dt_state.input_ff165e = 0x00
        spirit_poll_button_a_command_input(dt_state)  # release -> timer = 6
        spirit_poll_button_a_command_input(dt_state)  # idle -> timer = 5
        dt_state.input_ff165e = 0x40
        self.assertEqual(spirit_poll_button_a_command_input(dt_state), 2)
        self.assertEqual(dt_state.command_out_ff19a2, 2)
        self.assertEqual(dt_state.hold_counter_ff199e, 0)
        self.assertEqual(dt_state.double_tap_timer_ff19a1, 0)
        self.assertEqual(dt_state.prev_button_a_ff19a0, 0xFF)
        # Holding after double-tap emission remains suppressed until release
        self.assertEqual(spirit_poll_button_a_command_input(dt_state), 0)

        # Boundary: pressing on the 6th tick when timer == 1 decrements to 0 first -> Single Tap (1)
        edge_state = SpiritInputState(input_ff165e=0x40)
        spirit_poll_button_a_command_input(edge_state)
        edge_state.input_ff165e = 0x00
        spirit_poll_button_a_command_input(edge_state)  # timer = 6
        for _ in range(5):
            spirit_poll_button_a_command_input(edge_state)  # timer -> 1
        edge_state.input_ff165e = 0x40
        self.assertEqual(spirit_poll_button_a_command_input(edge_state), 1)

        # 3. 20-frame hold threshold (0x14): 19 frames enters tap window; 20 frames emits Command 3
        short_hold = SpiritInputState(input_ff165e=0x40)
        for _ in range(19):
            self.assertEqual(spirit_poll_button_a_command_input(short_hold), 0)
        self.assertEqual(short_hold.hold_counter_ff199e, 19)
        short_hold.input_ff165e = 0x00
        self.assertEqual(spirit_poll_button_a_command_input(short_hold), 0)
        self.assertEqual(short_hold.double_tap_timer_ff19a1, 6)

        long_hold = SpiritInputState(input_ff165e=0x40)
        for _ in range(20):
            self.assertEqual(spirit_poll_button_a_command_input(long_hold), 0)
        self.assertEqual(long_hold.hold_counter_ff199e, 20)
        long_hold.input_ff165e = 0x00
        self.assertEqual(spirit_poll_button_a_command_input(long_hold), 3)
        self.assertEqual(long_hold.command_out_ff19a2, 3)
        self.assertEqual(long_hold.hold_counter_ff199e, 0)
        self.assertEqual(long_hold.prev_button_a_ff19a0, 0xFF)

    def test_try_consume_player_sp_guard(self) -> None:
        # Sufficient SP leaving >= 1 SP succeeds and clears Carry
        self.assertEqual(spirit_try_consume_player_sp(20, 5), (15, False))
        self.assertEqual(spirit_try_consume_player_sp(6, 5), (1, False))
        self.assertEqual(spirit_try_consume_player_sp(10, 0), (10, False))

        # Leaving 0 or negative SP rejects without mutating SP and sets Carry
        self.assertEqual(spirit_try_consume_player_sp(5, 5), (5, True))
        self.assertEqual(spirit_try_consume_player_sp(1, 1), (1, True))
        self.assertEqual(spirit_try_consume_player_sp(3, 5), (3, True))
        self.assertEqual(spirit_try_consume_player_sp(0, 5), (0, True))
        self.assertEqual(spirit_try_consume_player_sp(2, 25), (2, True))

    def test_dytto_master_tick_sp_drain_and_low_sp_blink(self) -> None:
        # 1. Sub-entity flag (+$A2 != 0) bypasses input polling, helper, drain, and blink
        proj_input = SpiritInputState(input_ff165e=0x40)
        proj = DyttoMasterState(
            sub_entity_flag_a2=0xFF,
            drain_timer_8e=0,
            player_sp_8e=1,
            render_flags_36=0x80,
            input_state=proj_input,
        )
        self.assertEqual(ai_dytto_master_tick_and_sp_drain(proj), "DISPATCH_011BA0")
        self.assertEqual(proj.polled_input_count, 0)
        self.assertEqual(proj.collision_helper_count, 0)
        self.assertEqual(proj.drain_timer_8e, 0)
        self.assertEqual(proj.player_sp_8e, 1)
        self.assertEqual(proj.render_flags_36, 0x80)
        self.assertEqual(proj_input.hold_counter_ff199e, 0)

        # 2. Normal 45-frame (0x2D) drain cadence & blink bit clearing when SP > 1
        dytto = DyttoMasterState(
            sub_entity_flag_a2=0,
            drain_timer_8e=45,
            player_sp_8e=3,
            render_flags_36=0x86,  # bit 1 initially set along with unrelated bits
            input_state=SpiritInputState(),
        )
        for tick in range(45):
            res = ai_dytto_master_tick_and_sp_drain(dytto)
            self.assertEqual(res, "DISPATCH_011BA0")
            self.assertEqual(dytto.player_sp_8e, 3)
            self.assertEqual(dytto.drain_timer_8e, 44 - tick)
            self.assertEqual(dytto.render_flags_36, 0x84)  # bit 1 cleared while SP > 1

        # Next tick sees timer == 0: reloads 45, drains 3 -> 2, still clears bit 1
        self.assertEqual(ai_dytto_master_tick_and_sp_drain(dytto), "DISPATCH_011BA0")
        self.assertEqual(dytto.drain_timer_8e, 45)
        self.assertEqual(dytto.player_sp_8e, 2)
        self.assertEqual(dytto.render_flags_36, 0x84)
        self.assertEqual(dytto.polled_input_count, 46)
        self.assertEqual(dytto.collision_helper_count, 46)

        # 3. Drain 2 -> 1 enables low-SP blink toggling on bit 1 of +$36(a6)
        dytto.drain_timer_8e = 0
        self.assertEqual(ai_dytto_master_tick_and_sp_drain(dytto), "DISPATCH_011BA0")
        self.assertEqual(dytto.player_sp_8e, 1)
        self.assertEqual(dytto.render_flags_36, 0x86)  # toggled bit 1 on
        self.assertEqual(ai_dytto_master_tick_and_sp_drain(dytto), "DISPATCH_011BA0")
        self.assertEqual(dytto.render_flags_36, 0x84)  # toggled bit 1 off
        self.assertEqual(ai_dytto_master_tick_and_sp_drain(dytto), "DISPATCH_011BA0")
        self.assertEqual(dytto.render_flags_36, 0x86)  # toggled bit 1 on

        # 4. Drain 1 -> 0 branches to spirit dismissal 0x012314
        dytto.drain_timer_8e = 0
        self.assertEqual(ai_dytto_master_tick_and_sp_drain(dytto), "DISMISS_012314")
        self.assertEqual(dytto.player_sp_8e, 0)

        # Already-zero SP at drain tick also branches directly to 0x012314
        zero_sp = DyttoMasterState(drain_timer_8e=0, player_sp_8e=0)
        self.assertEqual(ai_dytto_master_tick_and_sp_drain(zero_sp), "DISMISS_012314")
        self.assertEqual(zero_sp.drain_timer_8e, 45)
        self.assertEqual(zero_sp.player_sp_8e, 0)


if __name__ == "__main__":
    unittest.main()
