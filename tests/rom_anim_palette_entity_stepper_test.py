#!/usr/bin/env python3
"""Unit tests for M15.397 ROM Animated Palette CRAM Sequencer & 3-Group Entity Master Stepper."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_anim_palette_entity_stepper import ROM_PATH, extract_evidence


def simulate_anim_palette_step(
    active: bool,
    header: dict,
    cram: list[int],
) -> tuple[bool, bool]:
    """Reference simulation of 0x008E22..0x008E8E returning (cram_sync_triggered, color_changed)."""
    if not active:
        return False, False

    timer = header["timer"] - 1
    if timer > 1:
        header["timer"] = timer
        return False, False

    # Timer expired, advance frame
    idx = header["cur_idx"] + 1
    if idx >= len(header["frames"]) or header["frames"][idx][0] == 0:
        idx = 0  # Loop
    header["cur_idx"] = idx
    header["timer"] = header["frames"][idx][0]  # duration
    color = header["frames"][idx][1]
    cram[header["target_cram_idx"]] = color
    return True, True


def simulate_entity_state_reset(entity: dict, new_state: int) -> None:
    """Reference simulation of 0x008EF8..0x008F10."""
    entity["state"] = new_state
    entity["state_timer"] = 0xFFFF
    entity["flags_37"] &= 0xF9
    entity["subtimer_30"] = 0
    entity["subtimer_2e"] = 0


class TestRomAnimPaletteEntityStepper(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.397")
        self.assertEqual(len(ev["routines"]["entity_master_stepper"]["groups"]), 3)
        self.assertEqual(ev["routines"]["entity_master_stepper"]["groups"][1]["slots"], 21)
        saved = json.loads(
            Path("data/thor-evidence/rom_anim_palette_entity_stepper_m15_397.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_anim_palette_sequencer(self) -> None:
        cram = [0] * 64
        header = {
            "target_cram_idx": 15,
            "cur_idx": 0,
            "timer": 3,
            "frames": [(4, 0x0E00), (4, 0x00E0), (0, 0)],
        }
        # Inactive -> no sync
        sync, changed = simulate_anim_palette_step(False, header, cram)
        self.assertFalse(sync)
        self.assertFalse(changed)

        # Active, timer count down
        sync, changed = simulate_anim_palette_step(True, header, cram)
        self.assertFalse(sync)
        self.assertEqual(header["timer"], 2)

        # Timer expired -> advances and triggers CRAM sync
        sync, changed = simulate_anim_palette_step(True, header, cram)
        self.assertTrue(sync)
        self.assertTrue(changed)
        self.assertEqual(cram[15], 0x00E0)

    def test_entity_state_reset(self) -> None:
        entity = {
            "state": 5,
            "state_timer": 10,
            "flags_37": 0xFF,
            "subtimer_30": 4,
            "subtimer_2e": 8,
        }
        simulate_entity_state_reset(entity, 2)
        self.assertEqual(entity["state"], 2)
        self.assertEqual(entity["state_timer"], 0xFFFF)
        self.assertEqual(entity["flags_37"], 0xF9)
        self.assertEqual(entity["subtimer_30"], 0)
        self.assertEqual(entity["subtimer_2e"], 0)


if __name__ == "__main__":
    unittest.main()
