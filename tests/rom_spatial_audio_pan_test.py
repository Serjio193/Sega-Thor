#!/usr/bin/env python3
"""Unit tests for M15.342 ROM Spatial 2D Audio Attenuation & Stereo Panning."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_spatial_audio_pan import ROM_PATH, extract_evidence


def compute_spatial_sfx(sound_id: int, dx: int, dy: int) -> tuple[bool, int, int]:
    """Reference simulation of 0x00E7E8..0x00E90C.

    Returns (should_play, attenuation_steps, pan_flag: 0=center, 0x40=left, 0x80=right).
    """
    if sound_id < 0x0200:
        return (True, 0, 0)
    adx = abs(dx)
    ady = abs(dy)
    dist = max(adx, ady + (ady >> 2))

    if sound_id >= 0x0300:
        drop = (max(0, dist - 0x58) >> 5) + 1
        if drop > 7:
            return (False, 0, 0)
        pan = (0x40 if dx < 0 else 0x80) if adx >= 0x40 else 0
        return (True, drop, pan)

    if dist >= 0x110:
        return (False, 0, 0)
    atten = 0
    if dist >= 0xB0:
        atten += 1
    if dist >= 0xE0:
        atten += 1
    pan = (0x40 if dx < 0 else 0x80) if adx >= 0x60 else 0
    return (True, atten, pan)


class TestRomSpatialAudioPan(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["fm_cull_dist_px"], 0x110)
        self.assertEqual(ev["constants"]["hires_pan_dx_px"], 0x40)
        saved = json.loads(
            Path("data/thor-evidence/rom_spatial_audio_pan_m15_342.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_attenuation_panning_and_culling(self) -> None:
        self.assertEqual(compute_spatial_sfx(0x0100, -200, 0), (True, 0, 0))
        self.assertEqual(compute_spatial_sfx(0x0250, -100, 0), (True, 0, 0x40))
        self.assertEqual(compute_spatial_sfx(0x0250, 230, 0), (True, 2, 0x80))
        self.assertEqual(compute_spatial_sfx(0x0250, 272, 0), (False, 0, 0))
        self.assertEqual(compute_spatial_sfx(0x0350, 80, 0), (True, 1, 0x80))
        self.assertEqual(compute_spatial_sfx(0x0350, 400, 0), (False, 0, 0))


if __name__ == "__main__":
    unittest.main()
