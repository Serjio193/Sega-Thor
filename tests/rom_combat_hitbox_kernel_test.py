"""Tests for M15.297 Combat Hitbox Collision & Kill-Rank Damage Scaling."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_combat_hitbox_kernel import (
    DEFAULT_ROM_PATH,
    compute_ali_kill_rank_bonus,
    extract_combat_hitbox_kernel,
)


class RomCombatHitboxKernelTest(unittest.TestCase):
    """Verify hitbox kernel constants and kill-rank damage bonus curve."""

    def test_hitbox_constants_and_kill_bonus(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_combat_hitbox_kernel(ROOT / DEFAULT_ROM_PATH)
        hk = data["hitbox_kernel"]
        self.assertEqual(hk["hitbox_record_bytes"], 0x2C)
        self.assertEqual(hk["enemy_hp_hud_frames"], 90)
        self.assertEqual(hk["hit_flash_frames"], 6)
        self.assertEqual(compute_ali_kill_rank_bonus(9), 0)
        self.assertEqual(compute_ali_kill_rank_bonus(10), 1)
        self.assertEqual(compute_ali_kill_rank_bonus(50), 5)
        self.assertEqual(compute_ali_kill_rank_bonus(100), 7)
        self.assertEqual(compute_ali_kill_rank_bonus(200), 11)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_combat_hitbox_kernel_m15_297.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.297")
        self.assertEqual(parsed["hitbox_kernel"]["player_attack_entry"], "0x00BCBA")


if __name__ == "__main__":
    unittest.main()
