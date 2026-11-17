"""Tests for M15.296 V-Int DMA Queue & 64-Frame Input History Ring extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_vint_and_input_ring import DEFAULT_ROM_PATH, extract_vint_and_input_ring


class RomVintAndInputRingTest(unittest.TestCase):
    """Verify V-Int vector, DMA queue, reset combo, and 64-entry input ring."""

    def test_vint_and_ring_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_vint_and_input_ring(ROOT / DEFAULT_ROM_PATH)
        self.assertEqual(data["vint"]["vector_addr"], "0x001F76")
        self.assertEqual(data["vint"]["dma_queue_base_addr"], "0xFF10B4")
        self.assertEqual(data["vint"]["soft_reset_combo_ff165e"], "0xE0000202")
        self.assertEqual(data["input_ring"]["ring_entries"], 64)
        self.assertEqual(data["input_ring"]["ring_max_count"], 0x3F)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_vint_and_input_ring_m15_296.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.296")
        self.assertEqual(parsed["vint"]["dma_entry_bytes"], 8)


if __name__ == "__main__":
    unittest.main()
