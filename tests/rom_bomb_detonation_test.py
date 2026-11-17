#!/usr/bin/env python3
"""Unit tests for M15.346 ROM Bomb Detonation Blast Spawner & Screen-Shake Trigger."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_bomb_detonation import ROM_PATH, extract_evidence


def spawn_bomb_blast(z_16_16: int) -> dict:
    """Reference simulation of 0x00ED04..0x00EDA8."""
    new_z = max(0, z_16_16 - 0x00040000)
    return {
        "entity_type": 0x003A,
        "z_16_16": new_z,
        "damage_word": 0x8007,
        "hit_type": 2,
        "state": 8,
        "shake_frames": 4,
    }


class TestRomBombDetonation(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["blast_damage_word"], "0x8007")
        self.assertEqual(ev["constants"]["screen_shake_frames"], 4)
        saved = json.loads(
            Path("data/thor-evidence/rom_bomb_detonation_m15_346.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_bomb_blast_z_clamp_and_params(self) -> None:
        self.assertEqual(spawn_bomb_blast(0x00060000)["z_16_16"], 0x00020000)
        self.assertEqual(spawn_bomb_blast(0x00020000)["z_16_16"], 0)
        self.assertEqual(spawn_bomb_blast(0)["shake_frames"], 4)


if __name__ == "__main__":
    unittest.main()
