#!/usr/bin/env python3
"""Unit tests for M15.352 ROM Resumable 21-Slot Masked 3D Hitbox Iterator."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_resumable_hitbox_iter import ROM_PATH, extract_evidence


def find_matching_entities(
    slot_mask: int,
    self_idx: int,
    probe_box: tuple[int, int, int, int, int],
    entities: list[dict],
) -> list[int]:
    """Reference simulation of 0x00B852..0x00B920 yielding all matching entity indices."""
    px1, py1, px2, py2, pz = probe_box
    hits: list[int] = []
    for idx, e in enumerate(entities):
        if not ((slot_mask >> idx) & 1):
            continue
        if idx == self_idx or e["state"] <= 0:
            continue
        ex, ey, ez = e["x"], e["y"], e["z"]
        if px1 > ex + e["rx"] or px2 < ex - e["rx"]:
            continue
        if py1 > ey + e["ry"] or py2 < ey - e["ry"]:
            continue
        if pz < ez or pz > ez + e["hz"]:
            continue
        hits.append(idx)
    return hits


class TestRomResumableHitboxIter(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["entity_slot_count"], 21)
        self.assertEqual(ev["constants"]["iter_save_ram_start"], "0x00FF194A")
        saved = json.loads(
            Path("data/thor-evidence/rom_resumable_hitbox_iter_m15_352.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_masked_multi_hit_iteration(self) -> None:
        entities = [
            {"state": 1, "x": 100, "y": 100, "z": 0, "rx": 10, "ry": 10, "hz": 20},
            {"state": 1, "x": 105, "y": 105, "z": 5, "rx": 10, "ry": 10, "hz": 20},
            {"state": 0, "x": 105, "y": 105, "z": 5, "rx": 10, "ry": 10, "hz": 20},
            {"state": 1, "x": 105, "y": 105, "z": 5, "rx": 10, "ry": 10, "hz": 20},
        ]
        # Mask skips slot 0 (bit 0 = 0), self=99, slot 2 inactive -> hits [1, 3]
        self.assertEqual(
            find_matching_entities(0b1110, 99, (98, 98, 112, 112, 10), entities),
            [1, 3],
        )


if __name__ == "__main__":
    unittest.main()
