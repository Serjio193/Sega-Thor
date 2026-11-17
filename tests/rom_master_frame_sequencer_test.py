#!/usr/bin/env python3
"""Unit tests for M15.395 ROM Master Gameplay Frame Sequencer, Screen Shake Dispatcher & Joypad Menu Hotkeys."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.extract_rom_master_frame_sequencer import ROM_PATH, extract_evidence


def simulate_screen_shake_dispatch(trigger_val: int) -> tuple[int, str]:
    """Reference simulation of 0x008B8E..0x008BAA."""
    if trigger_val == 0:
        return 0, "none"
    bit7_set = bool(trigger_val & 0x80)
    cleared = trigger_val & 0x7F
    if not bit7_set:
        return cleared, "palette_flash"  # 0x530C
    return cleared, "screen_shake"  # 0x7CC6


def simulate_joypad_menu_hotkeys(
    held_joypad: int,
    edge_joypad: int,
    extra_buttons: int,
    locks: tuple[int, int, int, int],  # (1987, 17b8, 1983, 1982)
    ali_state: int,
    spirit_active: int,
) -> tuple[bool, int, bool, int]:
    """Reference simulation of 0x008BAC..0x008C58 returning (menu_requested, menu_idx, key_reset, ring_count)."""
    key_reset = False
    ring_count = 10
    if not (held_joypad & 0x70):
        key_reset = True
        ring_count = 0

    l1987, l17b8, l1983, l1982 = locks
    if l1987 != 0 or (l17b8 & 4) != 0 or l1983 != 0 or l1982 != 0:
        return False, 0, key_reset, ring_count

    # Start button
    if edge_joypad & 0x80:
        return True, 0, key_reset, ring_count

    # Extra buttons (6-button controller)
    if extra_buttons & 4:
        return True, 3, key_reset, ring_count

    if ali_state <= 2 and spirit_active == 0:
        if extra_buttons & 2:
            return True, 1, key_reset, ring_count
        if extra_buttons & 1:
            return True, 2, key_reset, ring_count

    return False, 0, key_reset, ring_count


class TestRomMasterFrameSequencer(unittest.TestCase):
    def test_rom_evidence_matches(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["milestone"], "M15.395")
        self.assertEqual(ev["routines"]["screen_shake_dispatcher"]["flash_handler"], "0x00530C")
        self.assertEqual(ev["routines"]["screen_shake_dispatcher"]["shake_handler"], "0x007CC6")
        saved = json.loads(
            Path("data/thor-evidence/rom_master_frame_sequencer_m15_395.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_screen_shake_dispatch(self) -> None:
        self.assertEqual(simulate_screen_shake_dispatch(0), (0, "none"))
        self.assertEqual(simulate_screen_shake_dispatch(0x04), (0x04, "palette_flash"))
        self.assertEqual(simulate_screen_shake_dispatch(0x84), (0x04, "screen_shake"))

    def test_joypad_menu_hotkeys(self) -> None:
        # All released -> reset keys
        req, idx, reset, cnt = simulate_joypad_menu_hotkeys(0x0F, 0, 0, (0, 0, 0, 0), 0, 0)
        self.assertFalse(req)
        self.assertTrue(reset)
        self.assertEqual(cnt, 0)

        # Start button -> menu 0
        req, idx, _, _ = simulate_joypad_menu_hotkeys(0x70, 0x80, 0, (0, 0, 0, 0), 0, 0)
        self.assertTrue(req)
        self.assertEqual(idx, 0)

        # Extra button 2 -> Map menu 3
        req, idx, _, _ = simulate_joypad_menu_hotkeys(0x70, 0, 0x04, (0, 0, 0, 0), 0, 0)
        self.assertTrue(req)
        self.assertEqual(idx, 3)

        # Extra button 1 -> Item ring menu 1
        req, idx, _, _ = simulate_joypad_menu_hotkeys(0x70, 0, 0x02, (0, 0, 0, 0), 0, 0)
        self.assertTrue(req)
        self.assertEqual(idx, 1)

        # Extra button 0 -> Weapon ring menu 2
        req, idx, _, _ = simulate_joypad_menu_hotkeys(0x70, 0, 0x01, (0, 0, 0, 0), 0, 0)
        self.assertTrue(req)
        self.assertEqual(idx, 2)

        # Lock flag aborts
        req, idx, _, _ = simulate_joypad_menu_hotkeys(0x70, 0x80, 0, (1, 0, 0, 0), 0, 0)
        self.assertFalse(req)


if __name__ == "__main__":
    unittest.main()
