"""Tests for M15.280 Event VM main 31-opcode ISA extraction."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_event_vm_isa import DEFAULT_ROM_PATH, extract_event_vm_isa


class RomEventVmIsaTest(unittest.TestCase):
    """Verify Event VM jump table 0x00F7D0 (opcodes 0x00..0x1E)."""

    def test_opcode_count_and_key_targets(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_event_vm_isa(ROOT / DEFAULT_ROM_PATH)
        ops = data["opcodes"]
        self.assertEqual(len(ops), 31)
        by_op = {o["opcode"]: o["target_addr"] for o in ops}
        self.assertEqual(by_op[0x00], "0x00F91C")
        self.assertEqual(by_op[0x05], "0x00FAAC")
        self.assertEqual(by_op[0x08], "0x00FCA6")
        self.assertEqual(by_op[0x12], "0x00FAB2")
        self.assertEqual(by_op[0x13], "0x0101D0")
        self.assertEqual(by_op[0x14], "0x0101E4")
        self.assertEqual(by_op[0x1B], "0x0100E4")
        self.assertEqual(by_op[0x1E], "0x00F854")

    def test_json_artifact_exists(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_event_vm_isa_m15_280.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.280")
        self.assertEqual(parsed["vm_state"]["opcode_count"], 31)
        self.assertEqual(parsed["vm_state"]["ram_pc"], "0xFF17B2")


if __name__ == "__main__":
    unittest.main()
