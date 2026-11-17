"""Tests for M15.303 8-Direction Fixed-Point Velocity Vector Kernel."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_velocity_vector_math import (
    DEFAULT_ROM_PATH,
    compute_8dir_velocity,
    compute_cardinal_facing,
    extract_velocity_vector_math,
)


class RomVelocityVectorMathTest(unittest.TestCase):
    """Verify 0x00E2D4 8-dir velocity math and 0x00E338 cardinal facing."""

    def test_8dir_velocity_and_facing(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_velocity_vector_math(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(len(data["directions"]), 8)
        self.assertEqual(data["directions"][0]["target_addr"], "0x00E302")
        self.assertEqual(data["directions"][7]["target_addr"], "0x00E31E")
        self.assertEqual(compute_8dir_velocity(0x400, 0x400, 0), (0, -0x40000))
        self.assertEqual(compute_8dir_velocity(0x400, 0x400, 5), (0x30000, 0x30000))
        self.assertEqual(compute_8dir_velocity(0x400, 0x400, 7), (-0x30000, -0x30000))
        self.assertEqual(compute_cardinal_facing(0, -100), 0)
        self.assertEqual(compute_cardinal_facing(100, 10), 1)
        self.assertEqual(compute_cardinal_facing(0, 100), 2)
        self.assertEqual(compute_cardinal_facing(-100, 10), 3)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_velocity_vector_math_m15_303.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.303")
        self.assertEqual(parsed["velocity_math"]["entry_addr"], "0x00E2D4")


if __name__ == "__main__":
    unittest.main()
