"""Tests for M15.279 Event VM Opcode 0x12 conditional branch extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_event_vm_op12 import DEFAULT_ROM_PATH, extract_event_vm_op12


class RomEventVmOp12Test(unittest.TestCase):
    """Verify Opcode 0x12 jump tables and helper metadata against ROM."""

    def test_sub_action_table_targets(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_event_vm_op12(ROOT / DEFAULT_ROM_PATH)
        subs = data["sub_actions"]
        self.assertEqual(len(subs), 12)
        expected_targets = [
            "0x00FADA",
            "0x00FBAC",
            "0x00FBDC",
            "0x00FBFC",
            "0x00FC1C",
            "0x00FAE8",
            "0x00FAF6",
            "0x00FC42",
            "0x00FB04",
            "0x00FB14",
            "0x00FB24",
            "0x00FB40",
        ]
        self.assertEqual([s["target_addr"] for s in subs], expected_targets)

    def test_comparator_table_targets(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_event_vm_op12(ROOT / DEFAULT_ROM_PATH)
        comps = data["comparators"]
        self.assertEqual(len(comps), 6)
        self.assertEqual(
            [c["target_addr"] for c in comps],
            ["0x00FB70", "0x00FB78", "0x00FB80", "0x00FB88", "0x00FB90", "0x00FB98"],
        )
        self.assertEqual([c["operator"] for c in comps], ["eq", "ne", "gt", "ge", "le", "lt"])

    def test_json_roundtrip_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_event_vm_op12_m15_279.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.279")
        self.assertEqual(parsed["dispatcher"]["true_branch_addr"], "0x00FBA0")
        self.assertEqual(parsed["dispatcher"]["false_branch_addr"], "0x00FBA6")


if __name__ == "__main__":
    unittest.main()
