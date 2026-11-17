#!/usr/bin/env python3
"""Unit tests for M15.362 ROM CRAM palette animator & 3-pool AI dispatcher."""

import unittest
from pathlib import Path

from tools.extract_rom_palette_anim_pool_dispatch import extract_evidence


def step_palette_node(node: dict, cram: dict[int, int]) -> bool:
    """Mirror 0x008E3A..0x008E72 for one palette-animation node."""
    t = node["delay_timer"]
    t -= 1
    if t != 0:
        t -= 1
        if t != 0:
            node["delay_timer"] = t
            return False
    cur = node["cursor"] + 4
    keyframes = node["keyframes"]
    idx = (cur - 8) // 4
    if idx >= len(keyframes) or keyframes[idx][0] == 0:
        cur = 8
        idx = 0
    delay, color = keyframes[idx]
    node["cursor"] = cur
    node["delay_timer"] = delay
    cram[node["cram_offset"]] = color
    return True


class PaletteAnimPoolDispatchTest(unittest.TestCase):
    def test_rom_addresses_and_pools(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["palette_animator_008e22"]["cram_shadow_addr"], "0xFF134C")
        self.assertEqual(len(ev["ai_pool_dispatcher_008e90"]["pools"]), 3)
        self.assertEqual(ev["ai_pool_dispatcher_008e90"]["pools"][1]["slot_count"], 21)

    def test_palette_node_cycle_and_wrap(self) -> None:
        node = {
            "cram_offset": 0x20,
            "cursor": 8,
            "delay_timer": 4,
            "keyframes": [(4, 0x0EEE), (4, 0x0888)],
        }
        cram: dict[int, int] = {}
        self.assertFalse(step_palette_node(node, cram))
        self.assertEqual(node["delay_timer"], 2)
        self.assertTrue(step_palette_node(node, cram))
        self.assertEqual(cram[0x20], 0x0888)
        self.assertEqual(node["cursor"], 12)
        node["delay_timer"] = 1
        self.assertTrue(step_palette_node(node, cram))
        self.assertEqual(cram[0x20], 0x0EEE)
        self.assertEqual(node["cursor"], 8)


if __name__ == "__main__":
    unittest.main()
