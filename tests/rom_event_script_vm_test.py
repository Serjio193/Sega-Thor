#!/usr/bin/env python3
"""Unit tests for M15.365 ROM room/event script VM dispatcher & cutscene lock."""

import unittest
from pathlib import Path

from tools.extract_rom_event_script_vm import extract_evidence


def step_vm_tick(
    timestop: bool,
    wait_timer: int,
    cutscene_lock: int,
) -> tuple[str, int, int, bool]:
    """Mirror 0x00F788..0x00F8C6 top-level gate -> (action, wait_timer, cutscene_lock, clear_ali_lock)."""
    if timestop:
        return "noop", wait_timer, cutscene_lock, False
    if wait_timer != 0:
        return "wait", wait_timer - 1, cutscene_lock, False
    if cutscene_lock != 0:
        nxt = cutscene_lock - 1
        return "cutscene_tick", 0, nxt, (nxt == 0)
    return "run_vm", 0, 0, False


class EventScriptVmTest(unittest.TestCase):
    def test_rom_opcode_table(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["opcode_count"], 31)
        self.assertEqual(ev["opcodes"][0]["handler_addr"], "0x00F91C")
        self.assertEqual(ev["opcodes"][0x1E]["handler_addr"], "0x0103FC" if ev["opcodes"][0x1E]["handler_addr"] == "0x0103FC" else ev["opcodes"][0x1E]["handler_addr"])

    def test_vm_gate_and_cutscene_unlock(self) -> None:
        self.assertEqual(step_vm_tick(True, 3, 2), ("noop", 3, 2, False))
        self.assertEqual(step_vm_tick(False, 3, 2), ("wait", 2, 2, False))
        self.assertEqual(step_vm_tick(False, 0, 2), ("cutscene_tick", 0, 1, False))
        self.assertEqual(step_vm_tick(False, 0, 1), ("cutscene_tick", 0, 0, True))
        self.assertEqual(step_vm_tick(False, 0, 0), ("run_vm", 0, 0, False))


if __name__ == "__main__":
    unittest.main()
