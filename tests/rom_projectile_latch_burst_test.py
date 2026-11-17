#!/usr/bin/env python3
"""Unit tests for M15.344 ROM Death Explosion Burst, Latch Follow & Projectile Probe."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_projectile_latch_burst import ROM_PATH, extract_evidence


def step_death_burst_jitter(base_x: int, base_y: int, prng_x: int, prng_y: int, timer: int) -> tuple[int, int, int, bool]:
    """Reference simulation of 0x00EA20..0x00EA74."""
    timer -= 1
    if timer <= 0:
        return (0, base_x, base_y, True)
    jx = (prng_x & 0x1F) - 0x10
    jy = (prng_y & 0x1F) - 0x10
    return (timer, base_x + jx, base_y + jy, False)


def step_latch_follow(parent_xyz: tuple[int, int, int], local_xyz: tuple[int, int, int], latch_timer: int) -> tuple[int, tuple[int, int, int]]:
    """Reference simulation of 0x00EA7E..0x00EABE."""
    latch_timer -= 1
    pos = (
        parent_xyz[0] + local_xyz[0],
        parent_xyz[1] + local_xyz[1],
        parent_xyz[2] + local_xyz[2],
    )
    return (latch_timer, pos)


class TestRomProjectileLatchBurst(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["proj_probe_half_extent"], 6)
        self.assertEqual(ev["constants"]["proj_bounce_sfx"], "0x00700378")
        saved = json.loads(
            Path("data/thor-evidence/rom_projectile_latch_burst_m15_344.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_burst_jitter_and_latch_follow(self) -> None:
        self.assertEqual(step_death_burst_jitter(100, 200, 0x00, 0x1F, 5), (4, 84, 215, False))
        self.assertEqual(step_death_burst_jitter(100, 200, 0x00, 0x00, 1), (0, 100, 200, True))
        self.assertEqual(step_latch_follow((120, 80, 10), (-4, 6, 2), 8), (7, (116, 86, 12)))


if __name__ == "__main__":
    unittest.main()
