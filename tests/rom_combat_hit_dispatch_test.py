#!/usr/bin/env python3
"""Unit tests for M15.354 ROM combat hitbox dispatch & Ali damage scaling."""

import unittest
from pathlib import Path

from tools.extract_rom_combat_hit_dispatch import ALI_KILL_THRESHOLDS, extract_evidence


def compute_ali_damage_pack(
    base_damage: int,
    hit_flags: int,
    is_ali: bool,
    kill_count: int,
    power_bonus: int,
) -> tuple[int, int]:
    """Mirror 0x00BD36..0x00BD9E damage word + hitstop byte packing."""
    dmg = base_damage & 0xFF
    if is_ali:
        for th in ALI_KILL_THRESHOLDS:
            if kill_count >= th["min_kills"]:
                dmg += th["delta"]
    hitstop = (hit_flags & 0x0F) + 1
    upper_flags = (hit_flags & 0xF0) << 8
    packed_dmg = (upper_flags | dmg) + power_bonus
    return (packed_dmg & 0xFFFF), (hitstop & 0xFF)


class CombatHitDispatchTest(unittest.TestCase):
    def test_rom_opcodes_and_entrypoints(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(len(ev["entrypoints"]), 4)
        self.assertEqual(ev["combat_record_stride_bytes"], 0x2C)
        self.assertEqual(ev["post_loop_special_cancel"]["matching_indices"], [6, 10])

    def test_ali_kill_scaling_and_hitstop_pack(self) -> None:
        self.assertEqual(compute_ali_damage_pack(5, 0x23, True, 0, 2), (0x2007, 4))
        self.assertEqual(compute_ali_damage_pack(5, 0x23, True, 10, 0), (0x2006, 4))
        self.assertEqual(compute_ali_damage_pack(5, 0x23, True, 50, 0), (0x200A, 4))
        self.assertEqual(compute_ali_damage_pack(5, 0x23, True, 200, 3), (0x2013, 4))
        self.assertEqual(compute_ali_damage_pack(5, 0x23, False, 200, 3), (0x2008, 4))


if __name__ == "__main__":
    unittest.main()
