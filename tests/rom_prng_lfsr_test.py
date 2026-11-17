"""Tests for M15.308 16-Bit LFSR PRNG & Bounded Random Helpers."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_prng_lfsr import DEFAULT_ROM_PATH, extract_prng_lfsr, step_lfsr16


class RomPrngLfsrTest(unittest.TestCase):
    """Verify LFSR seed 0x7F28, 4-shift step, and JSON artifact."""

    def test_lfsr_seed_and_sequence(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_prng_lfsr(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(data["prng"]["initial_seed"], "0x7F28")
        s1 = step_lfsr16(0x7F28)
        self.assertNotEqual(s1, 0)
        self.assertNotEqual(s1, 0x7F28)
        self.assertEqual(f"0x{s1:04X}", data["prng"]["first_8_outputs_from_init"][0])

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_prng_lfsr_m15_308.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.308")
        self.assertEqual(len(parsed["prng"]["first_8_outputs_from_init"]), 8)


if __name__ == "__main__":
    unittest.main()
