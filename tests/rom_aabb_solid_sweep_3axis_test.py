#!/usr/bin/env python3
"""Unit tests for M15.332 ROM 3D AABB Solid Sweep (-X, +Y, -Y)."""

from __future__ import annotations

import json
import unittest
from pathlib import Path

from tools.extract_rom_aabb_solid_sweep_3axis import ROM_PATH, extract_evidence


def sweep_axis_3d(
    mover: dict,
    obstacles: list[dict],
    axis: str,
    direction: int,
    max_step: int,
) -> tuple[int, bool, int | None]:
    """Reference simulation of -X (0x009EBA), +Y (0x009FA0), -Y (0x00A09A).

    Returns (clamped_step, collided, pushed_index).
    """
    best = max_step
    collided = False
    pushed_idx: int | None = None

    mx, my, mz = mover["x"], mover["y"] + mover["y_off"], mover["z"] + mover["y_off"]
    mrx, mry, mhz = mover["rx"], mover["ry"], mover["hz"]

    for idx, ob in enumerate(obstacles):
        if ob["hp"] <= 0:
            continue
        ox, oy, oz = ob["x"], ob["y"] + ob["y_off"], ob["z"] + ob["y_off"]
        orx, ory, ohz = ob["rx"], ob["ry"], ob["hz"]

        # Z overlap check
        if oz + ohz <= mz or oz >= mz + mhz:
            continue

        if axis == "x" and direction < 0:
            if oy + ory <= my - mry or oy - ory >= my + mry:
                continue
            if ox >= mover["x"]:
                continue
            gap = max(0, (mover["x"] - mrx) - (ox + orx))
        elif axis == "y" and direction > 0:
            if ox + orx <= mx - mrx or ox - orx >= mx + mrx:
                continue
            if oy <= my:
                continue
            gap = max(0, (oy - ory) - (my + mry))
        elif axis == "y" and direction < 0:
            if ox + orx <= mx - mrx or ox - orx >= mx + mrx:
                continue
            if oy >= my:
                continue
            gap = max(0, (my - mry) - (oy + ory))
        else:
            raise ValueError("Unsupported axis/dir")

        if gap < best:
            best = gap
            collided = True
            if gap == 0 and mover.get("can_push", True) and ob.get("pushable", True):
                ob["push_delta"] = direction
                pushed_idx = idx

    return (best, collided, pushed_idx)


class TestRomAabbSolidSweep3Axis(unittest.TestCase):
    def test_rom_evidence_matches_constants(self) -> None:
        rom = ROM_PATH.read_bytes()
        ev = extract_evidence(rom)
        self.assertEqual(ev["constants"]["entity_slot_count"], 21)
        self.assertEqual(ev["constants"]["entity_stride_bytes"], "0x00BC")
        saved = json.loads(
            Path("data/thor-evidence/rom_aabb_solid_sweep_3axis_m15_332.json").read_text(encoding="utf-8")
        )
        self.assertEqual(ev, saved)

    def test_neg_x_pos_y_neg_y_sweeps(self) -> None:
        mover = {"x": 100, "y": 100, "z": 0, "y_off": 0, "rx": 8, "ry": 8, "hz": 16}
        ob_left = {"hp": 10, "x": 80, "y": 100, "z": 0, "y_off": 0, "rx": 8, "ry": 8, "hz": 16}
        ob_down = {"hp": 10, "x": 100, "y": 116, "z": 0, "y_off": 0, "rx": 8, "ry": 8, "hz": 16}
        # -X: mover left=92, ob_left right=88 -> gap=4
        self.assertEqual(sweep_axis_3d(mover, [ob_left], "x", -1, 10), (4, True, None))
        # +Y: mover bottom=108, ob_down top=108 -> gap=0, pushes +1
        self.assertEqual(sweep_axis_3d(mover, [ob_down], "y", 1, 10), (0, True, 0))
        self.assertEqual(ob_down["push_delta"], 1)


if __name__ == "__main__":
    unittest.main()
