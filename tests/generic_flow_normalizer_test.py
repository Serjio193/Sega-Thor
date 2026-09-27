from __future__ import annotations

import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
from generic_flow_normalizer import normalize  # noqa: E402


def record(seq: int, ins: int, pc: int, address: int, value: int,
           flags: int, cpu: int = 0, width: int = 0, domain: int = 0,
           auxiliary: int = 0) -> tuple[int, ...]:
    return (seq, ins, 100 + seq, pc, address, value, flags, cpu, width,
            domain, 0, auxiliary)


class GenericFlowNormalizerTests(unittest.TestCase):
    def test_v2_preserves_instruction_bus_cpu_and_control_identity(self) -> None:
        rom = bytearray(64)
        rom[4:6] = bytes.fromhex("6100")
        rows = [
            record(1, 1, 4, 16, 0x6100, 1 | 2 | 8, width=0),
            record(2, 1, 4, 0xFF0000, 0x1234, 0x8000 | (1 << 11),
                   width=16, domain=1),
            record(3, 2, 0x20, 0x21, 0xCBED, 1 | 2, cpu=1, width=2),
            record(4, 0, 7, 0, 0, 0x8000 | (3 << 11), cpu=0),
        ]
        corpus = normalize(rows, {"run_id": 9, "epoch": 2, "entry_frame": 5}, rom)
        self.assertEqual(corpus["schema"], "oasis.m13.normalized-generic-corpus.v2")
        self.assertEqual(corpus["instructions"][0]["opcode_verification"], "ROM_OPCODE_EXACT")
        self.assertEqual(corpus["instructions"][0]["control_flow_class"], "BSR")
        self.assertEqual(corpus["calls"][0]["target"], 16)
        self.assertEqual(corpus["memory"][0]["operation"], "read")
        self.assertEqual(corpus["memory"][0]["instruction_sequence"], 1)
        self.assertEqual(corpus["instructions"][1]["cpu_id"], "Z80")
        self.assertEqual(corpus["instructions"][1]["opcode_verification"], "NON_ROM_DOMAIN")
        self.assertEqual(corpus["instructions"][0]["domain"], "UNSPECIFIED")
        self.assertEqual(corpus["schema_coverage"]["OPCODE_VERIFICATION_CLASS_COUNTS"],
                         {"NON_ROM_DOMAIN": 1, "ROM_OPCODE_EXACT": 1})
        self.assertEqual(corpus["records"][-1]["frame"], 7)

    def test_missing_register_snapshot_is_explicit_not_invented(self) -> None:
        corpus = normalize([record(1, 1, 4, 6, 0x4E75, 1 | 2)],
                           {"run_id": 1, "epoch": 1, "entry_frame": 3},
                           bytes.fromhex("000000004E75"))
        self.assertEqual(corpus["register_snapshots"], [])
        self.assertIsNone(corpus["instructions"][0]["register_snapshot_id"])
        self.assertEqual(corpus["instructions"][0]["opcode_verification"], "ROM_OPCODE_EXACT")
        self.assertEqual(corpus["instructions"][0]["instruction_width"], None)

    def test_missing_frame_boundaries_remain_unresolved(self) -> None:
        corpus = normalize([record(1, 1, 4, 6, 0x4E75, 1 | 2)],
                           {"run_id": 1, "epoch": 1, "entry_frame": 0,
                            "exit_frame": 0}, bytes.fromhex("000000004E75"))
        self.assertIsNone(corpus["instructions"][0]["frame"])
        self.assertEqual(corpus["schema_coverage"]["INSTRUCTIONS_WITH_FRAME"], 0)
        self.assertEqual(corpus["capture_gaps"][0]["missing_evidence_type"],
                         "frame_identity")

    def test_bounded_worker_boundary_snapshots_link_to_exact_instruction(self) -> None:
        row = record(11, 9, 4, 6, 0x4E75, 1 | 2)
        state = {**{f"D{i}": i for i in range(8)},
                 **{f"A{i}": i + 8 for i in range(8)},
                 "PC": 4, "SR": 0x2700, "USP": 0x1000, "ISP": 0x2000}
        segment = {"run_id": 2, "epoch": 3, "entry_frame": 20, "exit_frame": 21,
                   "entry_instruction_sequence": 9, "exit_instruction_sequence": 10,
                   "entry_registers": state, "exit_registers": {**state, "D0": 99}}
        corpus = normalize([row], segment, bytes.fromhex("000000004E75"))
        self.assertEqual(len(corpus["register_snapshots"]), 2)
        self.assertEqual(corpus["register_snapshots"][1]["registers"]["D0"], 99)
        self.assertEqual(corpus["register_snapshots"][1]["frame"], 21)
        self.assertEqual(len(corpus["instructions"][0]["register_snapshot_ids"]), 2)
        self.assertEqual(corpus["schema_coverage"]["INSTRUCTIONS_WITH_REGISTER_REF"], 1)

    def test_indirect_jsr_target_uses_runtime_next_pc(self) -> None:
        rom = bytearray(32)
        rom[4:6] = bytes.fromhex("4E90")  # JSR (A0)
        corpus = normalize([record(1, 1, 4, 20, 0x4E90, 1 | 2 | 8)],
                           {"run_id": 1, "epoch": 1, "entry_frame": 2}, rom)
        self.assertEqual(corpus["calls"][0]["target"], 20)
        self.assertEqual(corpus["indirect_targets"][0]["target"], 20)
        self.assertEqual(corpus["indirect_targets"][0]["target_evidence"],
                         "runtime_next_pc")


if __name__ == "__main__":
    unittest.main(verbosity=2)
