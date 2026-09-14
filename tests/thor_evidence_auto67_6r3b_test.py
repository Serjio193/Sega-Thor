import json
import struct
import tempfile
import unittest
from pathlib import Path

import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))

from auto67_predecessor import decode, resolve  # noqa: E402


def write_canary(path: Path) -> None:
    records = []
    for sequence in range(1, 28):
        pc = 0x100 + sequence * 2
        opcode = 0
        if sequence == 1:
            pc, opcode = 0x2234, 0x4BF9
        elif sequence == 12:
            pc, opcode = 0x27BE, 0x49F9
        elif sequence == 27:
            pc, opcode = 0x27EC, 0x3955
        records.append((0, sequence, 10, pc, opcode, 0x00C00004, 0x00FF134C))
    rom = bytearray(0x2800)
    rom[0x2234:0x223A] = bytes.fromhex("4BF900FF134C")
    rom[0x27BE:0x27C4] = bytes.fromhex("49F900C00004")
    rom[0x27EC:0x27EE] = bytes.fromhex("3955")
    values = [2, 0, 0x27EC, 3, 1, 27, 27, 1, 0, 0, 27, 10, 0,
              64, 0, 0x27EC, 1]
    payload = bytearray(b"O67P") + bytearray(struct.pack("<17I", *values))
    for record in records:
        payload.extend(struct.pack("<7I", *record))
    path.write_bytes(payload)
    path.with_suffix(".rom").write_bytes(rom)


class Auto676R3BTest(unittest.TestCase):
    def test_bounded_burst_canary_resolves_both_reaching_definitions(self):
        with tempfile.TemporaryDirectory() as directory:
            capture_path = Path(directory) / "canary.o67p"
            write_canary(capture_path)
            capture = decode(capture_path)
            rom = capture_path.with_suffix(".rom").read_bytes()
            result = resolve(capture, rom, ["A4", "A5"])
        self.assertEqual(result["unresolved"], [])
        self.assertEqual({step["register"] for step in result["steps"]}, {"A4", "A5"})
        producers = {step["register"]: step["producer_pc"] for step in result["steps"]}
        self.assertEqual(producers, {"A4": "0x0027BE", "A5": "0x002234"})

    def test_real_rerun_has_bounded_positive_metrics(self):
        path = ROOT / "build/auto67-6r3b-targeted-burst64-rerun.json"
        if not path.exists():
            self.skipTest("real BizHawk artifact is not present")
        data = json.loads(path.read_text(encoding="utf-8"))
        prehistory = data["lua"]["prehistory"]
        frames = data["lua"]["frame_timing"]
        self.assertEqual(prehistory["mode"], "targeted_burst")
        self.assertEqual(prehistory["bursts_started"], prehistory["bursts_completed"])
        self.assertEqual(prehistory["bursts_budget_exhausted"], 0)
        self.assertFalse(prehistory["boundary_gap"])
        self.assertEqual(frames["over_50ms"], 0)
        self.assertEqual(data["dispatcher"]["metrics"]["duplicate_active_claims"], 0)


if __name__ == "__main__":
    unittest.main()
