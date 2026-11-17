#!/usr/bin/env python3
"""Unit tests for M15.343 ROM Explosion Spawner, 7-Frame SFX Cooldown & Shade Phase."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_explosion_spawner_cooldown import ROM_PATH, extract_evidence


def should_play_explosion_sfx(frame_now: int, last_sfx_frame: int) -> tuple[bool, int]:
    """Reference simulation of 0x00E9AA..0x00E9DC."""
    delta = (frame_now - last_sfx_frame) & 0xFFFF
    if delta >= 0x8000 or delta >= 7:
        return (True, frame_now & 0xFFFF)
    return (False, last_sfx_frame)


class TestRomExplosionSpawnerCooldown(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["sfx_cooldown_frames"], 7)
        self.assertEqual(ev["constants"]["shade_spirit_id"], "0x001E")
        saved = json.loads(
            Path("data/thor-evidence/rom_explosion_spawner_m15_343.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_7frame_sfx_cooldown_and_wrap(self) -> None:
        self.assertEqual(should_play_explosion_sfx(100, 94), (False, 94))
        self.assertEqual(should_play_explosion_sfx(101, 94), (True, 101))
        # 3 frames elapsed across 65535 wrap -> still in cooldown
        self.assertEqual(should_play_explosion_sfx(2, 65535), (False, 65535))
        # Signed 16-bit negative delta (bmi) resets & plays
        self.assertEqual(should_play_explosion_sfx(2, 32773), (True, 2))


if __name__ == "__main__":
    unittest.main()
